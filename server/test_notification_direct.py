#!/usr/bin/env python3
"""
Script để test gửi notification trực tiếp với message format đúng
Dựa trên parseNotificationMessage của ESP32
"""
import sys
import os
import asyncio
import json
from datetime import datetime

# Add parent directory to path để import websocket_manager
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

try:
    from app.api.websocket import websocket_manager
    print("✅ Đã import websocket_manager thành công")
except ImportError as e:
    print(f"❌ Không thể import websocket_manager: {e}")
    print("   Hãy đảm bảo server đang chạy và script chạy từ thư mục server")
    sys.exit(1)

def create_test_notification_message(notification_id: int, notification_type: str, message: str):
    """
    Tạo notification message đúng format cho ESP32
    Format dựa trên parseNotificationMessage:
    {"type":"notification","notification":{"id":123,"type":"...","message":"...","timestamp":"...","read":false}}
    """
    notification_data = {
        "id": notification_id,
        "type": notification_type,
        "message": message,
        "timestamp": datetime.now().isoformat(),
        "read": False
    }
    
    full_message = {
        "type": "notification",
        "notification": notification_data
    }
    
    return full_message, notification_data

async def send_notification_direct(user_id: int, message: str = None):
    """
    Gửi notification trực tiếp qua WebSocketManager
    """
    if message is None:
        message = f"Test notification - {datetime.now().strftime('%H:%M:%S')}"
    
    print("=" * 60)
    print("TEST GỬI NOTIFICATION TRỰC TIẾP")
    print("=" * 60)
    print()
    
    # Kiểm tra connection
    print(f"📋 Kiểm tra WebSocket connection cho user_id {user_id}...")
    if user_id not in websocket_manager.user_to_client:
        print(f"❌ User {user_id} không có trong user_to_client mapping")
        print(f"   Có thể ESP32 chưa kết nối hoặc chưa gửi init message")
        return False
    
    client_id = websocket_manager.user_to_client[user_id]
    print(f"   ✅ User {user_id} -> Client {client_id}")
    
    if client_id not in websocket_manager.active_connections:
        print(f"❌ Client {client_id} không trong active_connections")
        return False
    
    print(f"   ✅ Client {client_id} đang kết nối")
    print()
    
    # Tạo notification message với ID unique (dựa trên timestamp)
    import time
    unique_id = int(time.time() * 1000) % 1000000  # Use milliseconds timestamp as ID
    
    full_message, notification_data = create_test_notification_message(
        notification_id=unique_id,
        notification_type="friend_request",
        message=message
    )
    
    print("📤 Notification message sẽ được gửi:")
    print(json.dumps(full_message, indent=2))
    print()
    
    # Gửi qua WebSocketManager
    print("📡 Đang gửi notification...")
    result = await websocket_manager.send_notification_to_user(user_id, notification_data)
    
    if result:
        print("✅ Đã gửi notification thành công!")
        print()
        print("💡 Kiểm tra ESP32:")
        print("   1. Serial Monitor - xem có log 'Received notification message'")
        print("   2. Serial Monitor - xem có log 'Parsed notification - id: 9999'")
        print("   3. Serial Monitor - xem có log 'Main: Received socket notification'")
        print("   4. Màn hình TFT - xem có popup notification không")
    else:
        print("❌ Không thể gửi notification")
        print("   Kiểm tra lại WebSocket connection")
    
    return result

def main():
    if len(sys.argv) > 1:
        try:
            user_id = int(sys.argv[1])
        except ValueError:
            print(f"❌ User ID không hợp lệ: {sys.argv[1]}")
            print("Usage: python test_notification_direct.py <user_id> [message]")
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
    
    # Chạy async function
    try:
        loop = asyncio.get_event_loop()
    except RuntimeError:
        loop = asyncio.new_event_loop()
        asyncio.set_event_loop(loop)
    
    loop.run_until_complete(send_notification_direct(user_id, message))

if __name__ == "__main__":
    main()

