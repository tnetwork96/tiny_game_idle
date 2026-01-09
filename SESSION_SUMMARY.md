# Tóm Tắt Session - Đồng Bộ Điều Hướng Social Screen

**Ngày:** 10-11/2026

## Tổng Quan

Session này tập trung vào việc đồng bộ và cải thiện hệ thống điều hướng cho Social Screen, bao gồm việc fix các lỗi navigation, implement các tính năng mới, và xử lý các vấn đề về input/output.

---

## 1. Implement Sync Navigation

### Mục đích
Đồng bộ trạng thái điều hướng khi chuyển tab hoặc khi screen trở thành active.

### Thay đổi
- **Thêm hàm `syncNavigation()`** trong `SocialScreen`:
  - Reset `selectedFriendIndex`, `friendsScrollOffset` dựa trên `currentTab`
  - Reset `selectedNotificationIndex`, `notificationsScrollOffset` dựa trên `currentTab`
  - Reset `selectedGameIndex`, `selectedGameInviteIndex` dựa trên `currentTab`
  - Set `focusMode = FOCUS_CONTENT` cho `TAB_ADD_FRIEND`
  - Set `focusMode = FOCUS_SIDEBAR` cho các tab khác

- **Gọi `syncNavigation()` trong:**
  - `navigateToFriends()`, `navigateToNotifications()`, `navigateToGames()`, `navigateToAddFriend()`
  - `setActive(true)` khi screen trở thành active
  - `onLoginSuccess()` trong `main.cpp` trước khi navigate

- **Đổi tab mặc định:** Từ "Notifications" sang "Friends" khi login thành công

### Files thay đổi
- `src/social_screen.h` - Thêm declaration `void syncNavigation();`
- `src/social_screen.cpp` - Implement `syncNavigation()` và integrate vào các hàm navigate
- `src/main.cpp` - Gọi `syncNavigation()` trong `onLoginSuccess()`

---

## 2. Fix Tab Navigation

### Vấn đề
- Không thể điều hướng lên xuống các tabs (UP/DOWN không hoạt động)
- Bị kẹt khi điều hướng xuống tab Add Friend
- Không có circular navigation (từ tab cuối DOWN không quay về tab đầu)

### Giải pháp
- **Fix lỗi focusMode:** Xóa `focusMode = FOCUS_CONTENT` khỏi các hàm `navigateTo*()` để cho phép tab navigation
- **Implement circular navigation:**
  - UP từ `TAB_FRIENDS` → `TAB_GAMES`
  - DOWN từ `TAB_GAMES` → `TAB_FRIENDS`
- **Fix routing navigation keys:**
  - Trong `main.cpp`, route navigation keys (up/down/left/right/select/exit) đến `SocialScreen::handleKeyPress()` khi ở `TAB_ADD_FRIEND`
  - Chỉ route text input keys đến `MiniAddFriendScreen`

### Files thay đổi
- `src/social_screen.cpp` - Fix `handleTabNavigation()`, `handleUp()`, `handleDown()`
- `src/main.cpp` - Fix `onKeyboardKeySelected()` và `onMiniKeyboardKeySelected()`

---

## 3. Implement Left/Right Navigation

### Yêu cầu
- LEFT/RIGHT để điều hướng keyboard khi ở tab Add Friend
- LEFT để quay về sidebar từ content
- RIGHT để chuyển từ sidebar sang content
- Chỉ highlight content elements khi `focusMode = FOCUS_CONTENT`

### Implementation
- **`handleLeft()`:**
  - Khi `focusMode == FOCUS_CONTENT` và `currentTab == TAB_ADD_FRIEND` → điều hướng keyboard
  - Các tab khác → set `focusMode = FOCUS_SIDEBAR` và unhighlight content

- **`handleRight()`:**
  - Khi `focusMode == FOCUS_SIDEBAR` → set `focusMode = FOCUS_CONTENT` và highlight item đầu tiên
  - Khi `focusMode == FOCUS_CONTENT` → điều hướng trong content (keyboard hoặc lists)

- **`handleExit()`:**
  - Luôn set `focusMode = FOCUS_SIDEBAR` và unhighlight content (cho tất cả tabs)

- **`handleUp()`/`handleDown()`:**
  - Khi `focusMode == FOCUS_CONTENT` → luôn gọi `handleContentNavigation()` (không có exception cho `TAB_ADD_FRIEND`)

### Files thay đổi
- `src/social_screen.cpp` - Update `handleLeft()`, `handleRight()`, `handleExit()`, `handleUp()`, `handleDown()`

---

## 4. Fix Mini Add Friend Screen

