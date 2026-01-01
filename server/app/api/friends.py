"""
Friends API endpoints
Handles friend requests, accept, reject, cancel, and remove friend operations
"""
from fastapi import APIRouter, HTTPException
from pydantic import BaseModel
import psycopg2
from psycopg2.extras import RealDictCursor
import os
from typing import Optional
from datetime import datetime
from app.api.websocket import websocket_manager

router = APIRouter(prefix="/api", tags=["friends"])

# Pydantic Models
class AcceptFriendRequestRequest(BaseModel):
    user_id: int
    notification_id: int

class AcceptFriendRequestResponse(BaseModel):
    success: bool
    message: str
    friendship_id: Optional[int] = None

class RejectFriendRequestRequest(BaseModel):
    user_id: int
    notification_id: int

class RejectFriendRequestResponse(BaseModel):
    success: bool
    message: str

class CancelFriendRequestRequest(BaseModel):
    from_user_id: int
    to_user_id: int

class CancelFriendRequestResponse(BaseModel):
    success: bool
    message: str

class RemoveFriendResponse(BaseModel):
    success: bool
    message: str

# Database connection string from environment
DATABASE_URL = os.getenv("DATABASE_URL", "postgresql://tinygame:tinygame123@db:5432/tiny_game")

def get_db_connection():
    """Get PostgreSQL database connection"""
    return psycopg2.connect(DATABASE_URL)

def are_friends(user_id1: int, user_id2: int) -> bool:
    """
    Check if two users are friends.
    Returns True if friendship exists (bidirectional check).
    """
    conn = None
    try:
        conn = get_db_connection()
        cursor = conn.cursor()
        
        # Check if friendship exists in either direction
        cursor.execute('''
            SELECT id FROM friends 
            WHERE (user_id = %s AND friend_id = %s) OR (user_id = %s AND friend_id = %s)
            LIMIT 1
        ''', (user_id1, user_id2, user_id2, user_id1))
        
        result = cursor.fetchone()
        cursor.close()
        return result is not None
    except Exception as e:
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ❌ Error checking friendship: {str(e)}")
        return False
    finally:
        if conn:
            conn.close()

