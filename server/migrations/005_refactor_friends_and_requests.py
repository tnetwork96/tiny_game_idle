"""
Migration: Refactor friends table - remove status, create friend_requests table
Version: 005
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
    """Run migration - refactor friends table and create friend_requests"""
    print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] 📦 Migration 005: Refactoring friends table...")
    
    conn = get_db_connection()
    cursor = conn.cursor()
    
    try:
        # Step 1: Create friend_requests table
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] Creating friend_requests table...")
        cursor.execute('''
            CREATE TABLE IF NOT EXISTS friend_requests (
                id SERIAL PRIMARY KEY,
                from_user_id INTEGER NOT NULL REFERENCES users(id) ON DELETE CASCADE,
                to_user_id INTEGER NOT NULL REFERENCES users(id) ON DELETE CASCADE,
                status VARCHAR(20) DEFAULT 'pending' CHECK (status IN ('pending', 'accepted', 'rejected')),
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                UNIQUE(from_user_id, to_user_id),
                CHECK (from_user_id != to_user_id)
            )
        ''')
        
        # Create indexes for friend_requests
        cursor.execute('''
            CREATE INDEX IF NOT EXISTS idx_friend_requests_from_user ON friend_requests(from_user_id)
        ''')
        cursor.execute('''
            CREATE INDEX IF NOT EXISTS idx_friend_requests_to_user ON friend_requests(to_user_id)
        ''')
        cursor.execute('''
            CREATE INDEX IF NOT EXISTS idx_friend_requests_status ON friend_requests(status)
        ''')
        
        # Step 2: Migrate pending requests from friends to friend_requests
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] Migrating pending requests...")
        cursor.execute('''
            INSERT INTO friend_requests (from_user_id, to_user_id, status, created_at, updated_at)
            SELECT user_id, friend_id, status, created_at, updated_at
            FROM friends
            WHERE status = 'pending'
            ON CONFLICT (from_user_id, to_user_id) DO NOTHING
        ''')
        migrated_count = cursor.rowcount
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] Migrated {migrated_count} pending requests")
        
        # Step 3: Delete pending and blocked records from friends table
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] Removing non-accepted records from friends...")
        cursor.execute('''
            DELETE FROM friends 
            WHERE status != 'accepted' OR status IS NULL
        ''')
        deleted_count = cursor.rowcount
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] Deleted {deleted_count} non-accepted records")
        
        # Step 4: Remove status column from friends table
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] Removing status column from friends table...")
        cursor.execute('''
            ALTER TABLE friends 
            DROP COLUMN IF EXISTS status
        ''')
        
        # Step 5: Remove updated_at column from friends table (not needed)
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] Removing updated_at column from friends table...")
        cursor.execute('''
            ALTER TABLE friends 
            DROP COLUMN IF EXISTS updated_at
        ''')
        
        # Step 6: Remove status index (no longer needed)
        cursor.execute('''
            DROP INDEX IF EXISTS idx_friends_status
        ''')
        
        # Record this migration
        cursor.execute('''
            INSERT INTO migrations (version, name) 
            VALUES (%s, %s)
            ON CONFLICT (version) DO NOTHING
        ''', ('005', 'refactor_friends_and_requests'))
        
        conn.commit()
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ✅ Migration 005 completed successfully!")
        
    except Exception as e:
        conn.rollback()
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ❌ Migration 005 failed: {str(e)}")
        raise
    finally:
        conn.close()

def down():
    """Rollback migration"""
    print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ⬇️  Rolling back migration 005...")
    
    conn = get_db_connection()
    cursor = conn.cursor()
    
    try:
        # Add status and updated_at columns back
        cursor.execute('''
            ALTER TABLE friends 
            ADD COLUMN IF NOT EXISTS status VARCHAR(20) DEFAULT 'accepted'
        ''')
        cursor.execute('''
            ALTER TABLE friends 
            ADD COLUMN IF NOT EXISTS updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        ''')
        
        # Migrate back from friend_requests
        cursor.execute('''
            INSERT INTO friends (user_id, friend_id, status, created_at, updated_at)
            SELECT from_user_id, to_user_id, status, created_at, updated_at
            FROM friend_requests
            ON CONFLICT (user_id, friend_id) DO NOTHING
        ''')
        
        # Recreate status index
        cursor.execute('''
            CREATE INDEX IF NOT EXISTS idx_friends_status ON friends(status)
        ''')
        
        # Drop friend_requests table
        cursor.execute('DROP TABLE IF EXISTS friend_requests')
        
        cursor.execute('DELETE FROM migrations WHERE version = %s', ('005',))
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

