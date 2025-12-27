"""
Migration: Seed notifications (including friend requests)
Version: 008
Date: 2025-01-XX
"""
import psycopg2
import os
from datetime import datetime, timedelta

def get_db_connection():
    """Get PostgreSQL database connection"""
    database_url = os.getenv("DATABASE_URL", "postgresql://tinygame:tinygame123@db:5432/tiny_game")
    return psycopg2.connect(database_url)

def up():
    """Run migration - seed notifications"""
    print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] 📦 Migration 008: Seeding notifications...")
    
    conn = get_db_connection()
    cursor = conn.cursor()
    
    try:
        # Get user IDs
        cursor.execute('SELECT id, username FROM users ORDER BY id')
        users = cursor.fetchall()
        user_ids = {username: user_id for user_id, username in users}
        
        if len(user_ids) < 2:
            print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ⚠️ Need at least 2 users to seed notifications")
            return
        
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] Found {len(user_ids)} users")
        
        # Get list of usernames
        usernames = list(user_ids.keys())
        
        # Create notifications for testing
        notifications = []
        
        # For each user, create some notifications from other users
        for i, (username, user_id) in enumerate(user_ids.items()):
            # Create 2-3 notifications for each user from different senders
            other_users = [u for u in usernames if u != username]
            num_notifications = min(3, len(other_users))
            
            for j in range(num_notifications):
                sender_username = other_users[j % len(other_users)]
                sender_id = user_ids[sender_username]
                
                # Create friend request notification
                message = f"User '{sender_username}' sent you a friend request"
                created_at = datetime.now() - timedelta(hours=j*2)  # Stagger timestamps
                
                notifications.append({
                    'user_id': user_id,
                    'type': 'friend_request',
                    'message': message,
                    'related_id': None,  # Can be set to friend_request.id if exists
                    'read': False if j < 2 else True,  # First 2 unread, last one read
                    'created_at': created_at
                })
        
        # Insert notifications
        inserted_count = 0
        skipped_count = 0
        
        for notif in notifications:
            try:
                # Check if notification already exists
                cursor.execute('''
                    SELECT id FROM notifications 
                    WHERE user_id = %s AND type = %s AND message = %s
                ''', (notif['user_id'], notif['type'], notif['message']))
                
                if cursor.fetchone():
                    skipped_count += 1
                    continue
                
                # Insert notification
                cursor.execute('''
                    INSERT INTO notifications (user_id, type, message, related_id, read, created_at)
                    VALUES (%s, %s, %s, %s, %s, %s)
                ''', (
                    notif['user_id'],
                    notif['type'],
                    notif['message'],
                    notif['related_id'],
                    notif['read'],
                    notif['created_at']
                ))
                
                inserted_count += 1
                print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] [OK] Created notification: {notif['message']}")
                
            except Exception as e:
                print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] [ERROR] Failed to create notification: {str(e)}")
                skipped_count += 1
        
        # Also create some friend requests and their notifications
        # This ensures friend requests exist in friend_requests table too
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
                    
                    # Create notification for this friend request
                    message = f"User '{username}' sent you a friend request"
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
                    print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] [OK] Created friend request: {username} -> {to_username} (id: {request_id})")
                    
                except Exception as e:
                    print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] [ERROR] Failed to create friend request: {str(e)}")
        
        # Record this migration
        cursor.execute('''
            INSERT INTO migrations (version, name) 
            VALUES (%s, %s)
            ON CONFLICT (version) DO NOTHING
        ''', ('008', 'seed_notifications'))
        
        conn.commit()
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ✅ Migration 008 completed:")
        print(f"  - Inserted {inserted_count} notifications")
        print(f"  - Skipped {skipped_count} notifications")
        print(f"  - Created {friend_requests_created} friend requests")
        
    except Exception as e:
        conn.rollback()
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ❌ Migration 008 failed: {str(e)}")
        import traceback
        print(traceback.format_exc())
        raise
    finally:
        cursor.close()
        conn.close()

def down():
    """Rollback migration - delete seeded notifications"""
    print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] 🔄 Rolling back Migration 008: Deleting seeded notifications...")
    
    conn = get_db_connection()
    cursor = conn.cursor()
    
    try:
        # Delete notifications created by this migration
        # We'll delete notifications with type 'friend_request' that were created recently
        cursor.execute('''
            DELETE FROM notifications 
            WHERE type = 'friend_request' 
            AND created_at > NOW() - INTERVAL '1 day'
        ''')
        
        deleted_count = cursor.rowcount
        cursor.execute("DELETE FROM migrations WHERE version = '008'")
        conn.commit()
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ✅ Rollback completed: Deleted {deleted_count} notifications")
    except Exception as e:
        conn.rollback()
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ❌ Rollback failed: {str(e)}")
        raise
    finally:
        cursor.close()
        conn.close()

