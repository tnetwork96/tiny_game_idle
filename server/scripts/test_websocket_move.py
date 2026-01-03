"""
Script để test WebSocket và gửi move event để kiểm tra ESP32 có nhận được không.
"""
import os
import requests
import websocket
import json
import time
import threading

SERVER_HOST = os.getenv("SERVER_HOST", "http://localhost:8080")
WS_HOST = SERVER_HOST.replace("http://", "ws://").replace("https://", "wss://")
SESSION_ID = int(os.getenv("SESSION_ID", "34"))
USER_ID = int(os.getenv("USER_ID", "5"))  # User ID của ESP32


def on_message(ws, message):
    """Handle WebSocket messages"""
    print(f"\n[WebSocket] Received: {message[:200]}")
    try:
        data = json.loads(message)
        if data.get("type") == "game_event":
            event = data.get("event", {})
            if event.get("event_type") == "move":
                print(f"  ✅ Move event received!")
                print(f"     Session: {event.get('session_id')}")
                print(f"     User: {event.get('user_id')}")
                print(f"     Move: ({event.get('row')}, {event.get('col')})")
                print(f"     Status: {event.get('game_status')}")
    except:
        pass


def on_error(ws, error):
    print(f"[WebSocket] Error: {error}")


def on_close(ws, close_status_code, close_msg):
    print("[WebSocket] Connection closed")


def on_open(ws):
    """Send init message when connected"""
    init_msg = {
        "type": "init",
        "device": "test_script",
        "user_id": USER_ID
    }
    ws.send(json.dumps(init_msg))
    print(f"[WebSocket] Connected and sent init for User {USER_ID}")


def main():
    print(f"Testing WebSocket for User {USER_ID} on session {SESSION_ID}")
    print(f"WebSocket URL: {WS_HOST}/ws")
    print("Waiting for connection...\n")
    
    # Connect WebSocket
    ws_url = f"{WS_HOST}/ws"
    ws = websocket.WebSocketApp(
        ws_url,
        on_message=on_message,
        on_error=on_error,
        on_close=on_close,
        on_open=on_open
    )
    
    # Run WebSocket in a thread
    wst = threading.Thread(target=ws.run_forever)
    wst.daemon = True
    wst.start()
    
    # Wait for connection
    time.sleep(2)
    
    # Submit a move to trigger event
    print(f"\nSubmitting move for User {USER_ID}...")
    url = f"{SERVER_HOST}/api/games/{SESSION_ID}/move"
    payload = {
        "user_id": USER_ID,
        "row": 4,
        "col": 4,
    }
    
    try:
        resp = requests.post(url, json=payload, timeout=5)
        print(f"Move response: {resp.status_code} - {resp.text[:200]}")
        
        if resp.status_code == 200:
            result = resp.json()
            if result.get("success"):
                print("\nSUCCESS: Move submitted successfully!")
                print("Waiting for WebSocket event (10 seconds)...")
                time.sleep(10)
            else:
                print(f"Move failed: {result.get('message', resp.text)}")
        else:
            print(f"Move failed: {resp.text}")
    except Exception as e:
        print(f"ERROR: {e}")
    
    ws.close()
    print("\nTest completed")


if __name__ == "__main__":
    main()

