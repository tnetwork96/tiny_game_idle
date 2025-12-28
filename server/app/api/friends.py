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

@router.post("/friend-requests/accept", response_model=AcceptFriendRequestResponse)
async def accept_friend_request(request: AcceptFriendRequestRequest):
    """
    Accept a friend request
    Updates friend_request status, creates bidirectional friendship, and marks notification as read
    """
    conn = None
    try:
        conn = get_db_connection()
        cursor = conn.cursor(cursor_factory=RealDictCursor)
        
        # Get notification from database
        cursor.execute('''
            SELECT id, type, related_id, user_id, read
            FROM notifications
            WHERE id = %s AND user_id = %s
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
        
        # Get friend_request from database
        cursor.execute('''
            SELECT id, from_user_id, to_user_id, status
            FROM friend_requests
            WHERE id = %s
        ''', (friend_request_id,))
        
        friend_request = cursor.fetchone()
        if not friend_request:
            return AcceptFriendRequestResponse(
                success=False,
                message=f"Friend request {friend_request_id} not found"
            )
        
        # Check if user is the recipient (to_user_id)
        if friend_request['to_user_id'] != request.user_id:
            return AcceptFriendRequestResponse(
                success=False,
                message="Only the recipient can accept a friend request"
            )
        
        # Check if request is still pending
        if friend_request['status'] != 'pending':
            return AcceptFriendRequestResponse(
                success=False,
                message=f"Friend request {friend_request_id} is already {friend_request['status']}"
            )
        
        from_user_id = friend_request['from_user_id']
        to_user_id = friend_request['to_user_id']
        
        # Check if they're already friends
        cursor.execute('''
            SELECT id FROM friends 
            WHERE (user_id = %s AND friend_id = %s) OR (user_id = %s AND friend_id = %s)
        ''', (from_user_id, to_user_id, to_user_id, from_user_id))
        
        if cursor.fetchone():
            return AcceptFriendRequestResponse(
                success=False,
                message="Users are already friends"
            )
        
        # Start transaction: Update friend_request, create friendships, mark notification as read
        try:
            # Update friend_request status to 'accepted'
            cursor.execute('''
                UPDATE friend_requests
                SET status = 'accepted', updated_at = CURRENT_TIMESTAMP
                WHERE id = %s
            ''', (friend_request_id,))
            
            # Create bidirectional friendship
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
                SET read = TRUE
                WHERE id = %s
            ''', (request.notification_id,))
            
            conn.commit()
            
            print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ✅ Friend request {friend_request_id} accepted by user {request.user_id}")
            return AcceptFriendRequestResponse(
                success=True,
                message="Friend request accepted successfully",
                friendship_id=friendship_id
            )
            
        except Exception as e:
            conn.rollback()
            raise e
            
    except psycopg2.IntegrityError as e:
        if conn:
            conn.rollback()
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ❌ Accept friend request error (IntegrityError): {str(e)}")
        return AcceptFriendRequestResponse(
            success=False,
            message="Database integrity error: friendship may already exist"
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
    Reject a friend request
    Updates friend_request status to 'rejected' and marks notification as read
    """
    conn = None
    try:
        conn = get_db_connection()
        cursor = conn.cursor(cursor_factory=RealDictCursor)
        
        # Get notification from database
        cursor.execute('''
            SELECT id, type, related_id, user_id, read
            FROM notifications
            WHERE id = %s AND user_id = %s
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
        
        # Get friend_request from database
        cursor.execute('''
            SELECT id, from_user_id, to_user_id, status
            FROM friend_requests
            WHERE id = %s
        ''', (friend_request_id,))
        
        friend_request = cursor.fetchone()
        if not friend_request:
            return RejectFriendRequestResponse(
                success=False,
                message=f"Friend request {friend_request_id} not found"
            )
        
        # Check if user is the recipient (to_user_id)
        if friend_request['to_user_id'] != request.user_id:
            return RejectFriendRequestResponse(
                success=False,
                message="Only the recipient can reject a friend request"
            )
        
        # Check if request is still pending
        if friend_request['status'] != 'pending':
            return RejectFriendRequestResponse(
                success=False,
                message=f"Friend request {friend_request_id} is already {friend_request['status']}"
            )
        
        # Start transaction: Update friend_request status and mark notification as read
        try:
            # Update friend_request status to 'rejected'
            cursor.execute('''
                UPDATE friend_requests
                SET status = 'rejected', updated_at = CURRENT_TIMESTAMP
                WHERE id = %s
            ''', (friend_request_id,))
            
            # Mark notification as read
            cursor.execute('''
                UPDATE notifications
                SET read = TRUE
                WHERE id = %s
            ''', (request.notification_id,))
            
            conn.commit()
            
            print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ✅ Friend request {friend_request_id} rejected by user {request.user_id}")
            return RejectFriendRequestResponse(
                success=True,
                message="Friend request rejected successfully"
            )
            
        except Exception as e:
            conn.rollback()
            raise e
            
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

