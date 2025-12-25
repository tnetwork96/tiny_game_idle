# Database Template and Seed Data Documentation

## Overview
This document contains the complete database schema, sample data structure, and seeding instructions for the ESP32 Game Server. Use this as a reference when setting up or resetting the database.

## Database Schema

### Tables Structure

#### 1. Users Table
```sql
CREATE TABLE users (
    id SERIAL PRIMARY KEY,
    username VARCHAR(50) UNIQUE NOT NULL,
    pin VARCHAR(20) NOT NULL,
    nickname VARCHAR(50),
    active BOOLEAN DEFAULT TRUE,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    last_login TIMESTAMP
);
```

**Fields:**
- `id`: Primary key, auto-increment
- `username`: Unique username (indexed)
- `pin`: User PIN/password
- `nickname`: Display name (optional)
- `active`: Account status (indexed)
- `created_at`, `updated_at`, `last_login`: Timestamps

#### 2. Sessions Table
```sql
CREATE TABLE sessions (
    id SERIAL PRIMARY KEY,
    session_id VARCHAR(36) UNIQUE NOT NULL,
    user_id INTEGER NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    connected_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    last_activity TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    disconnected_at TIMESTAMP,
    is_active BOOLEAN DEFAULT TRUE
);
```

**Fields:**
- `id`: Primary key
- `session_id`: UUID string (indexed, unique)
- `user_id`: Foreign key to users (indexed)
- `connected_at`, `last_activity`, `disconnected_at`: Timestamps
- `is_active`: Session status (indexed)

#### 3. Friends Table
```sql
CREATE TABLE friends (
    id SERIAL PRIMARY KEY,
    user_id INTEGER NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    friend_id INTEGER NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    UNIQUE(user_id, friend_id),
    CHECK(user_id != friend_id)
);
```

**Fields:**
- `id`: Primary key
- `user_id`, `friend_id`: Foreign keys to users (both indexed)
- `created_at`: Timestamp
- **Constraints:**
  - Unique constraint on (user_id, friend_id)
  - Check constraint: user_id != friend_id (no self-friendship)

#### 4. Messages Table
```sql
CREATE TABLE messages (
    id SERIAL PRIMARY KEY,
    sender_id INTEGER NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    receiver_id INTEGER NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    message TEXT NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    read_at TIMESTAMP
);
```

**Fields:**
- `id`: Primary key
- `sender_id`, `receiver_id`: Foreign keys to users (both indexed)
- `message`: Message content
- `created_at`: Timestamp (indexed)
- `read_at`: Read timestamp (optional)

### Indexes
All foreign keys and frequently queried fields are indexed:
- `idx_users_username` on `users(username)`
- `idx_users_active` on `users(active)`
- `idx_sessions_user_id` on `sessions(user_id)`
- `idx_sessions_active` on `sessions(is_active)`
- `idx_sessions_session_id` on `sessions(session_id)`
- `idx_friends_user_id` on `friends(user_id)`
- `idx_friends_friend_id` on `friends(friend_id)`
- `idx_messages_sender` on `messages(sender_id)`
- `idx_messages_receiver` on `messages(receiver_id)`
- `idx_messages_created_at` on `messages(created_at)`

## Sample Data

### Default Users

The database is seeded with the following users:

| Username | PIN | Nickname | Active |
|----------|-----|----------|--------|
| player1 | 4321 | Player One | TRUE |
| admin | 1234 | Administrator | TRUE |
| alice | 1111 | Alice | TRUE |
| bob | 2222 | Bob | TRUE |
| charlie | 3333 | Charlie | TRUE |
| diana | 4444 | Diana | TRUE |
| eve | 5555 | Eve | TRUE |
| frank | 6666 | Frank | TRUE |
| grace | 7777 | Grace | TRUE |
| henry | 8888 | Henry | TRUE |
| ivy | 9999 | Ivy | TRUE |
| jack | 0000 | Jack | TRUE |

### Friend Relationships

Friendships are **bidirectional** (if A is friend of B, then B is also friend of A).

**player1's friends:**
- alice
- bob
- charlie
- diana

**alice's friends:**
- player1
- bob
- eve
- grace

**bob's friends:**
- player1
- alice
- charlie
- frank

**charlie's friends:**
- player1
- bob
- diana
- henry

**diana's friends:**
- player1
- charlie
- eve
- ivy

**eve's friends:**
- alice
- diana
- frank
- jack

**frank's friends:**
- bob
- eve
- grace
- henry

