# Database Migration Guide

## Quick Start

### Run migrations in Docker

```bash
# Option 1: Migrations run automatically when container starts
docker-compose up -d

# Option 2: Run migrations manually
docker exec tiny-game-api python3 /app/migrate.py
```

### Run migrations locally

```bash
cd server
python3 migrate.py
```

## Migration Files

- `migrations/001_create_users_table.py` - Creates users table and migrations tracking table
- `migrations/002_seed_test_users.py` - Seeds test users (user123, admin, test, player1, player2)

## How It Works

1. **Migration Tracking**: All applied migrations are recorded in the `migrations` table
2. **Automatic Execution**: Migrations run automatically when Docker container starts
3. **Idempotent**: Running migrations multiple times is safe (skips already applied migrations)
4. **Rollback Support**: Each migration has a `down()` function for rollback

## Database Schema

### users table
```sql
CREATE TABLE users (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    username TEXT UNIQUE NOT NULL,
    pin TEXT NOT NULL,  -- SHA256 hash
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
```

### migrations table
```sql
CREATE TABLE migrations (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    version TEXT UNIQUE NOT NULL,
    name TEXT NOT NULL,
    applied_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
```

## Test Users

After running migrations, these test users are available:

- `user123` / PIN: `1234`
- `admin` / PIN: `admin123`
- `test` / PIN: `test123`
- `player1` / PIN: `1111`
- `player2` / PIN: `2222`

## Verify Migrations

```bash
# Check applied migrations
docker exec tiny-game-api sqlite3 /app/tiny_game.db "SELECT * FROM migrations;"

# Check users
docker exec tiny-game-api sqlite3 /app/tiny_game.db "SELECT id, username, created_at FROM users;"
```

## Creating New Migrations

1. Create file: `migrations/00X_description.py`
2. Implement `up()` and `down()` functions
3. Use `get_db_path()` helper
4. Record migration in `migrations` table

Example:
```python
def up():
    db_path = get_db_path()
    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()
    try:
        # Your migration code
        cursor.execute('ALTER TABLE users ADD COLUMN email TEXT')
        cursor.execute('INSERT INTO migrations (version, name) VALUES (?, ?)', ('003', 'add_email_column'))
        conn.commit()
    finally:
        conn.close()
```

