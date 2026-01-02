"""
Game sessions API
Senior-level multi-user game invite and lobby management
"""
from datetime import datetime
import os
from typing import List, Optional

from fastapi import APIRouter, HTTPException
from pydantic import BaseModel, conint
import psycopg2
from psycopg2.extras import RealDictCursor

from app.api.websocket import websocket_manager
from app.api.friends import are_friends


router = APIRouter(prefix="/api", tags=["games"])

# Database connection string from environment
DATABASE_URL = os.getenv(
    "DATABASE_URL",
    "postgresql://tinygame:tinygame123@localhost:5432/tiny_game",
)


def get_db_connection():
    """Get PostgreSQL database connection"""
    return psycopg2.connect(DATABASE_URL)


class CreateGameSessionRequest(BaseModel):
    host_user_id: int
    game_type: str
    max_players: conint(ge=2, le=8) = 4
    participant_ids: List[int] = []


class GameSessionResponse(BaseModel):
    success: bool
    message: str
    session_id: Optional[int] = None
    status: Optional[str] = None
    participants: Optional[list] = None


class InviteRequest(BaseModel):
    host_user_id: int
    participant_ids: List[int]


class RespondInviteRequest(BaseModel):
    user_id: int
    accept: bool = True
    ready_on_accept: bool = True


class ReadyRequest(BaseModel):
    user_id: int
    ready: bool = True


class LeaveRequest(BaseModel):
    user_id: int


def _fetch_display_name(cursor, user_id: int) -> str:
    cursor.execute(
        """
        SELECT COALESCE(nickname, username) AS display_name
        FROM users
        WHERE id = %s
        """,
        (user_id,),
    )
    row = cursor.fetchone()
    return row["display_name"] if row else f"User {user_id}"


def _build_session_payload(cursor, session_id: int) -> dict:
    """Return a session payload with participant list for API responses."""
    cursor.execute(
        """
        SELECT id, host_user_id, game_type, status, max_players,
               created_at, updated_at, started_at, completed_at
        FROM game_sessions
        WHERE id = %s
        """,
        (session_id,),
    )
    session = cursor.fetchone()
    if not session:
        return {}

    cursor.execute(
        """
        SELECT user_id, role, status, ready, joined_at, updated_at
        FROM game_participants
        WHERE session_id = %s
        ORDER BY joined_at ASC
        """,
        (session_id,),
    )
    participants = cursor.fetchall()

    return {
        "session_id": session["id"],
        "host_user_id": session["host_user_id"],
        "game_type": session["game_type"],
        "status": session["status"],
        "max_players": session["max_players"],
        "participants": participants,
        "created_at": session["created_at"].isoformat() if session.get("created_at") else None,
        "updated_at": session["updated_at"].isoformat() if session.get("updated_at") else None,
        "started_at": session["started_at"].isoformat() if session.get("started_at") else None,
        "completed_at": session["completed_at"].isoformat() if session.get("completed_at") else None,
    }


async def _broadcast_game_event(user_ids: List[int], event_data: dict):
    """Best-effort broadcast of a game event to a list of users."""
    if not user_ids:
        return

    for uid in set(user_ids):
        try:
            await websocket_manager.send_game_event(uid, event_data)
        except Exception as exc:  # noqa: BLE001
            # Log but never fail the API because of socket delivery issues
            print(
                f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ⚠️  "
                f"Game event delivery failed to user {uid}: {str(exc)}"
            )


