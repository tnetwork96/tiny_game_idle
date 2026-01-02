#!/usr/bin/env python3
"""
Script giả lập gửi JSON status online qua WebSocket
Đơn giản: kết nối -> gửi init message với user_id 3
Server sẽ tự động broadcast đến các bạn bè
"""
import asyncio
import json
import websockets
from datetime import datetime

async def send_online_status():
    """
    Kết nối WebSocket và gửi message user 3 đang online
    """
    print("=" * 60)
    print("GIA LAP USER 3 ONLINE")
    print("=" * 60)
    print()
    
    user_id = 5
    # Thử cả localhost và IP local
    ws_urls = [
        "ws://192.168.1.7:8080/ws",
        "ws://localhost:8080/ws",
        "ws://127.0.0.1:8080/ws"
    ]
    
    websocket = None
    connected_url = None
    
    # Thử kết nối đến các URL
    for ws_url in ws_urls:
        print(f"[INFO] Dang thu ket noi: {ws_url}")
        try:
            websocket = await asyncio.wait_for(
                websockets.connect(ws_url),
                timeout=3.0
            )
            connected_url = ws_url
            print(f"[OK] Ket noi thanh cong: {ws_url}")
            print()
            break
        except Exception as e:
            print(f"[WARNING] Khong ket noi duoc {ws_url}: {type(e).__name__}")
    
    if not websocket:
        print()
        print("[ERROR] Khong the ket noi den server!")
        print("[TIP] Dam bao server dang chay (uvicorn hoac docker)")
        return
    
    try:
        print(f"[INFO] User ID: {user_id} (Tester - ban cua ESP32 user 5)")
        print()
        
        # Gửi init message
        init_message = {
            "type": "init",
            "device": "SCRIPT_TEST",
            "user_id": user_id
        }
        
        print(f"[SEND] Gui JSON message:")
        print(json.dumps(init_message, indent=2))
        print()
        
        await websocket.send(json.dumps(init_message))
        print(f"[OK] Da gui message thanh cong!")
        print()
        
        print(f"[INFO] Server se tu dong:")
        print(f"   1. Luu mapping user_id {user_id} <-> client_id")
        print(f"   2. Tim tat ca ban be cua user {user_id}")
        print(f"   3. Gui 'user_status_update' den cac ban be dang online")
        print(f"   4. ESP32 (user 5) se nhan duoc thong bao user 3 online")
        print()
        
        # Đợi nhận response
        print(f"[INFO] Doi response tu server...")
        try:
            response = await asyncio.wait_for(websocket.recv(), timeout=5.0)
            print(f"[RECV] Server response:")
            try:
                response_json = json.loads(response)
                print(json.dumps(response_json, indent=2, ensure_ascii=False))
            except:
                print(response)
            print()
        except asyncio.TimeoutError:
            print(f"[WARNING] Timeout (5s), nhung message da duoc gui")
            print()
        
        # Giữ kết nối 3 giây
        print(f"[INFO] Giu ket noi 3 giay de dam bao server xu ly xong...")
        await asyncio.sleep(3)
        
        print()
        print("=" * 60)
        print(f"[SUCCESS] HOAN THANH!")
        print("=" * 60)
        print()
        print(f"[CHECK] Kiem tra tren ESP32 (user 5):")
        print(f"   1. Serial Monitor: tim 'Received user_status_update'")
        print(f"   2. Serial Monitor: tim 'Parsed user_id: 3'")
        print(f"   3. Serial Monitor: tim 'Parsed status: online'")
        print(f"   4. Social Screen -> Friends: user 3 (Tester) phai hien thi ONLINE")
        print(f"   5. Xem dot indicator cua user 3 phai mau XANH")
        print()
        
    except Exception as e:
        print(f"[ERROR] Loi: {str(e)}")
        import traceback
        traceback.print_exc()
    finally:
        if websocket:
            await websocket.close()
            print(f"[INFO] Da dong ket noi WebSocket")

def main():
    asyncio.run(send_online_status())

if __name__ == "__main__":
    main()
