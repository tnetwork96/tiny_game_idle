# ESP32 Game Server

FastAPI server for ESP32 game device communication via WebSocket.

## Setup

### 1. Start PostgreSQL Database with Docker

```bash
# Start database
docker-compose up -d

# Check database status
docker-compose ps

# View database logs
docker-compose logs -f postgres
```

### 2. Install Python Dependencies

```bash
pip install -r requirements.txt
```

### 3. Configure Environment

```bash
cp .env.example .env
# Edit .env with your settings (database URL is already configured for Docker)
```

### 4. Run Server

```bash
uvicorn app.main:app --host 0.0.0.0 --port 8000 --reload
```

## Database Management

### Connect to Database

```bash
docker exec -it esp32_game_db psql -U gameuser -d esp32_game_db
```

### Seed Sample Data

Populate database with sample users and friend relationships:

**Method 1: Python Script (Recommended)**
```bash
cd server
python scripts/seed_data.py
```

**Method 2: SQL Script**
```bash
docker exec -i esp32_game_db psql -U gameuser -d esp32_game_db < seed_data.sql
```

**Method 3: Direct SQL**
```bash
docker exec -it esp32_game_db psql -U gameuser -d esp32_game_db
# Then paste and run seed_data.sql content
```

### Sample Login Credentials

After seeding, you can login with these credentials on ESP32:

| Username | PIN | Nickname |
|----------|-----|----------|
| player1 | 4321 | Player One |
| admin | 1234 | Administrator |
| alice | 1111 | Alice |
| bob | 2222 | Bob |
| charlie | 3333 | Charlie |
| diana | 4444 | Diana |
| eve | 5555 | Eve |
| frank | 6666 | Frank |
| grace | 7777 | Grace |
| henry | 8888 | Henry |
| ivy | 9999 | Ivy |
| jack | 0000 | Jack |

### Reset Database (Removes All Data)

```bash
docker-compose down -v  # Removes volumes
docker-compose up -d     # Recreates with fresh data
```

### Stop Database

```bash
docker-compose down
```

### Database Schema

The database includes the following tables:
- **users**: User accounts with username, PIN, nickname
- **sessions**: WebSocket sessions tracking
- **friends**: Friend relationships (bidirectional)
- **messages**: Chat messages (for future use)

## ESP32 Connection

ESP32 should connect to WebSocket endpoint:
```
ws://<server-ip>:8000/ws/esp32
```

### First Message (Login)
ESP32 must send login credentials as the first message:
```json
{
  "type": "login",
  "credentials": "username:player1-pin:4321"
}
```

### Server Response (Success)
```json
{
  "type": "login_success",
  "session_id": "uuid-string",
  "message": "Authentication successful"
}
```

### Server Response (Error)
```json
{
  "type": "error",
  "message": "Invalid credentials"
}
```

## API Endpoints

### HTTP Endpoints
- `GET /` - Server info
- `GET /health` - Health check
- `POST /api/login` - HTTP login (alternative to WebSocket)
- `GET /api/sessions` - List active WebSocket sessions
- `GET /api/sessions/count` - Get count of active sessions

### WebSocket Endpoint
- `WS /ws/esp32` - WebSocket for ESP32 connections

## Message Types

### Client to Server
- `login` - Authentication
- `ping` - Keep-alive
- `game_action` - Game-related actions
- `chat_message` - Chat messages

### Server to Client
- `login_success` - Authentication successful
- `error` - Error message
- `pong` - Response to ping
- `game_response` - Response to game action
- `chat_ack` - Chat message acknowledgment

## Project Structure

```
server/
├── app/
│   ├── main.py              # FastAPI app
│   ├── config.py            # Configuration
│   ├── api/                 # API routes
│   ├── models/              # Data models
│   ├── services/            # Business logic
│   └── websocket/           # WebSocket handlers
├── requirements.txt
├── .env.example
└── README.md
```

## Development

Run with auto-reload:
```bash
uvicorn app.main:app --reload
```

## Production

Run with production settings:
```bash
uvicorn app.main:app --host 0.0.0.0 --port 8000 --workers 4
```

## Notes

- Default users: `player1` (pin: `4321`), `admin` (pin: `1234`)
- Add more users in `app/services/auth_service.py`
- In production, replace in-memory user database with real database
- Configure CORS origins for security

