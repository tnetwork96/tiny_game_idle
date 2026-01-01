#!/usr/bin/env python3
"""
Script để test gửi notification qua WebSocket đến ESP32
Sử dụng API endpoint /api/test/notification để gửi notification
"""
import requests
import sys
import os
from datetime import datetime

# Cấu hình Server API
SERVER_HOST = os.getenv("SERVER_HOST", "192.168.1.7")
SERVER_PORT = os.getenv("SERVER_PORT", "8080")
BASE_URL = f"http://{SERVER_HOST}:{SERVER_PORT}"

def send_test_notification(user_id: int, message: str, notification_type: str = "friend_request"):
    """
    Gửi test notification đến user qua API endpoint
    """
    url = f"{BASE_URL}/api/test/notification"
    payload = {
        "user_id": user_id,
        "message": message,
        "notification_type": notification_type
    }
    
    print(f"📡 Đang gửi test notification đến user_id {user_id}...")
    print(f"   Message: {message}")
    print(f"   Type: {notification_type}")
    print()
    
    try:
        response = requests.post(url, json=payload, timeout=5)
        response.raise_for_status()
        result = response.json()
        
        if result.get("success"):
            print(f"✅ {result.get('message', 'Đã gửi thành công!')}")
            return True
        else:
            print(f"❌ {result.get('message', 'Không thể gửi')}")
            return False
    except requests.exceptions.RequestException as e:
        print(f"❌ Lỗi khi gọi API: {e}")
        return False

def main():
    print("=" * 60)
    print("SCRIPT TEST GỬI NOTIFICATION QUA WEBSOCKET")
    print("=" * 60)
    print()
    
    # Lấy user_id từ command line hoặc input
    if len(sys.argv) > 1:
        try:
            user_id = int(sys.argv[1])
        except ValueError:
            print(f"❌ User ID không hợp lệ: {sys.argv[1]}")
            return
    else:
        user_id_str = input("Nhập user_id để gửi notification (ví dụ: player2 = 2): ").strip()
        try:
            user_id = int(user_id_str)
        except ValueError:
            print(f"❌ User ID không hợp lệ: {user_id_str}")
            return
    
    # Lấy message từ command line hoặc input
    if len(sys.argv) > 2:
        message = " ".join(sys.argv[2:])
    else:
        message = input("Nhập message notification (hoặc Enter để dùng message mặc định): ").strip()
        if not message:
            message = f"Test notification từ script - {datetime.now().strftime('%H:%M:%S')}"
    
    print()
    print(f"📋 Thông tin:")
    print(f"   User ID: {user_id}")
    print(f"   Message: {message}")
    print()
    
    # Gửi notification
    result = send_test_notification(user_id, message)
    
    print()
    if result:
        print("=" * 60)
        print("✅ THÀNH CÔNG!")
        print("=" * 60)
        print("Notification đã được gửi đến ESP32")
        print("Kiểm tra màn hình ESP32 để xem popup notification")
    else:
        print("=" * 60)
        print("❌ THẤT BẠI!")
        print("=" * 60)
        print("Không thể gửi notification. Có thể:")
        print("   1. ESP32 chưa kết nối WebSocket")
        print("   2. ESP32 chưa gửi init message với user_id")
        print("   3. User_id không đúng")
        print()
        print("💡 Gợi ý:")
        print("   - Kiểm tra Serial Monitor của ESP32")
        print("   - Đảm bảo ESP32 đã login và kết nối WebSocket")
        print("   - Kiểm tra user_id trong init message")

if __name__ == "__main__":
    main()

