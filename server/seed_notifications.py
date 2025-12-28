"""
Simple script to seed notifications data for testing
Run this script to create fake notifications in the database
"""
import psycopg2
import os
from datetime import datetime, timedelta

def get_db_connection():
    """Get PostgreSQL database connection"""
    database_url = os.getenv("DATABASE_URL", "postgresql://tinygame:tinygame123@db:5432/tiny_game")
    return psycopg2.connect(database_url)

def seed_notifications():
    """Seed notifications data"""
    print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] Seeding notifications...")
    
    conn = get_db_connection()
    cursor = conn.cursor()
    
    try:
        # Get user IDs with display names (nickname with fallback to username)
        cursor.execute('SELECT id, username, COALESCE(nickname, username) as display_name FROM users ORDER BY id')
        users = cursor.fetchall()
        user_ids = {username: user_id for user_id, username, _ in users}
        user_display_names = {user_id: display_name for user_id, _, display_name in users}
        
        if len(user_ids) < 2:
            print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] Need at least 2 users to seed notifications")
            return
        
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] Found {len(user_ids)} users: {list(user_ids.keys())}")
        
        # Get list of usernames
        usernames = list(user_ids.keys())
        
        # Create notifications for testing
        inserted_count = 0
        
        # For each user, create some notifications from other users
        for i, (username, user_id) in enumerate(user_ids.items()):
            # Create 2-3 notifications for each user from different senders
            other_users = [u for u in usernames if u != username]
            num_notifications = min(3, len(other_users))
            
            for j in range(num_notifications):
                sender_username = other_users[j % len(other_users)]
                sender_id = user_ids[sender_username]
                sender_display_name = user_display_names.get(sender_id, sender_username)
                
                # Create friend request notification with nickname (consistent format)
                message = f"{sender_display_name} sent you a friend request"
                created_at = datetime.now() - timedelta(hours=j*2)  # Stagger timestamps
                
                # Check if notification already exists
                cursor.execute('''
                    SELECT id FROM notifications 
                    WHERE user_id = %s AND type = 'friend_request' AND message = %s
                ''', (user_id, message))
                
                if cursor.fetchone():
                    continue
                
                try:
                    # Insert notification
                    cursor.execute('''
                        INSERT INTO notifications (user_id, type, message, related_id, read, created_at)
                        VALUES (%s, 'friend_request', %s, NULL, %s, %s)
                    ''', (
                        user_id,
                        message,
                        False if j < 2 else True,  # First 2 unread, last one read
                        created_at
                    ))
                    
                    inserted_count += 1
                    print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] Created notification: {message}")
                    
                except Exception as e:
                    print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] Error creating notification: {str(e)}")
        
        # Also create some friend requests and their notifications
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] Creating friend requests...")
        
        friend_requests_created = 0
        for i, (username, user_id) in enumerate(user_ids.items()):
            other_users = [u for u in usernames if u != username]
            # Create 1-2 friend requests per user
            num_requests = min(2, len(other_users))
            
            for j in range(num_requests):
                to_username = other_users[j % len(other_users)]
                to_user_id = user_ids[to_username]
                
                # Check if already friends
                cursor.execute('''
                    SELECT id FROM friends 
                    WHERE (user_id = %s AND friend_id = %s) OR (user_id = %s AND friend_id = %s)
                ''', (user_id, to_user_id, to_user_id, user_id))
                
                if cursor.fetchone():
                    continue
                
                # Check if request already exists
                cursor.execute('''
                    SELECT id FROM friend_requests 
                    WHERE (from_user_id = %s AND to_user_id = %s) 
                       OR (from_user_id = %s AND to_user_id = %s)
                ''', (user_id, to_user_id, to_user_id, user_id))
                
                if cursor.fetchone():
                    continue
                
                try:
                    # Create friend request
                    cursor.execute('''
                        INSERT INTO friend_requests (from_user_id, to_user_id, status)
                        VALUES (%s, %s, 'pending')
                        RETURNING id
                    ''', (user_id, to_user_id))
                    
                    request_id = cursor.fetchone()[0]
                    
                    # Get sender display name (nickname with fallback to username)
                    cursor.execute('''
                        SELECT COALESCE(nickname, username) as display_name 
                        FROM users WHERE id = %s
                    ''', (user_id,))
                    sender_display = cursor.fetchone()
                    sender_display_name = sender_display[0] if sender_display else username
                    
                    # Create notification for this friend request with nickname (consistent format)
                    message = f"{sender_display_name} sent you a friend request"
                    # Check if notification already exists
                    cursor.execute('''
                        SELECT id FROM notifications 
                        WHERE user_id = %s AND type = 'friend_request' AND related_id = %s
                    ''', (to_user_id, request_id))
                    
                    if not cursor.fetchone():
                        cursor.execute('''
                            INSERT INTO notifications (user_id, type, message, related_id, read, created_at)
                            VALUES (%s, 'friend_request', %s, %s, FALSE, %s)
                        ''', (to_user_id, message, request_id, datetime.now() - timedelta(hours=j)))
                    
                    friend_requests_created += 1
                    print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] Created friend request: {username} -> {to_username} (id: {request_id})")
                    
                except Exception as e:
                    print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] Error creating friend request: {str(e)}")
        
        conn.commit()
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] Completed:")
        print(f"  - Inserted {inserted_count} notifications")
        print(f"  - Created {friend_requests_created} friend requests")
        
    except Exception as e:
        conn.rollback()
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] Error: {str(e)}")
        import traceback
        print(traceback.format_exc())
        raise
    finally:
        cursor.close()
        conn.close()

if __name__ == "__main__":
    seed_notifications()

