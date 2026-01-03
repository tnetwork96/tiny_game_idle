"""
Script để gửi các bước đi (moves) vào game session Caro.

Usage (PowerShell):
  $env:SERVER_HOST="http://localhost:8080"
  $env:SESSION_ID="2"
  $env:USER_ID="3"
  python scripts/submit_move.py

Hoặc với danh sách moves:
  $env:ROW="7"
  $env:COL="10"
  python scripts/submit_move.py

Hoặc gửi nhiều moves tự động:
  python scripts/submit_move.py --auto
"""
import os
import requests
import time
import sys

SERVER_HOST = os.getenv("SERVER_HOST", "http://localhost:8080")
SESSION_ID = int(os.getenv("SESSION_ID", "2"))
USER_ID = int(os.getenv("USER_ID", "3"))


def submit_move(session_id: int, user_id: int, row: int, col: int) -> dict:
    """Gửi một bước đi vào game session."""
    url = f"{SERVER_HOST}/api/games/{session_id}/move"
    payload = {
        "user_id": user_id,
        "row": row,
        "col": col,
    }
    
    try:
        print(f"  URL: {url}")
        print(f"  Payload: {payload}")
        resp = requests.post(url, json=payload, timeout=5)
        print(f"Submit move ({row}, {col}): {resp.status_code} - {resp.text}")
        
        if resp.status_code == 200:
            return resp.json()
        else:
            # Try to parse error message
            try:
                error_data = resp.json()
                error_msg = error_data.get("detail", error_data.get("message", resp.text))
                print(f"  Error detail: {error_msg}")
                return {"success": False, "message": error_msg}
            except:
                return {"success": False, "message": resp.text}
    except requests.exceptions.RequestException as e:
        print(f"Error submitting move: {e}")
        return {"success": False, "message": str(e)}


def get_game_state(session_id: int) -> dict:
    """Lấy trạng thái game hiện tại."""
    url = f"{SERVER_HOST}/api/games/{session_id}/state"
    
    try:
        print(f"  Getting state from: {url}")
        resp = requests.get(url, timeout=5)
        if resp.status_code == 200:
            return resp.json()
        else:
            print(f"Get state: {resp.status_code} - {resp.text}")
            # Try to parse error
            try:
                error_data = resp.json()
                error_msg = error_data.get("detail", error_data.get("message", resp.text))
                print(f"  Error detail: {error_msg}")
            except:
                pass
            return {"success": False}
    except requests.exceptions.RequestException as e:
        print(f"Error getting state: {e}")
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


def main():
    """Main function."""
    # Check if auto mode
    auto_mode = "--auto" in sys.argv
    
    if auto_mode:
        # Auto mode: gửi moves tự động theo pattern
        print(f"Auto mode: SESSION_ID={SESSION_ID}, USER_ID={USER_ID}")
        print("Gui moves tu dong (nhan Ctrl+C de dung)...\n")
        
        # Pattern moves: đường chéo từ (7, 7) đến (11, 11)
        moves = [
            (7, 7), (7, 8), (7, 9), (7, 10), (7, 11),  # Hàng ngang
            (8, 7), (8, 8), (8, 9), (8, 10), (8, 11),  # Hàng tiếp theo
        ]
        
        for i, (row, col) in enumerate(moves):
            print(f"Move {i+1}: ({row}, {col})")
            result = submit_move(SESSION_ID, USER_ID, row, col)
            
            if not result.get("success"):
                print(f"Move failed: {result.get('message', 'Unknown error')}")
                # Nếu không phải turn của mình, đợi một chút
                if "Not your turn" in result.get("message", ""):
                    print("Waiting for opponent's turn...")
                    time.sleep(2)
                    continue
                else:
                    break
            
            # Kiểm tra game status
            game_status = result.get("game_status", "playing")
            winner_id = result.get("winner_id")
            
            if game_status == "completed":
                if winner_id == USER_ID:
                    print("You won!")
                elif winner_id:
                    print(f"Player {winner_id} won!")
                else:
                    print("Draw!")
                break
            
            # Đợi trước move tiếp theo
            time.sleep(1)
        
        # In board cuối cùng
        state = get_game_state(SESSION_ID)
        if state.get("success") and "board" in state:
            print("\nFinal board:")
            print_board(state["board"])
    
    else:
        # Manual mode: gửi move từ environment variables hoặc input
        row = int(os.getenv("ROW", "7"))
        col = int(os.getenv("COL", "10"))
        
        print(f"Submitting move: SESSION_ID={SESSION_ID}, USER_ID={USER_ID}, ROW={row}, COL={col}")
        
        result = submit_move(SESSION_ID, USER_ID, row, col)
        
        if result.get("success"):
            print("Move submitted successfully!")
            print(f"   Move ID: {result.get('move_id')}")
            print(f"   Game Status: {result.get('game_status')}")
            if result.get("winner_id"):
                print(f"   Winner: {result.get('winner_id')}")
        else:
            print(f"Move failed: {result.get('message', 'Unknown error')}")
        
        # In board hiện tại
        state = get_game_state(SESSION_ID)
        if state.get("success") and "board" in state:
            print("\nCurrent board:")
            print_board(state["board"])
            print(f"Current turn: {state.get('current_turn')}")


if __name__ == "__main__":
    main()

