"""
ESP32-like login + websocket init helper.

Usage (PowerShell):
  cd "D:\\tiny game\\server"
  python scripts/esp32_login.py

Defaults:
  SERVER_HOST = "http://localhost:8080"
  USERNAME = "test"   # user_id=3 in DB
  PIN = "0000"

Behavior:
  - POST /api/login until success (small retries)
  - Extract user_id, then connect websocket ws://<host>/ws
  - Send init {"type":"init","device":"ESP32","user_id":<id>}
  - Log incoming messages (init_ack, game_event, etc.)
"""
import json
import time
import threading

import requests
import websocket

SERVER_HOST = "http://localhost:8080"
# Default: user 3 in DB (username: test, nickname: Tester)
USERNAME = "test"
PIN = "0000"


def login_loop(max_attempts: int = 5, delay: float = 1.0):
    url = f"{SERVER_HOST}/api/login"
    payload = {"username": USERNAME, "pin": PIN}
    for attempt in range(1, max_attempts + 1):
        try:
            resp = requests.post(url, json=payload, timeout=5)
            print(f"[login] attempt {attempt} status={resp.status_code} body={resp.text}")
            if resp.status_code == 200:
                data = resp.json()
                if data.get("success") and data.get("user_id"):
                    return data.get("user_id")
            time.sleep(delay)
        except Exception as exc:
            print(f"[login] error: {exc}")
            time.sleep(delay)
    return None


def ws_reader(ws: websocket.WebSocketApp):
    ws.run_forever()


def main():
    user_id = login_loop()
    if not user_id:
        print("Login failed, aborting.")
        return

    ws_url = SERVER_HOST.replace("http", "ws") + "/ws"
    print(f"[ws] connecting to {ws_url} with user_id={user_id}")

    def on_open(wsapp):
        init_msg = {
            "type": "init",
            "device": "ESP32",
            "user_id": user_id,
        }
        wsapp.send(json.dumps(init_msg))
        print("[ws] sent init:", init_msg)

    def on_message(wsapp, message):
        print("[ws] recv:", message)

    def on_error(wsapp, error):
        print("[ws] error:", error)

    def on_close(wsapp, close_status_code, close_msg):
        print(f"[ws] closed code={close_status_code} msg={close_msg}")

    wsapp = websocket.WebSocketApp(
        ws_url,
        on_open=on_open,
        on_message=on_message,
        on_error=on_error,
        on_close=on_close,
    )

    # Run in foreground
    wsapp.run_forever()


if __name__ == "__main__":
    main()

