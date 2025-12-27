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
    nickname: Optional[str] = None

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

class NotificationEntry(BaseModel):
    id: int
    type: str
    message: str
    timestamp: str
    read: bool

class NotificationsResponse(BaseModel):
    success: bool
    notifications: list[NotificationEntry]
    message: str

class SendFriendRequestRequest(BaseModel):
    from_user_id: int
    to_username: str  # Username of the user to send request to

class SendFriendRequestResponse(BaseModel):
    success: bool
    message: str
    request_id: Optional[int] = None

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
            INSERT INTO users (username, pin, nickname) 
            VALUES (%s, %s, %s)
            RETURNING id
        ''', (request.username, pin_hash, request.nickname))
        
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
    Note: friends table only contains accepted friendships (no status column)
    """
    conn = None
    try:
        conn = get_db_connection()
        cursor = conn.cursor(cursor_factory=RealDictCursor)
        
        # Get all friends for this user (all records in friends table are accepted)
        cursor.execute('''
            SELECT u.username 
            FROM friends f
            JOIN users u ON f.friend_id = u.id
            WHERE f.user_id = %s
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

@router.get("/friend-requests/{user_id}/pending")
async def get_pending_requests(user_id: int):
    """
    Get pending friend requests sent TO this user
    Format: "username1|username2|..."
    Example: "user123|admin|test|"
    """
    conn = None
    try:
        conn = get_db_connection()
        cursor = conn.cursor(cursor_factory=RealDictCursor)
        
        # Get pending requests sent TO this user
        cursor.execute('''
            SELECT u.username 
            FROM friend_requests fr
            JOIN users u ON fr.from_user_id = u.id
            WHERE fr.to_user_id = %s AND fr.status = 'pending'
            ORDER BY fr.created_at DESC
        ''', (user_id,))
        
        requests_list = []
        for row in cursor.fetchall():
            username = row['username']
            requests_list.append(username)
        
        # Join with | separator
        result_string = "|".join(requests_list)
        if result_string:
            result_string += "|"  # Add trailing | for easier parsing
        
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ✅ Pending requests for user_id {user_id}: {result_string}")
        return result_string
            
    except Exception as e:
        import traceback
        error_trace = traceback.format_exc()
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ❌ Get pending requests error: {str(e)}")
        print(f"Traceback: {error_trace}")
        raise HTTPException(status_code=500, detail=str(e))
    finally:
        if conn:
            conn.close()

@router.get("/friend-requests/{user_id}/sent")
async def get_sent_requests(user_id: int):
    """
    Get friend requests sent BY this user (pending status)
    Format: "username1|username2|..."
    Example: "user123|admin|test|"
    """
    conn = None
    try:
        conn = get_db_connection()
        cursor = conn.cursor(cursor_factory=RealDictCursor)
        
        # Get pending requests sent BY this user
        cursor.execute('''
            SELECT u.username 
            FROM friend_requests fr
            JOIN users u ON fr.to_user_id = u.id
            WHERE fr.from_user_id = %s AND fr.status = 'pending'
            ORDER BY fr.created_at DESC
        ''', (user_id,))
        
        requests_list = []
        for row in cursor.fetchall():
            username = row['username']
            requests_list.append(username)
        
        # Join with | separator
        result_string = "|".join(requests_list)
        if result_string:
            result_string += "|"  # Add trailing | for easier parsing
        
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ✅ Sent requests for user_id {user_id}: {result_string}")
        return result_string
            
    except Exception as e:
        import traceback
        error_trace = traceback.format_exc()
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ❌ Get sent requests error: {str(e)}")
        print(f"Traceback: {error_trace}")
        raise HTTPException(status_code=500, detail=str(e))
    finally:
        if conn:
            conn.close()

