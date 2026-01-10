from fastapi import WebSocket, WebSocketDisconnect
from datetime import datetime
import json
import logging
from typing import Dict, Optional
from collections import defaultdict, deque
import asyncio

logger = logging.getLogger(__name__)

class WebSocketManager:
    def __init__(self):
        self.active_connections: Dict[str, WebSocket] = {}  # client_id -> WebSocket
        self.user_to_client: Dict[int, str] = {}  # user_id -> client_id
        self.client_to_user: Dict[str, int] = {}  # client_id -> user_id
        
        # Chat-specific: Typing indicators tracking
        self.typing_users: Dict[int, Dict[int, datetime]] = {}  # user_id -> {friend_id: last_typing_time}
        
        # Rate limiting: Track message counts per user
        self.message_counts: Dict[int, deque] = {}  # user_id -> deque of timestamps
        self.max_messages_per_second = 10
    
    async def connect(self, websocket: WebSocket, client_id: str):
        await websocket.accept()
        self.active_connections[client_id] = websocket
        client_ip = websocket.client.host if websocket.client else "unknown"
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] WebSocket client connected")
        print(f"  Client ID: {client_id}")
        print(f"  Client IP: {client_ip}")
    
    async def disconnect(self, client_id: str):
        if client_id in self.active_connections:
            # Capture user_id (if any) before cleanup so we can broadcast offline
            user_id: Optional[int] = self.client_to_user.get(client_id)

            del self.active_connections[client_id]
            # Remove user_id mapping if exists
            if user_id is not None:
                if user_id in self.user_to_client and self.user_to_client[user_id] == client_id:
                    del self.user_to_client[user_id]
                if client_id in self.client_to_user:
                    del self.client_to_user[client_id]
            else:
                # No mapped user for this client
                if client_id in self.client_to_user:
                    del self.client_to_user[client_id]

            print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] WebSocket client disconnected: {client_id}")

            # Broadcast offline status to online friends
            if user_id is not None:
                try:
                    await self.send_status_update_to_friends(user_id, status="offline")
                    print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ✅ Auto-sent offline status to friends of user {user_id}")
                except Exception as status_error:
                    print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ⚠️ Warning: Failed to auto-send offline status: {str(status_error)}")
    
    async def send_notification_to_user(self, user_id: int, notification_data: dict):
        """
        Send notification to a specific user via WebSocket.
        Returns True if sent successfully, False if user not connected.
        """
        if user_id not in self.user_to_client:
            print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ⚠️ User {user_id} not connected via WebSocket")
            return False
        
        client_id = self.user_to_client[user_id]
        if client_id not in self.active_connections:
            print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ⚠️ Client {client_id} for user {user_id} not in active connections")
            # Clean up stale mapping
            if user_id in self.user_to_client:
                del self.user_to_client[user_id]
            if client_id in self.client_to_user:
                del self.client_to_user[client_id]
            return False
        
        websocket = self.active_connections[client_id]
        try:
            message = {
                "type": "notification",
                "notification": notification_data
            }
            await websocket.send_text(json.dumps(message))
            print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ✅ Sent notification to user {user_id} (client {client_id})")
            return True
        except Exception as e:
            print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ❌ Error sending notification to user {user_id}: {str(e)}")
            logger.error(f"Error sending notification: {str(e)}", exc_info=True)
            return False
    
    async def send_game_event(self, user_id: int, event_data: dict) -> bool:
        """
        Send a game-related event to a specific user.
        Payload format: {"type":"game_event","event":{...}}
        """
        if user_id not in self.user_to_client:
            print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ⚠️ User {user_id} not connected via WebSocket")
            return False

        client_id = self.user_to_client[user_id]
        if client_id not in self.active_connections:
            print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ⚠️ Client {client_id} for user {user_id} not in active connections")
            if user_id in self.user_to_client:
                del self.user_to_client[user_id]
            if client_id in self.client_to_user:
                del self.client_to_user[client_id]
            return False

        websocket = self.active_connections[client_id]
        try:
            message = {"type": "game_event", "event": event_data}
            await websocket.send_text(json.dumps(message))
            print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ✅ Sent game event to user {user_id} (client {client_id})")
            return True
        except Exception as e:
            print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ❌ Error sending game event to user {user_id}: {str(e)}")
            logger.error(f"Error sending game event: {str(e)}", exc_info=True)
            return False
    
    def _check_rate_limit(self, user_id: int) -> bool:
        """Check if user has exceeded rate limit. Returns True if allowed, False if rate limited."""
        now = datetime.now()
        if user_id not in self.message_counts:
            self.message_counts[user_id] = deque()
        
        # Remove timestamps older than 1 second
        while self.message_counts[user_id] and (now - self.message_counts[user_id][0]).total_seconds() > 1.0:
            self.message_counts[user_id].popleft()
        
        # Check if limit exceeded
        if len(self.message_counts[user_id]) >= self.max_messages_per_second:
            return False
        
        # Add current timestamp
        self.message_counts[user_id].append(now)
        return True
    
    async def send_chat_message_to_user(self, from_user_id: int, to_user_id: int, message_data: dict) -> dict:
        """
        Send chat message from one user to another.
        Validates friendship and forwards message.
        Returns dict with status: "success", "error", or "offline"
        """
        # Import here to avoid circular dependency
        from app.api.friends import are_friends
        
        # Validate friendship
        if not are_friends(from_user_id, to_user_id):
            return {
                "status": "error",
                "message": "Users are not friends",
                "code": "NOT_FRIENDS"
            }
        
        # Check rate limiting
        if not self._check_rate_limit(from_user_id):
            return {
                "status": "error",
                "message": "Rate limit exceeded",
                "code": "RATE_LIMIT"
            }
        
        # Check if recipient is online
        if to_user_id not in self.user_to_client:
            return {
                "status": "offline",
                "message": "Recipient is offline",
                "code": "OFFLINE"
            }
        
        client_id = self.user_to_client[to_user_id]
        if client_id not in self.active_connections:
            return {
                "status": "offline",
                "message": "Recipient connection not found",
                "code": "OFFLINE"
            }
        
        websocket = self.active_connections[client_id]
        try:
            # Forward message to recipient
            chat_message = {
                "type": "chat_message",
                "from_user_id": from_user_id,
                "message": message_data.get("message", ""),
                "message_id": message_data.get("message_id", ""),
                "timestamp": message_data.get("timestamp", datetime.now().isoformat())
            }
            
            # Get sender nickname
            try:
                import psycopg2
                from psycopg2.extras import RealDictCursor
                import os
                DATABASE_URL = os.getenv(
                    "DATABASE_URL",
                    "postgresql://tinygame:tinygame123@localhost:5432/tiny_game",
                )
                conn = psycopg2.connect(DATABASE_URL)
                cursor = conn.cursor(cursor_factory=RealDictCursor)
                cursor.execute('''
                    SELECT COALESCE(nickname, username) as display_name
                    FROM users WHERE id = %s
                ''', (from_user_id,))
                result = cursor.fetchone()
                if result:
                    chat_message["from_nickname"] = result['display_name']
                cursor.close()
                conn.close()
            except:
                chat_message["from_nickname"] = f"User{from_user_id}"
            
            await websocket.send_text(json.dumps(chat_message))
            
            # Send delivery confirmation to sender
            await self.send_delivery_status(from_user_id, message_data.get("message_id", ""), "delivered")
            
            print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ✅ Chat message sent from user {from_user_id} to user {to_user_id}")
            return {
                "status": "success",
                "message": "Message sent successfully"
            }
        except Exception as e:
            print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ❌ Error sending chat message: {str(e)}")
            logger.error(f"Error sending chat message: {str(e)}", exc_info=True)
            return {
                "status": "error",
                "message": f"Error: {str(e)}",
                "code": "SEND_ERROR"
            }
    
    async def send_typing_indicator(self, from_user_id: int, to_user_id: int, typing_type: str) -> bool:
        """
        Send typing indicator (typing_start or typing_stop) to recipient.
        typing_type should be "typing_start" or "typing_stop"
        """
        # Validate friendship
        from app.api.friends import are_friends
        if not are_friends(from_user_id, to_user_id):
            return False
        
        # Check if recipient is online
        if to_user_id not in self.user_to_client:
            return False
        
        client_id = self.user_to_client[to_user_id]
        if client_id not in self.active_connections:
            return False
        
        websocket = self.active_connections[client_id]
        try:
            # Get sender nickname
            try:
                import psycopg2
                from psycopg2.extras import RealDictCursor
                import os
                DATABASE_URL = os.getenv(
                    "DATABASE_URL",
                    "postgresql://tinygame:tinygame123@localhost:5432/tiny_game",
                )
                conn = psycopg2.connect(DATABASE_URL)
                cursor = conn.cursor(cursor_factory=RealDictCursor)
                cursor.execute('''
                    SELECT COALESCE(nickname, username) as display_name
                    FROM users WHERE id = %s
                ''', (from_user_id,))
                result = cursor.fetchone()
                sender_nickname = result['display_name'] if result else f"User{from_user_id}"
                cursor.close()
                conn.close()
            except:
                sender_nickname = f"User{from_user_id}"
            
            typing_message = {
                "type": typing_type,
                "from_user_id": from_user_id,
                "from_nickname": sender_nickname
            }
            
            await websocket.send_text(json.dumps(typing_message))
            
            # Track typing state
            if typing_type == "typing_start":
                if from_user_id not in self.typing_users:
                    self.typing_users[from_user_id] = {}
                self.typing_users[from_user_id][to_user_id] = datetime.now()
            elif typing_type == "typing_stop":
                if from_user_id in self.typing_users and to_user_id in self.typing_users[from_user_id]:
                    del self.typing_users[from_user_id][to_user_id]
            
            return True
        except Exception as e:
            print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ❌ Error sending typing indicator: {str(e)}")
            logger.error(f"Error sending typing indicator: {str(e)}", exc_info=True)
            return False
    
    async def send_delivery_status(self, to_user_id: int, message_id: str, status: str) -> bool:
        """
        Send delivery status (delivered) to message sender.
        status should be "delivered" or "read"
        """
        if to_user_id not in self.user_to_client:
            return False
        
        client_id = self.user_to_client[to_user_id]
        if client_id not in self.active_connections:
            return False
        
        websocket = self.active_connections[client_id]
        try:
            status_message = {
                "type": f"message_{status}",
                "message_id": message_id,
                "timestamp": datetime.now().isoformat()
            }
            await websocket.send_text(json.dumps(status_message))
            return True
        except Exception as e:
            print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ❌ Error sending delivery status: {str(e)}")
            logger.error(f"Error sending delivery status: {str(e)}", exc_info=True)
            return False
    
    async def send_read_receipt(self, from_user_id: int, to_user_id: int, message_id: str) -> bool:
        """
        Forward read receipt from recipient to sender.
        """
        # Validate friendship
        from app.api.friends import are_friends
        if not are_friends(from_user_id, to_user_id):
            return False
        
        if to_user_id not in self.user_to_client:
            return False
        
        client_id = self.user_to_client[to_user_id]
        if client_id not in self.active_connections:
            return False
        
        websocket = self.active_connections[client_id]
        try:
            read_receipt = {
                "type": "message_read",
                "message_id": message_id,
                "timestamp": datetime.now().isoformat()
            }
            await websocket.send_text(json.dumps(read_receipt))
            return True
        except Exception as e:
            print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ❌ Error sending read receipt: {str(e)}")
            logger.error(f"Error sending read receipt: {str(e)}", exc_info=True)
            return False
    
    async def send_status_update_to_friends(self, user_id: int, status: str = "online"):
        """
        Send status update (online/offline) to all friends of a user.
        Only sends to friends who are currently online.
        """
        # Import here to avoid circular dependency
        from app.api.auth import get_friend_ids
        
        try:
            # Get list of friend IDs
            friend_ids = get_friend_ids(user_id)
            
            if not friend_ids:
                print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ℹ️ User {user_id} has no friends, skipping status update")
                return
            
            print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] 📤 Sending status update to {len(friend_ids)} friends of user {user_id}")
            print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] 📋 Friend IDs: {friend_ids}")
            print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] 📋 Online users: {list(self.user_to_client.keys())}")
            
            # Prepare status update message
            status_message = {
                "type": "user_status_update",
                "user_id": user_id,
                "status": status,
                "timestamp": datetime.now().isoformat()
            }
            message_json = json.dumps(status_message)
            
            # Send to each friend who is online
            notified_count = 0
            for friend_id in friend_ids:
                if friend_id in self.user_to_client:
                    client_id = self.user_to_client[friend_id]
                    if client_id in self.active_connections:
                        websocket = self.active_connections[client_id]
                        try:
                            await websocket.send_text(message_json)
                            notified_count += 1
                            print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ✅ Sent status update to friend {friend_id} (client {client_id})")
                        except Exception as e:
                            print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ⚠️ Error sending status update to friend {friend_id}: {str(e)}")
                    else:
                        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ⚠️ Friend {friend_id} mapped to client {client_id} but client not in active_connections")
                else:
                    print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ⚠️ Friend {friend_id} not online (not in user_to_client)")
            
            print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ✅ Status update sent to {notified_count}/{len(friend_ids)} online friends")
            
        except Exception as e:
            print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ❌ Error in send_status_update_to_friends: {str(e)}")
            logger.error(f"Error sending status update to friends: {str(e)}", exc_info=True)
            # Don't raise exception, just log error

    async def send_online_friends_to_user(self, user_id: int):
        """
        Sync presence state: send "online" updates of currently-online friends to the given user.

        This fixes the one-way broadcast issue:
        - If B is online first, then A comes online later, A must learn that B is already online.
        """
        from app.api.auth import get_friend_ids

        try:
            # Ensure target user is connected
            if user_id not in self.user_to_client:
                return
            target_client_id = self.user_to_client[user_id]
            if target_client_id not in self.active_connections:
                return
            target_ws = self.active_connections[target_client_id]

            friend_ids = get_friend_ids(user_id)
            if not friend_ids:
                return

            sent = 0
            for friend_id in friend_ids:
                # Only report friends who are online (mapped + active connection exists)
                if friend_id in self.user_to_client:
                    friend_client_id = self.user_to_client[friend_id]
                    if friend_client_id in self.active_connections:
                        msg = {
                            "type": "user_status_update",
                            "user_id": friend_id,
                            "status": "online",
                            "timestamp": datetime.now().isoformat()
                        }
                        try:
                            await target_ws.send_text(json.dumps(msg))
                            sent += 1
                        except Exception as e:
                            print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ⚠️ Error syncing online friend {friend_id} to user {user_id}: {str(e)}")

            if sent > 0:
                print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ✅ Synced {sent} online friends to user {user_id}")

        except Exception as e:
            print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ❌ Error in send_online_friends_to_user: {str(e)}")
            logger.error(f"Error syncing online friends: {str(e)}", exc_info=True)
    
    async def cleanup_typing_indicators(self):
        """Clean up expired typing indicators (auto-timeout after 5 seconds)"""
        now = datetime.now()
        users_to_clean = []
        
        for user_id, typing_dict in self.typing_users.items():
            for friend_id, last_typing_time in list(typing_dict.items()):
                elapsed = (now - last_typing_time).total_seconds()
                if elapsed > 5.0:  # 5 seconds timeout
                    # Auto-send typing_stop
                    await self.send_typing_indicator(user_id, friend_id, "typing_stop")
                    users_to_clean.append((user_id, friend_id))
        
        # Clean up
        for user_id, friend_id in users_to_clean:
            if user_id in self.typing_users and friend_id in self.typing_users[user_id]:
                del self.typing_users[user_id][friend_id]
    
    async def handle_message(self, websocket: WebSocket, message: str, client_id: str):
        try:
            data = json.loads(message)
            message_type = data.get("type", "unknown")
            
            # Handle ping for keep-alive
            if message_type == "ping":
                # Send pong response
                pong = {
                    "type": "pong",
                    "timestamp": datetime.now().isoformat()
                }
                await websocket.send_text(json.dumps(pong))
                # Don't log ping/pong to reduce noise
                return
            
            # Handle pong (acknowledgment from client)
            if message_type == "pong":
                # Connection is alive, no action needed
                return
            
            # Handle init message
            if message_type == "init":
                device = data.get("device", "unknown")
                user_id = data.get("user_id", None)
                client_ip = websocket.client.host if websocket.client else "unknown"
                
                print("=" * 60)
                print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ⚡ INIT SOCKET MESSAGE RECEIVED ⚡")
                print(f"  Client ID: {client_id}")
                print(f"  Client IP: {client_ip}")
                print(f"  Device: {device}")
                if user_id is not None:
                    print(f"  User ID: {user_id}")
                print(f"  Full Message:")
                print(json.dumps(data, indent=4))
                print("=" * 60)
                
                # Store user_id mapping if provided
                if user_id is not None:
                    # Remove old mapping if user_id was already connected
                    if user_id in self.user_to_client:
                        old_client_id = self.user_to_client[user_id]
                        if old_client_id in self.client_to_user:
                            del self.client_to_user[old_client_id]
                    # Store new mapping
                    self.user_to_client[user_id] = client_id
                    self.client_to_user[client_id] = user_id
                    print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] Mapped user_id {user_id} to client_id {client_id}")
                    
                    # 1) Broadcast: notify online friends that this user is online
                    try:
                        await self.send_status_update_to_friends(user_id)
                        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ✅ Auto-sent status update to friends of user {user_id}")
                    except Exception as status_error:
                        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ⚠️ Warning: Failed to auto-send status update: {str(status_error)}")

                    # 2) Sync: tell this user which friends are already online
                    try:
                        await self.send_online_friends_to_user(user_id)
                    except Exception as sync_error:
                        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ⚠️ Warning: Failed to sync online friends: {str(sync_error)}")
                
                # Send acknowledgment
                ack = {
                    "type": "init_ack",
                    "status": "success",
                    "message": "Socket initialized successfully",
                    "timestamp": datetime.now().isoformat()
                }
                await websocket.send_text(json.dumps(ack))
                print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] Sent init acknowledgment to {client_id}")
            
            # Handle chat message
            elif message_type == "chat_message":
                sender_id = self.client_to_user.get(client_id)
                if not sender_id:
                    error_response = {
                        "type": "chat_error",
                        "message": "User not authenticated",
                        "code": "NOT_AUTHENTICATED"
                    }
                    await websocket.send_text(json.dumps(error_response))
                    return
                
                to_user_id = data.get("to_user_id")
                message_text = data.get("message", "").strip()
                message_id = data.get("message_id", "")
                timestamp = data.get("timestamp", datetime.now().isoformat())
                
                # Validate message
                if not to_user_id:
                    error_response = {
                        "type": "chat_error",
                        "message": "Missing to_user_id",
                        "code": "INVALID_RECIPIENT"
                    }
                    await websocket.send_text(json.dumps(error_response))
                    return
                
                if not message_text:
                    error_response = {
                        "type": "chat_error",
                        "message": "Message cannot be empty",
                        "code": "EMPTY_MESSAGE"
                    }
                    await websocket.send_text(json.dumps(error_response))
                    return
                
                if len(message_text) > 500:
                    error_response = {
                        "type": "chat_error",
                        "message": "Message too long (max 500 characters)",
                        "code": "MESSAGE_TOO_LONG"
                    }
                    await websocket.send_text(json.dumps(error_response))
                    return
                
                # Send chat message
                result = await self.send_chat_message_to_user(
                    sender_id,
                    to_user_id,
                    {
                        "message": message_text,
                        "message_id": message_id,
                        "timestamp": timestamp
                    }
                )
                
                # Send error response to sender if failed
                if result.get("status") != "success":
                    error_response = {
                        "type": "chat_error",
                        "message": result.get("message", "Failed to send message"),
                        "code": result.get("code", "UNKNOWN_ERROR")
                    }
                    await websocket.send_text(json.dumps(error_response))
            
            # Handle typing indicators
            elif message_type in ["typing_start", "typing_stop"]:
                sender_id = self.client_to_user.get(client_id)
                if not sender_id:
                    return  # Silently ignore if not authenticated
                
                to_user_id = data.get("to_user_id")
                if not to_user_id:
                    return  # Silently ignore invalid typing indicator
                
                await self.send_typing_indicator(sender_id, to_user_id, message_type)
            
            # Handle read receipt
            elif message_type == "read_receipt":
                sender_id = self.client_to_user.get(client_id)
                if not sender_id:
                    return  # Silently ignore if not authenticated
                
                to_user_id = data.get("to_user_id")
                message_id = data.get("message_id", "")
                
                if not to_user_id or not message_id:
                    return  # Silently ignore invalid read receipt
                
                await self.send_read_receipt(sender_id, to_user_id, message_id)
            
            else:
                # Handle other message types
                print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] Received message type: {message_type} from {client_id}")
                if message_type not in ["ping", "pong"]:  # Don't log ping/pong
                    print(f"  Message: {json.dumps(data, indent=2)}")
                
        except json.JSONDecodeError as e:
            print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ❌ Invalid JSON received from {client_id}")
            print(f"  Error: {str(e)}")
            print(f"  Raw message: {message}")
            error_response = {
                "type": "error",
                "message": "Invalid JSON format",
                "error": str(e)
            }
            await websocket.send_text(json.dumps(error_response))
        except Exception as e:
            print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ❌ Error handling message from {client_id}: {str(e)}")
            logger.error(f"WebSocket error: {str(e)}", exc_info=True)

