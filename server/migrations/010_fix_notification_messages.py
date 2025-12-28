"""
Migration: Fix old notification messages format
Version: 010
Date: 2025-12-28
Description: Updates old notifications with format "User 'username' sent you a friend request" 
             to new format "{display_name} sent you a friend request"
"""
import psycopg2
import os
import re
from datetime import datetime

def get_db_connection():
    """Get PostgreSQL database connection"""
    database_url = os.getenv("DATABASE_URL", "postgresql://tinygame:tinygame123@db:5432/tiny_game")
    return psycopg2.connect(database_url)

def up():
    """Run migration - fix old notification messages"""
    print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] 📦 Migration 010: Fixing old notification messages...")
    
    conn = get_db_connection()
    cursor = conn.cursor()
    
    try:
        # Find all notifications with old format "User 'username' sent you a friend request"
        cursor.execute('''
            SELECT id, message, related_id
            FROM notifications
            WHERE message LIKE 'User ''%'' sent you a friend request'
        ''')
        
        old_notifications = cursor.fetchall()
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] Found {len(old_notifications)} notifications with old format")
        
        updated_count = 0
        
        for notif_id, old_message, related_id in old_notifications:
            # Extract username from old format: "User 'username' sent you a friend request"
            match = re.match(r"User '([^']+)' sent you a friend request", old_message)
            if not match:
                print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ⚠️ Could not parse message: {old_message}")
                continue
            
            extracted_username = match.group(1)
            
            # Get the actual display_name (nickname with fallback to username) from the related friend_request
            if related_id:
                # Try to get from friend_requests table
                cursor.execute('''
                    SELECT fr.from_user_id, COALESCE(u.nickname, u.username) as display_name
                    FROM friend_requests fr
                    JOIN users u ON fr.from_user_id = u.id
                    WHERE fr.id = %s
                ''', (related_id,))
                
                result = cursor.fetchone()
                if result:
                    display_name = result[1]
                else:
                    # If friend_request not found, try to get from users table directly
                    cursor.execute('''
                        SELECT COALESCE(nickname, username) as display_name
                        FROM users
                        WHERE username = %s
                    ''', (extracted_username,))
                    user_result = cursor.fetchone()
                    display_name = user_result[0] if user_result else extracted_username
            else:
                # No related_id, try to get from users table directly
                cursor.execute('''
                    SELECT COALESCE(nickname, username) as display_name
                    FROM users
                    WHERE username = %s
                ''', (extracted_username,))
                user_result = cursor.fetchone()
                display_name = user_result[0] if user_result else extracted_username
            
            # Create new message with correct format
            new_message = f"{display_name} sent you a friend request"
            
            # Update the notification
            cursor.execute('''
                UPDATE notifications
                SET message = %s
                WHERE id = %s
            ''', (new_message, notif_id))
            
            updated_count += 1
            print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] Updated notification {notif_id}: '{old_message}' -> '{new_message}'")
        
        conn.commit()
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ✅ Updated {updated_count} notifications")
        
        # Record this migration
        cursor.execute('''
            INSERT INTO migrations (version, name) 
            VALUES (%s, %s)
            ON CONFLICT (version) DO NOTHING
        ''', ('010', 'fix_notification_messages'))
        
        conn.commit()
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ✅ Migration 010 completed successfully")
        
    except Exception as e:
        conn.rollback()
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ❌ Migration 010 failed: {str(e)}")
        import traceback
        print(traceback.format_exc())
        raise
    finally:
        cursor.close()
        conn.close()

def down():
    """Rollback migration - this migration only fixes data, no rollback needed"""
    print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] 🔄 Migration 010 is a data fix, no rollback needed")
    pass

