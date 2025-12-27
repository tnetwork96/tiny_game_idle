"""
Migration: Add nickname column to users table
Version: 009
Date: 2025-01-27
"""
import psycopg2
import os
from datetime import datetime

def get_db_connection():
    """Get PostgreSQL database connection"""
    database_url = os.getenv("DATABASE_URL", "postgresql://tinygame:tinygame123@db:5432/tiny_game")
    return psycopg2.connect(database_url)

def up():
    """Run migration - add nickname column to users table"""
    print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] 📦 Migration 009: Adding nickname column to users table...")
    
    conn = get_db_connection()
    cursor = conn.cursor()
    
    try:
        # Add nickname column to users table
        cursor.execute('''
            ALTER TABLE users 
            ADD COLUMN IF NOT EXISTS nickname VARCHAR(255)
        ''')
        
        # Update existing users with default nicknames
        default_nicknames = {
            "user123": "User123",
            "admin": "Admin",
            "test": "TestUser",
            "player1": "Player One",
            "player2": "Player Two"
        }
        
        updated_count = 0
        for username, nickname in default_nicknames.items():
            cursor.execute('''
                UPDATE users 
                SET nickname = %s 
                WHERE username = %s AND (nickname IS NULL OR nickname = '')
            ''', (nickname, username))
            if cursor.rowcount > 0:
                print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] [OK] Updated nickname for {username}: {nickname}")
                updated_count += 1
            else:
                # Check if user exists
                cursor.execute('SELECT id FROM users WHERE username = %s', (username,))
                if cursor.fetchone():
                    print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] [SKIP] User {username} already has a nickname")
                else:
                    print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] [SKIP] User {username} does not exist")
        
        # Record this migration
        cursor.execute('''
            INSERT INTO migrations (version, name) 
            VALUES (%s, %s)
            ON CONFLICT (version) DO NOTHING
        ''', ('009', 'add_nickname_column'))
        
        conn.commit()
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ✅ Migration 009 completed successfully!")
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}]    Updated: {updated_count} users with nicknames")
        
    except Exception as e:
        conn.rollback()
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ❌ Migration 009 failed: {str(e)}")
        raise
    finally:
        conn.close()

def down():
    """Rollback migration - remove nickname column"""
    print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ⬇️  Rolling back migration 009...")
    
    conn = get_db_connection()
    cursor = conn.cursor()
    
    try:
        cursor.execute('ALTER TABLE users DROP COLUMN IF EXISTS nickname')
        cursor.execute('DELETE FROM migrations WHERE version = %s', ('009',))
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

