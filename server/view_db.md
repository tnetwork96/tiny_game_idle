# Database Information

## Database Location
- **Docker Container**: `/app/tiny_game.db`
- **Local Path** (if mounted): `server/tiny_game.db`

## Tables in Database

### 1. `users` table
- **Columns**: `id`, `username`, `pin` (SHA256 hash), `created_at`
- **Rows**: 5 test users

### 2. `migrations` table  
- **Columns**: `id`, `version`, `name`, `applied_at`
- **Rows**: 2 migrations (001, 002)

## How to View Database

### Option 1: Using Docker exec
```bash
# View all tables
docker exec tiny-game-api python3 -c "import sqlite3; c=sqlite3.connect('/app/tiny_game.db'); print([r[0] for r in c.execute('SELECT name FROM sqlite_master WHERE type=\"table\"').fetchall()])"

# View users
docker exec tiny-game-api python3 -c "import sqlite3; c=sqlite3.connect('/app/tiny_game.db'); [print(r) for r in c.execute('SELECT id, username FROM users').fetchall()]"

# View migrations
docker exec tiny-game-api python3 -c "import sqlite3; c=sqlite3.connect('/app/tiny_game.db'); [print(r) for r in c.execute('SELECT * FROM migrations').fetchall()]"
```

### Option 2: Copy database to local
```bash
# Copy database from container to local
docker cp tiny-game-api:/app/tiny_game.db ./server/tiny_game.db

# Then use SQLite browser or command line tool
sqlite3 server/tiny_game.db ".tables"
sqlite3 server/tiny_game.db "SELECT * FROM users;"
```

### Option 3: Use Python script
```bash
# Run in container
docker exec tiny-game-api python3 << 'EOF'
import sqlite3
conn = sqlite3.connect('/app/tiny_game.db')
cursor = conn.cursor()

# List tables
cursor.execute("SELECT name FROM sqlite_master WHERE type='table'")
tables = cursor.fetchall()
print("Tables:", [t[0] for t in tables])

# Show users
cursor.execute("SELECT id, username, created_at FROM users")
users = cursor.fetchall()
print("\nUsers:")
for user in users:
    print(f"  ID: {user[0]}, Username: {user[1]}, Created: {user[2]}")

conn.close()
EOF
```

## Test Users
- `user123` / PIN: `1234`
- `admin` / PIN: `admin123`
- `test` / PIN: `test123`
- `player1` / PIN: `1111`
- `player2` / PIN: `2222`