@router.post("/friend-requests/accept", response_model=AcceptFriendRequestResponse)
async def accept_friend_request(request: AcceptFriendRequestRequest):
    """
    Accept a friend request - Senior-level implementation with comprehensive validation and error handling
    Updates friend_request status, creates bidirectional friendship, marks notification as read,
    and creates notification for the sender
    """
    conn = None
    try:
        # Input validation
        if request.user_id <= 0:
            return AcceptFriendRequestResponse(
                success=False,
                message="Invalid user_id"
            )
        
        if request.notification_id <= 0:
            return AcceptFriendRequestResponse(
                success=False,
                message="Invalid notification_id"
            )
        
        conn = get_db_connection()
        cursor = conn.cursor(cursor_factory=RealDictCursor)
        
        # Verify user exists
        cursor.execute('SELECT id FROM users WHERE id = %s', (request.user_id,))
        if not cursor.fetchone():
            return AcceptFriendRequestResponse(
                success=False,
                message=f"User {request.user_id} not found"
            )
        
        # Get notification from database with lock to prevent race conditions
        cursor.execute('''
            SELECT id, type, related_id, user_id, read
            FROM notifications
            WHERE id = %s AND user_id = %s
            FOR UPDATE
        ''', (request.notification_id, request.user_id))
        
        notification = cursor.fetchone()
        if not notification:
            return AcceptFriendRequestResponse(
                success=False,
                message=f"Notification {request.notification_id} not found or does not belong to user {request.user_id}"
            )
        
        # Check notification type
        if notification['type'] != 'friend_request':
            return AcceptFriendRequestResponse(
                success=False,
                message=f"Notification {request.notification_id} is not a friend request"
            )
        
        # Get friend_request_id from notification's related_id
        friend_request_id = notification['related_id']
        if not friend_request_id:
            return AcceptFriendRequestResponse(
                success=False,
                message=f"Notification {request.notification_id} does not have a related friend request"
            )
        
        # Get friend_request from database with lock to prevent concurrent modifications
        cursor.execute('''
            SELECT id, from_user_id, to_user_id, status, created_at
            FROM friend_requests
            WHERE id = %s
            FOR UPDATE
        ''', (friend_request_id,))
        
        friend_request = cursor.fetchone()
        if not friend_request:
            return AcceptFriendRequestResponse(
                success=False,
                message=f"Friend request {friend_request_id} not found"
            )
        
        from_user_id = friend_request['from_user_id']
        to_user_id = friend_request['to_user_id']
        
        # Verify user is the recipient (to_user_id)
        if to_user_id != request.user_id:
            return AcceptFriendRequestResponse(
                success=False,
                message="Only the recipient can accept a friend request"
            )
        
        # Verify sender still exists
        cursor.execute('SELECT id, COALESCE(nickname, username) as display_name FROM users WHERE id = %s', (from_user_id,))
        sender = cursor.fetchone()
        if not sender:
            return AcceptFriendRequestResponse(
                success=False,
                message="The user who sent this friend request no longer exists"
            )
        sender_display_name = sender['display_name']
        
        # Check if request is still pending (handle race conditions)
        if friend_request['status'] != 'pending':
            status_msg = friend_request['status']
            if status_msg == 'accepted':
                return AcceptFriendRequestResponse(
                    success=False,
                    message=f"Friend request has already been accepted"
                )
            elif status_msg == 'rejected':
                return AcceptFriendRequestResponse(
                    success=False,
                    message=f"Friend request has already been rejected"
                )
            else:
                return AcceptFriendRequestResponse(
                    success=False,
                    message=f"Friend request is in '{status_msg}' status and cannot be accepted"
                )
        
        # Check if they're already friends (double-check to prevent duplicate friendships)
        cursor.execute('''
            SELECT id FROM friends 
            WHERE (user_id = %s AND friend_id = %s) OR (user_id = %s AND friend_id = %s)
            LIMIT 1
        ''', (from_user_id, to_user_id, to_user_id, from_user_id))
        
        if cursor.fetchone():
            # If already friends, update notification and return success (idempotent)
            cursor.execute('''
                UPDATE notifications
                SET read = TRUE
                WHERE id = %s
            ''', (request.notification_id,))
            conn.commit()
            return AcceptFriendRequestResponse(
                success=True,
                message="You are already friends with this user",
                friendship_id=None
            )
        
        # Start transaction: Update friend_request, create friendships, mark notification as read, create sender notification
        try:
            # Update friend_request status to 'accepted' (atomic update with status check)
            cursor.execute('''
                UPDATE friend_requests
                SET status = 'accepted', updated_at = CURRENT_TIMESTAMP
                WHERE id = %s AND status = 'pending'
            ''', (friend_request_id,))
            
            if cursor.rowcount == 0:
                # Another request may have changed the status
                conn.rollback()
                return AcceptFriendRequestResponse(
                    success=False,
                    message="Friend request status changed. Please refresh and try again."
                )
            
            # Create bidirectional friendship (both directions)
            # First direction: from_user -> to_user
            cursor.execute('''
                INSERT INTO friends (user_id, friend_id)
                VALUES (%s, %s)
                ON CONFLICT (user_id, friend_id) DO NOTHING
            ''', (from_user_id, to_user_id))
            
            # Second direction: to_user -> from_user
            cursor.execute('''
                INSERT INTO friends (user_id, friend_id)
                VALUES (%s, %s)
                ON CONFLICT (user_id, friend_id) DO NOTHING
            ''', (to_user_id, from_user_id))
            
            # Get friendship_id (use the first one created)
            cursor.execute('''
                SELECT id FROM friends
                WHERE user_id = %s AND friend_id = %s
            ''', (from_user_id, to_user_id))
            friendship = cursor.fetchone()
            friendship_id = friendship['id'] if friendship else None
            
            # Mark notification as read
            cursor.execute('''
                UPDATE notifications
                SET read = TRUE, updated_at = CURRENT_TIMESTAMP
                WHERE id = %s
            ''', (request.notification_id,))
            
            # Create notification for the sender (they sent the request, now it's accepted)
            # Get recipient display name for the notification
            cursor.execute('SELECT COALESCE(nickname, username) as display_name FROM users WHERE id = %s', (to_user_id,))
            recipient = cursor.fetchone()
            recipient_display_name = recipient['display_name'] if recipient else f"User {to_user_id}"
            
            # Check if notification already exists for the sender
            cursor.execute('''
                SELECT id FROM notifications
                WHERE user_id = %s AND type = 'friend_request_accepted' AND related_id = %s
            ''', (from_user_id, friend_request_id))
            
            notification_id = None
            notification_created_at = None
            notification_message = None
            
            if not cursor.fetchone():
                # Create acceptance notification for sender
                notification_message = f"{recipient_display_name} accepted your friend request"
                cursor.execute('''
                    INSERT INTO notifications (user_id, type, message, related_id, read)
                    VALUES (%s, 'friend_request_accepted', %s, %s, FALSE)
                    RETURNING id, created_at
                ''', (from_user_id, notification_message, friend_request_id))
                
                notification_result = cursor.fetchone()
                notification_id = notification_result['id'] if notification_result else None
                notification_created_at = notification_result['created_at'] if notification_result else None
            
            # Commit all database operations together
            conn.commit()
            
            # Send notification via WebSocket if user is connected and notification was created
            if notification_id is not None and notification_message is not None:
                notification_data = {
                "id": notification_id,
                "type": "friend_request_accepted",
                "message": notification_message,
                "timestamp": notification_created_at.isoformat() if notification_created_at else datetime.now().isoformat(),
                "read": False
            }
                await websocket_manager.send_notification_to_user(from_user_id, notification_data)
            
            print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ✅ Friend request {friend_request_id} accepted by user {request.user_id} (from {from_user_id})")
            return AcceptFriendRequestResponse(
                success=True,
                message=f"Friend request from {sender_display_name} accepted successfully",
                friendship_id=friendship_id
            )
            
        except psycopg2.IntegrityError as e:
            conn.rollback()
            error_str = str(e)
            print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ❌ Accept friend request error (IntegrityError): {error_str}")
            
            # Check if it's a duplicate key error (friendship already exists)
            if "duplicate key" in error_str.lower() or "unique constraint" in error_str.lower():
                return AcceptFriendRequestResponse(
                    success=False,
                    message="Friendship already exists. Please refresh your friends list."
                )
            else:
                return AcceptFriendRequestResponse(
                    success=False,
                    message="Database integrity error occurred. Please try again."
                )
        except Exception as e:
            conn.rollback()
            raise e
            
    except psycopg2.OperationalError as e:
        if conn:
            conn.rollback()
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ❌ Accept friend request error (OperationalError): {str(e)}")
        return AcceptFriendRequestResponse(
            success=False,
            message="Database connection error. Please try again later."
        )
    except Exception as e:
        if conn:
            conn.rollback()
        import traceback
        error_trace = traceback.format_exc()
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ❌ Accept friend request error: {str(e)}")
        print(f"Traceback: {error_trace}")
        return AcceptFriendRequestResponse(
            success=False,
            message=f"Error accepting friend request: {str(e)}"
        )
    finally:
        if conn:
            conn.close()

