"""
Database seeding script - creates fake/test users
"""
import sqlite3
import hashlib
from datetime import datetime

def seed_database():
    """Create database and insert test users"""
    conn = sqlite3.connect('tiny_game.db')
    cursor = conn.cursor()
    
    # Create users table
    cursor.execute('''
        CREATE TABLE IF NOT EXISTS users (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            username TEXT UNIQUE NOT NULL,
            pin TEXT NOT NULL,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        )
    ''')
    
    # Test users data
    test_users = [
        {"username": "user123", "pin": "1234"},
        {"username": "admin", "pin": "admin123"},
        {"username": "test", "pin": "test123"},
        {"username": "player1", "pin": "1111"},
        {"username": "player2", "pin": "2222"},
    ]
    
    # Insert users
    for user in test_users:
        pin_hash = hashlib.sha256(user["pin"].encode()).hexdigest()
        try:
            cursor.execute('''
                INSERT INTO users (username, pin) 
                VALUES (?, ?)
            ''', (user["username"], pin_hash))
            print(f"[OK] Created user: {user['username']} (PIN: {user['pin']})")
        except sqlite3.IntegrityError:
            print(f"[SKIP] User already exists: {user['username']}")
    
    conn.commit()
    conn.close()
    print("\n[SUCCESS] Database seeded successfully!")
    print(f"Total users: {len(test_users)}")

if __name__ == "__main__":
    seed_database()

