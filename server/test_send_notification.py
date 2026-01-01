#!/usr/bin/env python3
"""
Script để test gửi notification với message format đúng
Dựa trên parseNotificationMessage của ESP32
"""
import requests
import json
import sys
import os
from datetime import datetime

# Cấu hình Server API
SERVER_HOST = os.getenv("SERVER_HOST", "192.168.1.7")
SERVER_PORT = os.getenv("SERVER_PORT", "8080")
BASE_URL = f"http://{SERVER_HOST}:{SERVER_PORT}"

def send_test_notification(user_id: int, message: str = None):
    """
    Gửi notification với format đúng cho ESP32
    Format: {"type":"notification","notification":{"id":...,"type":"...","message":"...","timestamp":"...","read":false}}
    """
    if message is None:
        message = f"Test notification - {datetime.now().strftime('%H:%M:%S')}"
    
    url = f"{BASE_URL}/api/test/notification"
    payload = {
        "user_id": user_id,
        "message": message,
        "notification_type": "friend_request"
    }
    
    print("=" * 60)
    print("TEST GỬI NOTIFICATION")
    print("=" * 60)
    print()
    print(f"📤 Gửi đến user_id: {user_id}")
    print(f"📝 Message: {message}")
    print()
    print("📋 Format message sẽ được gửi:")
    print('   {"type":"notification","notification":{')
    print(f'     "id": 9999,')
    print(f'     "type": "friend_request",')
    print(f'     "message": "{message}",')
    print(f'     "timestamp": "...",')
    print(f'     "read": false')
    print('   }}')
    print()
    
    try:
        response = requests.post(url, json=payload, timeout=5)
        response.raise_for_status()
        result = response.json()
        
        if result.get("success"):
            print("✅ " + result.get('message', 'Đã gửi thành công!'))
            print()
            print("💡 Kiểm tra:")
            print("   1. Serial Monitor của ESP32 - xem có log 'Received notification message' không")
            print("   2. Serial Monitor - xem có log 'Parsed notification' không")
            print("   3. Serial Monitor - xem có log 'Main: Received socket notification' không")
            print("   4. Màn hình TFT - xem có popup notification không")
            return True
        else:
            print("❌ " + result.get('message', 'Không thể gửi'))
            print()
            print("💡 Có thể:")
            print("   1. User chưa kết nối WebSocket")
            print("   2. ESP32 chưa gửi init message với user_id")
            print("   3. User_id không đúng")
            return False
    except requests.exceptions.RequestException as e:
        print(f"❌ Lỗi khi gọi API: {e}")
        return False

def main():
    if len(sys.argv) > 1:
        try:
            user_id = int(sys.argv[1])
        except ValueError:
            print(f"❌ User ID không hợp lệ: {sys.argv[1]}")
            print("Usage: python test_send_notification.py <user_id> [message]")
            return
    else:
        user_id_str = input("Nhập user_id để gửi notification (ví dụ: 5): ").strip()
        try:
            user_id = int(user_id_str)
        except ValueError:
            print(f"❌ User ID không hợp lệ: {user_id_str}")
            return
    
    message = None
    if len(sys.argv) > 2:
        message = " ".join(sys.argv[2:])
    else:
        message_input = input("Nhập message (hoặc Enter để dùng message mặc định): ").strip()
        if message_input:
            message = message_input
    
    print()
    send_test_notification(user_id, message)

if __name__ == "__main__":
    main()

