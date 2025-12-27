"""
Migration: Seed friends relationships
Version: 004
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
    """Run migration - seed friends relationships"""
    print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] 📦 Migration 004: Seeding friends relationships...")
    
    conn = get_db_connection()
    cursor = conn.cursor()
    
    try:
        # Get user IDs by username
        user_ids = {}
        cursor.execute('SELECT id, username FROM users')
        for row in cursor.fetchall():
            user_ids[row[1]] = row[0]
        
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] Found users: {user_ids}")
        
        # Define friendships (bidirectional - both users are friends)
        # Format: (user1_username, user2_username, status)
        friendships = [
            ("user123", "admin", "accepted"),
            ("user123", "test", "accepted"),
            ("user123", "player1", "pending"),
            ("admin", "test", "accepted"),
            ("admin", "player1", "accepted"),
            ("test", "player2", "accepted"),
            ("player1", "player2", "accepted"),
        ]
        
        inserted_count = 0
        skipped_count = 0
        
        # Insert friendships
        for user1_name, user2_name, status in friendships:
            if user1_name not in user_ids or user2_name not in user_ids:
                print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] [SKIP] User not found: {user1_name} or {user2_name}")
                skipped_count += 1
                continue
            
            user1_id = user_ids[user1_name]
            user2_id = user_ids[user2_name]
            
            try:
                # Insert friendship (user1 -> user2)
                cursor.execute('''
                    INSERT INTO friends (user_id, friend_id, status) 
                    VALUES (%s, %s, %s)
                ''', (user1_id, user2_id, status))
                
                # Insert reverse friendship (user2 -> user1) if accepted
                # This makes the friendship bidirectional
                if status == 'accepted':
                    cursor.execute('''
                        INSERT INTO friends (user_id, friend_id, status) 
                        VALUES (%s, %s, %s)
                    ''', (user2_id, user1_id, status))
                
                print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] [OK] Created friendship: {user1_name} -> {user2_name} ({status})")
                if status == 'accepted':
                    print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] [OK] Created reverse friendship: {user2_name} -> {user1_name} ({status})")
                inserted_count += 1
            except psycopg2.IntegrityError:
                print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] [SKIP] Friendship already exists: {user1_name} -> {user2_name}")
                skipped_count += 1
        
        # Record this migration
        cursor.execute('''
            INSERT INTO migrations (version, name) 
            VALUES (%s, %s)
            ON CONFLICT (version) DO NOTHING
        ''', ('004', 'seed_friends'))
        
        conn.commit()
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ✅ Migration 004 completed!")
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}]    Inserted: {inserted_count} friendships")
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}]    Skipped: {skipped_count} friendships")
        
    except Exception as e:
        conn.rollback()
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ❌ Migration 004 failed: {str(e)}")
        raise
    finally:
        conn.close()

def down():
    """Rollback migration - remove friends relationships"""
    print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ⬇️  Rolling back migration 004...")
    
    conn = get_db_connection()
    cursor = conn.cursor()
    
    try:
        # Get user IDs by username
        user_ids = {}
        cursor.execute('SELECT id, username FROM users')
        for row in cursor.fetchall():
            user_ids[row[1]] = row[0]
        
        # Remove friendships
        friendships = [
            ("user123", "admin"),
            ("user123", "test"),
            ("user123", "player1"),
            ("admin", "test"),
            ("admin", "player1"),
            ("test", "player2"),
            ("player1", "player2"),
        ]
        
        for user1_name, user2_name in friendships:
            if user1_name in user_ids and user2_name in user_ids:
                user1_id = user_ids[user1_name]
                user2_id = user_ids[user2_name]
                cursor.execute('DELETE FROM friends WHERE (user_id = %s AND friend_id = %s) OR (user_id = %s AND friend_id = %s)', 
                             (user1_id, user2_id, user2_id, user1_id))
        
        cursor.execute('DELETE FROM migrations WHERE version = %s', ('004',))
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

