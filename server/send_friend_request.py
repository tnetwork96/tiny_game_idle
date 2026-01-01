#!/usr/bin/env python3
"""
Script để gửi friend request từ player2 đến các user khác
Sử dụng database tiny_game và API endpoint
"""
import requests
import json
import os
import sys
import asyncio
import websockets
from datetime import datetime
from typing import Optional, List, Dict

# Cấu hình Database
DB_HOST = os.getenv("DB_HOST", "localhost")
DB_PORT = os.getenv("DB_PORT", "5432")
DB_NAME = "tiny_game"
DB_USER = os.getenv("DB_USER", "tinygame")
DB_PASSWORD = os.getenv("DB_PASSWORD", "tinygame123")

# Cấu hình Server API
SERVER_HOST = os.getenv("SERVER_HOST", "192.168.1.7")
SERVER_PORT = os.getenv("SERVER_PORT", "8080")
BASE_URL = f"http://{SERVER_HOST}:{SERVER_PORT}"
WS_URL = f"ws://{SERVER_HOST}:{SERVER_PORT}/ws"

# Username hiện tại
CURRENT_USERNAME = "player2"

# WebSocket connections cache
websocket_connections = {}

def get_db_connection():
    """
    Tạo kết nối đến database tiny_game
    Hỗ trợ cả localhost và docker
    """
    import psycopg2
    from psycopg2.extras import RealDictCursor
    
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
            print(f"   - Database: {DB_NAME}")
            print(f"   - Host: {DB_HOST} hoặc 'db'")
            raise

def get_user_id(username: str) -> Optional[int]:
    """
    Lấy user_id từ username bằng cách query database tiny_game
    """
    from psycopg2.extras import RealDictCursor
    
    try:
        conn = get_db_connection()
        cursor = conn.cursor(cursor_factory=RealDictCursor)
        
        cursor.execute('SELECT id FROM users WHERE username = %s', (username,))
        result = cursor.fetchone()
        
        cursor.close()
        conn.close()
        
        if result:
            return result['id']
        return None
    except Exception as e:
        print(f"❌ Lỗi khi lấy user_id: {e}")
        return None

def get_all_users() -> List[Dict]:
    """
    Lấy danh sách tất cả users từ database tiny_game
    """
    from psycopg2.extras import RealDictCursor
    
    try:
        conn = get_db_connection()
        cursor = conn.cursor(cursor_factory=RealDictCursor)
        
        cursor.execute('''
            SELECT id, username, COALESCE(nickname, username) as display_name 
            FROM users 
            ORDER BY id
        ''')
        users = cursor.fetchall()
        
        cursor.close()
        conn.close()
        
        return [{"id": u['id'], "username": u['username'], "display_name": u['display_name']} for u in users]
    except Exception as e:
        print(f"❌ Lỗi khi lấy danh sách users: {e}")
        return []

def check_existing_friendship(from_user_id: int, to_user_id: int) -> bool:
    """
    Kiểm tra xem đã là bạn chưa
    """
    from psycopg2.extras import RealDictCursor
    
    try:
        conn = get_db_connection()
        cursor = conn.cursor(cursor_factory=RealDictCursor)
        
        cursor.execute('''
            SELECT id FROM friends 
            WHERE (user_id = %s AND friend_id = %s) OR (user_id = %s AND friend_id = %s)
        ''', (from_user_id, to_user_id, to_user_id, from_user_id))
        
        result = cursor.fetchone()
        
        cursor.close()
        conn.close()
        
        return result is not None
    except Exception as e:
        print(f"⚠️  Lỗi khi kiểm tra friendship: {e}")
        return False

def check_existing_request(from_user_id: int, to_user_id: int) -> Optional[str]:
    """
    Kiểm tra xem đã có friend request chưa
    Trả về status nếu có, None nếu chưa có
    """
    from psycopg2.extras import RealDictCursor
    
    try:
        conn = get_db_connection()
        cursor = conn.cursor(cursor_factory=RealDictCursor)
        
        cursor.execute('''
            SELECT status FROM friend_requests 
            WHERE (from_user_id = %s AND to_user_id = %s) 
               OR (from_user_id = %s AND to_user_id = %s)
            ORDER BY created_at DESC
            LIMIT 1
        ''', (from_user_id, to_user_id, to_user_id, from_user_id))
        
        result = cursor.fetchone()
        
        cursor.close()
        conn.close()
        
        if result:
            return result['status']
        return None
    except Exception as e:
        print(f"⚠️  Lỗi khi kiểm tra friend request: {e}")
        return None

