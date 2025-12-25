from pydantic_settings import BaseSettings
from typing import Optional

class Settings(BaseSettings):
    # Server settings
    HOST: str = "0.0.0.0"
    PORT: int = 8000
    DEBUG: bool = True
    
    # WebSocket settings
    WS_MAX_CONNECTIONS: int = 100
    WS_HEARTBEAT_INTERVAL: int = 30  # seconds
    
    # Authentication
    SECRET_KEY: str = "your-secret-key-change-in-production"
    TOKEN_EXPIRE_MINUTES: int = 60
    
    # Database
    DATABASE_URL: Optional[str] = None
    # If DATABASE_URL not set, use default PostgreSQL connection
    DB_HOST: str = "localhost"
    DB_PORT: int = 5432
    DB_NAME: str = "esp32_game_db"
    DB_USER: str = "gameuser"
    DB_PASSWORD: str = "gamepass123"
    
    @property
    def database_url(self) -> str:
        """Generate database URL from components if DATABASE_URL not set"""
        if self.DATABASE_URL:
            return self.DATABASE_URL
        return f"postgresql://{self.DB_USER}:{self.DB_PASSWORD}@{self.DB_HOST}:{self.DB_PORT}/{self.DB_NAME}"
    
    class Config:
        env_file = ".env"
        case_sensitive = True

settings = Settings()

