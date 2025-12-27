FROM python:3.11-slim

WORKDIR /app

# Install system dependencies
RUN apt-get update && apt-get install -y \
    gcc \
    && rm -rf /var/lib/apt/lists/*

# Copy requirements first for better caching
COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

# Copy application code
COPY app/ ./app/

# Copy migrations
COPY migrations/ ./migrations/
COPY migrate.py ./

# Expose port
EXPOSE 8080

# Run migrations and then start the application
CMD ["sh", "-c", "python3 migrate.py && uvicorn app.main:app --host 0.0.0.0 --port 8080"]