async def connect_websocket(user_id: int) -> Optional[websockets.WebSocketClientProtocol]:
    """
    Kết nối WebSocket cho user và gửi init message
    """
    try:
        ws = await websockets.connect(WS_URL)
        
        # Gửi init message với user_id
        init_message = {
            "type": "init",
            "device": "PythonScript",
            "user_id": user_id
        }
        await ws.send(json.dumps(init_message))
        
        # Đợi ack
        response = await asyncio.wait_for(ws.recv(), timeout=5.0)
        ack = json.loads(response)
        
        if ack.get("type") == "init_ack":
            print(f"   ✅ WebSocket connected cho user_id {user_id}")
            return ws
        else:
            await ws.close()
            return None
    except Exception as e:
        print(f"   ⚠️  Không thể kết nối WebSocket cho user_id {user_id}: {e}")
        return None

async def send_notification_via_websocket(to_user_id: int, notification_data: dict) -> bool:
    """
    Gửi notification qua WebSocket đến user
    """
    # Kiểm tra xem đã có connection chưa
    if to_user_id not in websocket_connections:
        ws = await connect_websocket(to_user_id)
        if ws:
            websocket_connections[to_user_id] = ws
        else:
            return False
    
    ws = websocket_connections[to_user_id]
    
    try:
        message = {
            "type": "notification",
            "notification": notification_data
        }
        await ws.send(json.dumps(message))
        print(f"   📡 Đã gửi notification qua WebSocket")
        return True
    except Exception as e:
        print(f"   ⚠️  Lỗi khi gửi notification qua WebSocket: {e}")
        # Xóa connection cũ và thử lại
        if to_user_id in websocket_connections:
            try:
                await websocket_connections[to_user_id].close()
            except:
                pass
            del websocket_connections[to_user_id]
        return False

def send_friend_request(from_user_id: int, to_nickname: str, to_user_id: Optional[int] = None) -> dict:
    """
    Gửi friend request qua API
    """
    url = f"{BASE_URL}/api/friend-requests/send"
    
    payload = {
        "from_user_id": from_user_id,
        "to_nickname": to_nickname
    }
    
    try:
        response = requests.post(url, json=payload, timeout=10)
        response.raise_for_status()
        return response.json()
    except requests.exceptions.RequestException as e:
        print(f"❌ Lỗi khi gửi request: {e}")
        if hasattr(e, 'response') and e.response is not None:
            try:
                error_data = e.response.json()
                return error_data
            except:
                return {"success": False, "message": str(e)}
        return {"success": False, "message": str(e)}