@router.post("/games/create", response_model=GameSessionResponse)
async def create_game_session(request: CreateGameSessionRequest):
    """Create a new game session and invite multiple participants."""
    conn = None
    try:
        # Game-specific max_players validation
        if request.game_type.lower() == "caro":
            if request.max_players != 2:
                return GameSessionResponse(
                    success=False,
                    message="Caro game only supports 2 players",
                )
        
        # Deduplicate participant IDs and remove host if present
        unique_participants = []
        seen = set()
        for pid in request.participant_ids:
            if pid == request.host_user_id:
                continue
            if pid not in seen:
                unique_participants.append(pid)
                seen.add(pid)

        if len(unique_participants) > request.max_players - 1:
            return GameSessionResponse(
                success=False,
                message="Too many participants for requested max_players",
            )

        conn = get_db_connection()
        cursor = conn.cursor(cursor_factory=RealDictCursor)

        # Validate host exists
        cursor.execute("SELECT id FROM users WHERE id = %s", (request.host_user_id,))
        if not cursor.fetchone():
            return GameSessionResponse(success=False, message="Host user not found")

        # Validate participants exist and are friends with host (best-effort)
        valid_participants = []
        for pid in unique_participants:
            cursor.execute("SELECT id FROM users WHERE id = %s", (pid,))
            if cursor.fetchone() and are_friends(request.host_user_id, pid):
                valid_participants.append(pid)

        # Create session
        cursor.execute(
            """
            INSERT INTO game_sessions (host_user_id, game_type, status, max_players)
            VALUES (%s, %s, 'waiting', %s)
            RETURNING id
            """,
            (request.host_user_id, request.game_type, request.max_players),
        )
        session_id = cursor.fetchone()["id"]

        # Add host as participant (ready by default)
        cursor.execute(
            """
            INSERT INTO game_participants (session_id, user_id, role, status, ready)
            VALUES (%s, %s, 'host', 'ready', TRUE)
            """,
            (session_id, request.host_user_id),
        )

        # Add invited participants
        for pid in valid_participants:
            cursor.execute(
                """
                INSERT INTO game_participants (session_id, user_id, role, status, ready)
                VALUES (%s, %s, 'player', 'invited', FALSE)
                ON CONFLICT (session_id, user_id) DO NOTHING
                """,
                (session_id, pid),
            )

        conn.commit()

        event_data = {
            "event_type": "invite",
            "session_id": session_id,
            "game_type": request.game_type,
            "host_user_id": request.host_user_id,
            "host_nickname": _fetch_display_name(cursor, request.host_user_id),
            "max_players": request.max_players,
            "status": "waiting",
        }

        await _broadcast_game_event(valid_participants, event_data)

        payload = _build_session_payload(cursor, session_id)
        return GameSessionResponse(
            success=True,
            message="Game session created",
            session_id=session_id,
            status=payload.get("status"),
            participants=payload.get("participants"),
        )
    except Exception as exc:  # noqa: BLE001
        if conn:
            conn.rollback()
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ❌ Create game session error: {str(exc)}")
        raise HTTPException(status_code=500, detail="Failed to create game session")
    finally:
        if conn:
            conn.close()


@router.post("/games/{session_id}/invite", response_model=GameSessionResponse)
async def invite_to_session(session_id: int, request: InviteRequest):
    """Invite additional players to an existing session (host only)."""
    conn = None
    try:
        conn = get_db_connection()
        cursor = conn.cursor(cursor_factory=RealDictCursor)

        cursor.execute(
            """
            SELECT id, host_user_id, status, max_players, game_type
            FROM game_sessions
            WHERE id = %s
            """,
            (session_id,),
        )
        session = cursor.fetchone()
        if not session:
            return GameSessionResponse(success=False, message="Session not found")

        if session["host_user_id"] != request.host_user_id:
            return GameSessionResponse(success=False, message="Only host can invite players")

        if session["status"] not in ["waiting", "ready"]:
            return GameSessionResponse(success=False, message="Cannot invite when session is not waiting/ready")

        # Game-specific validation: Caro only supports 2 players
        if session["game_type"].lower() == "caro":
            cursor.execute(
                "SELECT COUNT(*) AS count FROM game_participants WHERE session_id = %s",
                (session_id,),
            )
            current_count = cursor.fetchone()["count"]
            if current_count >= 1:  # Host + 1 participant = 2 players
                return GameSessionResponse(
                    success=False,
                    message="Caro game only supports 2 players (host + 1 participant)",
                )

        # Count current participants
        cursor.execute(
            "SELECT COUNT(*) AS count FROM game_participants WHERE session_id = %s",
            (session_id,),
        )
        current_count = cursor.fetchone()["count"]

        # Prepare invitees
        valid_invitees = []
        for pid in request.participant_ids:
            if pid == request.host_user_id:
                continue
            cursor.execute("SELECT id FROM users WHERE id = %s", (pid,))
            user_exists = cursor.fetchone()
            if not user_exists:
                continue
            if not are_friends(request.host_user_id, pid):
                continue
            valid_invitees.append(pid)

        remaining_slots = session["max_players"] - current_count
        if remaining_slots <= 0:
            return GameSessionResponse(success=False, message="Session is full")

        invited_now = []
        for pid in valid_invitees:
            if len(invited_now) >= remaining_slots:
                break
            cursor.execute(
                """
                INSERT INTO game_participants (session_id, user_id, role, status, ready)
                VALUES (%s, %s, 'player', 'invited', FALSE)
                ON CONFLICT (session_id, user_id) DO NOTHING
                RETURNING id
                """,
                (session_id, pid),
            )
            if cursor.fetchone():
                invited_now.append(pid)

        conn.commit()

        event_data = {
            "event_type": "invite",
            "session_id": session_id,
            "game_type": session["game_type"],
            "host_user_id": session["host_user_id"],
            "host_nickname": _fetch_display_name(cursor, session["host_user_id"]),
            "max_players": session["max_players"],
            "status": session["status"],
        }
        await _broadcast_game_event(invited_now, event_data)

        payload = _build_session_payload(cursor, session_id)
        return GameSessionResponse(
            success=True,
            message=f"Invited {len(invited_now)} player(s)",
            session_id=session_id,
            status=payload.get("status"),
            participants=payload.get("participants"),
        )
    except Exception as exc:  # noqa: BLE001
        if conn:
            conn.rollback()
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ❌ Invite error: {str(exc)}")
        raise HTTPException(status_code=500, detail="Failed to invite players")
    finally:
        if conn:
            conn.close()


