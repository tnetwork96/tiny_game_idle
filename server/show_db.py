#!/usr/bin/env python3
import sqlite3

db = '/app/tiny_game.db'
conn = sqlite3.connect(db)
cur = conn.cursor()

print("=" * 60)
print("DATABASE: /app/tiny_game.db")
print("=" * 60)

# List all tables
cur.execute("SELECT name FROM sqlite_master WHERE type='table'")
tables = cur.fetchall()

print(f"\nFound {len(tables)} table(s):\n")
for table in tables:
    table_name = table[0]
    print(f"Table: {table_name}")
    
    # Count rows
    cur.execute(f"SELECT COUNT(*) FROM {table_name}")
    count = cur.fetchone()[0]
    print(f"  Rows: {count}")
    
    # Get schema
    cur.execute(f"SELECT sql FROM sqlite_master WHERE type='table' AND name='{table_name}'")
    schema = cur.fetchone()
    if schema:
        print(f"  Schema: {schema[0]}")
    
    # Show sample data
    if count > 0:
        cur.execute(f"SELECT * FROM {table_name} LIMIT 3")
        rows = cur.fetchall()
        print(f"  Sample data (first 3 rows):")
        for row in rows:
            print(f"    {row}")
    print()

conn.close()

