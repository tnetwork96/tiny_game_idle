"""
Migration: Create friends table
Version: 003
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
    """Run migration - create friends table"""
    print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] 📦 Migration 003: Creating friends table...")
    
    conn = get_db_connection()
    cursor = conn.cursor()
    
    try:
        # Create friends table
        cursor.execute('''
            CREATE TABLE IF NOT EXISTS friends (
                id SERIAL PRIMARY KEY,
                user_id INTEGER NOT NULL REFERENCES users(id) ON DELETE CASCADE,
                friend_id INTEGER NOT NULL REFERENCES users(id) ON DELETE CASCADE,
                status VARCHAR(20) DEFAULT 'pending' CHECK (status IN ('pending', 'accepted', 'blocked')),
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                UNIQUE(user_id, friend_id),
                CHECK (user_id != friend_id)
            )
        ''')
        
        # Create indexes for faster lookups
        cursor.execute('''
            CREATE INDEX IF NOT EXISTS idx_friends_user_id ON friends(user_id)
        ''')
        
        cursor.execute('''
            CREATE INDEX IF NOT EXISTS idx_friends_friend_id ON friends(friend_id)
        ''')
        
        cursor.execute('''
            CREATE INDEX IF NOT EXISTS idx_friends_status ON friends(status)
        ''')
        
        # Record this migration
        cursor.execute('''
            INSERT INTO migrations (version, name) 
            VALUES (%s, %s)
            ON CONFLICT (version) DO NOTHING
        ''', ('003', 'create_friends_table'))
        
        conn.commit()
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ✅ Migration 003 completed successfully!")
        
    except Exception as e:
        conn.rollback()
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ❌ Migration 003 failed: {str(e)}")
        raise
    finally:
        conn.close()

def down():
    """Rollback migration - drop friends table"""
    print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ⬇️  Rolling back migration 003...")
    
    conn = get_db_connection()
    cursor = conn.cursor()
    
    try:
        cursor.execute('DROP TABLE IF EXISTS friends')
        cursor.execute('DELETE FROM migrations WHERE version = %s', ('003',))
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