**grace's friends:**
- alice
- frank
- henry
- ivy

**henry's friends:**
- charlie
- frank
- grace
- jack

**ivy's friends:**
- diana
- grace
- jack

**jack's friends:**
- eve
- henry
- ivy

### Friend Pairs (for Seeding)

The following pairs represent bidirectional friendships (21 unique pairs = 42 friend records):

```
player1 <-> alice
player1 <-> bob
player1 <-> charlie
player1 <-> diana
alice <-> bob
alice <-> eve
alice <-> grace
bob <-> charlie
bob <-> frank
charlie <-> diana
charlie <-> henry
diana <-> eve
diana <-> ivy
eve <-> frank
eve <-> jack
frank <-> grace
frank <-> henry
grace <-> henry
grace <-> ivy
henry <-> jack
ivy <-> jack
```

## Seeding Scripts

### Python Script: `scripts/seed_data.py`

**Usage:**
```bash
cd server
python -m scripts.seed_data
# Or from project root:
python -m server.scripts.seed_data
```

**Features:**
- Idempotent: Can be run multiple times safely
- Skips existing users and friendships
- Logs progress and results
- Creates bidirectional friendships automatically

### SQL Script: `seed_data.sql`

**Usage:**
```bash
docker exec -i esp32_game_db psql -U gameuser -d esp32_game_db < seed_data.sql
```

**Or direct SQL:**
```bash
docker exec -it esp32_game_db psql -U gameuser -d esp32_game_db
# Then paste and run seed_data.sql content
```

## Database Connection

### Docker Setup

**Start Database:**
```bash
cd server
docker-compose up -d
```

**Stop Database:**
```bash
docker-compose down
```

**Reset Database (removes all data):**
```bash
docker-compose down -v  # Removes volumes
docker-compose up -d     # Recreates with fresh data
```

### Connection Details

- **Host:** localhost
- **Port:** 5432
- **Database:** esp32_game_db
- **User:** gameuser
- **Password:** gamepass123
- **Connection URL:** `postgresql://gameuser:gamepass123@localhost:5432/esp32_game_db`

### Connect to Database

```bash
docker exec -it esp32_game_db psql -U gameuser -d esp32_game_db
```

## Verification Queries

### Count Users
```sql
SELECT COUNT(*) as total_users FROM users;
```

### List All Users
```sql
SELECT id, username, nickname, active, created_at FROM users ORDER BY id;
```

### Count Friendships
```sql
SELECT COUNT(*) / 2 as total_friendships FROM friends;
```

### List player1's Friends
```sql
SELECT u.username, u.nickname 
FROM friends f
JOIN users u ON f.friend_id = u.id
WHERE f.user_id = (SELECT id FROM users WHERE username = 'player1')
ORDER BY u.username;
```

### List All Friendships (Unique Pairs)
```sql
SELECT 
    u1.username as user1,
    u2.username as user2,
    f.created_at
FROM friends f
JOIN users u1 ON f.user_id = u1.id
JOIN users u2 ON f.friend_id = u2.id
WHERE f.user_id < f.friend_id  -- Avoid duplicates (bidirectional)
ORDER BY u1.username, u2.username;
```

### Check User's Friend Count
```sql
SELECT 
    u.username,
    COUNT(f.friend_id) as friend_count
FROM users u
LEFT JOIN friends f ON u.id = f.user_id
GROUP BY u.id, u.username
ORDER BY friend_count DESC, u.username;
```

## Expected Results After Seeding

- **Total Users:** 12 (2 default + 10 new)
- **Total Friend Records:** 42 (21 bidirectional friendships)
- **player1 Friends:** 4 (alice, bob, charlie, diana)
- **All Users Active:** TRUE

## Sample Login Credentials for ESP32 Testing

| Username | PIN | Nickname | Friend Count |
|----------|-----|----------|--------------|
| player1 | 4321 | Player One | 4 |
| admin | 1234 | Administrator | 0 |
| alice | 1111 | Alice | 4 |
| bob | 2222 | Bob | 4 |
| charlie | 3333 | Charlie | 3 |
| diana | 4444 | Diana | 3 |
| eve | 5555 | Eve | 3 |
| frank | 6666 | Frank | 3 |
| grace | 7777 | Grace | 3 |
| henry | 8888 | Henry | 3 |
| ivy | 9999 | Ivy | 3 |
| jack | 0000 | Jack | 3 |

## Database Models (SQLAlchemy)

