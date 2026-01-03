"""Quick test script to check move submission."""
import requests
import sys

SERVER_HOST = "http://localhost:8080"

# Create new session
print("1. Creating new session...")
resp = requests.post(
    f"{SERVER_HOST}/api/games/create",
    json={
        "host_user_id": 5,
        "game_type": "caro",
        "max_players": 2,
        "participant_ids": [3]
    },
    timeout=5
)
print(f"   Status: {resp.status_code}")
if resp.status_code != 200:
    print(f"   Error: {resp.text}")
    sys.exit(1)

data = resp.json()
session_id = data.get("session_id")
print(f"   Session ID: {session_id}")

# Join as participant
print("\n2. Joining as participant...")
resp2 = requests.post(
    f"{SERVER_HOST}/api/games/{session_id}/respond",
    json={
        "user_id": 3,
        "accept": True,
        "ready_on_accept": True
    },
    timeout=5
)
print(f"   Status: {resp2.status_code}")
if resp2.status_code != 200:
    print(f"   Error: {resp2.text}")

# Get game state
print("\n3. Getting game state...")
resp3 = requests.get(f"{SERVER_HOST}/api/games/{session_id}/state", timeout=5)
print(f"   Status: {resp3.status_code}")
if resp3.status_code == 200:
    state = resp3.json()
    print(f"   Success: {state.get('success')}")
    print(f"   Current turn: {state.get('current_turn')}")
    print(f"   Status: {state.get('status')}")
else:
    print(f"   Error: {resp3.text}")

# Submit move as host (user 5)
print("\n4. Submitting move as host (user 5)...")
resp4 = requests.post(
    f"{SERVER_HOST}/api/games/{session_id}/move",
    json={
        "user_id": 5,
        "row": 7,
        "col": 10
    },
    timeout=5
)
print(f"   Status: {resp4.status_code}")
print(f"   Response: {resp4.text[:200]}")

if resp4.status_code == 200:
    move_data = resp4.json()
    print(f"   Success: {move_data.get('success')}")
    print(f"   Game status: {move_data.get('game_status')}")
else:
    print(f"   Error details: {resp4.text}")

print(f"\nTest completed. Session ID: {session_id}")

