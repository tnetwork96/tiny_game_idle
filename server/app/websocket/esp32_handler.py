from fastapi import WebSocket
from typing import Dict, List
from sqlalchemy.orm import Session
from app.database import SessionLocal
from app.models.db_models import Session as DBSession, User
from app.utils.message_parser import build_message
from app.services.auth_service import validate_username
import logging
import uuid
from datetime import datetime

logger = logging.getLogger(__name__)

class ESP32ConnectionManager:
    def __init__(self):
        # Active connections: {session_id: websocket}
        self.active_connections: Dict[str, WebSocket] = {}
        # Session info: {session_id: {"username": str, "connected_at": datetime}}
        self.sessions: Dict[str, Dict] = {}
        # Username to session mapping: {username: session_id}
        self.username_to_session: Dict[str, str] = {}
    
    async def connect(self, websocket: WebSocket):
        """Accept WebSocket connection"""
        await websocket.accept()
        client_host = websocket.client.host if websocket.client else "unknown"
        logger.info(f"[CONNECTION] WebSocket accepted from {client_host}")
    
    async def disconnect(self, websocket: WebSocket):
        """Disconnect and cleanup session"""
        # Find session by websocket
        session_id = None
        for sid, ws in self.active_connections.items():
            if ws == websocket:
                session_id = sid
                break
        
        if session_id:
            session_info = self.sessions.get(session_id, {})
            username = session_info.get("username", "unknown")
            connected_at = session_info.get("connected_at")
            
            # Calculate connection duration if available
            duration_str = "unknown"
            if connected_at:
                duration = datetime.now() - connected_at
                duration_str = f"{duration.total_seconds():.1f}s"
            
            # Update session in database
            db: Session = SessionLocal()
            try:
                db_session = db.query(DBSession).filter(DBSession.session_id == session_id).first()
                if db_session:
                    db_session.is_active = False
                    db_session.disconnected_at = datetime.now()
                    db.commit()
                    logger.info(f"[DISCONNECTION] Session deactivated in database: {session_id}")
            except Exception as e:
                logger.error(f"[ERROR] Error updating session in database: {e}")
                db.rollback()
            finally:
                db.close()
            
            # Cleanup in-memory
            del self.active_connections[session_id]
            del self.sessions[session_id]
            if username in self.username_to_session:
                del self.username_to_session[username]
            
            logger.info(f"[DISCONNECTION] Session: {session_id}, User: {username}, Duration: {duration_str}")
        
        try:
            await websocket.close()
        except:
            pass
    
    async def register_session(self, websocket: WebSocket, username: str) -> str:
        """Register a new session after successful login"""
        session_id = str(uuid.uuid4())
        
        db: Session = SessionLocal()
        try:
            # Get user from database
            user = db.query(User).filter(User.username == username).first()
            if not user:
                logger.error(f"User not found for session: {username}")
                return session_id
            
            # Disconnect existing session for same username if any
            if username in self.username_to_session:
                old_session = self.username_to_session[username]
                if old_session in self.active_connections:
                    old_ws = self.active_connections[old_session]
                    await self.disconnect(old_ws)
            
            # Deactivate old sessions in database
            db.query(DBSession).filter(
                DBSession.user_id == user.id,
                DBSession.is_active == True
            ).update({"is_active": False, "disconnected_at": datetime.now()})
            db.commit()
            
            # Register new session in memory
            self.active_connections[session_id] = websocket
            self.sessions[session_id] = {
                "username": username,
                "user_id": user.id,
                "connected_at": datetime.now(),
                "last_activity": datetime.now()
            }
            self.username_to_session[username] = session_id
            
            # Store session in database
            db_session = DBSession(
                session_id=session_id,
                user_id=user.id,
                connected_at=datetime.now(),
                last_activity=datetime.now(),
                is_active=True
            )
            db.add(db_session)
            db.commit()
            
            logger.info(f"Session registered in database: {session_id} for user {username}")
        except Exception as e:
            logger.error(f"Error registering session in database: {e}")
            db.rollback()
        finally:
            db.close()
        
        return session_id
    
    async def send_message(self, websocket: WebSocket, message: dict):
        """Send string pattern message to ESP32"""
        try:
            if websocket.client_state.name == "CONNECTED":
                message_str = build_message(**message)
                await websocket.send_text(message_str)
        except Exception as e:
            logger.error(f"Error sending message: {e}")
    
    async def send_error(self, websocket: WebSocket, error_message: str):
        """Send error message to ESP32"""
        await self.send_message(websocket, {
            "type": "error",
            "message": error_message
        })
    
    async def handle_message(self, websocket: WebSocket, data: dict, session_id: str):
        """Handle incoming message from ESP32"""
        message_type = data.get("type")
        username = self.sessions.get(session_id, {}).get("username", "unknown")
        now = datetime.now()
        
        logger.info(f"[MESSAGE] Session: {session_id}, User: {username}, Type: {message_type}")
        logger.info(f"[MESSAGE] Full data: {data}")
        
        # Update last activity in memory
        if session_id in self.sessions:
            self.sessions[session_id]["last_activity"] = now
        
        # Update last activity in database
        db: Session = SessionLocal()
        try:
            db_session = db.query(DBSession).filter(DBSession.session_id == session_id).first()
            if db_session:
                db_session.last_activity = now
                db.commit()
        except Exception as e:
            logger.error(f"Error updating session activity: {e}")
            db.rollback()
        finally:
            db.close()
        
        # Handle different message types
        if message_type == "validate_username":
            # Handle username validation (before login)
            username_to_validate = data.get("username", "")
            logger.info(f"[VALIDATE_USERNAME] Request for username: {username_to_validate}")
            if not username_to_validate:
                logger.warning(f"[VALIDATE_USERNAME] Username required but not provided")
                await self.send_error(websocket, "Username required")
            else:
                is_valid, message = await validate_username(username_to_validate)
                logger.info(f"[VALIDATE_USERNAME] Validation result: valid={is_valid}, message={message}")
                if is_valid:
                    response = build_message(
                        type="username_valid",
                        message="Username exists"
                    )
                else:
                    response = build_message(
                        type="error",
                        message=message
                    )
                await websocket.send_text(response)
                logger.info(f"[RESPONSE] Username validation response sent: valid={is_valid}")
        
        elif message_type == "ping":
            logger.info(f"[PING] Ping received from {username}")
            response = build_message(type="pong")
            await websocket.send_text(response)
            logger.info(f"[RESPONSE] Sending pong")
        
        elif message_type == "game_action":
            # Handle game-related actions
            action = data.get("action", "")
            logger.info(f"[GAME] Action received from {username}: {action}")
            response = build_message(
                type="game_response",
                action=action,
                status="received"
            )
            await websocket.send_text(response)
            logger.info(f"[RESPONSE] Game response sent")
        
        elif message_type == "chat_message":
            # Handle chat messages
            message_text = data.get("message", "")
            logger.info(f"[CHAT] Message from {username}: {message_text}")
            response = build_message(
                type="chat_ack",
                message="Message received"
            )
            await websocket.send_text(response)
            logger.info(f"[RESPONSE] Chat acknowledgment sent")
        
        else:
            logger.warning(f"[MESSAGE] Unknown message type: {message_type} from {username}")
            await self.send_error(websocket, f"Unknown message type: {message_type}")
    
    async def broadcast_to_all(self, message: dict):
        """Broadcast message to all connected ESP32 devices"""
        disconnected = []
        for session_id, websocket in self.active_connections.items():
            try:
                await self.send_message(websocket, message)
            except:
                disconnected.append(session_id)
        
        # Cleanup disconnected sessions
        for session_id in disconnected:
            if session_id in self.sessions:
                username = self.sessions[session_id].get("username", "unknown")
                del self.active_connections[session_id]
                del self.sessions[session_id]
                if username in self.username_to_session:
                    del self.username_to_session[username]
    
    def get_active_sessions(self) -> List[Dict]:
        """Get list of active sessions"""
        return [
            {
                "session_id": sid,
                "username": info["username"],
                "connected_at": info["connected_at"].isoformat(),
                "last_activity": info["last_activity"].isoformat()
            }
            for sid, info in self.sessions.items()
        ]

# Global instance
manager = ESP32ConnectionManager()

