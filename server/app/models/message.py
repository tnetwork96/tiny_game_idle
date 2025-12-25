from pydantic import BaseModel
from typing import Optional, Dict, Any

class WebSocketMessage(BaseModel):
    type: str
    data: Optional[Dict[str, Any]] = None
    message: Optional[str] = None
    action: Optional[str] = None

class LoginMessage(BaseModel):
    type: str = "login"
    credentials: str

class PingMessage(BaseModel):
    type: str = "ping"

class GameActionMessage(BaseModel):
    type: str = "game_action"
    action: str
    game_type: Optional[str] = None
    data: Optional[Dict[str, Any]] = None

class ChatMessage(BaseModel):
    type: str = "chat_message"
    message: str
    recipient: Optional[str] = None