@router.post("/friend-requests/send", response_model=SendFriendRequestResponse)
async def send_friend_request(request: SendFriendRequestRequest):
    """
    Send a friend request to another user
    Creates a friend request and automatically creates a notification
    """
    conn = None
    try:
        conn = get_db_connection()
        cursor = conn.cursor(cursor_factory=RealDictCursor)
        
        # Get to_user_id from username
        cursor.execute('SELECT id FROM users WHERE username = %s', (request.to_username,))
        to_user = cursor.fetchone()
        
        if not to_user:
            return SendFriendRequestResponse(
                success=False,
                message=f"User '{request.to_username}' not found"
            )
        
        to_user_id = to_user['id']
        
        # Check if trying to send request to self
        if request.from_user_id == to_user_id:
            return SendFriendRequestResponse(
                success=False,
                message="Cannot send friend request to yourself"
            )
        
        # Check if they're already friends
        cursor.execute('''
            SELECT id FROM friends 
            WHERE (user_id = %s AND friend_id = %s) OR (user_id = %s AND friend_id = %s)
        ''', (request.from_user_id, to_user_id, to_user_id, request.from_user_id))
        
        if cursor.fetchone():
            return SendFriendRequestResponse(
                success=False,
                message=f"You are already friends with '{request.to_username}'"
            )
        
        # Check if there's already a pending request (in either direction)
        cursor.execute('''
            SELECT id, status, from_user_id FROM friend_requests 
            WHERE (from_user_id = %s AND to_user_id = %s) 
               OR (from_user_id = %s AND to_user_id = %s)
        ''', (request.from_user_id, to_user_id, to_user_id, request.from_user_id))
        
        existing_request = cursor.fetchone()
        if existing_request:
            existing_status = existing_request['status']
            existing_from_id = existing_request['from_user_id']
            if existing_status == 'pending':
                if existing_from_id == request.from_user_id:
                    return SendFriendRequestResponse(
                        success=False,
                        message=f"Friend request to '{request.to_username}' is already pending"
                    )
                else:
                    return SendFriendRequestResponse(
                        success=False,
                        message=f"'{request.to_username}' has already sent you a friend request"
                    )
        
        # Create friend request
        cursor.execute('''
            INSERT INTO friend_requests (from_user_id, to_user_id, status)
            VALUES (%s, %s, 'pending')
            RETURNING id
        ''', (request.from_user_id, to_user_id))
        
        request_id = cursor.fetchone()['id']
        conn.commit()
        
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ✅ Friend request created: user_id {request.from_user_id} -> {request.to_username} (id: {request_id})")
        
        # Create notification for the recipient
        create_friend_request_notification(request.from_user_id, to_user_id, request_id)
        
        return SendFriendRequestResponse(
            success=True,
            message=f"Friend request sent to '{request.to_username}'",
            request_id=request_id
        )
        
    except psycopg2.IntegrityError as e:
        if conn:
            conn.rollback()
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ❌ Send friend request error (IntegrityError): {str(e)}")
        return SendFriendRequestResponse(
            success=False,
            message="Friend request already exists or invalid data"
        )
    except Exception as e:
        if conn:
            conn.rollback()
        import traceback
        error_trace = traceback.format_exc()
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ❌ Send friend request error: {str(e)}")
        print(f"Traceback: {error_trace}")
        return SendFriendRequestResponse(
            success=False,
            message=f"Error sending friend request: {str(e)}"
        )
    finally:
        if conn:
            conn.close()

@router.post("/friend-requests/send", response_model=SendFriendRequestResponse)
async def send_friend_request(request: SendFriendRequestRequest):
    """
    Send a friend request to another user
    Creates a friend request and automatically creates a notification
    """
    conn = None
    try:
        conn = get_db_connection()
        cursor = conn.cursor(cursor_factory=RealDictCursor)
        
        # Get to_user_id from username
        cursor.execute('SELECT id FROM users WHERE username = %s', (request.to_username,))
        to_user = cursor.fetchone()
        
        if not to_user:
            return SendFriendRequestResponse(
                success=False,
                message=f"User '{request.to_username}' not found"
            )
        
        to_user_id = to_user['id']
        
        # Check if trying to send request to self
        if request.from_user_id == to_user_id:
            return SendFriendRequestResponse(
                success=False,
                message="Cannot send friend request to yourself"
            )
        
        # Check if they're already friends
        cursor.execute('''
            SELECT id FROM friends 
            WHERE (user_id = %s AND friend_id = %s) OR (user_id = %s AND friend_id = %s)
        ''', (request.from_user_id, to_user_id, to_user_id, request.from_user_id))
        
        if cursor.fetchone():
            return SendFriendRequestResponse(
                success=False,
                message=f"You are already friends with '{request.to_username}'"
            )
        
        # Check if there's already a pending request (in either direction)
        cursor.execute('''
            SELECT id, status, from_user_id FROM friend_requests 
            WHERE (from_user_id = %s AND to_user_id = %s) 
               OR (from_user_id = %s AND to_user_id = %s)
        ''', (request.from_user_id, to_user_id, to_user_id, request.from_user_id))
        
        existing_request = cursor.fetchone()
        if existing_request:
            existing_status = existing_request['status']
            existing_from_id = existing_request['from_user_id']
            if existing_status == 'pending':
                if existing_from_id == request.from_user_id:
                    return SendFriendRequestResponse(
                        success=False,
                        message=f"Friend request to '{request.to_username}' is already pending"
                    )
                else:
                    return SendFriendRequestResponse(
                        success=False,
                        message=f"'{request.to_username}' has already sent you a friend request"
                    )
        
        # Create friend request
        cursor.execute('''
            INSERT INTO friend_requests (from_user_id, to_user_id, status)
            VALUES (%s, %s, 'pending')
            RETURNING id
        ''', (request.from_user_id, to_user_id))
        
        request_id = cursor.fetchone()['id']
        conn.commit()
        
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ✅ Friend request created: user_id {request.from_user_id} -> {request.to_username} (id: {request_id})")
        
        # Create notification for the recipient
        create_friend_request_notification(request.from_user_id, to_user_id, request_id)
        
        return SendFriendRequestResponse(
            success=True,
            message=f"Friend request sent to '{request.to_username}'",
            request_id=request_id
        )
        
    except psycopg2.IntegrityError as e:
        if conn:
            conn.rollback()
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ❌ Send friend request error (IntegrityError): {str(e)}")
        return SendFriendRequestResponse(
            success=False,
            message="Friend request already exists or invalid data"
        )
    except Exception as e:
        if conn:
            conn.rollback()
        import traceback
        error_trace = traceback.format_exc()
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ❌ Send friend request error: {str(e)}")
        print(f"Traceback: {error_trace}")
        return SendFriendRequestResponse(
            success=False,
            message=f"Error sending friend request: {str(e)}"
        )
    finally:
        if conn:
            conn.close()

