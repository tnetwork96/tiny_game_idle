from fastapi import APIRouter, HTTPException
from app.websocket.esp32_handler import manager
from app.services.auth_service import verify_credentials, add_user, get_user, user_exists
from app.models.auth import LoginRequest, UserResponse

router = APIRouter()

@router.post("/login")
async def login(credentials: LoginRequest):
    """HTTP login endpoint (alternative to WebSocket)"""
    is_valid, error_message = await verify_credentials(credentials.username, credentials.pin)
    if not is_valid:
        raise HTTPException(status_code=401, detail=error_message)
    return {"status": "success", "message": "Login successful"}

@router.post("/register")
async def register(credentials: LoginRequest):
    """Register a new user"""
    if user_exists(credentials.username):
        raise HTTPException(status_code=400, detail="Username already exists")
    
    success = add_user(credentials.username, credentials.pin)
    if not success:
        raise HTTPException(status_code=500, detail="Failed to create user")
    
    return {"status": "success", "message": "User registered successfully"}

@router.get("/users/{username}")
async def get_user_info(username: str):
    """Get user information"""
    user = get_user(username)
    if not user:
        raise HTTPException(status_code=404, detail="User not found")
    return user

@router.get("/sessions")
async def get_sessions():
    """Get list of active WebSocket sessions"""
    return {"sessions": manager.get_active_sessions()}

@router.get("/sessions/count")
async def get_session_count():
    """Get count of active sessions"""
    return {"count": len(manager.active_connections)}

