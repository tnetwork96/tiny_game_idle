# Session management utilities
# Currently handled by ESP32ConnectionManager in websocket/esp32_handler.py
# This file can be extended for additional session management features

from typing import Dict, List
from datetime import datetime

def format_session_info(session_id: str, username: str, connected_at: datetime, last_activity: datetime) -> Dict:
    """Format session information for API responses"""
    return {
        "session_id": session_id,
        "username": username,
        "connected_at": connected_at.isoformat(),
        "last_activity": last_activity.isoformat()
    }

