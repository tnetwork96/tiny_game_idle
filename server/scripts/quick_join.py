"""
Quick test script:
- If SESSION_ID is set -> only join that session as PARTICIPANT_ID (accept + ready).
- If SESSION_ID not set -> create a session as HOST_ID, invite PARTICIPANT_ID, then join.

Usage (PowerShell):
  $env:SERVER_HOST="http://localhost:8080"
  $env:HOST_ID="1"          # only used when creating
  $env:PARTICIPANT_ID="2"   # user joining
  # Optional: $env:SESSION_ID="123"  # skip create, just join this session
  python scripts/quick_join.py
"""
import os
import requests

SERVER_HOST = os.getenv("SERVER_HOST", "http://localhost:8080")
HOST_ID = int(os.getenv("HOST_ID", "5"))
PARTICIPANT_ID = int(os.getenv("PARTICIPANT_ID", "3"))
GAME_TYPE = os.getenv("GAME_TYPE", "caro")
# Hard default 4 (API requires >=2; 1 would cause 422)
MAX_PLAYERS = int(os.getenv("MAX_PLAYERS", "4"))
SESSION_ID_ENV = os.getenv("SESSION_ID", "2")


def create_session() -> int:
    url = f"{SERVER_HOST}/api/games/create"
    payload = {
        "host_user_id": HOST_ID,
        "game_type": GAME_TYPE,
        "max_players": MAX_PLAYERS,
        "participant_ids": [PARTICIPANT_ID],
    }
    resp = requests.post(url, json=payload, timeout=5)
    print("Create session:", resp.status_code, resp.text)
    resp.raise_for_status()
    data = resp.json()
    return data.get("session_id")


def participant_accept(session_id: int):
    url = f"{SERVER_HOST}/api/games/{session_id}/respond"
    payload = {
        "user_id": PARTICIPANT_ID,
        "accept": True,
        "ready_on_accept": True,
    }
    resp = requests.post(url, json=payload, timeout=5)
    print("Participant join:", resp.status_code, resp.text)
    resp.raise_for_status()


def main():
    if SESSION_ID_ENV:
        session_id = int(SESSION_ID_ENV)
        print(f"Using existing session_id={session_id} (skip create)")
    else:
        session_id = create_session()
    if not session_id:
        print("No session_id returned, abort.")
        return
    participant_accept(session_id)


if __name__ == "__main__":
    main()

