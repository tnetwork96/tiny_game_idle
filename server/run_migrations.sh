#!/bin/bash
# Helper script to run database migrations

echo "🚀 Running database migrations..."

# Check if running in Docker
if [ -f /.dockerenv ]; then
    echo "📦 Running migrations in Docker container..."
    python3 /app/migrate.py
else
    echo "💻 Running migrations locally..."
    cd "$(dirname "$0")"
    python3 migrate.py
fi

echo "✅ Migrations completed!"