def main():
    print("=" * 60)
    print("SCRIPT GỬI FRIEND REQUEST TỪ PLAYER2")
    print("Database: tiny_game")
    print("=" * 60)
    print()
    
    # Test kết nối database
    print("🔌 Đang kiểm tra kết nối database...")
    try:
        conn = get_db_connection()
        conn.close()
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
    
    # Lấy user_id của player2 (người nhận)
    print(f"📋 Đang lấy user_id của {CURRENT_USERNAME} (người nhận)...")
    to_user_id = get_user_id(CURRENT_USERNAME)
    
    if not to_user_id:
        print(f"❌ Không tìm thấy user '{CURRENT_USERNAME}' trong database tiny_game!")
        return
    
    print(f"✅ Tìm thấy {CURRENT_USERNAME} với user_id: {to_user_id}")
    print()
    
    # Lấy danh sách tất cả users
    print("📋 Đang lấy danh sách users từ database tiny_game...")
    all_users = get_all_users()
    
    if not all_users:
        print("❌ Không lấy được danh sách users!")
        return
    
    print(f"✅ Tìm thấy {len(all_users)} users trong database:")
    for i, user in enumerate(all_users, 1):
        print(f"   {i}. {user['display_name']} (username: {user['username']}, id: {user['id']})")
    print()
    
    # Lọc ra các user khác (không phải player2) - đây là những người sẽ gửi request
    sender_users = [u for u in all_users if u['username'] != CURRENT_USERNAME]
    
    if not sender_users:
        print("❌ Không có user nào khác để gửi friend request đến player2!")
        return
    
    # Kiểm tra trạng thái với từng user (từ user khác đến player2)
    print("🔍 Đang kiểm tra trạng thái với các user...")
    users_to_send = []
    users_skipped = []
    
    for user in sender_users:
        from_user_id = user['id']
        
        # Kiểm tra đã là bạn chưa
        if check_existing_friendship(from_user_id, to_user_id):
            users_skipped.append((user, "Đã là bạn"))
            continue
        
        # Kiểm tra đã có request chưa (từ user này đến player2)
        existing_status = check_existing_request(from_user_id, to_user_id)
        if existing_status:
            if existing_status == "pending":
                users_skipped.append((user, f"Đã có request pending"))
            elif existing_status == "accepted":
                users_skipped.append((user, "Đã accepted (có thể đã là bạn)"))
            else:
                users_skipped.append((user, f"Request status: {existing_status}"))
            continue
        
        users_to_send.append(user)
    
    # Hiển thị kết quả kiểm tra
    if users_skipped:
        print(f"⏭️  Bỏ qua {len(users_skipped)} user(s):")
        for user, reason in users_skipped:
            print(f"   - {user['display_name']}: {reason}")
        print()
    
    if not users_to_send:
        print("❌ Không có user nào cần gửi friend request!")
        print("   (Tất cả đã là bạn hoặc đã có request)")
        return
    
    print(f"📤 Có {len(users_to_send)} user(s) sẽ gửi friend request đến {CURRENT_USERNAME}:")
    for i, user in enumerate(users_to_send, 1):
        print(f"   {i}. {user['display_name']} (id: {user['id']}) -> {CURRENT_USERNAME} (id: {to_user_id})")
    print()
    
    print("=" * 60)
    print("ĐANG GỬI FRIEND REQUESTS (TỪNG NGƯỜI MỘT)...")
    print(f"Từ các user khác -> {CURRENT_USERNAME}")
    print("=" * 60)
    print()
    
    success_count = 0
    fail_count = 0
    skipped_count = 0
    
    # Kết nối WebSocket cho player2 (người nhận) trước
    print(f"🔌 Đang kết nối WebSocket cho {CURRENT_USERNAME} (user_id: {to_user_id})...")
    try:
        loop = asyncio.get_event_loop()
    except RuntimeError:
        loop = asyncio.new_event_loop()
        asyncio.set_event_loop(loop)
    
    player2_ws = loop.run_until_complete(connect_websocket(to_user_id))
    if player2_ws:
        websocket_connections[to_user_id] = player2_ws
        print(f"✅ WebSocket đã kết nối cho {CURRENT_USERNAME}")
    else:
        print(f"⚠️  Không thể kết nối WebSocket cho {CURRENT_USERNAME} (sẽ thử lại sau mỗi request)")
    print()
    
    # Gửi friend request từ từng user đến player2
    for i, user in enumerate(users_to_send, 1):
        from_user_id = user['id']
        from_nickname = user['display_name']
        
        print(f"[{i}/{len(users_to_send)}] {from_nickname} (id: {from_user_id}) -> {CURRENT_USERNAME}")
        
        # Hỏi xác nhận cho từng user
        confirm = input(f"   Gửi friend request từ {from_nickname} đến {CURRENT_USERNAME}? (y/n/s=skip all): ").strip().lower()
        
        if confirm == 's':
            print("   ⏭️  Bỏ qua tất cả các user còn lại")
            skipped_count = len(users_to_send) - i
            break
        elif confirm != 'y':
            print("   ⏭️  Đã bỏ qua")
            skipped_count += 1
            print()
            continue
        
        # Lấy display_name của player2 (nickname hoặc username)
        player2_display_name = None
        for u in all_users:
            if u['username'] == CURRENT_USERNAME:
                player2_display_name = u['display_name']
                break
        
        if not player2_display_name:
            player2_display_name = CURRENT_USERNAME
        
        # Gửi friend request (từ user này đến player2)
        print(f"   📤 Đang gửi...", end=" ")
        result = send_friend_request(from_user_id, player2_display_name, to_user_id)
        
        if result.get('success', False):
            print(f"✅ {result.get('message', 'Thành công')}")
            success_count += 1
            
            # Giả lập gửi notification qua WebSocket đến player2
            try:
                # Lấy notification_id từ database (notification mới nhất của player2)
                from psycopg2.extras import RealDictCursor
                conn = get_db_connection()
                cursor = conn.cursor(cursor_factory=RealDictCursor)
                
                cursor.execute('''
                    SELECT id, type, message, created_at 
                    FROM notifications 
                    WHERE user_id = %s AND type = 'friend_request'
                    ORDER BY created_at DESC 
                    LIMIT 1
                ''', (to_user_id,))
                
                notification = cursor.fetchone()
                cursor.close()
                conn.close()
                
                if notification:
                    # Tạo notification data
                    notification_data = {
                        "id": notification['id'],
                        "type": notification['type'],
                        "message": notification['message'],
                        "timestamp": notification['created_at'].isoformat() if isinstance(notification['created_at'], datetime) else str(notification['created_at']),
                        "read": False
                    }
                    
                    # Gửi qua WebSocket đến player2
                    print(f"   🔔 Đang gửi notification qua WebSocket đến {CURRENT_USERNAME}...", end=" ")
                    
                    if USE_SERVER_WEBSOCKET:
                        # Sử dụng WebSocketManager từ server
                        try:
                            loop = asyncio.get_event_loop()
                        except RuntimeError:
                            loop = asyncio.new_event_loop()
                            asyncio.set_event_loop(loop)
                        
                        sent = loop.run_until_complete(websocket_manager.send_notification_to_user(to_user_id, notification_data))
                        if sent:
                            print("✅")
                        else:
                            print("⚠️  (User chưa kết nối WebSocket)")
                    else:
                        # Fallback: sử dụng WebSocket connection riêng (có thể không hoạt động)
                        if to_user_id not in websocket_connections:
                            ws = loop.run_until_complete(connect_websocket(to_user_id))
                            if ws:
                                websocket_connections[to_user_id] = ws
                        
                        if to_user_id in websocket_connections:
                            sent = loop.run_until_complete(send_notification_via_websocket(to_user_id, notification_data))
                            if sent:
                                print("✅")
                            else:
                                print("⚠️  (Không thể gửi)")
                        else:
                            print("⚠️  (WebSocket chưa kết nối)")
                else:
                    print(f"   ⚠️  Không tìm thấy notification trong database")
            except Exception as e:
                print(f"   ⚠️  Lỗi khi gửi notification qua WebSocket: {e}")
        else:
            error_msg = result.get('message', 'Lỗi không xác định')
            print(f"❌ {error_msg}")
            fail_count += 1
        
        print()
    
    # Đóng tất cả WebSocket connections (chỉ nếu dùng connection riêng)
    if not USE_SERVER_WEBSOCKET and websocket_connections:
        print("🔌 Đang đóng WebSocket connections...")
        try:
            loop = asyncio.get_event_loop()
        except RuntimeError:
            loop = asyncio.new_event_loop()
            asyncio.set_event_loop(loop)
        
        for user_id, ws in websocket_connections.items():
            try:
                loop.run_until_complete(ws.close())
            except:
                pass
        websocket_connections.clear()
        print("✅ Đã đóng tất cả WebSocket connections")
        print()
    
    print("=" * 60)
    print("KẾT QUẢ")
    print("=" * 60)
    print(f"✅ Thành công: {success_count}")
    print(f"❌ Thất bại: {fail_count}")
    print(f"⏭️  Đã bỏ qua: {skipped_count}")
    print(f"📊 Tổng cộng đã xử lý: {success_count + fail_count + skipped_count}/{len(users_to_send)}")
    if users_skipped:
        print(f"⏭️  Đã bỏ qua từ đầu (đã là bạn/có request): {len(users_skipped)}")
    print()

if __name__ == "__main__":
    main()

