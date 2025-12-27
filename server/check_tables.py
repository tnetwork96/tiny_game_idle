import sqlite3
import os

db_path = '/app/tiny_game.db'
print(f"Checking database: {db_path}")
print(f"Exists: {os.path.exists(db_path)}")
print(f"Size: {os.path.getsize(db_path)} bytes\n")

conn = sqlite3.connect(db_path)
cursor = conn.cursor()

# Get all tables
cursor.execute("SELECT name FROM sqlite_master WHERE type='table'")
tables = cursor.fetchall()

print(f"Found {len(tables)} table(s):")
for table in tables:
    table_name = table[0]
    print(f"\n=== Table: {table_name} ===")
    
    # Get row count
    cursor.execute(f"SELECT COUNT(*) FROM {table_name}")
    count = cursor.fetchone()[0]
    print(f"Rows: {count}")
    
    # Get schema
    cursor.execute(f"SELECT sql FROM sqlite_master WHERE type='table' AND name='{table_name}'")
    schema = cursor.fetchone()
    if schema:
        print(f"Schema: {schema[0]}")
    
    # Get sample data
    if count > 0:
        cursor.execute(f"SELECT * FROM {table_name} LIMIT 5")
        rows = cursor.fetchall()
        print("Sample data:")
        for row in rows:
            print(f"  {row}")

conn.close()