### User Model
```python
class User(Base):
    __tablename__ = "users"
    
    id = Column(Integer, primary_key=True, index=True)
    username = Column(String(50), unique=True, nullable=False, index=True)
    pin = Column(String(20), nullable=False)
    nickname = Column(String(50))
    active = Column(Boolean, default=True, index=True)
    created_at = Column(DateTime(timezone=True), server_default=func.now())
    updated_at = Column(DateTime(timezone=True), server_default=func.now(), onupdate=func.now())
    last_login = Column(DateTime(timezone=True))
```

### Session Model
```python
class Session(Base):
    __tablename__ = "sessions"
    
    id = Column(Integer, primary_key=True, index=True)
    session_id = Column(String(36), unique=True, nullable=False, index=True)
    user_id = Column(Integer, ForeignKey("users.id", ondelete="CASCADE"), nullable=False, index=True)
    connected_at = Column(DateTime(timezone=True), server_default=func.now())
    last_activity = Column(DateTime(timezone=True), server_default=func.now())
    disconnected_at = Column(DateTime(timezone=True))
    is_active = Column(Boolean, default=True, index=True)
```

### Friend Model
```python
class Friend(Base):
    __tablename__ = "friends"
    
    id = Column(Integer, primary_key=True, index=True)
    user_id = Column(Integer, ForeignKey("users.id", ondelete="CASCADE"), nullable=False, index=True)
    friend_id = Column(Integer, ForeignKey("users.id", ondelete="CASCADE"), nullable=False, index=True)
    created_at = Column(DateTime(timezone=True), server_default=func.now())
    
    __table_args__ = (
        UniqueConstraint('user_id', 'friend_id', name='unique_friendship'),
        CheckConstraint('user_id != friend_id', name='no_self_friend')
    )
```

### Message Model
```python
class Message(Base):
    __tablename__ = "messages"
    
    id = Column(Integer, primary_key=True, index=True)
    sender_id = Column(Integer, ForeignKey("users.id", ondelete="CASCADE"), nullable=False, index=True)
    receiver_id = Column(Integer, ForeignKey("users.id", ondelete="CASCADE"), nullable=False, index=True)
    message = Column(Text, nullable=False)
    created_at = Column(DateTime(timezone=True), server_default=func.now(), index=True)
    read_at = Column(DateTime(timezone=True))
```

## Seeding Script Structure

### Python Script Location
- `server/scripts/seed_data.py`
- `server/scripts/__init__.py`

### SQL Script Location
- `server/seed_data.sql`

### Key Functions

**seed_users(db: Session)**
- Creates sample users
- Skips existing users
- Returns: (created_count, skipped_count)

**seed_friends(db: Session)**
- Creates bidirectional friendships
- Skips existing friendships
- Returns: (created_count, skipped_count)

**seed_database()**
- Main function
- Initializes database tables
- Calls seed_users() and seed_friends()
- Logs summary

## Quick Reference

### Reset and Reseed Database
```bash
# Stop and remove volumes
cd server
docker-compose down -v

# Start fresh database
docker-compose up -d

# Wait for database to be ready (5-10 seconds)
sleep 5

# Seed data
python -m scripts.seed_data
```

### Check Database Status
```bash
# Check container status
docker-compose ps

# Check database connection
docker exec esp32_game_db psql -U gameuser -d esp32_game_db -c "SELECT version();"

# Count users
docker exec esp32_game_db psql -U gameuser -d esp32_game_db -c "SELECT COUNT(*) FROM users;"
```

## Notes

- **Bidirectional Friendships:** Each friendship is stored as two records (A->B and B->A)
- **Idempotent Seeding:** Scripts can be run multiple times without creating duplicates
- **Cascade Delete:** Deleting a user automatically deletes their sessions, friends, and messages
- **No Self-Friendship:** Database constraint prevents users from being friends with themselves
- **Unique Constraint:** Prevents duplicate friendships between the same two users

## File Locations

- Database models: `server/app/models/db_models.py`
- Database connection: `server/app/database.py`
- Seeding script: `server/scripts/seed_data.py`
- SQL script: `server/seed_data.sql`
- Docker compose: `server/docker-compose.yml`
- Database init: `server/init.sql`

## Environment Variables

Default database configuration (can be overridden in `.env`):
- `DB_HOST=localhost`
- `DB_PORT=5432`
- `DB_NAME=esp32_game_db`
- `DB_USER=gameuser`
- `DB_PASSWORD=gamepass123`

Or use full connection string:
- `DATABASE_URL=postgresql://gameuser:gamepass123@localhost:5432/esp32_game_db`

