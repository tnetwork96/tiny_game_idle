# Navigation System Refactor Template

## Overview
Template này mô tả cách refactor một screen để sử dụng hệ thống điều hướng mới với các lệnh rõ ràng: `up`, `down`, `left`, `right`, `select`, `exit` thay vì format cũ: `|u`, `|d`, `|l`, `|r`, `|e`, `<`.

## Pattern đã áp dụng cho WiFi Manager

### Bước 1: Thêm Navigation Methods vào Header File

**File**: `your_screen.h`

```cpp
class YourScreen {
private:
    // ... existing members ...
    
public:
    // ... existing methods ...
    
    // Navigation handlers
    void handleUp();
    void handleDown();
    void handleLeft();
    void handleRight();
    void handleSelect();
    void handleExit();
    
    // Handle keyboard input
    void handleKeyboardInput(String key);
};
```

### Bước 2: Implement Navigation Methods trong .cpp File

**File**: `your_screen.cpp`

```cpp
// Implement navigation methods
void YourScreen::handleUp() {
    // Logic để điều hướng lên
    // Ví dụ: selectPrevious(), moveCursorUp(), etc.
    if (currentState == STATE_LIST) {
        listScreen->selectPrevious();
    }
}

void YourScreen::handleDown() {
    // Logic để điều hướng xuống
    if (currentState == STATE_LIST) {
        listScreen->selectNext();
    }
}

void YourScreen::handleLeft() {
    // Logic để điều hướng trái (nếu cần)
    // Có thể để trống nếu không cần
}

void YourScreen::handleRight() {
    // Logic để điều hướng phải (nếu cần)
    // Có thể để trống nếu không cần
}

void YourScreen::handleSelect() {
    // Logic để chọn/confirm
    if (currentState == STATE_LIST) {
        onItemSelected();
    }
}

void YourScreen::handleExit() {
    // Logic để exit/back
    // Tùy thuộc vào state hiện tại
    switch (currentState) {
        case STATE_LIST:
            // Exit từ list - có thể reset hoặc không làm gì
            break;
        case STATE_DETAIL:
            // Quay lại list
            currentState = STATE_LIST;
            draw();
            break;
        case STATE_INPUT:
            // Quay lại màn hình trước
            currentState = STATE_LIST;
            draw();
            break;
    }
}
```

### Bước 3: Update handleKeyboardInput() để Map Keys Mới

**File**: `your_screen.cpp`

```cpp
void YourScreen::handleKeyboardInput(String key) {
    // Handle new navigation key format
    if (key == "up") {
        handleUp();
        return;
    } else if (key == "down") {
        handleDown();
        return;
    } else if (key == "left") {
        handleLeft();
        return;
    } else if (key == "right") {
        handleRight();
        return;
    } else if (key == "select") {
        handleSelect();
        return;
    } else if (key == "exit") {
        handleExit();
        return;
    }
    
    // Backward compatibility: handle old key format
    if (key == "|u") {
        handleUp();
        return;
    } else if (key == "|d") {
        handleDown();
        return;
    } else if (key == "|l") {
        handleLeft();
        return;
    } else if (key == "|r") {
        handleRight();
        return;
    } else if (key == "|e") {
        handleSelect();
        return;
    } else if (key == "<" || key == "|b") {
        handleExit();
        return;
    }
    
    // Handle other input (text input, special keys, etc.)
    // Ví dụ: password input, text input, etc.
    if (currentState == STATE_INPUT) {
        inputField->handleKeyPress(key);
    }
}
```

### Bước 4: Update Routing trong main.cpp

**File**: `main.cpp`

Thêm vào hàm `onKeyboardKeySelected()`:

```cpp
void onKeyboardKeySelected(String key) {
    // 1. ChatScreen (child of SocialScreen)
    // ... existing code ...
    
    // 2. SocialScreen and its children
    // ... existing code ...
    
    // 3. YourScreen - Check BEFORE other screens if needed
    // Đặt ở vị trí phù hợp dựa trên priority
    if (yourScreen != nullptr) {
        YourScreenState screenState = yourScreen->getState();
        // Check if screen is active and should handle input
        if (screenState != STATE_INACTIVE) {
            yourScreen->handleKeyboardInput(key);
            return;
        }
    }
    
    // 4. Other screens...
    // ... existing code ...
}
```

**Lưu ý về thứ tự routing:**
- Screen nào cần ưu tiên xử lý input thì đặt trước
- Ví dụ: WiFi Manager được check trước LoginScreen vì WiFi selection xảy ra trước login
- Screen nào active thì được check trước screen inactive

### Bước 5: Serial Navigation Support (Đã có sẵn)

**File**: `main.cpp`

