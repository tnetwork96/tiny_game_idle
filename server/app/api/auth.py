from fastapi import APIRouter, HTTPException
from pydantic import BaseModel
import psycopg2
from psycopg2.extras import RealDictCursor
import hashlib
import os
from typing import Optional
from datetime import datetime

router = APIRouter(prefix="/api", tags=["auth"])

class LoginRequest(BaseModel):
    username: str
    pin: str

class LoginResponse(BaseModel):
    success: bool
    message: str
    user_id: Optional[int] = None
    username: Optional[str] = None
    account_exists: Optional[bool] = None  # True if account exists (even if PIN wrong), False if not found

class RegisterRequest(BaseModel):
    username: str
    pin: str

class RegisterResponse(BaseModel):
    success: bool
    message: str
    user_id: Optional[int] = None
    username: Optional[str] = None

class FriendResponse(BaseModel):
    username: str
    online: bool = False  # Default to offline, can be updated later

class FriendsListResponse(BaseModel):
    success: bool
    friends: list[FriendResponse]
    message: Optional[str] = None

# Database connection string from environment
DATABASE_URL = os.getenv("DATABASE_URL", "postgresql://tinygame:tinygame123@db:5432/tiny_game")

def get_db_connection():
    """Get PostgreSQL database connection"""
    return psycopg2.connect(DATABASE_URL)

@router.post("/login", response_model=LoginResponse)
async def login(request: LoginRequest):
    """
    Login endpoint - checks username and PIN against PostgreSQL database
    All credentials are verified against database, no hardcoded values
    """
    conn = None
    try:
        # Connect to PostgreSQL database
        conn = get_db_connection()
        cursor = conn.cursor(cursor_factory=RealDictCursor)
        
        # Hash the PIN input (never log plaintext PIN)
        pin_hash = hashlib.sha256(request.pin.encode()).hexdigest()
        
        # Query database for username (all credentials come from database)
        cursor.execute('SELECT id, username, pin FROM users WHERE username = %s', (request.username,))
        user_row = cursor.fetchone()
        
        if user_row:
            # Username exists in database, check PIN hash
            user_id = user_row['id']
            username = user_row['username']
            stored_pin_hash = user_row['pin']
            
            if stored_pin_hash == pin_hash:
                # Correct credentials - verified against database
                print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ✅ Login successful: {username} (ID: {user_id})")
                return LoginResponse(
                    success=True,
                    message=f"Login successful for {username}",
                    user_id=user_id,
                    username=username,
                    account_exists=True
                )
            else:
                # Wrong PIN - account exists but PIN doesn't match database
                print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ❌ Login failed: Wrong PIN for {request.username}")
                return LoginResponse(
                    success=False,
                    message="Invalid PIN",
                    account_exists=True  # Account exists but PIN is wrong
                )
        else:
            # Username doesn't exist in database
            print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ❌ Login failed: Account not found: {request.username}")
            return LoginResponse(
                success=False,
                message="Account not found",
                account_exists=False  # Account doesn't exist
            )
            
    except HTTPException:
        raise
    except Exception as e:
        import traceback
        error_trace = traceback.format_exc()
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ❌ Login error: {str(e)}")
        print(f"Traceback: {error_trace}")
        raise HTTPException(status_code=500, detail=str(e))
    finally:
        # Ensure database connection is always closed
        if conn:
            conn.close()

@router.post("/register", response_model=RegisterResponse)
async def register(request: RegisterRequest):
    """
    Register/Create new account endpoint - stores in PostgreSQL database
    All account creation is stored in database, no hardcoded values
    """
    conn = None
    try:
        # Connect to PostgreSQL database
        conn = get_db_connection()
        cursor = conn.cursor()
        
        # Check if username already exists in database
        cursor.execute('SELECT id FROM users WHERE username = %s', (request.username,))
        existing_user = cursor.fetchone()
        
        if existing_user:
            print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ❌ Register failed: Username already exists: {request.username}")
            return RegisterResponse(
                success=False,
                message=f"Username '{request.username}' already exists"
            )
        
        # Hash the PIN before storing (never store plaintext PIN)
        pin_hash = hashlib.sha256(request.pin.encode()).hexdigest()
        
        # Insert new user into database
        cursor.execute('''
            INSERT INTO users (username, pin) 
            VALUES (%s, %s)
            RETURNING id
        ''', (request.username, pin_hash))
        
        user_id = cursor.fetchone()[0]
        conn.commit()
        
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ✅ Account created: {request.username} (ID: {user_id})")
        return RegisterResponse(
            success=True,
            message=f"Account created successfully for {request.username}",
            user_id=user_id,
            username=request.username
        )
            
    except HTTPException:
        raise
    except psycopg2.IntegrityError as e:
        if conn:
            conn.rollback()
        # Handle database integrity errors (e.g., unique constraint violation)
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ❌ Register failed: Database integrity error: {str(e)}")
        raise HTTPException(status_code=400, detail="Username already exists or database constraint violation")
    except Exception as e:
        if conn:
            conn.rollback()
        import traceback
        error_trace = traceback.format_exc()
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ❌ Register error: {str(e)}")
        print(f"Traceback: {error_trace}")
        raise HTTPException(status_code=500, detail=str(e))
    finally:
        # Ensure database connection is always closed
        if conn:
            conn.close()

@router.get("/friends/{user_id}/list")
async def get_friends_list(user_id: int):
    """
    Get list of friends as simple string format for ESP32
    Format: "username1,online1|username2,online2|..."
    Example: "user123,0|admin,0|test,1|"
    online: 0 = offline, 1 = online
    """
    conn = None
    try:
        conn = get_db_connection()
        cursor = conn.cursor(cursor_factory=RealDictCursor)
        
        # Get all accepted friends for this user
        cursor.execute('''
            SELECT u.username 
            FROM friends f
            JOIN users u ON f.friend_id = u.id
            WHERE f.user_id = %s AND f.status = 'accepted'
            ORDER BY u.username
        ''', (user_id,))
        
        friends_list = []
        for row in cursor.fetchall():
            username = row['username']
            online = "0"  # Default to offline (0), can be updated later with WebSocket status
            friends_list.append(f"{username},{online}")
        
        # Join with | separator
        result_string = "|".join(friends_list)
        if result_string:
            result_string += "|"  # Add trailing | for easier parsing
        
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ✅ Friends list string for user_id {user_id}: {result_string}")
        return result_string
            
    except Exception as e:
        import traceback
        error_trace = traceback.format_exc()
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ❌ Get friends list error: {str(e)}")
        print(f"Traceback: {error_trace}")
        raise HTTPException(status_code=500, detail=str(e))
    finally:
        if conn:
            conn.close()
