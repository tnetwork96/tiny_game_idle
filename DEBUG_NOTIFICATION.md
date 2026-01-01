# Debug Notification Popup

## Cách mở Serial Monitor

### Với PlatformIO (VS Code):
1. Mở VS Code
2. Click vào icon "Serial Monitor" ở bottom bar (biểu tượng ổ cắm)
3. Hoặc dùng shortcut: `Ctrl+Alt+S` (Windows/Linux) hoặc `Cmd+Alt+S` (Mac)
4. Chọn đúng COM port của ESP32
5. Baud rate: 115200

### Với PlatformIO CLI:
```bash
pio device monitor
```

## Logs cần kiểm tra khi gửi notification

### 1. Khi nhận notification từ WebSocket:
```
Socket Manager: Received message: {"type":"notification","notification":{...}}
Socket Manager: Received notification message
Socket Manager: Parsed notification - id: 9999, type: friend_request, message: ...
```

### 2. Main callback:
```
Main: Received socket notification - id: 9999, type: friend_request, message: ...
Main: Forwarding notification to SocialScreen...
Main: Notification forwarded, popup should be visible
```

### 3. SocialScreen xử lý:
```
Social Screen: Adding notification from socket - id: 9999, type: friend_request, message: ...
Social Screen: Notification added successfully, showing popup...
Social Screen: showNotificationPopup() called - message: ...
Social Screen: popupVisible set to: 1
Social Screen: popupMessage length: XX
```

### 4. Redraw:
```
Social Screen: Not on notifications tab, doing full redraw...
```
HOẶC
```
Social Screen: On notifications tab, redrawing content area...
```

### 5. Draw popup:
```
Social Screen: draw() - Drawing popup (popupVisible=true)
Social Screen: Drawing notification popup on screen...
```

## Các vấn đề thường gặp

### ❌ Không thấy "Main: Received socket notification"
- **Vấn đề**: ESP32 không nhận được message từ server
- **Kiểm tra**: 
  - WebSocket có kết nối không?
  - Server có gửi notification không?
  - User_id có đúng không?

### ❌ Thấy "Main: ⚠️ SocialScreen not active!"
- **Vấn đề**: SocialScreen chưa được khởi tạo hoặc chưa active
- **Kiểm tra**: 
  - ESP32 đã login chưa?
  - `isSocialScreenActive` có = true không?

### ❌ Thấy "popupVisible set to: 0" hoặc không thấy log này
- **Vấn đề**: `showNotificationPopup()` không được gọi hoặc bị lỗi
- **Kiểm tra**: 
  - Notification có được add thành công không?
  - `notificationExists` có = true không?

### ❌ Thấy "drawNotificationPopup skipped"
- **Vấn đề**: Popup không được vẽ vì điều kiện không đúng
- **Kiểm tra**: 
  - `popupVisible` có = true không?
  - `popupMessage.length()` có > 0 không?

### ❌ Không thấy "draw() - Drawing popup"
- **Vấn đề**: `draw()` không được gọi hoặc `popupVisible` = false khi draw
- **Kiểm tra**: 
  - `draw()` có được gọi sau khi set popup không?
  - Popup có bị reset không?

## Test notification

Chạy script test:
```bash
cd server
python test_send_notification.py 5 "Test notification"
```

Hoặc:
```bash
python test_notification_direct.py 5 "Test notification"
```

## Checklist debug

- [ ] Serial Monitor đã mở và kết nối đúng COM port
- [ ] Baud rate = 115200
- [ ] ESP32 đã login và vào SocialScreen
- [ ] WebSocket đã kết nối (xem log "Socket Manager: WebSocket connected")
- [ ] User_id đúng với user đang login
- [ ] Server đang chạy
- [ ] Đã gửi notification qua script test

