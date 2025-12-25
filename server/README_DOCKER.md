# Docker Setup for ESP32 Game Server

## Quick Start

### Start All Services (Database + API)
```bash
cd server
docker-compose up -d
```

### Stop All Services
```bash
docker-compose down
```

### View Logs
```bash
# All services
docker-compose logs -f

# API only
docker-compose logs -f api

# Database only
docker-compose logs -f postgres
```

### Rebuild API Container
```bash
docker-compose build api
docker-compose up -d api
```

## Services

### PostgreSQL Database
- **Container:** `esp32_game_db`
- **Port:** `5432`
- **Database:** `esp32_game_db`
- **User:** `gameuser`
- **Password:** `gamepass123`

### FastAPI Server
- **Container:** `esp32_game_api`
- **Port:** `8000`
- **URL:** `http://localhost:8000`
- **WebSocket:** `ws://localhost:8000/ws/esp32`

## Configuration

### Environment Variables

The API service uses environment variables defined in `docker-compose.yml`. To override:

1. Create `.env` file in `server/` directory
2. Set variables (see `.env.example`)
3. Restart services: `docker-compose restart api`

### Database Connection

In Docker, the database host is `postgres` (service name). For local development, use `localhost`.

**Docker (default):**
```
DB_HOST=postgres
```

**Local Development:**
```
DB_HOST=localhost
```

## Development

### Run with Hot Reload
The API service is configured with `--reload` flag for development. Code changes will automatically restart the server.

### Access Database
```bash
# From host
docker exec -it esp32_game_db psql -U gameuser -d esp32_game_db

# Or use docker-compose
docker-compose exec postgres psql -U gameuser -d esp32_game_db
```

### Seed Database
```bash
# Using Python script
docker-compose exec api python scripts/seed_data.py

# Or from host (if Python installed)
cd server
python scripts/seed_data.py
```

## Production

### Build for Production
```bash
docker-compose -f docker-compose.yml build
```

### Remove Development Volumes
```bash
docker-compose down -v
```

### Update Environment Variables
Edit `docker-compose.yml` or use `.env` file, then:
```bash
docker-compose up -d --force-recreate api
```

## Troubleshooting

### Check Service Status
```bash
docker-compose ps
```

### Check API Health
```bash
curl http://localhost:8000/health
```

### View API Logs
```bash
docker-compose logs api | tail -50
```

### Restart Services
```bash
docker-compose restart
```

### Rebuild Everything
```bash
docker-compose down
docker-compose build --no-cache
docker-compose up -d
```

## Network

Services communicate via Docker network `esp32_game_network`. The API connects to database using service name `postgres`.

## Volumes

- `postgres_data`: PostgreSQL data persistence
- `./app:/app/app`: API code (for hot reload in development)
- `./scripts:/app/scripts`: Scripts directory

## Ports

- `8000`: FastAPI server (HTTP + WebSocket)
- `5432`: PostgreSQL database

## Health Checks

- **PostgreSQL:** Checks if database is ready
- **API:** HTTP health check endpoint

## Notes

- Database initialization runs automatically on first start via `init.sql`
- API waits for database to be healthy before starting
- Use `--reload` flag for development (already configured)
- Remove `--reload` for production