### Vấn đề 1: Duplicate Characters
**Nguyên nhân:** Ký tự được xử lý 2 lần:
1. `MiniKeyboard::moveCursor("select")` gọi callback `onKeySelected` → route đến `MiniAddFriendScreen::handleKeyPress()` → thêm ký tự
2. `MiniAddFriendScreen::handleSelect()` cũng xử lý ký tự → thêm lần nữa

**Giải pháp:**
- Modify `MiniAddFriendScreen::handleSelect()` để chỉ xử lý special keys (`|e` cho submit, `<` cho backspace)
- Dựa vào callback `onKeySelected` để thêm regular characters

### Vấn đề 2: Ký tự "|" xuất hiện trong input box
**Nguyên nhân:** Ký tự "|" ban đầu là blinking cursor, nhưng bị persist không mong muốn.

**Giải pháp (multi-layer defense):**
1. Xóa logic vẽ blinking cursor "|" trong `MiniAddFriendScreen::draw()`
2. Thêm check trong `MiniAddFriendScreen::handleKeyPress()`: `if (key.indexOf("|") >= 0) return;`
3. Thêm `enteredName.replace("|", "")` sau khi add character và trong `reset()`
4. Thêm `displayText.replace("|", "")` trong `draw()` trước khi render

### Vấn đề 3: Ký tự không hiển thị ngay khi select
**Nguyên nhân:** `drawContentArea()` chỉ được gọi conditionally, bỏ sót "select" events.

**Giải pháp:**
- Trong `SocialScreen::handleContentNavigation()`, cho `TAB_ADD_FRIEND`, luôn gọi `drawContentArea()` sau `miniAddFriend->handleKeyPress(key)`

### Files thay đổi
- `src/mini_add_friend_screen.cpp` - Fix `handleSelect()`, `handleKeyPress()`, `draw()`, `reset()`
- `src/social_screen.cpp` - Fix `handleContentNavigation()` để luôn redraw cho Add Friend tab

---

## 5. Update Content Navigation

### Friends Tab
- **UP/DOWN:** Điều hướng lên xuống các friend items
- **SELECT:** Mở chat với friend được chọn (gọi `onOpenChatCallback`)

### Games Tab
- **UP/DOWN:** Điều hướng lên xuống game invites và game list
- **SELECT:** 
  - Nếu đang ở game invite → accept invite (gọi `ApiClient::respondGameInvite`)
  - Nếu đang ở game list → mở game lobby (gọi `ApiClient::createGameSession`)

### Files thay đổi
- `src/social_screen.cpp` - Update `handleContentNavigation()` cho `TAB_FRIENDS` và `TAB_GAMES`

---

## 6. Fix JSON Quotes Issue

### Vấn đề
Tên người bạn đầu tiên trong danh sách có dấu ngoặc kép `"` ở đầu (ví dụ: `"User123` thay vì `User123`).

### Nguyên nhân
FastAPI có thể tự động serialize string response thành JSON string, thêm dấu ngoặc kép ở đầu và cuối response (ví dụ: `"User123,5,0|Admin,3,0|"` thay vì `User123,5,0|Admin,3,0|`).

### Giải pháp
Thêm logic trong `ApiClient::getFriendsList()` để remove leading và trailing quotes:
```cpp
if (result.length() >= 2 && result.charAt(0) == '"' && result.charAt(result.length() - 1) == '"') {
    result = result.substring(1, result.length() - 1);
}
```

### Files thay đổi
- `src/api_client.cpp` - Thêm quote removal logic trong `getFriendsList()`

---

## Tổng Kết Files Đã Thay Đổi

1. **`src/social_screen.h`** - Thêm `syncNavigation()` declaration
2. **`src/social_screen.cpp`** - Multiple changes:
   - Implement `syncNavigation()`
   - Fix tab navigation và circular navigation
   - Implement left/right navigation logic
   - Update content navigation cho Friends và Games
   - Fix redraw logic cho Add Friend tab
3. **`src/main.cpp`** - Fix routing navigation keys và gọi `syncNavigation()` trong `onLoginSuccess()`
4. **`src/mini_add_friend_screen.cpp`** - Fix duplicate characters, remove "|" character, fix display update
5. **`src/api_client.cpp`** - Fix JSON quotes issue

---

## Kết Quả

- ✅ Navigation state được đồng bộ khi chuyển tab
- ✅ Tab navigation hoạt động đúng với circular navigation
- ✅ Left/Right navigation hoạt động đúng cho keyboard và sidebar
- ✅ Mini Add Friend Screen không còn duplicate characters và ký tự "|"
- ✅ Content navigation hoạt động đúng cho Friends và Games
- ✅ Tên người bạn đầu tiên không còn dấu ngoặc kép

---

## TODO Next

- [ ] Check điều hướng lên xuống cho item của thông báo
- [ ] Có thể chat được với thiết bị
- [ ] Có thể nhận thông báo kết bạn được
- [ ] Có thể nhận thông báo mời chơi game

