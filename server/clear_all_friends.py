#!/usr/bin/env python3
"""
Script để xóa tất cả friendships và friend requests trong database tiny_game
Đồng bộ lại database về trạng thái không có bạn bè
"""
import psycopg2
from psycopg2.extras import RealDictCursor
import os
from datetime import datetime

# Cấu hình Database
DB_HOST = os.getenv("DB_HOST", "localhost")
DB_PORT = os.getenv("DB_PORT", "5432")
DB_NAME = "tiny_game"
DB_USER = os.getenv("DB_USER", "tinygame")
DB_PASSWORD = os.getenv("DB_PASSWORD", "tinygame123")

def get_db_connection():
    """
    Tạo kết nối đến database tiny_game
    """
    # Thử kết nối với DATABASE_URL từ env trước
    database_url = os.getenv("DATABASE_URL")
    if database_url:
        try:
            return psycopg2.connect(database_url)
        except:
            pass
    
    # Nếu không có DATABASE_URL, dùng thông tin từ env hoặc default
    try:
        # Thử localhost trước
        conn = psycopg2.connect(
            host=DB_HOST,
            port=DB_PORT,
            database=DB_NAME,
            user=DB_USER,
            password=DB_PASSWORD
        )
        return conn
    except Exception as e:
        # Nếu localhost không được, thử docker container name
        try:
            conn = psycopg2.connect(
                host="db",  # Docker container name
                port=DB_PORT,
                database=DB_NAME,
                user=DB_USER,
                password=DB_PASSWORD
            )
            return conn
        except Exception as e2:
            print(f"❌ Không thể kết nối database:")
            print(f"   - Thử localhost: {e}")
            print(f"   - Thử docker 'db': {e2}")
            raise

def get_counts(conn):
    """
    Lấy số lượng friendships và friend requests hiện tại
    """
    cursor = conn.cursor(cursor_factory=RealDictCursor)
    
    cursor.execute('SELECT COUNT(*) as count FROM friends')
    friends_count = cursor.fetchone()['count']
    
    cursor.execute('SELECT COUNT(*) as count FROM friend_requests')
    requests_count = cursor.fetchone()['count']
    
    cursor.close()
    return friends_count, requests_count

def clear_all_friendships(conn):
    """
    Xóa tất cả friendships
    """
    cursor = conn.cursor()
    cursor.execute('DELETE FROM friends')
    deleted_count = cursor.rowcount
    cursor.close()
    return deleted_count

def clear_all_friend_requests(conn):
    """
    Xóa tất cả friend requests
    """
    cursor = conn.cursor()
    cursor.execute('DELETE FROM friend_requests')
    deleted_count = cursor.rowcount
    cursor.close()
    return deleted_count

def clear_all_notifications(conn):
    """
    Xóa tất cả notifications (tùy chọn)
    """
    cursor = conn.cursor()
    cursor.execute('DELETE FROM notifications')
    deleted_count = cursor.rowcount
    cursor.close()
    return deleted_count

def main():
    print("=" * 60)
    print("SCRIPT XÓA TẤT CẢ FRIENDSHIPS VÀ FRIEND REQUESTS")
    print("Database: tiny_game")
    print("=" * 60)
    print()
    
    # Test kết nối database
    print("🔌 Đang kiểm tra kết nối database...")
    try:
        conn = get_db_connection()
        print("✅ Kết nối database thành công!")
    except Exception as e:
        print(f"❌ Không thể kết nối database: {e}")
        print("\n💡 Gợi ý:")
        print("   - Kiểm tra PostgreSQL đang chạy")
        print("   - Kiểm tra thông tin kết nối:")
        print(f"     + Host: {DB_HOST} hoặc 'db' (docker)")
        print(f"     + Port: {DB_PORT}")
        print(f"     + Database: {DB_NAME}")
        print(f"     + User: {DB_USER}")
        print("   - Hoặc set biến môi trường DATABASE_URL")
        return
    print()
    
    # Lấy số lượng hiện tại
    print("📊 Đang kiểm tra số lượng hiện tại...")
    friends_count, requests_count = get_counts(conn)
    print(f"   - Friendships: {friends_count}")
    print(f"   - Friend requests: {requests_count}")
    print()
    
    if friends_count == 0 and requests_count == 0:
        print("✅ Database đã sạch, không có gì để xóa!")
        conn.close()
        return
    
    # Hỏi xác nhận
    print("⚠️  CẢNH BÁO: Thao tác này sẽ xóa TẤT CẢ:")
    print("   - Tất cả friendships")
    print("   - Tất cả friend requests")
    print()
    
    clear_notifications = input("Bạn có muốn xóa cả notifications? (y/n): ").strip().lower() == 'y'
    
    confirm = input("\nBạn có chắc chắn muốn xóa tất cả? (yes/no): ").strip().lower()
    if confirm != "yes":
        print("❌ Đã hủy!")
        conn.close()
        return
    
    print()
    print("=" * 60)
    print("ĐANG XÓA...")
    print("=" * 60)
    print()
    
    try:
        # Xóa friend requests trước (vì có foreign key)
        print("🗑️  Đang xóa friend requests...", end=" ")
        requests_deleted = clear_all_friend_requests(conn)
        print(f"✅ Đã xóa {requests_deleted} friend request(s)")
        
        # Xóa friendships
        print("🗑️  Đang xóa friendships...", end=" ")
        friends_deleted = clear_all_friendships(conn)
        print(f"✅ Đã xóa {friends_deleted} friendship(s)")
        
        # Xóa notifications nếu được yêu cầu
        notifications_deleted = 0
        if clear_notifications:
            print("🗑️  Đang xóa notifications...", end=" ")
            notifications_deleted = clear_all_notifications(conn)
            print(f"✅ Đã xóa {notifications_deleted} notification(s)")
        
        # Commit
        conn.commit()
        print()
        print("✅ Đã commit thành công!")
        
    except Exception as e:
        conn.rollback()
        print()
        print(f"❌ Lỗi khi xóa: {e}")
        print("   Đã rollback, không có thay đổi nào được lưu")
        conn.close()
        return
    
    # Kiểm tra lại
    print()
    print("📊 Kiểm tra lại sau khi xóa...")
    friends_count_after, requests_count_after = get_counts(conn)
    print(f"   - Friendships: {friends_count_after}")
    print(f"   - Friend requests: {requests_count_after}")
    
    if friends_count_after == 0 and requests_count_after == 0:
        print()
        print("=" * 60)
        print("✅ HOÀN TẤT!")
        print("=" * 60)
        print("Database đã được đồng bộ - không còn bạn bè nào!")
    else:
        print()
        print("⚠️  Vẫn còn dữ liệu, có thể có lỗi!")
    
    conn.close()
    print()

if __name__ == "__main__":
    main()

