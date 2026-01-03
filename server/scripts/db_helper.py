"""
Script để kiểm tra và can thiệp database cho game sessions.
"""
import os
import psycopg2
from psycopg2.extras import RealDictCursor

DATABASE_URL = os.getenv(
    "DATABASE_URL",
    "postgresql://tinygame:tinygame123@localhost:5432/tiny_game",
)


def get_db_connection():
    """Get PostgreSQL database connection"""
    return psycopg2.connect(DATABASE_URL)


def check_session(session_id: int):
    """Kiểm tra session và in thông tin chi tiết"""
    conn = get_db_connection()
    cursor = conn.cursor(cursor_factory=RealDictCursor)
    
    try:
        # Get session info
        cursor.execute(
            """
            SELECT id, host_user_id, game_type, status, created_at, updated_at, started_at, completed_at
            FROM game_sessions
            WHERE id = %s
            """,
            (session_id,),
        )
        session = cursor.fetchone()
        
        if not session:
            print(f"ERROR: Session {session_id} not found!")
            return None
        
        print(f"\nSession {session_id}:")
        print(f"   Host User ID: {session['host_user_id']}")
        print(f"   Game Type: {session['game_type']}")
        print(f"   Status: {session['status']}")
        print(f"   Created: {session['created_at']}")
        print(f"   Started: {session.get('started_at', 'N/A')}")
        
        # Get participants
        cursor.execute(
            """
            SELECT user_id, role, status, ready
            FROM game_participants
            WHERE session_id = %s
            ORDER BY role DESC, user_id ASC
            """,
            (session_id,),
        )
        participants = cursor.fetchall()
        print(f"\nParticipants ({len(participants)}):")
        for p in participants:
            print(f"   User {p['user_id']}: {p['role']} - {p['status']} (ready: {p['ready']})")
        
        # Get moves
        cursor.execute(
            """
            SELECT id, user_id, row, col, move_number, created_at
            FROM game_moves
            WHERE session_id = %s
            ORDER BY move_number ASC
            """,
            (session_id,),
        )
        moves = cursor.fetchall()
        print(f"\nMoves ({len(moves)}):")
        for m in moves:
            print(f"   Move {m['move_number']}: User {m['user_id']} -> ({m['row']}, {m['col']})")
        
        # Determine current turn
        host_id = session['host_user_id']
        guest_id = next((p['user_id'] for p in participants if p['user_id'] != host_id), None)
        move_count = len(moves)
        current_turn = host_id if move_count % 2 == 0 else guest_id
        print(f"\nCurrent turn: User {current_turn} (move count: {move_count})")
        
        return {
            'session': session,
            'participants': participants,
            'moves': moves,
            'current_turn': current_turn,
            'host_id': host_id,
            'guest_id': guest_id,
        }
    finally:
        cursor.close()
        conn.close()


def reset_session(session_id: int):
    """Reset session: xóa tất cả moves và set status về in_progress"""
    conn = get_db_connection()
    cursor = conn.cursor()
    
    try:
        # Delete all moves
        cursor.execute("DELETE FROM game_moves WHERE session_id = %s", (session_id,))
        deleted_moves = cursor.rowcount
        
        # Set status to in_progress
        cursor.execute(
            """
            UPDATE game_sessions
            SET status = 'in_progress', updated_at = CURRENT_TIMESTAMP
            WHERE id = %s
            """,
            (session_id,),
        )
        
        conn.commit()
        print(f"SUCCESS: Reset session {session_id}:")
        print(f"   Deleted {deleted_moves} moves")
        print(f"   Status set to 'in_progress'")
    except Exception as e:
        conn.rollback()
        print(f"ERROR: Error resetting session: {e}")
        raise
    finally:
        cursor.close()
        conn.close()


def delete_session(session_id: int):
    """Xóa session và tất cả dữ liệu liên quan"""
    conn = get_db_connection()
    cursor = conn.cursor()
    
    try:
        # Delete moves
        cursor.execute("DELETE FROM game_moves WHERE session_id = %s", (session_id,))
        deleted_moves = cursor.rowcount
        
        # Delete participants
        cursor.execute("DELETE FROM game_participants WHERE session_id = %s", (session_id,))
        deleted_participants = cursor.rowcount
        
        # Delete session
        cursor.execute("DELETE FROM game_sessions WHERE id = %s", (session_id,))
        deleted_session = cursor.rowcount
        
        conn.commit()
        print(f"SUCCESS: Deleted session {session_id}:")
        print(f"   Deleted {deleted_moves} moves")
        print(f"   Deleted {deleted_participants} participants")
        print(f"   Deleted {deleted_session} session")
    except Exception as e:
        conn.rollback()
        print(f"ERROR: Error deleting session: {e}")
        raise
    finally:
        cursor.close()
        conn.close()