@router.post("/friend-requests/reject", response_model=RejectFriendRequestResponse)
async def reject_friend_request(request: RejectFriendRequestRequest):
    """
    Reject a friend request - Senior-level implementation with comprehensive validation and error handling
    Updates friend_request status to 'rejected', marks notification as read
    """
    conn = None
    try:
        # Input validation
        if request.user_id <= 0:
            return RejectFriendRequestResponse(
                success=False,
                message="Invalid user_id"
            )
        
        if request.notification_id <= 0:
            return RejectFriendRequestResponse(
                success=False,
                message="Invalid notification_id"
            )
        
        conn = get_db_connection()
        cursor = conn.cursor(cursor_factory=RealDictCursor)
        
        # Verify user exists
        cursor.execute('SELECT id FROM users WHERE id = %s', (request.user_id,))
        if not cursor.fetchone():
            return RejectFriendRequestResponse(
                success=False,
                message=f"User {request.user_id} not found"
            )
        
        # Get notification from database with lock to prevent race conditions
        cursor.execute('''
            SELECT id, type, related_id, user_id, read
            FROM notifications
            WHERE id = %s AND user_id = %s
            FOR UPDATE
        ''', (request.notification_id, request.user_id))
        
        notification = cursor.fetchone()
        if not notification:
            return RejectFriendRequestResponse(
                success=False,
                message=f"Notification {request.notification_id} not found or does not belong to user {request.user_id}"
            )
        
        # Check notification type
        if notification['type'] != 'friend_request':
            return RejectFriendRequestResponse(
                success=False,
                message=f"Notification {request.notification_id} is not a friend request"
            )
        
        # Get friend_request_id from notification's related_id
        friend_request_id = notification['related_id']
        if not friend_request_id:
            return RejectFriendRequestResponse(
                success=False,
                message=f"Notification {request.notification_id} does not have a related friend request"
            )
        
        # Get friend_request from database with lock to prevent concurrent modifications
        cursor.execute('''
            SELECT id, from_user_id, to_user_id, status, created_at
            FROM friend_requests
            WHERE id = %s
            FOR UPDATE
        ''', (friend_request_id,))
        
        friend_request = cursor.fetchone()
        if not friend_request:
            return RejectFriendRequestResponse(
                success=False,
                message=f"Friend request {friend_request_id} not found"
            )
        
        from_user_id = friend_request['from_user_id']
        to_user_id = friend_request['to_user_id']
        
        # Verify user is the recipient (to_user_id)
        if to_user_id != request.user_id:
            return RejectFriendRequestResponse(
                success=False,
                message="Only the recipient can reject a friend request"
            )
        
        # Verify sender still exists (for display name in message)
        cursor.execute('SELECT id, COALESCE(nickname, username) as display_name FROM users WHERE id = %s', (from_user_id,))
        sender = cursor.fetchone()
        sender_display_name = sender['display_name'] if sender else f"User {from_user_id}"
        
        # Check if request is still pending (handle race conditions)
        if friend_request['status'] != 'pending':
            status_msg = friend_request['status']
            if status_msg == 'accepted':
                return RejectFriendRequestResponse(
                    success=False,
                    message="Friend request has already been accepted"
                )
            elif status_msg == 'rejected':
                return RejectFriendRequestResponse(
                    success=False,
                    message="Friend request has already been rejected"
                )
            else:
                return RejectFriendRequestResponse(
                    success=False,
                    message=f"Friend request is in '{status_msg}' status and cannot be rejected"
                )
        
        # Start transaction: Update friend_request status and mark notification as read
        try:
            # Update friend_request status to 'rejected' (atomic update with status check)
            cursor.execute('''
                UPDATE friend_requests
                SET status = 'rejected', updated_at = CURRENT_TIMESTAMP
                WHERE id = %s AND status = 'pending'
            ''', (friend_request_id,))
            
            if cursor.rowcount == 0:
                # Another request may have changed the status
                conn.rollback()
                return RejectFriendRequestResponse(
                    success=False,
                    message="Friend request status changed. Please refresh and try again."
                )
            
            # Mark notification as read
            cursor.execute('''
                UPDATE notifications
                SET read = TRUE, updated_at = CURRENT_TIMESTAMP
                WHERE id = %s
            ''', (request.notification_id,))
            
            conn.commit()
            
            print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ✅ Friend request {friend_request_id} rejected by user {request.user_id} (from {from_user_id})")
            return RejectFriendRequestResponse(
                success=True,
                message=f"Friend request from {sender_display_name} rejected successfully"
            )
            
        except Exception as e:
            conn.rollback()
            raise e
            
    except psycopg2.OperationalError as e:
        if conn:
            conn.rollback()
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ❌ Reject friend request error (OperationalError): {str(e)}")
        return RejectFriendRequestResponse(
            success=False,
            message="Database connection error. Please try again later."
        )
    except Exception as e:
        if conn:
            conn.rollback()
        import traceback
        error_trace = traceback.format_exc()
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ❌ Reject friend request error: {str(e)}")
        print(f"Traceback: {error_trace}")
        return RejectFriendRequestResponse(
            success=False,
            message=f"Error rejecting friend request: {str(e)}"
        )
    finally:
        if conn:
            conn.close()

