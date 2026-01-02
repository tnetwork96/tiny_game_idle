"""
Migration: Create game session tables for multi-user games
Version: 012
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
    """Run migration - create game session tables"""
    print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] 📦 Migration 012: Creating game tables...")
    conn = get_db_connection()
    cursor = conn.cursor()

    try:
        cursor.execute(
            """
            CREATE TABLE IF NOT EXISTS game_sessions (
                id SERIAL PRIMARY KEY,
                host_user_id INTEGER NOT NULL REFERENCES users(id),
                game_type VARCHAR(100) NOT NULL,
                status VARCHAR(20) NOT NULL DEFAULT 'waiting',
                max_players INTEGER NOT NULL DEFAULT 2 CHECK (max_players >= 2),
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                started_at TIMESTAMP NULL,
                completed_at TIMESTAMP NULL
            )
            """
        )

        cursor.execute(
            """
            CREATE TABLE IF NOT EXISTS game_participants (
                id SERIAL PRIMARY KEY,
                session_id INTEGER NOT NULL REFERENCES game_sessions(id) ON DELETE CASCADE,
                user_id INTEGER NOT NULL REFERENCES users(id),
                role VARCHAR(20) NOT NULL DEFAULT 'player',
                status VARCHAR(20) NOT NULL DEFAULT 'invited',
                ready BOOLEAN NOT NULL DEFAULT FALSE,
                joined_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                UNIQUE (session_id, user_id)
            )
            """
        )

        cursor.execute(
            """
            CREATE INDEX IF NOT EXISTS idx_game_participants_user
            ON game_participants(user_id)
            """
        )

        cursor.execute(
            """
            INSERT INTO migrations (version, name)
            VALUES (%s, %s)
            ON CONFLICT (version) DO NOTHING
            """,
            ("012", "create_game_tables"),
        )

        conn.commit()
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ✅ Migration 012 completed successfully!")
    except Exception as exc:
        conn.rollback()
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ❌ Migration 012 failed: {str(exc)}")
        raise
    finally:
        conn.close()


def down():
    """Rollback migration - drop game session tables"""
    print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ⬇️  Rolling back migration 012...")
    conn = get_db_connection()
    cursor = conn.cursor()

    try:
        cursor.execute("DROP TABLE IF EXISTS game_participants")
        cursor.execute("DROP TABLE IF EXISTS game_sessions")
        cursor.execute("DELETE FROM migrations WHERE version = %s", ("012",))
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

