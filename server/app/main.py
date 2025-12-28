"""
FastAPI main application file
Example integration of WebSocket endpoint
"""
from fastapi import FastAPI, WebSocket
from fastapi.middleware.cors import CORSMiddleware
from app.api.websocket import websocket_endpoint
from app.api.auth import router as auth_router
from app.api.friends import router as friends_router
from datetime import datetime
import uvicorn

app = FastAPI(title="Tiny Game Server", version="1.0.0")

# Add CORS middleware to handle OPTIONS requests
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],  # Allow all origins (can be restricted in production)
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

# Include routers
app.include_router(auth_router)
app.include_router(friends_router)

@app.on_event("startup")
async def startup_event():
    print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] 🚀 Tiny Game Server starting...")
    print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] 📡 WebSocket endpoint available at: ws://0.0.0.0:8080/ws")

@app.get("/")
async def root():
    return {"message": "Tiny Game Server API", "status": "running"}

@app.get("/health")
async def health():
    return {"status": "healthy"}

@app.websocket("/ws")
async def websocket_route(websocket: WebSocket):
    """
    WebSocket endpoint at /ws
    Clients should connect and send init message: {"type":"init","device":"ESP32"}
    """
    print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ========================================")
    print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] WebSocket connection attempt received")
    client_ip = websocket.client.host if websocket.client else "unknown"
    print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] Client IP: {client_ip}")
    print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ========================================")
    await websocket_endpoint(websocket)

if __name__ == "__main__":
    uvicorn.run(app, host="0.0.0.0", port=8080)