# Global WebSocket manager instance
websocket_manager = WebSocketManager()

async def websocket_endpoint(websocket: WebSocket, client_id: str = None):
    """
    WebSocket endpoint for handling client connections.
    Expects init message format: {"type":"init","device":"ESP32"}
    """
    # Generate client ID if not provided
    if client_id is None:
        client_ip = websocket.client.host if websocket.client else "unknown"
        client_id = f"{client_ip}_{datetime.now().timestamp()}"
    
    print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ========================================")
    print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] 🔌 WebSocket endpoint processing")
    print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}]   Client IP: {client_ip}")
    print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}]   Client ID: {client_id}")
    print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ========================================")
    
    await websocket_manager.connect(websocket, client_id)
    
    try:
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ✅ WebSocket connection established, waiting for messages...")
        while True:
            data = await websocket.receive_text()
            print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] 📨 Received raw message from {client_id}: {data}")
            await websocket_manager.handle_message(websocket, data, client_id)
            
            # Cleanup typing indicators periodically (every message)
            await websocket_manager.cleanup_typing_indicators()
            
    except WebSocketDisconnect:
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] 🔌 WebSocket disconnected: {client_id}")
        await websocket_manager.disconnect(client_id)
    except Exception as e:
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ❌ WebSocket exception: {str(e)}")
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] Exception type: {type(e).__name__}")
        import traceback
        print(traceback.format_exc())
        logger.error(f"WebSocket exception: {str(e)}", exc_info=True)
        await websocket_manager.disconnect(client_id)

