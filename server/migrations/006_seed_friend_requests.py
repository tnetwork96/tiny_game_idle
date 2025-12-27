"""
Migration: Seed friend requests
Version: 006
Date: 2025-12-27
"""
import psycopg2
import os
from datetime import datetime

def get_db_connection():
    """Get PostgreSQL database connection"""
    database_url = os.getenv("DATABASE_URL", "postgresql://tinygame:tinygame123@db:5432/tiny_game")
    return psycopg2.connect(database_url)

def up():
    """Run migration - seed friend requests"""
    print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] 📦 Migration 006: Seeding friend requests...")
    
    conn = get_db_connection()
    cursor = conn.cursor()
    
    try:
        # Get user IDs by username
        user_ids = {}
        cursor.execute('SELECT id, username FROM users')
        for row in cursor.fetchall():
            user_ids[row[1]] = row[0]
        
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] Found users: {user_ids}")
        
        # Define friend requests (pending requests for testing)
        # Format: (from_username, to_username, status)
        # These should be users who are NOT already friends
        friend_requests = [
            # Pending requests - user123 wants to add new users
            ("user123", "newuser129", "pending"),
            ("user123", "newuser1229", "pending"),
            # Pending requests - other users want to add user123
            ("newuser132", "user123", "pending"),
            # Pending requests between other users
            ("admin", "newuser129", "pending"),
            ("player1", "newuser1222", "pending"),
            # Some accepted/rejected requests for variety (historical data)
            ("player2", "newuser132", "accepted"),
            ("admin", "newuser132", "rejected"),
        ]
        
        inserted_count = 0
        skipped_count = 0
        
        # Insert friend requests
        for from_name, to_name, status in friend_requests:
            if from_name not in user_ids or to_name not in user_ids:
                print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] [SKIP] User not found: {from_name} or {to_name}")
                skipped_count += 1
                continue
            
            from_id = user_ids[from_name]
            to_id = user_ids[to_name]
            
            # Check if they're already friends (should not create request if already friends)
            cursor.execute('''
                SELECT id FROM friends 
                WHERE (user_id = %s AND friend_id = %s) OR (user_id = %s AND friend_id = %s)
            ''', (from_id, to_id, to_id, from_id))
            
            if cursor.fetchone():
                print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] [SKIP] {from_name} and {to_name} are already friends")
                skipped_count += 1
                continue
            
            try:
                # Insert friend request
                cursor.execute('''
                    INSERT INTO friend_requests (from_user_id, to_user_id, status) 
                    VALUES (%s, %s, %s)
                ''', (from_id, to_id, status))
                
                print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] [OK] Created friend request: {from_name} -> {to_name} ({status})")
                inserted_count += 1
            except psycopg2.IntegrityError:
                print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] [SKIP] Friend request already exists: {from_name} -> {to_name}")
                skipped_count += 1
        
        # Record this migration
        cursor.execute('''
            INSERT INTO migrations (version, name) 
            VALUES (%s, %s)
            ON CONFLICT (version) DO NOTHING
        ''', ('006', 'seed_friend_requests'))
        
        conn.commit()
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ✅ Migration 006 completed!")
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}]    Inserted: {inserted_count} friend requests")
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}]    Skipped: {skipped_count} friend requests")
        
    except Exception as e:
        conn.rollback()
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ❌ Migration 006 failed: {str(e)}")
        raise
    finally:
        conn.close()

def down():
    """Rollback migration - remove friend requests"""
    print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ⬇️  Rolling back migration 006...")
    
    conn = get_db_connection()
    cursor = conn.cursor()
    
    try:
        # Get user IDs by username
        user_ids = {}
        cursor.execute('SELECT id, username FROM users')
        for row in cursor.fetchall():
            user_ids[row[1]] = row[0]
        
        # Remove friend requests
        friend_requests = [
            ("user123", "newuser129"),
            ("user123", "newuser1229"),
            ("newuser132", "user123"),
            ("newuser1222", "user123"),
            ("admin", "newuser129"),
            ("test", "newuser1222"),
            ("player1", "newuser1222"),
        ]
        
        for from_name, to_name in friend_requests:
            if from_name in user_ids and to_name in user_ids:
                from_id = user_ids[from_name]
                to_id = user_ids[to_name]
                cursor.execute('DELETE FROM friend_requests WHERE from_user_id = %s AND to_user_id = %s', 
                             (from_id, to_id))
        
        cursor.execute('DELETE FROM migrations WHERE version = %s', ('006',))
        conn.commit()
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ✅ Rollback completed!")
    except Exception as e:
        conn.rollback()
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ❌ Rollback failed: {str(e)}")
        raise
    finally:
        conn.close()

if __name__ == "__main__":
    up()

