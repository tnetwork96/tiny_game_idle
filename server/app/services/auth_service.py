from typing import Optional, Tuple
from sqlalchemy.orm import Session
from app.database import SessionLocal
from app.models.db_models import User
from datetime import datetime
import logging

logger = logging.getLogger(__name__)

async def verify_credentials(username: str, pin: str) -> Tuple[bool, str]:
    """
    Verify login credentials against database.
    Returns: (is_valid, error_message)
    Error messages: "Username not found", "Invalid PIN", "Account inactive", "Success"
    """
    if not username or not pin:
        return False, "Username and PIN required"
    
    db: Session = SessionLocal()
    try:
        user = db.query(User).filter(User.username == username).first()
        
        if not user:
            logger.warning(f"Login attempt with unknown username: {username}")
            return False, "Username not found"
        
        if not user.active:
            logger.warning(f"Login attempt with inactive user: {username}")
            return False, "Account inactive"
        
        if user.pin != pin:
            logger.warning(f"Invalid PIN for user: {username}")
            return False, "Invalid PIN"
        
        # Update last login
        user.last_login = datetime.now()
        db.commit()
        
        logger.info(f"Successful login: {username}")
        return True, "Success"
    except Exception as e:
        logger.error(f"Database error during login: {e}")
        db.rollback()
        return False, "Database error"
    finally:
        db.close()

def add_user(username: str, pin: str, nickname: Optional[str] = None) -> bool:
    """Add a new user to database"""
    db: Session = SessionLocal()
    try:
        # Check if user already exists
        existing_user = db.query(User).filter(User.username == username).first()
        if existing_user:
            logger.warning(f"User already exists: {username}")
            return False
        
        # Create new user
        new_user = User(
            username=username,
            pin=pin,
            nickname=nickname or username,
            active=True
        )
        db.add(new_user)
        db.commit()
        logger.info(f"User created: {username}")
        return True
    except Exception as e:
        logger.error(f"Error creating user: {e}")
        db.rollback()
        return False
    finally:
        db.close()

def get_user(username: str) -> Optional[dict]:
    """Get user information from database"""
    db: Session = SessionLocal()
    try:
        user = db.query(User).filter(User.username == username).first()
        if user:
            return {
                "id": user.id,
                "username": user.username,
                "nickname": user.nickname,
                "active": user.active,
                "created_at": user.created_at.isoformat() if user.created_at else None,
                "last_login": user.last_login.isoformat() if user.last_login else None
            }
        return None
    except Exception as e:
        logger.error(f"Error getting user: {e}")
        return None
    finally:
        db.close()

def user_exists(username: str) -> bool:
    """Check if username exists in database"""
    db: Session = SessionLocal()
    try:
        user = db.query(User).filter(User.username == username).first()
        return user is not None
    except Exception as e:
        logger.error(f"Error checking user existence: {e}")
        return False
    finally:
        db.close()

async def validate_username(username: str) -> Tuple[bool, str]:
    """
    Validate if username exists in database.
    Returns: (is_valid, message)
    - is_valid: True if username exists and is active, False otherwise
    - message: "Username exists" or error message
    """
    if not username:
        return False, "Username required"
    
    db: Session = SessionLocal()
    try:
        user = db.query(User).filter(User.username == username).first()
        
        if not user:
            logger.info(f"Username validation failed: {username} not found")
            return False, "Username not found"
        
        if not user.active:
            logger.info(f"Username validation failed: {username} is inactive")
            return False, "Account inactive"
        
        logger.info(f"Username validation successful: {username}")
        return True, "Username exists"
    except Exception as e:
        logger.error(f"Database error during username validation: {e}")
        db.rollback()
        return False, "Database error"
    finally:
        db.close()

