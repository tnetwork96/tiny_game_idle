"""
Database seeding script for ESP32 Game Server
Populates database with sample users and friend relationships
"""
from sqlalchemy.orm import Session
from app.database import SessionLocal, init_db
from app.models.db_models import User, Friend
import logging

logger = logging.getLogger(__name__)

# Sample users data
SAMPLE_USERS = [
    {"username": "player1", "pin": "4321", "nickname": "Player One"},
    {"username": "admin", "pin": "1234", "nickname": "Administrator"},
    {"username": "alice", "pin": "1111", "nickname": "Alice"},
    {"username": "bob", "pin": "2222", "nickname": "Bob"},
    {"username": "charlie", "pin": "3333", "nickname": "Charlie"},
    {"username": "diana", "pin": "4444", "nickname": "Diana"},
    {"username": "eve", "pin": "5555", "nickname": "Eve"},
    {"username": "frank", "pin": "6666", "nickname": "Frank"},
    {"username": "grace", "pin": "7777", "nickname": "Grace"},
    {"username": "henry", "pin": "8888", "nickname": "Henry"},
    {"username": "ivy", "pin": "9999", "nickname": "Ivy"},
    {"username": "jack", "pin": "0000", "nickname": "Jack"},
]

# Friend relationships (bidirectional)
# Format: (username1, username2) means both are friends
FRIEND_PAIRS = [
    ("player1", "alice"),
    ("player1", "bob"),
    ("player1", "charlie"),
    ("player1", "diana"),
    ("alice", "bob"),
    ("alice", "eve"),
    ("alice", "grace"),
    ("bob", "charlie"),
    ("bob", "frank"),
    ("charlie", "diana"),
    ("charlie", "henry"),
    ("diana", "eve"),
    ("diana", "ivy"),
    ("eve", "frank"),
    ("eve", "jack"),
    ("frank", "grace"),
    ("frank", "henry"),
    ("grace", "henry"),
    ("grace", "ivy"),
    ("henry", "jack"),
    ("ivy", "jack"),
]

def seed_users(db: Session):
    """Create sample users"""
    created = 0
    skipped = 0
    
    for user_data in SAMPLE_USERS:
        existing = db.query(User).filter(User.username == user_data["username"]).first()
        if existing:
            logger.info(f"User '{user_data['username']}' already exists, skipping")
            skipped += 1
            continue
        
        user = User(
            username=user_data["username"],
            pin=user_data["pin"],
            nickname=user_data["nickname"],
            active=True
        )
        db.add(user)
        created += 1
        logger.info(f"Created user: {user_data['username']} ({user_data['nickname']})")
    
    db.commit()
    logger.info(f"Users seeding complete: {created} created, {skipped} skipped")
    return created, skipped

def seed_friends(db: Session):
    """Create friend relationships"""
    created = 0
    skipped = 0
    
    # Get all users as dict for quick lookup
    users = {u.username: u.id for u in db.query(User).all()}
    
    for username1, username2 in FRIEND_PAIRS:
        if username1 not in users or username2 not in users:
            logger.warning(f"One or both users not found: {username1}, {username2}")
            continue
        
        user_id1 = users[username1]
        user_id2 = users[username2]
        
        # Check if friendship already exists (either direction)
        existing = db.query(Friend).filter(
            ((Friend.user_id == user_id1) & (Friend.friend_id == user_id2)) |
            ((Friend.user_id == user_id2) & (Friend.friend_id == user_id1))
        ).first()
        
        if existing:
            logger.info(f"Friendship between '{username1}' and '{username2}' already exists, skipping")
            skipped += 1
            continue
        
        # Create bidirectional friendship
        friend1 = Friend(user_id=user_id1, friend_id=user_id2)
        friend2 = Friend(user_id=user_id2, friend_id=user_id1)
        
        db.add(friend1)
        db.add(friend2)
        created += 2
        logger.info(f"Created friendship: {username1} <-> {username2}")
    
    db.commit()
    logger.info(f"Friends seeding complete: {created} relationships created, {skipped} skipped")
    return created, skipped

def seed_database():
    """Main function to seed database"""
    logger.info("Starting database seeding...")
    
    # Initialize database tables
    init_db()
    
    db: Session = SessionLocal()
    try:
        # Seed users
        users_created, users_skipped = seed_users(db)
        
        # Seed friends
        friends_created, friends_skipped = seed_friends(db)
        
        logger.info("=" * 60)
        logger.info("Database seeding complete!")
        logger.info(f"Users: {users_created} created, {users_skipped} skipped")
        logger.info(f"Friends: {friends_created} relationships created, {friends_skipped} skipped")
        logger.info("=" * 60)
        
    except Exception as e:
        logger.error(f"Error seeding database: {e}")
        db.rollback()
        raise
    finally:
        db.close()

if __name__ == "__main__":
    logging.basicConfig(level=logging.INFO)
    seed_database()

