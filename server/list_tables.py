import sqlite3

db = '/app/tiny_game.db'
conn = sqlite3.connect(db)
cur = conn.cursor()

cur.execute("SELECT name FROM sqlite_master WHERE type='table'")
tables = cur.fetchall()

print(f"Database: {db}")
print(f"Tables found: {len(tables)}")
for table in tables:
    name = table[0]
    cur.execute(f"SELECT COUNT(*) FROM {name}")
    count = cur.fetchone()[0]
    print(f"  - {name}: {count} rows")

conn.close()

