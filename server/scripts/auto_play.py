"""
Script để chơi tự động cho cả 2 players trong một session.
"""
import os
import requests
import time
import sys

SERVER_HOST = os.getenv("SERVER_HOST", "http://localhost:8080")
SESSION_ID = int(os.getenv("SESSION_ID", "32"))


def submit_move(session_id: int, user_id: int, row: int, col: int) -> dict:
    """Gửi một bước đi vào game session."""
    url = f"{SERVER_HOST}/api/games/{session_id}/move"
    payload = {
        "user_id": user_id,
        "row": row,
        "col": col,
    }
    
    try:
        resp = requests.post(url, json=payload, timeout=5)
        if resp.status_code == 200:
            return resp.json()
        else:
            try:
                error_data = resp.json()
                return {"success": False, "message": error_data.get("detail", resp.text)}
            except:
                return {"success": False, "message": resp.text}
    except requests.exceptions.RequestException as e:
        return {"success": False, "message": str(e)}


def get_game_state(session_id: int) -> dict:
    """Lấy trạng thái game hiện tại."""
    url = f"{SERVER_HOST}/api/games/{session_id}/state"
    
    try:
        resp = requests.get(url, timeout=5)
        if resp.status_code == 200:
            return resp.json()
        else:
            return {"success": False}
    except requests.exceptions.RequestException as e:
        return {"success": False}


def print_board(board: list):
    """In bàn cờ ra console (15x20)."""
    print("\n   ", end="")
    for col in range(min(20, len(board[0]) if board else 0)):
        print(f"{col:2}", end="")
    print()
    
    for row in range(min(15, len(board) if board else 0)):
        print(f"{row:2} ", end="")
        for col in range(min(20, len(board[row]) if board else 0)):
            cell = board[row][col]
            if cell is None:
                print(". ", end="")
            else:
                print(f"{cell} ", end="")
        print()
    print()


def auto_play(session_id: int, host_id: int, guest_id: int):
    """Chơi tự động cho cả 2 players."""
    print(f"Auto play: Session {session_id}, Host: {host_id}, Guest: {guest_id}")
    print("Playing automatically (Ctrl+C to stop)...\n")
    
    # Pattern moves cho cả 2 players
    moves_pattern = [
        (7, 7), (7, 8), (7, 9), (7, 10), (7, 11),
        (8, 7), (8, 8), (8, 9), (8, 10), (8, 11),
        (9, 7), (9, 8), (9, 9), (9, 10), (9, 11),
    ]
    
    move_index = 0
    max_moves = len(moves_pattern)
    
    while move_index < max_moves:
        # Get current state
        state = get_game_state(session_id)
        if not state.get("success"):
            print("Failed to get game state")
            break
        
        current_turn = state.get("current_turn")
        game_status = state.get("status")
        
        if game_status != "in_progress":
            print(f"Game ended with status: {game_status}")
            break
        
        # Determine which player to move
        if current_turn == host_id:
            user_id = host_id
            player_name = "Host"
        elif current_turn == guest_id:
            user_id = guest_id
            player_name = "Guest"
        else:
            print(f"Unknown current turn: {current_turn}")
            time.sleep(1)
            continue
        
        # Get next move
        if move_index >= len(moves_pattern):
            print("No more moves in pattern")
            break
        
        row, col = moves_pattern[move_index]
        move_index += 1
        
        print(f"Move {move_index}: {player_name} (User {user_id}) -> ({row}, {col})")
        
        # Submit move
        result = submit_move(session_id, user_id, row, col)
        
        if result.get("success"):
            print(f"  SUCCESS: Move submitted")
            game_status = result.get("game_status", "playing")
            winner_id = result.get("winner_id")
            
            if game_status == "completed":
                if winner_id:
                    print(f"  WINNER: User {winner_id}")
                else:
                    print("  DRAW")
                break
        else:
            error_msg = result.get("message", "Unknown error")
            print(f"  FAILED: {error_msg}")
            
            if "Not your turn" in error_msg:
                # Wait a bit and retry
                time.sleep(0.5)
                move_index -= 1  # Retry this move
                continue
            else:
                break
        
        # Print board
        state = get_game_state(session_id)
        if state.get("success") and "board" in state:
            print_board(state["board"])
        
        # Wait before next move
        time.sleep(0.5)
    
    # Print final board
    print("\n=== Final Board ===")
    state = get_game_state(session_id)
    if state.get("success") and "board" in state:
        print_board(state["board"])
        print(f"Game Status: {state.get('status')}")
        print(f"Move Count: {state.get('move_count', 0)}")


if __name__ == "__main__":
    # Get session info to determine host and guest
    url = f"{SERVER_HOST}/api/games/{SESSION_ID}"
    try:
        resp = requests.get(url, timeout=5)
        if resp.status_code == 200:
            data = resp.json()
            if data.get("success"):
                participants = data.get("participants", [])
                host_id = data.get("session_id")  # This is wrong, need to get from participants
                
                # Find host and guest from participants
                host_id = None
                guest_id = None
                for p in participants:
                    if p.get("role") == "host":
                        host_id = p.get("user_id")
                    elif p.get("role") == "player":
                        guest_id = p.get("user_id")
                
                if host_id and guest_id:
                    auto_play(SESSION_ID, host_id, guest_id)
                else:
                    print(f"ERROR: Could not determine host and guest from session {SESSION_ID}")
                    print(f"Participants: {participants}")
            else:
                print(f"ERROR: Session {SESSION_ID} not found or invalid")
        else:
            print(f"ERROR: Failed to get session info: {resp.status_code}")
    except Exception as e:
        print(f"ERROR: {e}")

