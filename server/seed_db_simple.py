import sqlite3
import hashlib

DB_PATH = '/app/tiny_game.db'

conn = sqlite3.connect(DB_PATH)
cursor = conn.cursor()

# Create table
cursor.execute('''
    CREATE TABLE IF NOT EXISTS users (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        username TEXT UNIQUE NOT NULL,
        pin TEXT NOT NULL,
        created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
    )
''')

# Test users
test_users = [
    {'username': 'user123', 'pin': '1234'},
    {'username': 'admin', 'pin': 'admin123'},
    {'username': 'test', 'pin': 'test123'},
    {'username': 'player1', 'pin': '1111'},
    {'username': 'player2', 'pin': '2222'},
]

# Insert users
for user in test_users:
    pin_hash = hashlib.sha256(user['pin'].encode()).hexdigest()
    try:
        cursor.execute('INSERT INTO users (username, pin) VALUES (?, ?)', (user['username'], pin_hash))
        print(f'[OK] Created user: {user["username"]} (PIN: {user["pin"]})')
    except sqlite3.IntegrityError:
        print(f'[SKIP] User already exists: {user["username"]}')

conn.commit()
conn.close()
print('\n[SUCCESS] Database seeded successfully!')

