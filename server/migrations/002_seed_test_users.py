"""
Migration: Seed test users
Version: 002
Date: 2025-01-27
"""
import psycopg2
import hashlib
import os
from datetime import datetime

def get_db_connection():
    """Get PostgreSQL database connection"""
    database_url = os.getenv("DATABASE_URL", "postgresql://tinygame:tinygame123@db:5432/tiny_game")
    return psycopg2.connect(database_url)

def up():
    """Run migration - seed test users"""
    print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] 📦 Migration 002: Seeding test users...")
    
    conn = get_db_connection()
    cursor = conn.cursor()
    
    try:
        # Test users data
        test_users = [
            {"username": "user123", "pin": "1234"},
            {"username": "admin", "pin": "admin123"},
            {"username": "test", "pin": "test123"},
            {"username": "player1", "pin": "1111"},
            {"username": "player2", "pin": "2222"},
        ]
        
        inserted_count = 0
        skipped_count = 0
        
        # Insert users
        for user in test_users:
            pin_hash = hashlib.sha256(user["pin"].encode()).hexdigest()
            try:
                cursor.execute('''
                    INSERT INTO users (username, pin) 
                    VALUES (%s, %s)
                ''', (user["username"], pin_hash))
                print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] [OK] Created user: {user['username']} (PIN: {user['pin']})")
                inserted_count += 1
            except psycopg2.IntegrityError:
                print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] [SKIP] User already exists: {user['username']}")
                skipped_count += 1
        
        # Record this migration
        cursor.execute('''
            INSERT INTO migrations (version, name) 
            VALUES (%s, %s)
            ON CONFLICT (version) DO NOTHING
        ''', ('002', 'seed_test_users'))
        
        conn.commit()
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ✅ Migration 002 completed!")
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}]    Inserted: {inserted_count} users")
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}]    Skipped: {skipped_count} users")
        
    except Exception as e:
        conn.rollback()
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ❌ Migration 002 failed: {str(e)}")
        raise
    finally:
        conn.close()

def down():
    """Rollback migration - remove test users"""
    print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ⬇️  Rolling back migration 002...")
    
    conn = get_db_connection()
    cursor = conn.cursor()
    
    try:
        test_usernames = ["user123", "admin", "test", "player1", "player2"]
        for username in test_usernames:
            cursor.execute('DELETE FROM users WHERE username = %s', (username,))
        
        cursor.execute('DELETE FROM migrations WHERE version = %s', ('002',))
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