@router.post("/games/{session_id}/respond", response_model=GameSessionResponse)
async def respond_invite(session_id: int, request: RespondInviteRequest):
    """Accept or decline an invite."""
    conn = None
    try:
        conn = get_db_connection()
        cursor = conn.cursor(cursor_factory=RealDictCursor)

        cursor.execute(
            """
            SELECT id, host_user_id, status, game_type
            FROM game_sessions
            WHERE id = %s
            """,
            (session_id,),
        )
        session = cursor.fetchone()
        if not session:
            return GameSessionResponse(success=False, message="Session not found")

        cursor.execute(
            """
            SELECT status, ready
            FROM game_participants
            WHERE session_id = %s AND user_id = %s
            FOR UPDATE
            """,
            (session_id, request.user_id),
        )
        participant = cursor.fetchone()
        if not participant:
            return GameSessionResponse(success=False, message="Invite not found for user")

        new_status = "accepted" if request.accept else "declined"
        new_ready = request.ready_on_accept and request.accept

        cursor.execute(
            """
            UPDATE game_participants
            SET status = %s, ready = %s, updated_at = CURRENT_TIMESTAMP
            WHERE session_id = %s AND user_id = %s
            """,
            (new_status, new_ready, session_id, request.user_id),
        )

        # Recompute session status
        cursor.execute(
            """
            SELECT user_id, status, ready
            FROM game_participants
            WHERE session_id = %s
            """,
            (session_id,),
        )
        participants = cursor.fetchall()
        accepted_count = len([p for p in participants if p["status"] in ("accepted", "ready")])
        ready_count = len([p for p in participants if p["ready"]])
        total = len(participants)

        session_status = session["status"]
        if request.accept and ready_count == total:
            session_status = "in_progress"
            cursor.execute(
                """
                UPDATE game_sessions
                SET status = 'in_progress', started_at = CURRENT_TIMESTAMP, updated_at = CURRENT_TIMESTAMP
                WHERE id = %s
                """,
                (session_id,),
            )
        elif request.accept and accepted_count == total:
            session_status = "ready"
            cursor.execute(
                """
                UPDATE game_sessions
                SET status = 'ready', updated_at = CURRENT_TIMESTAMP
                WHERE id = %s
                """,
                (session_id,),
            )
        elif not request.accept:
            session_status = "waiting"
            cursor.execute(
                """
                UPDATE game_sessions
                SET status = 'waiting', updated_at = CURRENT_TIMESTAMP
                WHERE id = %s
                """,
                (session_id,),
            )

        conn.commit()

        # Fetch participant nickname
        participant_nickname = _fetch_display_name(cursor, request.user_id)

        event_data = {
            "event_type": "respond",
            "session_id": session_id,
            "game_type": session["game_type"],
            "user_id": request.user_id,
            "user_nickname": participant_nickname,
            "accepted": request.accept,
            "ready": new_ready,
            "status": session_status,
        }

        # Notify host and all participants
        target_user_ids = [session["host_user_id"]]
        target_user_ids.extend([p["user_id"] for p in participants if "user_id" in p])
        await _broadcast_game_event(target_user_ids, event_data)

        payload = _build_session_payload(cursor, session_id)
        return GameSessionResponse(
            success=True,
            message="Response recorded",
            session_id=session_id,
            status=session_status,
            participants=payload.get("participants"),
        )
    except Exception as exc:  # noqa: BLE001
        if conn:
            conn.rollback()
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ❌ Respond invite error: {str(exc)}")
        raise HTTPException(status_code=500, detail="Failed to respond to invite")
    finally:
        if conn:
            conn.close()


