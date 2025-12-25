-- Seed sample users
INSERT INTO users (username, pin, nickname, active) VALUES
    ('alice', '1111', 'Alice', TRUE),
    ('bob', '2222', 'Bob', TRUE),
    ('charlie', '3333', 'Charlie', TRUE),
    ('diana', '4444', 'Diana', TRUE),
    ('eve', '5555', 'Eve', TRUE),
    ('frank', '6666', 'Frank', TRUE),
    ('grace', '7777', 'Grace', TRUE),
    ('henry', '8888', 'Henry', TRUE),
    ('ivy', '9999', 'Ivy', TRUE),
    ('jack', '0000', 'Jack', TRUE)
ON CONFLICT (username) DO NOTHING;

-- Seed friend relationships (bidirectional)
-- Get user IDs first, then create friendships
DO $$
DECLARE
    player1_id INTEGER;
    alice_id INTEGER;
    bob_id INTEGER;
    charlie_id INTEGER;
    diana_id INTEGER;
    eve_id INTEGER;
    frank_id INTEGER;
    grace_id INTEGER;
    henry_id INTEGER;
    ivy_id INTEGER;
    jack_id INTEGER;
BEGIN
    -- Get user IDs
    SELECT id INTO player1_id FROM users WHERE username = 'player1';
    SELECT id INTO alice_id FROM users WHERE username = 'alice';
    SELECT id INTO bob_id FROM users WHERE username = 'bob';
    SELECT id INTO charlie_id FROM users WHERE username = 'charlie';
    SELECT id INTO diana_id FROM users WHERE username = 'diana';
    SELECT id INTO eve_id FROM users WHERE username = 'eve';
    SELECT id INTO frank_id FROM users WHERE username = 'frank';
    SELECT id INTO grace_id FROM users WHERE username = 'grace';
    SELECT id INTO henry_id FROM users WHERE username = 'henry';
    SELECT id INTO ivy_id FROM users WHERE username = 'ivy';
    SELECT id INTO jack_id FROM users WHERE username = 'jack';
    
    -- Create friendships (bidirectional)
    INSERT INTO friends (user_id, friend_id) VALUES
        (player1_id, alice_id), (alice_id, player1_id),
        (player1_id, bob_id), (bob_id, player1_id),
        (player1_id, charlie_id), (charlie_id, player1_id),
        (player1_id, diana_id), (diana_id, player1_id),
        (alice_id, bob_id), (bob_id, alice_id),
        (alice_id, eve_id), (eve_id, alice_id),
        (alice_id, grace_id), (grace_id, alice_id),
        (bob_id, charlie_id), (charlie_id, bob_id),
        (bob_id, frank_id), (frank_id, bob_id),
        (charlie_id, diana_id), (diana_id, charlie_id),
        (charlie_id, henry_id), (henry_id, charlie_id),
        (diana_id, eve_id), (eve_id, diana_id),
        (diana_id, ivy_id), (ivy_id, diana_id),
        (eve_id, frank_id), (frank_id, eve_id),
        (eve_id, jack_id), (jack_id, eve_id),
        (frank_id, grace_id), (grace_id, frank_id),
        (frank_id, henry_id), (henry_id, frank_id),
        (grace_id, henry_id), (henry_id, grace_id),
        (grace_id, ivy_id), (ivy_id, grace_id),
        (henry_id, jack_id), (jack_id, henry_id),
        (ivy_id, jack_id), (jack_id, ivy_id)
    ON CONFLICT (user_id, friend_id) DO NOTHING;
END $$;

