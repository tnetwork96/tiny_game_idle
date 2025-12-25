from pydantic import BaseModel
from typing import Optional

class LoginRequest(BaseModel):
    username: str
    pin: str

class LoginCredentials(BaseModel):
    credentials: str  # Format: "username:xxx-pin:yyy"

class UserResponse(BaseModel):
    username: str
    active: bool

class SessionInfo(BaseModel):
    session_id: str
    username: str
    connected_at: str
    last_activity: str