@router.post("/friend-requests/cancel", response_model=CancelFriendRequestResponse)
async def cancel_friend_request(request: CancelFriendRequestRequest):
    """
    Cancel a friend request that was sent
    Only the sender (from_user_id) can cancel their own pending request
    """
    conn = None
    try:
        conn = get_db_connection()
        cursor = conn.cursor(cursor_factory=RealDictCursor)
        
        # Find friend_request with from_user_id and to_user_id, status = 'pending'
        cursor.execute('''
            SELECT id, from_user_id, to_user_id, status
            FROM friend_requests
            WHERE from_user_id = %s AND to_user_id = %s AND status = 'pending'
        ''', (request.from_user_id, request.to_user_id))
        
        friend_request = cursor.fetchone()
        if not friend_request:
            return CancelFriendRequestResponse(
                success=False,
                message=f"No pending friend request found from user {request.from_user_id} to user {request.to_user_id}"
            )
        
        # Verify that the request's from_user_id matches the request (security check)
        if friend_request['from_user_id'] != request.from_user_id:
            return CancelFriendRequestResponse(
                success=False,
                message="Only the sender can cancel their own friend request"
            )
        
        friend_request_id = friend_request['id']
        
        # Start transaction: Delete friend_request and related notification
        try:
            # Delete related notification (if exists)
            cursor.execute('''
                DELETE FROM notifications
                WHERE user_id = %s AND type = 'friend_request' AND related_id = %s
            ''', (request.to_user_id, friend_request_id))
            
            # Delete friend_request (or update status to 'cancelled' if we want to keep history)
            # For now, we'll delete it to keep database clean
            cursor.execute('''
                DELETE FROM friend_requests
                WHERE id = %s
            ''', (friend_request_id,))
            
            conn.commit()
            
            print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ✅ Friend request {friend_request_id} cancelled by user {request.from_user_id}")
            return CancelFriendRequestResponse(
                success=True,
                message="Friend request cancelled successfully"
            )
            
        except Exception as e:
            conn.rollback()
            raise e
            
    except Exception as e:
        if conn:
            conn.rollback()
        import traceback
        error_trace = traceback.format_exc()
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ❌ Cancel friend request error: {str(e)}")
        print(f"Traceback: {error_trace}")
        return CancelFriendRequestResponse(
            success=False,
            message=f"Error cancelling friend request: {str(e)}"
        )
    finally:
        if conn:
            conn.close()

