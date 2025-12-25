from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware
from app.api import routes, websocket
from app.config import settings
from app.database import init_db
import logging
import sys

# Configure logging to output to stdout for Docker
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
    datefmt='%Y-%m-%d %H:%M:%S',
    stream=sys.stdout,
    force=True
)

# Ensure all loggers output to stdout
for logger_name in ['app', 'app.api', 'app.websocket', 'app.services', 'uvicorn', 'uvicorn.access']:
    logger_obj = logging.getLogger(logger_name)
    logger_obj.setLevel(logging.INFO)
    # Don't clear handlers, just ensure they use stdout
    for handler in logger_obj.handlers:
        if hasattr(handler, 'stream') and handler.stream != sys.stdout:
            handler.stream = sys.stdout

logger = logging.getLogger(__name__)

app = FastAPI(
    title="ESP32 Game Server",
    description="Server for ESP32 game device communication",
    version="1.0.0"
)

# Initialize database on startup
@app.on_event("startup")
async def startup_event():
    logger.info("Initializing database...")
    try:
        init_db()
        logger.info("Database initialized successfully")
    except Exception as e:
        logger.error(f"Failed to initialize database: {e}")

# CORS middleware for ESP32
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],  # In production, specify ESP32 IPs
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

# Include routers
app.include_router(routes.router, prefix="/api", tags=["api"])
app.include_router(websocket.router, tags=["websocket"])

@app.get("/")
async def root():
    return {"message": "ESP32 Game Server", "status": "running"}

@app.get("/health")
async def health_check():
    return {"status": "healthy"}

