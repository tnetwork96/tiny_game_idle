# Database Migrations

This directory contains database migration scripts for the Tiny Game database.

## Structure

- `001_create_users_table.py` - Creates the `users` table
- `002_seed_test_users.py` - Seeds test users data

## Usage

### Run all migrations

```bash
# In Docker container
docker exec tiny-game-api python3 /app/migrate.py

# Local
cd server
python3 migrate.py
```

### Run specific migration

```bash
# In Docker container
docker exec tiny-game-api python3 /app/migrations/001_create_users_table.py

# Local
cd server
python3 migrations/001_create_users_table.py
```

### Check applied migrations

```bash
# In Docker container
docker exec tiny-game-api sqlite3 /app/tiny_game.db "SELECT * FROM migrations;"

# Local
sqlite3 server/tiny_game.db "SELECT * FROM migrations;"
```

## Migration Format

Each migration file should have:
- `up()` function - runs the migration
- `down()` function - rolls back the migration
- Version number in filename (e.g., `001_`, `002_`)

## Creating New Migrations

1. Create a new file: `00X_description.py`
2. Implement `up()` and `down()` functions
3. Use `get_db_path()` helper to get database path
4. Record migration in `migrations` table

Example:
```python
def up():
    conn = sqlite3.connect(get_db_path())
    cursor = conn.cursor()
    try:
        # Your migration code here
        cursor.execute('CREATE TABLE ...')
        cursor.execute('INSERT INTO migrations (version, name) VALUES (?, ?)', ('00X', 'description'))
        conn.commit()
    finally:
        conn.close()
```

