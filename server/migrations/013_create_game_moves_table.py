"""
Migration: Create game_moves table for storing game moves
Version: 013
Date: 2026-01-02
"""
import os
from datetime import datetime

import psycopg2


def get_db_connection():
    """Get PostgreSQL database connection"""
    database_url = os.getenv(
        "DATABASE_URL",
        "postgresql://tinygame:tinygame123@db:5432/tiny_game",
    )
    return psycopg2.connect(database_url)


def up():
    """Run migration - create game_moves table"""
    print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] 📦 Migration 013: Creating game_moves table...")
    conn = get_db_connection()
    cursor = conn.cursor()

    try:
        cursor.execute(
            """
            CREATE TABLE IF NOT EXISTS game_moves (
                id SERIAL PRIMARY KEY,
                session_id INTEGER NOT NULL REFERENCES game_sessions(id) ON DELETE CASCADE,
                user_id INTEGER NOT NULL REFERENCES users(id),
                row INTEGER NOT NULL CHECK (row >= 0),
                col INTEGER NOT NULL CHECK (col >= 0),
                move_number INTEGER NOT NULL,
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                UNIQUE (session_id, row, col)
            )
            """
        )

        cursor.execute(
            """
            CREATE INDEX IF NOT EXISTS idx_game_moves_session
            ON game_moves(session_id)
            """
        )

        cursor.execute(
            """
            CREATE INDEX IF NOT EXISTS idx_game_moves_session_move
            ON game_moves(session_id, move_number)
            """
        )

        cursor.execute(
            """
            INSERT INTO migrations (version, name)
            VALUES (%s, %s)
            ON CONFLICT (version) DO NOTHING
            """,
            ("013", "create_game_moves_table"),
        )

        conn.commit()
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ✅ Migration 013 completed successfully!")
    except Exception as exc:
        conn.rollback()
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ❌ Migration 013 failed: {str(exc)}")
        raise
    finally:
        conn.close()


def down():
    """Rollback migration - drop game_moves table"""
    print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ⬇️  Rolling back migration 013...")
    conn = get_db_connection()
    cursor = conn.cursor()

    try:
        cursor.execute("DROP TABLE IF EXISTS game_moves CASCADE")
        cursor.execute("DELETE FROM migrations WHERE version = %s", ("013",))
        conn.commit()
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ✅ Rollback completed!")
    except Exception as exc:
        conn.rollback()
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ❌ Rollback failed: {str(exc)}")
        raise
    finally:
        conn.close()


if __name__ == "__main__":
    up()

