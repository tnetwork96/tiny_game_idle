from fastapi import WebSocket, WebSocketDisconnect
from datetime import datetime
import json
import logging
from typing import Dict

logger = logging.getLogger(__name__)

class WebSocketManager:
    def __init__(self):
        self.active_connections: Dict[str, WebSocket] = {}  # client_id -> WebSocket
        self.user_to_client: Dict[int, str] = {}  # user_id -> client_id
        self.client_to_user: Dict[str, int] = {}  # client_id -> user_id
    
    async def connect(self, websocket: WebSocket, client_id: str):
        await websocket.accept()
        self.active_connections[client_id] = websocket
        client_ip = websocket.client.host if websocket.client else "unknown"
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] WebSocket client connected")
        print(f"  Client ID: {client_id}")
        print(f"  Client IP: {client_ip}")
    
    async def disconnect(self, client_id: str):
        if client_id in self.active_connections:
            del self.active_connections[client_id]
            # Remove user_id mapping if exists
            if client_id in self.client_to_user:
                user_id = self.client_to_user[client_id]
                if user_id in self.user_to_client and self.user_to_client[user_id] == client_id:
                    del self.user_to_client[user_id]
                del self.client_to_user[client_id]
            print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] WebSocket client disconnected: {client_id}")
    
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
                
                # Send acknowledgment
                ack = {
                    "type": "init_ack",
                    "status": "success",
                    "message": "Socket initialized successfully",
                    "timestamp": datetime.now().isoformat()
                }
                await websocket.send_text(json.dumps(ack))
                print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] Sent init acknowledgment to {client_id}")
                
            else:
                # Handle other message types
                print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] Received message type: {message_type} from {client_id}")
                if message_type != "ping" and message_type != "pong":  # Don't log ping/pong
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