Hàm `handleSerialNavigation()` đã được implement và tự động xử lý:
- `up` hoặc `u` → gửi `"up"`
- `down` hoặc `d` → gửi `"down"`
- `left` hoặc `l` → gửi `"left"`
- `right` hoặc `r` → gửi `"right"`
- `select`, `s`, `enter`, hoặc `e` → gửi `"select"`
- `exit`, `x`, `back`, hoặc `b` → gửi `"exit"`

Không cần thêm code, chỉ cần đảm bảo screen được route đúng trong `onKeyboardKeySelected()`.

## Checklist khi Refactor một Screen Mới

- [ ] Thêm 6 navigation methods vào header: `handleUp()`, `handleDown()`, `handleLeft()`, `handleRight()`, `handleSelect()`, `handleExit()`
- [ ] Implement các navigation methods trong .cpp file
- [ ] Update `handleKeyboardInput()` để map keys mới (`up`, `down`, `left`, `right`, `select`, `exit`)
- [ ] Giữ backward compatibility với format cũ (`|u`, `|d`, `|l`, `|r`, `|e`, `<`)
- [ ] Thêm routing logic vào `onKeyboardKeySelected()` trong main.cpp
- [ ] Đặt routing ở vị trí phù hợp (priority)
- [ ] Test với Serial navigation (gửi `up`, `down`, etc. qua Serial Monitor)
- [ ] Test với keyboard navigation (nếu có)
- [ ] Test backward compatibility với format cũ

## Ví dụ: WiFi Manager Implementation

### Header (wifi_manager.h)
```cpp
class WiFiManager {
public:
    // Navigation handlers
    void handleUp();
    void handleDown();
    void handleLeft();
    void handleRight();
    void handleSelect();
    void handleExit();
    
    void handleKeyboardInput(String key);
    WiFiState getState() const;
};
```

### Implementation (wifi_manager.cpp)
```cpp
void WiFiManager::handleUp() {
    if (currentState == WIFI_STATE_SELECT || currentState == WIFI_STATE_SCAN) {
        wifiList->selectPrevious();
    }
}

void WiFiManager::handleDown() {
    if (currentState == WIFI_STATE_SELECT || currentState == WIFI_STATE_SCAN) {
        wifiList->selectNext();
    }
}

void WiFiManager::handleLeft() {
    // Reserved for future use
}

void WiFiManager::handleRight() {
    // Reserved for future use
}

void WiFiManager::handleSelect() {
    if (currentState == WIFI_STATE_SCAN) {
        currentState = WIFI_STATE_SELECT;
    } else if (currentState == WIFI_STATE_SELECT) {
        onWiFiSelected();
    }
}

void WiFiManager::handleExit() {
    switch (currentState) {
        case WIFI_STATE_PASSWORD:
            currentState = WIFI_STATE_SELECT;
            wifiList->draw();
            break;
        case WIFI_STATE_CONNECTING:
            WiFi.disconnect();
            currentState = WIFI_STATE_PASSWORD;
            wifiPassword->draw();
            break;
        case WIFI_STATE_ERROR:
            currentState = WIFI_STATE_SELECT;
            wifiList->draw();
            break;
        // Other states...
    }
}

void WiFiManager::handleKeyboardInput(String key) {
    // Map new format
    if (key == "up") {
        handleUp();
        return;
    } else if (key == "down") {
        handleDown();
        return;
    }
    // ... other mappings ...
    
    // Backward compatibility
    if (key == "|u") {
        handleUp();
        return;
    }
    // ... other old format mappings ...
    
    // Handle other input
    if (currentState == WIFI_STATE_PASSWORD) {
        wifiPassword->handleKeyPress(key);
    }
}
```

### Routing trong main.cpp
```cpp
void onKeyboardKeySelected(String key) {
    // ... other screens ...
    
    // 3. WiFi Manager - Check BEFORE LoginScreen
    if (wifiManager != nullptr) {
        WiFiState wifiState = wifiManager->getState();
        if (wifiState != WIFI_STATE_CONNECTED) {
            wifiManager->handleKeyboardInput(key);
            return;
        }
    }
    
    // 4. LoginScreen - Only when WiFi is connected
    // ... existing code ...
}
```

## Lợi ích

1. **Rõ ràng hơn**: `"up"`, `"down"` dễ hiểu hơn `"|u"`, `"|d"`
2. **Dễ maintain**: Code dễ đọc và maintain hơn
3. **Serial support**: Tự động hỗ trợ điều hướng qua Serial
4. **Backward compatible**: Vẫn hỗ trợ format cũ
5. **Consistent**: Tất cả screens sử dụng cùng pattern

## Notes

- Format mới: `up`, `down`, `left`, `right`, `select`, `exit`
- Format cũ (backward compatibility): `|u`, `|d`, `|l`, `|r`, `|e`, `<`, `|b`
- Serial commands: `up/u`, `down/d`, `left/l`, `right/r`, `select/s/enter/e`, `exit/x/back/b`
- Tất cả commands không phân biệt hoa/thường

