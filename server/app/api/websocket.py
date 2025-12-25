from fastapi import APIRouter, WebSocket, WebSocketDisconnect, Depends
from fastapi.websockets import WebSocketState
from app.websocket.esp32_handler import ESP32ConnectionManager
from app.services.auth_service import verify_credentials, validate_username
from app.models.auth import LoginCredentials
from app.utils.message_parser import parse_message, build_message
import logging

logger = logging.getLogger(__name__)
router = APIRouter()

# Connection manager instance
manager = ESP32ConnectionManager()

@router.websocket("/ws/esp32")
async def websocket_endpoint(websocket: WebSocket):
    """
    WebSocket endpoint for ESP32 connections.
    ESP32 should connect and send login credentials first.
    Format: "type:login-*-credentials:username:xxx-pin:yyy"
    """
    client_host = websocket.client.host if websocket.client else "unknown"
    logger.info(f"[WEBSOCKET] New connection from {client_host}")
    await manager.connect(websocket)
    
    try:
        # Wait for first message (can be validate_username or login)
        first_message = await websocket.receive_text()
        logger.info(f"[REQUEST] First message received: {first_message}")
        data = parse_message(first_message)
        logger.info(f"[REQUEST] Parsed data: {data}")
        msg_type = data.get("type")
        logger.info(f"[REQUEST] Message type: {msg_type}")
        
        # Handle username validation request
        if msg_type == "validate_username":
            username = data.get("username", "")
            logger.info(f"[VALIDATE_USERNAME] Request for username: {username}")
            if not username:
                logger.warning(f"[VALIDATE_USERNAME] Username required but not provided")
                await manager.send_error(websocket, "Username required")
                # Don't disconnect, wait for login
            else:
                is_valid, message = await validate_username(username)
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
                # Don't disconnect, wait for login message
            
            # Continue to wait for login message
            login_data = await websocket.receive_text()
            logger.info(f"[REQUEST] Login message received: {login_data}")
            data = parse_message(login_data)
            logger.info(f"[REQUEST] Parsed login data: {data}")
            msg_type = data.get("type")
            logger.info(f"[REQUEST] Login message type: {msg_type}")
        
        if msg_type != "login":
            await manager.send_error(websocket, "Login message required")
            await manager.disconnect(websocket)
            return
        
        # Parse username and pin from separate fields
        username = data.get("username", "")
        pin = data.get("pin", "")
        
        logger.info(f"[LOGIN] Login request received")
        logger.info(f"[LOGIN] Username: {username}, PIN: {'*' * len(pin) if pin else 'empty'}")
        
        if not username or not pin:
            logger.warning(f"[LOGIN] Missing credentials - Username: {bool(username)}, PIN: {bool(pin)}")
            await manager.send_error(websocket, "Username and PIN required")
            await manager.disconnect(websocket)
            return
        
        # Verify credentials (returns tuple: (is_valid, error_message))
        is_valid, error_message = await verify_credentials(username, pin)
        logger.info(f"[LOGIN] Validation result: valid={is_valid}, message={error_message}")
        
        if not is_valid:
            logger.warning(f"[LOGIN] Authentication failed: {error_message}")
            await manager.send_error(websocket, error_message)
            await manager.disconnect(websocket)
            return
        
        # Register session
        session_id = await manager.register_session(websocket, username)
        logger.info(f"[LOGIN] Session registered: {session_id}")
        
        # Send success message using string pattern
        response = build_message(
            type="login_success",
            session_id=session_id,
            message="Authentication successful"
        )
        await websocket.send_text(response)
        logger.info(f"[RESPONSE] Login success sent, session_id: {session_id}")
        
        logger.info(f"ESP32 connected: {username} (session: {session_id})")
        
        # Keep connection alive and handle messages
        while True:
            try:
                # Receive message from ESP32
                message = await websocket.receive_text()
                logger.info(f"[REQUEST] Message received from session {session_id}: {message}")
                data = parse_message(message)
                logger.info(f"[REQUEST] Parsed data: {data}")
                
                # Handle different message types
                await manager.handle_message(websocket, data, session_id)
                
            except WebSocketDisconnect:
                logger.info(f"[WEBSOCKET] Client disconnected: {client_host}")
                break
            except Exception as e:
                logger.error(f"[ERROR] Error handling message from session {session_id}: {e}")
                await manager.send_error(websocket, f"Error: {str(e)}")
        
    except WebSocketDisconnect:
        logger.info(f"[WEBSOCKET] ESP32 disconnected: {client_host}")
    except Exception as e:
        logger.error(f"[ERROR] WebSocket error from {client_host}: {e}")
        await manager.send_error(websocket, f"Server error: {str(e)}")
    finally:
        await manager.disconnect(websocket)

