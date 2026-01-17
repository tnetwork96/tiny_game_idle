import psycopg2

conn = psycopg2.connect(
    host='localhost',
    port=5432,
    dbname='tiny_game',
    user='tinygame',
    password='tinygame123'
)

cur = conn.cursor()
cur.execute("SELECT id, type, message, related_id FROM notifications WHERE type='game_invite' ORDER BY created_at DESC LIMIT 5")
rows = cur.fetchall()

print('\n=== GAME INVITE NOTIFICATIONS ===')
if rows:
    for r in rows:
        print(f'ID: {r[0]}, Type: [{r[1]}], Message: [{r[2]}], RelatedID: {r[3]}')
else:
    print('No game_invite notifications found')

cur.close()
conn.close()