@router.post("/games/{session_id}/ready", response_model=GameSessionResponse)
async def set_ready(session_id: int, request: ReadyRequest):
    """Mark participant readiness."""
    conn = None
    try:
        conn = get_db_connection()
        cursor = conn.cursor(cursor_factory=RealDictCursor)

        cursor.execute(
            """
            SELECT id, host_user_id, status, game_type
            FROM game_sessions
            WHERE id = %s
            """,
            (session_id,),
        )
        session = cursor.fetchone()
        if not session:
            return GameSessionResponse(success=False, message="Session not found")

        cursor.execute(
            """
            UPDATE game_participants
            SET ready = %s, status = CASE WHEN %s THEN 'ready' ELSE status END, updated_at = CURRENT_TIMESTAMP
            WHERE session_id = %s AND user_id = %s
            RETURNING id
            """,
            (request.ready, request.ready, session_id, request.user_id),
        )
        if not cursor.fetchone():
            return GameSessionResponse(success=False, message="Participant not found")

        # Evaluate session start condition
        cursor.execute(
            """
            SELECT ready FROM game_participants
            WHERE session_id = %s
            """,
            (session_id,),
        )
        readiness = cursor.fetchall()
        all_ready = readiness and all(row["ready"] for row in readiness)

        session_status = session["status"]
        if all_ready and session_status != "in_progress":
            session_status = "in_progress"
            cursor.execute(
                """
                UPDATE game_sessions
                SET status = 'in_progress', started_at = CURRENT_TIMESTAMP, updated_at = CURRENT_TIMESTAMP
                WHERE id = %s
                """,
                (session_id,),
            )
        else:
            cursor.execute(
                """
                UPDATE game_sessions
                SET updated_at = CURRENT_TIMESTAMP
                WHERE id = %s
                """,
                (session_id,),
            )

        conn.commit()

        event_data = {
            "event_type": "ready",
            "session_id": session_id,
            "game_type": session["game_type"],
            "user_id": request.user_id,
            "ready": request.ready,
            "status": session_status,
        }

        cursor.execute(
            "SELECT user_id FROM game_participants WHERE session_id = %s",
            (session_id,),
        )
        target_user_ids = [row["user_id"] for row in cursor.fetchall()]
        await _broadcast_game_event(target_user_ids, event_data)

        payload = _build_session_payload(cursor, session_id)
        return GameSessionResponse(
            success=True,
            message="Ready state updated",
            session_id=session_id,
            status=session_status,
            participants=payload.get("participants"),
        )
    except Exception as exc:  # noqa: BLE001
        if conn:
            conn.rollback()
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ❌ Ready toggle error: {str(exc)}")
        raise HTTPException(status_code=500, detail="Failed to update ready status")
    finally:
        if conn:
            conn.close()


