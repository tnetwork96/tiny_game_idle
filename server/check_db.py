#!/usr/bin/env python3
"""Check database tables"""
import sqlite3
import os

# Check Docker container database
docker_db = '/app/tiny_game.db'
if os.path.exists(docker_db):
    print(f"=== Docker Database: {docker_db} ===")
    conn = sqlite3.connect(docker_db)
    cursor = conn.cursor()
    cursor.execute("SELECT name FROM sqlite_master WHERE type='table'")
    tables = cursor.fetchall()
    print(f"Tables: {[t[0] for t in tables]}")
    for table in tables:
        cursor.execute(f"SELECT COUNT(*) FROM {table[0]}")
        count = cursor.fetchone()[0]
        print(f"  {table[0]}: {count} rows")
    conn.close()
else:
    print(f"Docker database not found at {docker_db}")

# Check local database
local_db = os.path.join(os.path.dirname(__file__), 'tiny_game.db')
if os.path.exists(local_db) and os.path.isfile(local_db):
    print(f"\n=== Local Database: {local_db} ===")
    conn = sqlite3.connect(local_db)
    cursor = conn.cursor()
    cursor.execute("SELECT name FROM sqlite_master WHERE type='table'")
    tables = cursor.fetchall()
    print(f"Tables: {[t[0] for t in tables]}")
    for table in tables:
        cursor.execute(f"SELECT COUNT(*) FROM {table[0]}")
        count = cursor.fetchone()[0]
        print(f"  {table[0]}: {count} rows")
    conn.close()
else:
    print(f"\nLocal database not found at {local_db}")

# Check scripts database
scripts_db = os.path.join(os.path.dirname(__file__), 'scripts', 'tiny_game.db')
if os.path.exists(scripts_db) and os.path.isfile(scripts_db):
    print(f"\n=== Scripts Database: {scripts_db} ===")
    conn = sqlite3.connect(scripts_db)
    cursor = conn.cursor()
    cursor.execute("SELECT name FROM sqlite_master WHERE type='table'")
    tables = cursor.fetchall()
    print(f"Tables: {[t[0] for t in tables]}")
    for table in tables:
        cursor.execute(f"SELECT COUNT(*) FROM {table[0]}")
        count = cursor.fetchone()[0]
        print(f"  {table[0]}: {count} rows")
    conn.close()
else:
    print(f"\nScripts database not found at {scripts_db}")

