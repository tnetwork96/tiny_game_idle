#!/usr/bin/env python3
"""
Script để test gửi chat message từ user 1 đến user 5
Giả lập client gửi tin nhắn qua WebSocket
"""
import asyncio
import websockets
import json
import sys
import os
from datetime import datetime
import uuid

# Cấu hình Server WebSocket
SERVER_HOST = os.getenv("SERVER_HOST", "192.168.1.7")
SERVER_PORT = os.getenv("SERVER_PORT", "8080")
WS_URL = f"ws://{SERVER_HOST}:{SERVER_PORT}/ws"

print(f"🔧 WebSocket URL: {WS_URL}")

async def send_chat_message(from_user_id: int, to_user_id: int, message: str = None):
    """
    Kết nối WebSocket với from_user_id và gửi chat message đến to_user_id
    """
    if message is None:
        message = f"tao ne"
    
    print("=" * 60)
    print("TEST GỬI CHAT MESSAGE")
    print("=" * 60)
    print()
    print(f"👤 From user_id: {from_user_id}")
    print(f"👤 To user_id: {to_user_id}")
    print(f"📝 Message: {message}")
    print()
    
    try:
        # Kết nối WebSocket
        print(f"🔌 Đang kết nối WebSocket đến {WS_URL}...")
        ws = await websockets.connect(WS_URL)
        print("   ✅ WebSocket connected")
        
        # Gửi init message với user_id
        init_message = {
            "type": "init",
            "device": "PythonScript",
            "user_id": from_user_id
        }
        print()
        print("📤 Gửi init message...")
        print(f"   {json.dumps(init_message, indent=2)}")
        await ws.send(json.dumps(init_message))
        
        # Đợi init_ack
        try:
            response = await asyncio.wait_for(ws.recv(), timeout=5.0)
            ack = json.loads(response)
            if ack.get("type") == "init_ack":
                print("   ✅ Received init_ack")
            else:
                print(f"   ⚠️  Unexpected response: {ack}")
        except asyncio.TimeoutError:
            print("   ⚠️  Timeout waiting for init_ack")
        
        # Đợi một chút để server xử lý
        await asyncio.sleep(0.5)
        
        # Tạo message_id unique
        message_id = str(uuid.uuid4())
        timestamp = datetime.now().isoformat()
        
        # Gửi chat message
        chat_message = {
            "type": "chat_message",
            "to_user_id": to_user_id,
            "message": message,
            "message_id": message_id,
            "timestamp": timestamp
        }
        
        print()
        print("📤 Gửi chat message...")
        print("   Format gửi đi (client -> server):")
        print(f"   {json.dumps(chat_message, indent=2, ensure_ascii=False)}")
        
        # Gửi message
        message_json = json.dumps(chat_message)
        await ws.send(message_json)
        print(f"   ✅ Đã gửi {len(message_json)} bytes")
        
        # Đợi response (có thể là ack hoặc error)
        try:
            response = await asyncio.wait_for(ws.recv(), timeout=5.0)
            result = json.loads(response)
            print()
            print("📥 Response từ server (sender):")
            print(f"   {json.dumps(result, indent=2, ensure_ascii=False)}")
            
            if result.get("type") == "chat_error":
                print()
                print(f"❌ Lỗi: {result.get('message')} (code: {result.get('code')})")
                print()
                print("💡 Có thể:")
                print("   - Users không phải là friends")
                print("   - Rate limit exceeded")
                print("   - Message quá dài hoặc rỗng")
            elif result.get("type") == "chat_ack":
                print()
                print("✅ Chat message đã được gửi thành công!")
                print(f"   Message ID: {result.get('message_id')}")
                print()
                print("💡 Server sẽ forward message đến recipient với format:")
                print("   {")
                print('     "type": "chat_message",')
                print(f'     "from_user_id": {from_user_id},')
                print('     "from_nickname": "...",')
                print('     "message": "...",')
                print('     "message_id": "...",')
                print('     "timestamp": "..."')
                print("   }")
        except asyncio.TimeoutError:
            print()
            print("   ⚠️  Không nhận được response từ server (timeout)")
            print("   (Có thể server đã xử lý nhưng không gửi ack)")
        
        # Đợi một chút để xem có delivery status không
        print()
        print("⏳ Đợi delivery status từ server (nếu recipient online)...")
        print("   (Server sẽ gửi message_delivered nếu recipient nhận được)")
        try:
            response = await asyncio.wait_for(ws.recv(), timeout=3.0)
            result = json.loads(response)
            print()
            print(f"📥 Delivery status: {json.dumps(result, indent=2, ensure_ascii=False)}")
            if result.get("type") == "message_delivered":
                print("   ✅ Message đã được delivered đến recipient!")
        except asyncio.TimeoutError:
            print("   (Không có delivery status - recipient có thể offline hoặc chưa nhận)")
        
        # Đóng kết nối
        await ws.close()
        print()
        print("🔌 WebSocket disconnected")
        print()
        print("💡 Kiểm tra:")
        print(f"   1. User {to_user_id} có nhận được tin nhắn không (nếu online)")
        print(f"   2. Serial Monitor của ESP32 (user {to_user_id}) - xem có log 'Received chat message' không")
        print(f"   3. Màn hình TFT - xem có hiển thị tin nhắn không")
        
        return True
        
    except websockets.exceptions.InvalidURI:
        print(f"❌ Invalid WebSocket URL: {WS_URL}")
        return False
    except websockets.exceptions.ConnectionClosed:
        print("❌ WebSocket connection closed unexpectedly")
        return False
    except Exception as e:
        print(f"❌ Lỗi: {e}")
        import traceback
        traceback.print_exc()
        return False

async def main():
    from_user_id = 3
    to_user_id = 5
    
    if len(sys.argv) > 1:
        try:
            from_user_id = int(sys.argv[1])
        except ValueError:
            print(f"❌ From user ID không hợp lệ: {sys.argv[1]}")
            print("Usage: python test_send_chat_message.py [from_user_id] [to_user_id] [message]")
            return
    
    if len(sys.argv) > 2:
        try:
            to_user_id = int(sys.argv[2])
        except ValueError:
            print(f"❌ To user ID không hợp lệ: {sys.argv[2]}")
            print("Usage: python test_send_chat_message.py [from_user_id] [to_user_id] [message]")
            return
    
    message = None
    if len(sys.argv) > 3:
        message = " ".join(sys.argv[3:])
    
    await send_chat_message(from_user_id, to_user_id, message)

if __name__ == "__main__":
    asyncio.run(main())