def create_friend_request_notification(from_user_id: int, to_user_id: int, request_id: int):
    """
    Create a notification when a friend request is sent
    This function is called after a friend request is created
    """
    conn = None
    try:
        conn = get_db_connection()
        cursor = conn.cursor()
        
        # Get sender username
        cursor.execute('SELECT username FROM users WHERE id = %s', (from_user_id,))
        sender = cursor.fetchone()
        if not sender:
            return
        
        sender_username = sender[0]
        
        # Check if notification already exists for this friend request
        cursor.execute('''
            SELECT id FROM notifications 
            WHERE user_id = %s AND type = 'friend_request' AND related_id = %s
        ''', (to_user_id, request_id))
        
        if cursor.fetchone():
            # Notification already exists, skip
            return
        
        # Insert notification
        cursor.execute('''
            INSERT INTO notifications (user_id, type, message, related_id, read)
            VALUES (%s, 'friend_request', %s, %s, FALSE)
        ''', (to_user_id, f"User '{sender_username}' sent you a friend request", request_id))
        
        conn.commit()
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ✅ Created notification for friend request {request_id}")
        
    except Exception as e:
        if conn:
            conn.rollback()
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ❌ Error creating notification: {str(e)}")
    finally:
        if conn:
            conn.close()

@router.get("/notifications/{user_id}", response_model=NotificationsResponse)
async def get_notifications(user_id: int):
    """
    Get all notifications for a user
    Includes friend requests and other notification types
    Returns JSON format with notifications array
    Friend requests are also stored as notifications
    """
    conn = None
    try:
        conn = get_db_connection()
        cursor = conn.cursor(cursor_factory=RealDictCursor)
        
        # First, try to get notifications from notifications table
        cursor.execute('''
            SELECT id, type, message, created_at, read
            FROM notifications
            WHERE user_id = %s
            ORDER BY created_at DESC
            LIMIT 50
        ''', (user_id,))
        
        notifications = []
        for row in cursor.fetchall():
            notification_id = row['id']
            notification_type = row['type']
            message = row['message']
            created_at = row['created_at']
            read = row['read']
            
            # Format timestamp as ISO 8601
            if isinstance(created_at, datetime):
                timestamp_str = created_at.isoformat() + 'Z'
            else:
                timestamp_str = str(created_at) + 'Z' if not str(created_at).endswith('Z') else str(created_at)
            
            # Create notification entry
            notification = NotificationEntry(
                id=notification_id,
                type=notification_type,
                message=message,
                timestamp=timestamp_str,
                read=bool(read)
            )
            notifications.append(notification)
        
        # If no notifications in table, fallback to friend_requests (for backward compatibility)
        # This ensures existing friend requests are still shown
        if len(notifications) == 0:
            # Get pending friend requests sent TO this user
            cursor.execute('''
                SELECT fr.id, fr.created_at, u.username
                FROM friend_requests fr
                JOIN users u ON fr.from_user_id = u.id
                WHERE fr.to_user_id = %s AND fr.status = 'pending'
                ORDER BY fr.created_at DESC
                LIMIT 50
            ''', (user_id,))
            
            for row in cursor.fetchall():
                notification_id = row['id']
                sender_username = row['username']
                created_at = row['created_at']
                
                if isinstance(created_at, datetime):
                    timestamp_str = created_at.isoformat() + 'Z'
                else:
                    timestamp_str = str(created_at) + 'Z' if not str(created_at).endswith('Z') else str(created_at)
                
                notification = NotificationEntry(
                    id=notification_id,
                    type="friend_request",
                    message=f"User '{sender_username}' sent you a friend request",
                    timestamp=timestamp_str,
                    read=False
                )
                notifications.append(notification)
        
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ✅ Notifications for user_id {user_id}: {len(notifications)} notifications")
        return NotificationsResponse(
            success=True,
            notifications=notifications,
            message="Notifications retrieved successfully"
        )
            
    except Exception as e:
        import traceback
        error_trace = traceback.format_exc()
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ❌ Get notifications error: {str(e)}")
        print(f"Traceback: {error_trace}")
        raise HTTPException(status_code=500, detail=str(e))
    finally:
        if conn:
            conn.close()
