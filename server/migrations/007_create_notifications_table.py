"""
Migration: Create notifications table and migrate friend requests
Version: 007
Date: 2025-01-XX
"""
import psycopg2
import os
from datetime import datetime

def get_db_connection():
    """Get PostgreSQL database connection"""
    database_url = os.getenv("DATABASE_URL", "postgresql://tinygame:tinygame123@db:5432/tiny_game")
    return psycopg2.connect(database_url)

def up():
    """Run migration - create notifications table and migrate friend requests"""
    print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] 📦 Migration 007: Creating notifications table...")
    
    conn = get_db_connection()
    cursor = conn.cursor()
    
    try:
        # Step 1: Create notifications table
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] Creating notifications table...")
        cursor.execute('''
            CREATE TABLE IF NOT EXISTS notifications (
                id SERIAL PRIMARY KEY,
                user_id INTEGER NOT NULL REFERENCES users(id) ON DELETE CASCADE,
                type VARCHAR(50) NOT NULL,
                title VARCHAR(255),
                message TEXT NOT NULL,
                related_id INTEGER,
                read BOOLEAN DEFAULT FALSE,
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
            )
        ''')
        
        # Create indexes
        cursor.execute('CREATE INDEX IF NOT EXISTS idx_notifications_user_id ON notifications(user_id)')
        cursor.execute('CREATE INDEX IF NOT EXISTS idx_notifications_read ON notifications(read)')
        cursor.execute('CREATE INDEX IF NOT EXISTS idx_notifications_created_at ON notifications(created_at DESC)')
        cursor.execute('CREATE INDEX IF NOT EXISTS idx_notifications_type ON notifications(type)')
        
        # Step 2: Migrate existing friend requests to notifications
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] Migrating friend requests to notifications...")
        # Get friend requests and create notifications
        cursor.execute('''
            SELECT fr.id, fr.to_user_id, fr.created_at, u.username
            FROM friend_requests fr
            JOIN users u ON fr.from_user_id = u.id
            WHERE fr.status = 'pending'
        ''')
        
        friend_requests = cursor.fetchall()
        migrated_count = 0
        
        for fr_id, to_user_id, created_at, username in friend_requests:
            # Check if notification already exists
            cursor.execute('''
                SELECT id FROM notifications 
                WHERE user_id = %s AND type = 'friend_request' AND related_id = %s
            ''', (to_user_id, fr_id))
            
            if cursor.fetchone():
                continue
            
            # Create notification
            message = f"User '{username}' sent you a friend request"
            cursor.execute('''
                INSERT INTO notifications (user_id, type, message, related_id, read, created_at)
                VALUES (%s, 'friend_request', %s, %s, FALSE, %s)
            ''', (to_user_id, message, fr_id, created_at))
            migrated_count += 1
        
        migrated_count = cursor.rowcount
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ✅ Migrated {migrated_count} friend requests to notifications")
        
        # Record this migration
        cursor.execute('''
            INSERT INTO migrations (version, name) 
            VALUES (%s, %s)
            ON CONFLICT (version) DO NOTHING
        ''', ('007', 'create_notifications_table'))
        
        conn.commit()
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ✅ Migration 007 completed successfully")
        
    except Exception as e:
        conn.rollback()
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ❌ Migration 007 failed: {str(e)}")
        raise
    finally:
        cursor.close()
        conn.close()

def down():
    """Rollback migration - drop notifications table"""
    print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] 🔄 Rolling back Migration 007: Dropping notifications table...")
    
    conn = get_db_connection()
    cursor = conn.cursor()
    
    try:
        cursor.execute('DROP TABLE IF EXISTS notifications CASCADE')
        cursor.execute("DELETE FROM migrations WHERE version = '007'")
        conn.commit()
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ✅ Rollback completed")
    except Exception as e:
        conn.rollback()
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ❌ Rollback failed: {str(e)}")
        raise
    finally:
        cursor.close()
        conn.close()

