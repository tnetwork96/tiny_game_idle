"""
Migration: Sync all test data for consistency
Version: 011
Date: 2025-01-XX

This migration:
1. Cleans up inconsistent data
2. Adds nicknames to existing users
3. Creates consistent friendships
4. Creates pending friend requests (between users who are NOT friends)
5. Creates notifications for pending friend requests
"""
import psycopg2
import os
from datetime import datetime, timedelta

def get_db_connection():
    """Get PostgreSQL database connection"""
    database_url = os.getenv("DATABASE_URL", "postgresql://tinygame:tinygame123@db:5432/tiny_game")
    return psycopg2.connect(database_url)

def up():
    """Run migration - sync all test data"""
    print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] 📦 Migration 011: Syncing all test data...")
    
    conn = get_db_connection()
    cursor = conn.cursor()
    
    try:
        # Step 1: Clean up inconsistent data
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] Step 1: Cleaning up inconsistent data...")
        
        # Delete all existing friendships, friend requests and notifications (we'll recreate them)
        cursor.execute('DELETE FROM friends')
        cursor.execute('DELETE FROM notifications')
        cursor.execute('DELETE FROM friend_requests')
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}]    Deleted all friendships, notifications and friend requests")
        
        # Step 2: Ensure all test users exist and have nicknames
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] Step 2: Ensuring users exist with nicknames...")
        
        test_users = [
            {"username": "user123", "pin": "1234", "nickname": "CoolPlayer"},
            {"username": "admin", "pin": "admin123", "nickname": "Admin"},
            {"username": "test", "pin": "test123", "nickname": "Tester"},
            {"username": "player1", "pin": "1111", "nickname": "PlayerOne"},
            {"username": "player2", "pin": "2222", "nickname": "PlayerTwo"},
        ]
        
        import hashlib
        user_ids = {}
        for user in test_users:
            # Check if user exists
            cursor.execute('SELECT id, nickname FROM users WHERE username = %s', (user["username"],))
            result = cursor.fetchone()
            
            if result:
                user_id, existing_nickname = result
                # Update nickname if not set or different
                if not existing_nickname or existing_nickname != user["nickname"]:
                    cursor.execute('UPDATE users SET nickname = %s WHERE id = %s', (user["nickname"], user_id))
                    print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}]    Updated nickname for {user['username']}: {user['nickname']}")
                user_ids[user["username"]] = user_id
            else:
                # Create user if doesn't exist
                pin_hash = hashlib.sha256(user["pin"].encode()).hexdigest()
                cursor.execute('''
                    INSERT INTO users (username, pin, nickname) 
                    VALUES (%s, %s, %s)
                    RETURNING id
                ''', (user["username"], pin_hash, user["nickname"]))
                user_id = cursor.fetchone()[0]
                user_ids[user["username"]] = user_id
                print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}]    Created user: {user['username']} (nickname: {user['nickname']})")
        
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}]    User IDs: {user_ids}")
        
        # Step 3: Create consistent friendships
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] Step 3: Creating consistent friendships...")
        
        # Define friendships (bidirectional)
        # user123 is friends with admin and test
        # admin is friends with test and player1
        # test is friends with player1
        # player1 is friends with player2
        friendships = [
            ("user123", "admin"),
            ("user123", "test"),
            ("admin", "test"),
            ("admin", "player1"),
            ("test", "player1"),
            ("player1", "player2"),
        ]
        
        friendship_count = 0
        for user1_name, user2_name in friendships:
            if user1_name not in user_ids or user2_name not in user_ids:
                continue
            
            user1_id = user_ids[user1_name]
            user2_id = user_ids[user2_name]
            
            # Check if already friends
            cursor.execute('''
                SELECT id FROM friends 
                WHERE (user_id = %s AND friend_id = %s) OR (user_id = %s AND friend_id = %s)
            ''', (user1_id, user2_id, user2_id, user1_id))
            
            if not cursor.fetchone():
                # Create bidirectional friendship
                cursor.execute('INSERT INTO friends (user_id, friend_id) VALUES (%s, %s)', (user1_id, user2_id))
                cursor.execute('INSERT INTO friends (user_id, friend_id) VALUES (%s, %s)', (user2_id, user1_id))
                friendship_count += 1
                print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}]    Created friendship: {user1_name} <-> {user2_name}")
        
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}]    Created {friendship_count} new friendships")
        
        # Step 4: Create pending friend requests (between users who are NOT friends)
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] Step 4: Creating pending friend requests...")
        
        # Define pending friend requests
        # These should be between users who are NOT already friends
        # user123 -> player1 (already friends, skip)
        # user123 -> player2 (not friends, create)
        # admin -> player2 (not friends, create)
        # test -> player2 (not friends, create)
        # player1 -> user123 (already friends, skip)
        # player2 -> user123 (not friends, create)
        # player2 -> admin (not friends, create)
        # player2 -> test (not friends, create)
        
        pending_requests = [
            ("user123", "player2"),  # user123 wants to add player2
            ("admin", "player2"),    # admin wants to add player2
            ("test", "player2"),     # test wants to add player2
            ("player2", "user123"),  # player2 wants to add user123
            ("player2", "admin"),    # player2 wants to add admin
            ("player2", "test"),     # player2 wants to add test
        ]
        
        request_count = 0
        for from_name, to_name in pending_requests:
            if from_name not in user_ids or to_name not in user_ids:
                continue
            
            from_id = user_ids[from_name]
            to_id = user_ids[to_name]
            
            # Skip if already friends
            cursor.execute('''
                SELECT id FROM friends 
                WHERE (user_id = %s AND friend_id = %s) OR (user_id = %s AND friend_id = %s)
            ''', (from_id, to_id, to_id, from_id))
            
            if cursor.fetchone():
                print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}]    [SKIP] {from_name} and {to_name} are already friends")
                continue
            
            # Check if request already exists (in either direction)
            cursor.execute('''
                SELECT id FROM friend_requests 
                WHERE (from_user_id = %s AND to_user_id = %s) 
                   OR (from_user_id = %s AND to_user_id = %s)
            ''', (from_id, to_id, to_id, from_id))
            
            if cursor.fetchone():
                print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}]    [SKIP] Friend request already exists between {from_name} and {to_name}")
                continue
            
            # Create friend request
            cursor.execute('''
                INSERT INTO friend_requests (from_user_id, to_user_id, status)
                VALUES (%s, %s, 'pending')
                RETURNING id
            ''', (from_id, to_id))
            
            request_id = cursor.fetchone()[0]
            request_count += 1
            
            # Get sender display name (nickname with fallback to username)
            cursor.execute('''
                SELECT COALESCE(nickname, username) as display_name 
                FROM users WHERE id = %s
            ''', (from_id,))
            sender_display = cursor.fetchone()
            sender_display_name = sender_display[0] if sender_display else from_name
            
            # Create notification for this friend request
            message = f"{sender_display_name} sent you a friend request"
            created_at = datetime.now() - timedelta(hours=request_count)  # Stagger timestamps
            
            cursor.execute('''
                INSERT INTO notifications (user_id, type, message, related_id, read, created_at)
                VALUES (%s, 'friend_request', %s, %s, FALSE, %s)
            ''', (to_id, message, request_id, created_at))
            
            print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}]    Created friend request: {from_name} -> {to_name} (id: {request_id}, notification created)")
        
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}]    Created {request_count} pending friend requests with notifications")
        
        # Record this migration
        cursor.execute('''
            INSERT INTO migrations (version, name) 
            VALUES (%s, %s)
            ON CONFLICT (version) DO NOTHING
        ''', ('011', 'sync_all_test_data'))
        
        conn.commit()
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ✅ Migration 011 completed!")
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] Summary:")
        print(f"  - Users: {len(user_ids)} users with nicknames")
        print(f"  - Friendships: {friendship_count} new friendships created")
        print(f"  - Friend requests: {request_count} pending requests with notifications")
        
    except Exception as e:
        conn.rollback()
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ❌ Migration 011 failed: {str(e)}")
        import traceback
        print(traceback.format_exc())
        raise
    finally:
        cursor.close()
        conn.close()

def down():
    """Rollback migration - remove synced test data"""
    print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ⬇️  Rolling back migration 011...")
    
    conn = get_db_connection()
    cursor = conn.cursor()
    
    try:
        # Delete notifications and friend requests created by this migration
        cursor.execute('DELETE FROM notifications WHERE type = %s', ('friend_request',))
        cursor.execute('DELETE FROM friend_requests WHERE status = %s', ('pending',))
        
        cursor.execute('DELETE FROM migrations WHERE version = %s', ('011',))
        conn.commit()
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ✅ Rollback completed!")
    except Exception as e:
        conn.rollback()
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ❌ Rollback failed: {str(e)}")
        raise
    finally:
        cursor.close()
        conn.close()

if __name__ == "__main__":
    up()