@router.delete("/friends/{user_id}/{friend_id}", response_model=RemoveFriendResponse)
async def remove_friend(user_id: int, friend_id: int):
    """
    Remove a friend relationship
    Deletes both bidirectional friendship records from friends table
    """
    conn = None
    try:
        conn = get_db_connection()
        cursor = conn.cursor(cursor_factory=RealDictCursor)
        
        # Check if friendship exists (both directions)
        cursor.execute('''
            SELECT id FROM friends 
            WHERE (user_id = %s AND friend_id = %s) OR (user_id = %s AND friend_id = %s)
        ''', (user_id, friend_id, friend_id, user_id))
        
        friendships = cursor.fetchall()
        if not friendships:
            return RemoveFriendResponse(
                success=False,
                message=f"Friendship between user {user_id} and user {friend_id} not found"
            )
        
        # Verify that user_id is one of the parties (security check)
        # Check if user_id is involved in this friendship
        cursor.execute('''
            SELECT id FROM friends 
            WHERE user_id = %s AND friend_id = %s
        ''', (user_id, friend_id))
        
        if not cursor.fetchone():
            return RemoveFriendResponse(
                success=False,
                message="You can only remove friendships where you are one of the parties"
            )
        
        # Start transaction: Delete both friendship records
        try:
            # Delete first direction: user_id -> friend_id
            cursor.execute('''
                DELETE FROM friends
                WHERE user_id = %s AND friend_id = %s
            ''', (user_id, friend_id))
            
            # Delete second direction: friend_id -> user_id
            cursor.execute('''
                DELETE FROM friends
                WHERE user_id = %s AND friend_id = %s
            ''', (friend_id, user_id))
            
            conn.commit()
            
            print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ✅ Friendship removed between user {user_id} and user {friend_id}")
            return RemoveFriendResponse(
                success=True,
                message="Friend removed successfully"
            )
            
        except Exception as e:
            conn.rollback()
            raise e
            
    except Exception as e:
        if conn:
            conn.rollback()
        import traceback
        error_trace = traceback.format_exc()
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ❌ Remove friend error: {str(e)}")
        print(f"Traceback: {error_trace}")
        return RemoveFriendResponse(
            success=False,
            message=f"Error removing friend: {str(e)}"
        )
    finally:
        if conn:
            conn.close()

# Test endpoint để gửi notification qua WebSocket
class TestNotificationRequest(BaseModel):
    user_id: int
    message: str
    notification_type: Optional[str] = "friend_request"

class TestNotificationResponse(BaseModel):
    success: bool
    message: str

@router.post("/test/notification", response_model=TestNotificationResponse)
async def test_send_notification(request: TestNotificationRequest):
    """
    Test endpoint để gửi notification qua WebSocket đến user
    Dùng để test tính năng socket notification
    """
    try:
        # Generate unique ID based on timestamp to avoid duplicates
        import time
        unique_id = int(time.time() * 1000) % 1000000  # Use milliseconds timestamp as ID
        
        notification_data = {
            "id": unique_id,  # Unique ID for each test notification
            "type": request.notification_type,
            "message": request.message,
            "timestamp": datetime.now().isoformat(),
            "read": False
        }
        
        result = await websocket_manager.send_notification_to_user(request.user_id, notification_data)
        
        if result:
            return TestNotificationResponse(
                success=True,
                message=f"Notification sent successfully to user {request.user_id}"
            )
        else:
            return TestNotificationResponse(
                success=False,
                message=f"User {request.user_id} not connected via WebSocket"
            )
    except Exception as e:
        import traceback
        error_trace = traceback.format_exc()
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ❌ Test notification error: {str(e)}")
        print(f"Traceback: {error_trace}")
        return TestNotificationResponse(
            success=False,
            message=f"Error: {str(e)}"
        )