@router.post("/games/{session_id}/leave", response_model=GameSessionResponse)
async def leave_session(session_id: int, request: LeaveRequest):
    """Allow a participant (or host) to leave/cancel the session."""
    conn = None
    try:
        conn = get_db_connection()
        cursor = conn.cursor(cursor_factory=RealDictCursor)

        cursor.execute(
            """
            SELECT id, host_user_id, status, game_type
            FROM game_sessions
            WHERE id = %s
            """,
            (session_id,),
        )
        session = cursor.fetchone()
        if not session:
            return GameSessionResponse(success=False, message="Session not found")

        # If host leaves, cancel session
        if session["host_user_id"] == request.user_id:
            cursor.execute(
                """
                UPDATE game_sessions
                SET status = 'cancelled', updated_at = CURRENT_TIMESTAMP
                WHERE id = %s
                """,
                (session_id,),
            )
            cursor.execute(
                """
                UPDATE game_participants
                SET status = 'cancelled', ready = FALSE, updated_at = CURRENT_TIMESTAMP
                WHERE session_id = %s
                """,
                (session_id,),
            )
            conn.commit()

            cursor.execute(
                "SELECT user_id FROM game_participants WHERE session_id = %s",
                (session_id,),
            )
            target_user_ids = [row["user_id"] for row in cursor.fetchall()]
            event_data = {
                "event_type": "cancelled",
                "session_id": session_id,
                "game_type": session["game_type"],
                "user_id": request.user_id,
                "status": "cancelled",
            }
            await _broadcast_game_event(target_user_ids, event_data)
        else:
            cursor.execute(
                """
                UPDATE game_participants
                SET status = 'left', ready = FALSE, updated_at = CURRENT_TIMESTAMP
                WHERE session_id = %s AND user_id = %s
                """,
                (session_id, request.user_id),
            )
            conn.commit()

            cursor.execute(
                "SELECT user_id FROM game_participants WHERE session_id = %s",
                (session_id,),
            )
            target_user_ids = [row["user_id"] for row in cursor.fetchall()]
            event_data = {
                "event_type": "left",
                "session_id": session_id,
                "game_type": session["game_type"],
                "user_id": request.user_id,
                "status": "waiting",
            }
            await _broadcast_game_event(target_user_ids, event_data)

        payload = _build_session_payload(cursor, session_id)
        return GameSessionResponse(
            success=True,
            message="Left session",
            session_id=session_id,
            status=payload.get("status"),
            participants=payload.get("participants"),
        )
    except Exception as exc:  # noqa: BLE001
        if conn:
            conn.rollback()
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ❌ Leave session error: {str(exc)}")
        raise HTTPException(status_code=500, detail="Failed to leave session")
    finally:
        if conn:
            conn.close()


@router.get("/games/{session_id}", response_model=GameSessionResponse)
async def get_session(session_id: int):
    """Return session details."""
    conn = None
    try:
        conn = get_db_connection()
        cursor = conn.cursor(cursor_factory=RealDictCursor)

        payload = _build_session_payload(cursor, session_id)
        if not payload:
            return GameSessionResponse(success=False, message="Session not found")

        return GameSessionResponse(
            success=True,
            message="OK",
            session_id=payload.get("session_id"),
            status=payload.get("status"),
            participants=payload.get("participants"),
        )
    except Exception as exc:  # noqa: BLE001
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ❌ Get session error: {str(exc)}")
        raise HTTPException(status_code=500, detail="Failed to fetch session")
    finally:
        if conn:
            conn.close()


@router.get("/games/active/{user_id}")
async def get_active_sessions(user_id: int):
    """
    Return active sessions a user participates in.
    Useful for reconnect and resync on boot.
    """
    conn = None
    try:
        conn = get_db_connection()
        cursor = conn.cursor(cursor_factory=RealDictCursor)

        cursor.execute(
            """
            SELECT gp.session_id
            FROM game_participants gp
            JOIN game_sessions gs ON gp.session_id = gs.id
            WHERE gp.user_id = %s
              AND gs.status IN ('waiting', 'ready', 'in_progress')
            """,
            (user_id,),
        )
        session_ids = [row["session_id"] for row in cursor.fetchall()]

        sessions = []
        for sid in session_ids:
            sessions.append(_build_session_payload(cursor, sid))

        return {"success": True, "sessions": sessions}
    except Exception as exc:  # noqa: BLE001
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ❌ Get active sessions error: {str(exc)}")
        raise HTTPException(status_code=500, detail="Failed to fetch active sessions")
    finally:
        if conn:
            conn.close()