def create_test_session(host_user_id: int, guest_user_id: int, game_type: str = "caro"):
    """Tạo session mới cho testing"""
    conn = get_db_connection()
    cursor = conn.cursor(cursor_factory=RealDictCursor)
    
    try:
        # Create session
        cursor.execute(
            """
            INSERT INTO game_sessions (host_user_id, game_type, status, max_players)
            VALUES (%s, %s, 'in_progress', 2)
            RETURNING id
            """,
            (host_user_id, game_type),
        )
        session_id = cursor.fetchone()['id']
        
        # Add host participant
        cursor.execute(
            """
            INSERT INTO game_participants (session_id, user_id, role, status, ready)
            VALUES (%s, %s, 'host', 'accepted', true)
            """,
            (session_id, host_user_id),
        )
        
        # Add guest participant
        cursor.execute(
            """
            INSERT INTO game_participants (session_id, user_id, role, status, ready)
            VALUES (%s, %s, 'player', 'accepted', true)
            """,
            (session_id, guest_user_id),
        )
        
        conn.commit()
        print(f"SUCCESS: Created test session {session_id}:")
        print(f"   Host: User {host_user_id}")
        print(f"   Guest: User {guest_user_id}")
        print(f"   Game Type: {game_type}")
        print(f"   Status: in_progress")
        return session_id
    except Exception as e:
        conn.rollback()
        print(f"ERROR: Error creating session: {e}")
        raise
    finally:
        cursor.close()
        conn.close()


def delete_all_sessions():
    """Xóa tất cả sessions và dữ liệu liên quan."""
    conn = get_db_connection()
    cursor = conn.cursor()
    
    try:
        # Count sessions first
        cursor.execute("SELECT COUNT(*) FROM game_sessions")
        session_count = cursor.fetchone()[0]
        
        # Count moves
        cursor.execute("SELECT COUNT(*) FROM game_moves")
        move_count = cursor.fetchone()[0]
        
        # Count participants
        cursor.execute("SELECT COUNT(*) FROM game_participants")
        participant_count = cursor.fetchone()[0]
        
        # Delete all moves (will be deleted by CASCADE anyway, but let's be explicit)
        cursor.execute("DELETE FROM game_moves")
        deleted_moves = cursor.rowcount
        
        # Delete all participants
        cursor.execute("DELETE FROM game_participants")
        deleted_participants = cursor.rowcount
        
        # Delete all sessions
        cursor.execute("DELETE FROM game_sessions")
        deleted_sessions = cursor.rowcount
        
        conn.commit()
        print(f"SUCCESS: Deleted all sessions:")
        print(f"   Sessions: {deleted_sessions} (was {session_count})")
        print(f"   Participants: {deleted_participants} (was {participant_count})")
        print(f"   Moves: {deleted_moves} (was {move_count})")
    except Exception as e:
        conn.rollback()
        print(f"ERROR: Error deleting all sessions: {e}")
        raise
    finally:
        cursor.close()
        conn.close()


if __name__ == "__main__":
    import sys
    
    if len(sys.argv) < 2:
        print("Usage:")
        print("  python db_helper.py check <session_id>")
        print("  python db_helper.py reset <session_id>")
        print("  python db_helper.py delete <session_id>")
        print("  python db_helper.py create <host_user_id> <guest_user_id> [game_type]")
        print("  python db_helper.py delete_all")
        sys.exit(1)
    
    command = sys.argv[1]
    
    if command == "check":
        if len(sys.argv) < 3:
            print("Usage: python db_helper.py check <session_id>")
            sys.exit(1)
        session_id = int(sys.argv[2])
        check_session(session_id)
    elif command == "reset":
        if len(sys.argv) < 3:
            print("Usage: python db_helper.py reset <session_id>")
            sys.exit(1)
        session_id = int(sys.argv[2])
        reset_session(session_id)
        check_session(session_id)
    elif command == "delete":
        if len(sys.argv) < 3:
            print("Usage: python db_helper.py delete <session_id>")
            sys.exit(1)
        session_id = int(sys.argv[2])
        delete_session(session_id)
    elif command == "create":
        if len(sys.argv) < 4:
            print("Usage: python db_helper.py create <host_user_id> <guest_user_id> [game_type]")
            sys.exit(1)
        host_id = int(sys.argv[2])
        guest_id = int(sys.argv[3])
        game_type = sys.argv[4] if len(sys.argv) > 4 else "caro"
        session_id = create_test_session(host_id, guest_id, game_type)
        check_session(session_id)
    elif command == "delete_all":
        print("WARNING: This will delete ALL game sessions, participants, and moves!")
        print("Press Ctrl+C to cancel, or wait 3 seconds to continue...")
        import time
        time.sleep(3)
        delete_all_sessions()
    else:
        print(f"Unknown command: {command}")
        sys.exit(1)

