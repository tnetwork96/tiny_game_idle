#!/usr/bin/env python3
"""Show database information"""
import sqlite3
import os
from app.api.auth import DB_PATH

print("=" * 60)
print("Database Information")
print("=" * 60)
print(f"DB_PATH from auth.py: {DB_PATH}")
print(f"Database exists: {os.path.exists(DB_PATH)}")
print(f"Is file: {os.path.isfile(DB_PATH) if os.path.exists(DB_PATH) else False}")
if os.path.exists(DB_PATH):
    print(f"Size: {os.path.getsize(DB_PATH)} bytes")
print()

if os.path.exists(DB_PATH) and os.path.isfile(DB_PATH):
    conn = sqlite3.connect(DB_PATH)
    cursor = conn.cursor()
    
    # Get all tables
    cursor.execute("SELECT name FROM sqlite_master WHERE type='table'")
    tables = cursor.fetchall()
    
    print(f"Found {len(tables)} table(s):")
    for table in tables:
        table_name = table[0]
        print(f"\n  Table: {table_name}")
        
        # Get row count
        cursor.execute(f"SELECT COUNT(*) FROM {table_name}")
        count = cursor.fetchone()[0]
        print(f"    Rows: {count}")
        
        # Get column names
        cursor.execute(f"PRAGMA table_info({table_name})")
        columns = cursor.fetchall()
        print(f"    Columns: {[col[1] for col in columns]}")
    
    conn.close()
else:
    print("Database file not found or is not a file!")

