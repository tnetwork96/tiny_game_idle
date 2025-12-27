"""
Database Migration Tool
Run all pending migrations or specific migration
"""
import os
import sys
import importlib.util
from datetime import datetime
import psycopg2

def get_db_connection():
    """Get PostgreSQL database connection"""
    database_url = os.getenv("DATABASE_URL", "postgresql://tinygame:tinygame123@db:5432/tiny_game")
    return psycopg2.connect(database_url)

def get_migration_files():
    """Get all migration files in order"""
    migrations_dir = os.path.join(os.path.dirname(__file__), 'migrations')
    if not os.path.exists(migrations_dir):
        return []
    
    migration_files = []
    for filename in sorted(os.listdir(migrations_dir)):
        if filename.startswith('00') and filename.endswith('.py') and filename != '__init__.py':
            migration_files.append(os.path.join(migrations_dir, filename))
    
    return migration_files

def get_applied_migrations():
    """Get list of applied migrations from database"""
    try:
        conn = get_db_connection()
        cursor = conn.cursor()
        
        try:
            # Check if migrations table exists
            cursor.execute("""
                SELECT EXISTS (
                    SELECT FROM information_schema.tables 
                    WHERE table_name = 'migrations'
                )
            """)
            if not cursor.fetchone()[0]:
                return []
            
            cursor.execute('SELECT version FROM migrations ORDER BY version')
            return [row[0] for row in cursor.fetchall()]
        finally:
            conn.close()
    except Exception as e:
        # If database can't be opened, return empty list (will be created by first migration)
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ⚠️  Could not read migrations table: {str(e)}")
        return []

def run_migration(migration_file):
    """Run a single migration file"""
    spec = importlib.util.spec_from_file_location("migration", migration_file)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    
    # Extract version from filename (e.g., "001_create_users_table.py" -> "001")
    filename = os.path.basename(migration_file)
    version = filename.split('_')[0]
    
    print(f"\n[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] {'='*60}")
    print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] Running migration: {filename}")
    print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] {'='*60}")
    
    try:
        module.up()
        return True
    except Exception as e:
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ❌ Migration failed: {str(e)}")
        return False

def main():
    """Main migration runner"""
    print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] 🚀 Starting database migrations...")
    
    database_url = os.getenv("DATABASE_URL", "postgresql://tinygame:tinygame123@db:5432/tiny_game")
    print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] Database URL: {database_url.split('@')[1] if '@' in database_url else 'local'}")
    
    # Get all migration files
    migration_files = get_migration_files()
    if not migration_files:
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ❌ No migration files found!")
        return
    
    print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] Found {len(migration_files)} migration(s)")
    
    # Get applied migrations
    applied_migrations = get_applied_migrations()
    print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] Applied migrations: {applied_migrations}")
    
    # Run pending migrations
    success_count = 0
    failed_count = 0
    
    for migration_file in migration_files:
        filename = os.path.basename(migration_file)
        version = filename.split('_')[0]
        
        if version in applied_migrations:
            print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ⏭️  Skipping {filename} (already applied)")
            continue
        
        if run_migration(migration_file):
            success_count += 1
        else:
            failed_count += 1
            print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] ❌ Stopping migrations due to error")
            break
    
    print(f"\n[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] {'='*60}")
    print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] Migration Summary:")
    print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}]    Success: {success_count}")
    print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}]    Failed: {failed_count}")
    print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] {'='*60}")

if __name__ == "__main__":
    main()

