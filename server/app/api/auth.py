from fastapi import APIRouter, HTTPException
from pydantic import BaseModel
import psycopg2
from psycopg2.extras import RealDictCursor
import hashlib
import os
from typing import Optional
from datetime import datetime
from app.api.websocket import websocket_manager

router = APIRouter(prefix="/api", tags=["auth"])

class LoginRequest(BaseModel):
    username: str
    pin: str

class LoginResponse(BaseModel):
    success: bool
    message: str
    user_id: Optional[int] = None
    username: Optional[str] = None
    nickname: Optional[str] = None  # Display name for UI
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
    nickname: str  # Display name (nickname or username fallback)
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
    to_nickname: str  # Nickname of the user to send request to

class SendFriendRequestResponse(BaseModel):
    success: bool
    message: str
    request_id: Optional[int] = None
    status: Optional[str] = None  # "pending", "accepted", "rejected"

# Database connection string from environment
DATABASE_URL = os.getenv(
    "DATABASE_URL",
    "postgresql://tinygame:tinygame123@localhost:5432/tiny_game",
)

def get_db_connection():
    """Get PostgreSQL database connection"""
    return psycopg2.connect(DATABASE_URL)

def get_friend_ids(user_id: int) -> list[int]:
    """
    Get list of friend IDs for a user.
    Returns list of friend user IDs.
    """
    conn = None
    try:
        conn = get_db_connection()
        cursor = conn.cursor()
        
        # Get all friend IDs for this user
        cursor.execute('''
            SELECT friend_id
            FROM friends
            WHERE user_id = %s
        ''', (user_id,))
        
        friend_ids = [row[0] for row in cursor.fetchall()]
        cursor.close()
        return friend_ids
    except Exception as e:
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ❌ Error getting friend IDs: {str(e)}")
        return []
    finally:
        if conn:
            conn.close()

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
        cursor.execute('SELECT id, username, pin, nickname FROM users WHERE username = %s', (request.username,))
        user_row = cursor.fetchone()
        
        if user_row:
            # Username exists in database, check PIN hash
            user_id = user_row['id']
            username = user_row['username']
            nickname = user_row.get('nickname')  # Get nickname, may be None
            stored_pin_hash = user_row['pin']
            
            if stored_pin_hash == pin_hash:
                # Correct credentials - verified against database
                print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ✅ Login successful: {username} (ID: {user_id})")
                
                # Send status update to friends (non-blocking, don't fail login if this fails)
                try:
                    await websocket_manager.send_status_update_to_friends(user_id)
                except Exception as status_error:
                    # Log warning but don't fail login
                    print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ⚠️ Warning: Failed to send status update to friends: {str(status_error)}")
                
                return LoginResponse(
                    success=True,
                    message=f"Login successful for {username}",
                    user_id=user_id,
                    username=username,
                    nickname=nickname,  # Return nickname for display
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
    Format: "nickname1,userId1,online1|nickname2,userId2,online2|..."
    Example: "User123,5,0|Admin,3,0|TestUser,7,1|"
    online: 0 = offline, 1 = online
    Note: friends table only contains accepted friendships (no status column)
    Uses nickname for display, falls back to username if nickname is NULL
    """
    conn = None
    try:
        conn = get_db_connection()
        cursor = conn.cursor(cursor_factory=RealDictCursor)
        
        # Get all friends for this user (all records in friends table are accepted)
        # Use COALESCE to fallback to username if nickname is NULL
        # Include friend_id (user.id) for tracking unread messages
        cursor.execute('''
            SELECT u.id as friend_id, COALESCE(u.nickname, u.username) as display_name
            FROM friends f
            JOIN users u ON f.friend_id = u.id
            WHERE f.user_id = %s
            ORDER BY COALESCE(u.nickname, u.username)
        ''', (user_id,))
        
        friends_list = []
        for row in cursor.fetchall():
            display_name = row['display_name']
            friend_id = row['friend_id']
            online = "0"  # Default to offline (0), can be updated later with WebSocket status
            friends_list.append(f"{display_name},{friend_id},{online}")
        
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
    Format: "nickname1|nickname2|..."
    Example: "User123|Admin|TestUser|"
    Uses nickname for display, falls back to username if nickname is NULL
    """
    conn = None
    try:
        conn = get_db_connection()
        cursor = conn.cursor(cursor_factory=RealDictCursor)
        
        # Get pending requests sent TO this user
        # Use COALESCE to fallback to username if nickname is NULL
        cursor.execute('''
            SELECT COALESCE(u.nickname, u.username) as display_name
            FROM friend_requests fr
            JOIN users u ON fr.from_user_id = u.id
            WHERE fr.to_user_id = %s AND fr.status = 'pending'
            ORDER BY fr.created_at DESC
        ''', (user_id,))
        
        requests_list = []
        for row in cursor.fetchall():
            display_name = row['display_name']
            requests_list.append(display_name)
        
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
    Format: "nickname1|nickname2|..."
    Example: "User123|Admin|TestUser|"
    Uses nickname for display, falls back to username if nickname is NULL
    """
    conn = None
    try:
        conn = get_db_connection()
        cursor = conn.cursor(cursor_factory=RealDictCursor)
        
        # Get pending requests sent BY this user
        # Use COALESCE to fallback to username if nickname is NULL
        cursor.execute('''
            SELECT COALESCE(u.nickname, u.username) as display_name
            FROM friend_requests fr
            JOIN users u ON fr.to_user_id = u.id
            WHERE fr.from_user_id = %s AND fr.status = 'pending'
            ORDER BY fr.created_at DESC
        ''', (user_id,))
        
        requests_list = []
        for row in cursor.fetchall():
            display_name = row['display_name']
            requests_list.append(display_name)
        
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
    
    Comprehensive validation and error handling:
    - Input validation (trim, empty check, length check)
    - Case-insensitive lookup
    - Self-request prevention
    - Already friends check
    - Duplicate request prevention (pending, accepted, rejected)
    - User existence validation
    - Transaction safety
    """
    conn = None
    try:
        def normalize_lookup_name(value: str) -> str:
            """
            Normalize user-supplied nickname/username for lookup.
            - Trims whitespace
            - Strips wrapping quotes (handles users typing/pasting: "Admin")
            - Removes CR/LF to avoid accidental multi-line inputs
            """
            if value is None:
                return ""
            v = value.strip()
            v = v.replace("\r", "").replace("\n", "")
            # Strip wrapping quotes defensively (multiple times)
            while len(v) > 0 and v[0] in ("'", '"'):
                v = v[1:].strip()
            while len(v) > 0 and v[-1] in ("'", '"'):
                v = v[:-1].strip()
            return v

        # Input validation: trim whitespace
        to_nickname = normalize_lookup_name(request.to_nickname) if request.to_nickname else ""
        
        # Validate input
        if not to_nickname:
            return SendFriendRequestResponse(
                success=False,
                message="Nickname cannot be empty"
            )
        
        if len(to_nickname) > 255:  # Match database VARCHAR(255) limit
            return SendFriendRequestResponse(
                success=False,
                message="Nickname is too long (max 255 characters)"
            )
        
        # Validate from_user_id
        if request.from_user_id <= 0:
            return SendFriendRequestResponse(
                success=False,
                message="Invalid user ID"
            )
        
        conn = get_db_connection()
        cursor = conn.cursor(cursor_factory=RealDictCursor)
        
        # Verify from_user_id exists
        cursor.execute('SELECT id FROM users WHERE id = %s', (request.from_user_id,))
        from_user = cursor.fetchone()
        if not from_user:
            return SendFriendRequestResponse(
                success=False,
                message="Sender user not found"
            )
        
        # Resolve to_user_id from nickname (case-insensitive lookup with fallback to username).
        # Note: nickname is not enforced unique at DB level, so we must handle ambiguous matches.
        cursor.execute('''
            SELECT id, COALESCE(nickname, username) as display_name
            FROM users 
            WHERE LOWER(COALESCE(nickname, username)) = LOWER(%s)
            ORDER BY id ASC
        ''', (to_nickname,))
        matches = cursor.fetchall()

        if not matches:
            return SendFriendRequestResponse(
                success=False,
                message=f"User '{to_nickname}' not found"
            )

        if len(matches) > 1:
            # Without UI changes, we cannot prompt user to choose.
            # Encourage using a unique identifier (username) or changing nickname.
            return SendFriendRequestResponse(
                success=False,
                message=f"Nickname '{to_nickname}' is not unique. Please use username or choose a unique nickname."
            )

        to_user = matches[0]
        to_user_id = to_user["id"]
        display_name = to_user["display_name"]
        
        # Check if trying to send request to self
        if request.from_user_id == to_user_id:
            return SendFriendRequestResponse(
                success=False,
                message="Cannot send friend request to yourself"
            )
        
        # Check if they're already friends (bidirectional check)
        cursor.execute('''
            SELECT id FROM friends 
            WHERE (user_id = %s AND friend_id = %s) OR (user_id = %s AND friend_id = %s)
        ''', (request.from_user_id, to_user_id, to_user_id, request.from_user_id))
        
        if cursor.fetchone():
            return SendFriendRequestResponse(
                success=False,
                message=f"You are already friends with '{display_name}'"
            )
        
        # Check for existing friend requests (both directions).
        # Note: friend_requests has UNIQUE(from_user_id, to_user_id), so "resend after reject"
        # must UPDATE the existing row back to pending instead of INSERTing a new one.
        cursor.execute('''
            SELECT id, status, from_user_id, to_user_id, created_at, updated_at
            FROM friend_requests
            WHERE (from_user_id = %s AND to_user_id = %s)
               OR (from_user_id = %s AND to_user_id = %s)
        ''', (request.from_user_id, to_user_id, to_user_id, request.from_user_id))
        request_rows = cursor.fetchall()

        outgoing = None  # from current user -> to_user
        incoming = None  # from to_user -> current user
        for r in request_rows:
            if r["from_user_id"] == request.from_user_id and r["to_user_id"] == to_user_id:
                outgoing = r
            elif r["from_user_id"] == to_user_id and r["to_user_id"] == request.from_user_id:
                incoming = r

        # If the other user already sent a pending request, keep UX consistent:
        # user should accept it from notifications (no UI changes).
        if incoming and incoming["status"] == "pending":
            return SendFriendRequestResponse(
                success=False,
                message=f"'{display_name}' has already sent you a friend request. Check your notifications to accept it.",
                request_id=incoming["id"],
                status="pending"
            )

        # Idempotent: outgoing pending = already pending.
        if outgoing and outgoing["status"] == "pending":
            return SendFriendRequestResponse(
                success=False,
                message=f"Friend request to '{display_name}' is already pending. Please wait for a response.",
                request_id=outgoing["id"],
                status="pending"
            )

        # Outgoing accepted should be impossible if friends table is consistent, but handle anyway.
        if outgoing and outgoing["status"] == "accepted":
            return SendFriendRequestResponse(
                success=False,
                message=f"You are already friends with '{display_name}'",
                request_id=outgoing["id"],
                status="accepted"
            )

        # Create or revive outgoing request
        request_id = None
        try:
            if outgoing and outgoing["status"] == "rejected":
                # Revive the same request row back to pending (fits UNIQUE constraint).
                cursor.execute('''
                    UPDATE friend_requests
                    SET status = 'pending', updated_at = CURRENT_TIMESTAMP
                    WHERE id = %s AND status = 'rejected'
                    RETURNING id
                ''', (outgoing["id"],))
                row = cursor.fetchone()
                request_id = row["id"] if row else outgoing["id"]
            else:
                # Insert a new outgoing request. If a race happens, UNIQUE will trip and we'll handle below.
                cursor.execute('''
                    INSERT INTO friend_requests (from_user_id, to_user_id, status)
                    VALUES (%s, %s, 'pending')
                    RETURNING id
                ''', (request.from_user_id, to_user_id))
                row = cursor.fetchone()
                request_id = row["id"] if row else None

            if not request_id:
                raise Exception("Failed to create/update friend request")

            conn.commit()

            # Ensure recipient notification exists and is unread (best-effort).
            try:
                create_friend_request_notification(request.from_user_id, to_user_id, request_id)
            except Exception as notif_error:
                print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ⚠️ Warning: Failed to create/refresh notification for friend request {request_id}: {str(notif_error)}")

            print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ✅ Friend request pending: user_id {request.from_user_id} -> {display_name} (id: {request_id})")

            return SendFriendRequestResponse(
                success=True,
                message=f"Friend request sent to '{display_name}'",
                request_id=request_id,
                status="pending"
            )

        except psycopg2.IntegrityError as integrity_err:
            # Handle UNIQUE constraint violation (race condition) and re-check state.
            conn.rollback()
            error_msg = str(integrity_err)
            if "unique" in error_msg.lower() or "duplicate" in error_msg.lower():
                cursor.execute('''
                    SELECT id, status, from_user_id
                    FROM friend_requests
                    WHERE (from_user_id = %s AND to_user_id = %s)
                       OR (from_user_id = %s AND to_user_id = %s)
                    ORDER BY created_at DESC
                    LIMIT 1
                ''', (request.from_user_id, to_user_id, to_user_id, request.from_user_id))
                duplicate = cursor.fetchone()
                if duplicate:
                    if duplicate["status"] == "pending":
                        if duplicate["from_user_id"] == request.from_user_id:
                            return SendFriendRequestResponse(
                                success=False,
                                message=f"Friend request to '{display_name}' is already pending",
                                request_id=duplicate["id"],
                                status="pending"
                            )
                        return SendFriendRequestResponse(
                            success=False,
                            message=f"'{display_name}' has already sent you a friend request",
                            request_id=duplicate["id"],
                            status="pending"
                        )
                    if duplicate["status"] == "accepted":
                        return SendFriendRequestResponse(
                            success=False,
                            message=f"You are already friends with '{display_name}'",
                            request_id=duplicate["id"],
                            status="accepted"
                        )
                return SendFriendRequestResponse(
                    success=False,
                    message="Friend request already exists"
                )
            raise
        
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
    Uses nickname for display (with fallback to username)
    """
    conn = None
    try:
        conn = get_db_connection()
        cursor = conn.cursor(cursor_factory=RealDictCursor)
        
        # Get sender display name (nickname with fallback to username)
        cursor.execute('''
            SELECT COALESCE(nickname, username) as display_name 
            FROM users WHERE id = %s
        ''', (from_user_id,))
        sender = cursor.fetchone()
        if not sender:
            print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ⚠️ Warning: Sender user {from_user_id} not found when creating notification")
            return
        
        sender_display_name = sender['display_name']
        if not sender_display_name or len(sender_display_name.strip()) == 0:
            # Fallback: get username if display_name is empty
            cursor.execute('SELECT username FROM users WHERE id = %s', (from_user_id,))
            username_row = cursor.fetchone()
            if username_row:
                sender_display_name = username_row['username']
            else:
                print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ⚠️ Warning: Could not get display name for user {from_user_id}")
                return
        
        # Insert/refresh notification with nickname-based message.
        # On resend/revive, we want the recipient to see it again (set read=FALSE).
        cursor.execute('''
            SELECT id, read FROM notifications 
            WHERE user_id = %s AND type = 'friend_request' AND related_id = %s
        ''', (to_user_id, request_id))
        
        notification_message = f"{sender_display_name} sent you a friend request"
        existing = cursor.fetchone()
        if existing:
            # Refresh existing notification to be unread again (idempotent).
            cursor.execute('''
                UPDATE notifications
                SET message = %s, read = FALSE, updated_at = CURRENT_TIMESTAMP
                WHERE id = %s
            ''', (notification_message, existing["id"]))
        else:
            cursor.execute('''
                INSERT INTO notifications (user_id, type, message, related_id, read)
                VALUES (%s, 'friend_request', %s, %s, FALSE)
            ''', (to_user_id, notification_message, request_id))
        
        conn.commit()
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ✅ Created/refreshed notification for friend request {request_id}: '{notification_message}'")
        
    except Exception as e:
        if conn:
            conn.rollback()
        import traceback
        error_trace = traceback.format_exc()
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ❌ Error creating notification: {str(e)}")
        print(f"Traceback: {error_trace}")
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
            # Use nickname with fallback to username for display
            cursor.execute('''
                SELECT fr.id, fr.created_at, COALESCE(u.nickname, u.username) as display_name
                FROM friend_requests fr
                JOIN users u ON fr.from_user_id = u.id
                WHERE fr.to_user_id = %s AND fr.status = 'pending'
                ORDER BY fr.created_at DESC
                LIMIT 50
            ''', (user_id,))
            
            for row in cursor.fetchall():
                notification_id = row['id']
                sender_display_name = row['display_name']
                created_at = row['created_at']
                
                if isinstance(created_at, datetime):
                    timestamp_str = created_at.isoformat() + 'Z'
                else:
                    timestamp_str = str(created_at) + 'Z' if not str(created_at).endswith('Z') else str(created_at)
                
                # Use consistent message format with create_friend_request_notification()
                notification_message = f"{sender_display_name} sent you a friend request"
                notification = NotificationEntry(
                    id=notification_id,
                    type="friend_request",
                    message=notification_message,
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
