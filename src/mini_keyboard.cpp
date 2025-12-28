#include "mini_keyboard.h"

// Local Theme Macros (Pink/Feminine Style) - Private to this class
#define MKB_BG_COLOR      0xE73F  // Light purple background
#define MKB_KEY_BG        0xF81F  // Pink/Magenta (màu hồng)
#define MKB_BORDER_COLOR  0xF81F  // Pink border
#define MKB_TEXT_COLOR    0xFFFF  // White (chữ trắng nổi bật)
#define MKB_HIGHLIGHT     0xFE1F  // Light pink (hồng nhạt) for selection
#define MKB_SELECTED_TEXT 0xF81F  // Pink text on selected key

// QWERTY Layout (3 rows x 10 cols) - GIỐNG HỆT Keyboard gốc
// Row 0: q w e r t y u i o p
// Row 1: 123 a s d f g h j k l
// Row 2: shift z x c v b n m |e <
// Row 3: Spacebar (vẽ riêng, không có trong layout array)
const String MiniKeyboard::qwertyLayout[3][10] = {
    { "q", "w", "e", "r", "t", "y", "u", "i", "o", "p" },
    { "123", "a", "s", "d", "f", "g", "h", "j", "k", "l" },
    { "shift", "z", "x", "c", "v", "b", "n", "m", "|e", "<" }
};

// Numeric Layout (3 rows x 10 cols) - GIỐNG HỆT Keyboard
// Row 0: 1 2 3 4 5 6 7 8 9 0
// Row 1: ABC / : ; ( ) $ & @ "
// Row 2: (empty) # . , ? ! ' - |e <
const String MiniKeyboard::numericLayout[3][10] = {
    { "1", "2", "3", "4", "5", "6", "7", "8", "9", "0" },
    { "ABC", "/", ":", ";", "(", ")", "$", "&", "@", "\"" },
    { "", "#", ".", ",", "?", "!", "'", "-", "|e", "<" }
};

MiniKeyboard::MiniKeyboard(Adafruit_ST7789* tft) {
    this->tft = tft;
    this->cursorRow = 0;
    this->cursorCol = 0;
    this->capsLock = false;
    this->isNumericMode = false;
    this->lastDrawX = 0;
    this->lastDrawY = 0;
    this->onKeySelected = nullptr;  // Khởi tạo callback = null (giống Keyboard gốc)
}

void MiniKeyboard::draw(uint16_t x, uint16_t y) {
    // Store position for refresh after mode toggle
    lastDrawX = x;
    lastDrawY = y;
    
    // Center calculation: Hardcoded for SocialScreen fit
    // Content area: 294px wide starting at x=26
    // Keyboard width: 10*24 + 9*3 = 267px
    // Center: 26 + (294 - 267) / 2 = 26 + 13 = 39
    uint16_t xStart = x;
    if (x == 0) {
        // If x not provided, use hardcoded center calculation
        xStart = 26 + (294 - (10 * KEY_WIDTH + 9 * SPACING)) / 2;
    }
    
    uint16_t totalW = 10 * KEY_WIDTH + 9 * SPACING;  // 267px
    
    if (isNumericMode) {
        // Draw numeric keys (3 rows, no spacebar)
        for (uint16_t row = 0; row < 3; row++) {
            for (uint16_t col = 0; col < COLS; col++) {
                String key = numericLayout[row][col];
                if (key.length() == 0) continue;
                
                uint16_t xPos = xStart + col * (KEY_WIDTH + SPACING);
                uint16_t yPos = y + row * (KEY_HEIGHT + SPACING);
                bool isSelected = (row == cursorRow && col == cursorCol);
                drawKey(xPos, yPos, key, isSelected);
            }
        }
    } else {
        // Draw QWERTY keys (3 rows first)
        for (uint16_t row = 0; row < 3; row++) {
            for (uint16_t col = 0; col < COLS; col++) {
                String key = qwertyLayout[row][col];
                if (key.length() == 0) continue;
                
                uint16_t xPos = xStart + col * (KEY_WIDTH + SPACING);
                uint16_t yPos = y + row * (KEY_HEIGHT + SPACING);
                bool isSelected = (row == cursorRow && col == cursorCol);
                drawKey(xPos, yPos, key, isSelected);
            }
        }
        
        // Vẽ phím Space ở hàng dưới cùng và căn giữa - COPY TỪ Keyboard
        uint16_t spaceKeyWidth = 8 * (KEY_WIDTH + SPACING) - SPACING; // Phím Space chiếm 8 ô
        uint16_t spaceXStart = xStart + (totalW - spaceKeyWidth) / 2; // Căn giữa trong keyboard width
        uint16_t spaceYStart = y + 3 * (KEY_HEIGHT + SPACING); // Đặt phím Space ở hàng 3
        
        // BƯỚC 1: Vẽ nền phím Space trước
        uint16_t spaceBgColor;
        if (cursorRow == 3) {
            spaceBgColor = MKB_HIGHLIGHT; // Tô sáng phím Space khi chọn
        } else {
            spaceBgColor = MKB_KEY_BG;  // Màu nền phím bình thường
        }
        
        // Vẽ phím Space với bo góc (radius 3)
        tft->fillRoundRect(spaceXStart, spaceYStart, spaceKeyWidth, KEY_HEIGHT, 3, spaceBgColor);
        tft->drawRoundRect(spaceXStart, spaceYStart, spaceKeyWidth, KEY_HEIGHT, 3, MKB_BORDER_COLOR);
        
        // BƯỚC 2: Không in text cho spacebar (giống Keyboard - đã comment)
    }
}

void MiniKeyboard::drawKey(uint16_t x, uint16_t y, const String& key, bool isSelected) {
    uint16_t keyWidth = KEY_WIDTH;
    
    // Use local theme colors
    uint16_t bgColor = isSelected ? MKB_HIGHLIGHT : MKB_KEY_BG;
    uint16_t borderColor = MKB_BORDER_COLOR;
    uint16_t textColor = isSelected ? MKB_SELECTED_TEXT : MKB_TEXT_COLOR;
    
    // Draw key background with rounded corners (radius 3)
    tft->fillRoundRect(x, y, keyWidth, KEY_HEIGHT, 3, bgColor);
    
    // Draw border
    tft->drawRoundRect(x, y, keyWidth, KEY_HEIGHT, 3, borderColor);
    
    // Pink theme doesn't use grid pattern (cleaner look)
    
    // Draw key text
    String displayText = getKeyDisplayText(key);
    
    // Skip drawing if text is empty
    if (displayText.length() == 0) {
        return;
    }
    
    // Truncate text if too long (max 2 chars)
    if (displayText.length() > 2) {
        displayText = displayText.substring(0, 2);
    }
    
    tft->setTextSize(1);
    tft->setTextColor(textColor, bgColor);
    
    // Calculate text position (center text in key)
    uint16_t textWidth = displayText.length() * 6;  // Approximate width (6px per char)
    uint16_t marginX = 2;
    uint16_t textX = x + marginX;
    if (textWidth < keyWidth - (marginX * 2)) {
        textX = x + (keyWidth - textWidth) / 2;  // Center if enough space
    }
    uint16_t textY = y + (KEY_HEIGHT - 8) / 2;  // Center vertically
    
    tft->setCursor(textX, textY);
    tft->print(displayText);
}

String MiniKeyboard::getKeyDisplayText(const String& key) const {
    // Handle shift key - show "Aa" or "AA" based on capsLock
    if (key == "shift") {
        return capsLock ? "AA" : "Aa";
    }
    // Handle Enter key
    if (key == "|e") {
        return "Ent";
    }
    // Handle Backspace
    if (key == "<") {
        return "<";
    }
    // Handle "123" key (switch to numeric mode)
    if (key == "123") {
        return "123";
    }
    // Handle "ABC" key (switch back to QWERTY mode)
    if (key == "ABC") {
        return "ABC";
    }
    // Handle letters - apply caps lock
    if (key.length() == 1 && key >= "a" && key <= "z") {
        if (capsLock) {
            String upperKey = key;
            upperKey.toUpperCase();
            return upperKey;
        }
        return key;
    }
    // Return key as-is for other cases
    return key;
}

void MiniKeyboard::moveCursorInternal(int8_t deltaRow, int8_t deltaCol) {
    int16_t newRow = (int16_t)cursorRow + deltaRow;
    int16_t newCol = (int16_t)cursorCol + deltaCol;
    
    uint16_t maxRow = isNumericMode ? 2 : 3;  // Numeric has 3 rows (0-2), QWERTY has 4 rows (0-3)
    
    // Handle row wrapping - COPY TỪ Keyboard
    if (newRow < 0) {
        if (isNumericMode) {
            newRow = maxRow;  // Wrap to bottom row
        } else {
            newRow = 3;  // Nếu nhấn "up" từ hàng đầu tiên, quay lại hàng phím Space
        }
    } else if (newRow > maxRow) {
        newRow = 0;  // Quay lại hàng đầu tiên
    }
    
    // Handle column wrapping
    if (newCol < 0) {
        newCol = 9;  // Wrap to right
    } else if (newCol > 9) {
        newCol = 0;  // Wrap to left
    }
    
    // Nếu con trỏ đang ở hàng Space (hàng 3), cố định nó ở giữa phím Space - COPY TỪ Keyboard
    if (!isNumericMode && newRow == 3) {
        newCol = 4;  // Phím Space nằm ở giữa (cột 4)
    }
    
    // Get active layout
    const String (*activeLayout)[10] = isNumericMode ? numericLayout : qwertyLayout;
    
    // Skip empty keys (chỉ cho các hàng 0-2, không check hàng 3 vì đó là spacebar)
    if (newRow < 3) {
        if (activeLayout[newRow][newCol].length() == 0) {
            // Find nearest valid key
            if (deltaCol > 0) {
                // Moving right, find next valid key
                for (int8_t c = newCol + 1; c < COLS; c++) {
                    if (activeLayout[newRow][c].length() > 0) {
                        newCol = c;
                        break;
                    }
                }
            } else if (deltaCol < 0) {
                // Moving left, find previous valid key
                for (int8_t c = newCol - 1; c >= 0; c--) {
                    if (activeLayout[newRow][c].length() > 0) {
                        newCol = c;
                        break;
                    }
                }
            }
        }
    }
    
    cursorRow = (uint16_t)newRow;
    cursorCol = (int8_t)newCol;
}

void MiniKeyboard::moveCursor(String direction) {
    if (direction == "up") {
        moveCursorInternal(-1, 0);
    } else if (direction == "down") {
        moveCursorInternal(1, 0);
    } else if (direction == "left") {
        moveCursorInternal(0, -1);
    } else if (direction == "right") {
        moveCursorInternal(0, 1);
    } else if (direction == "select") {
        String currentKey = getCurrentChar();
        
        // Handle mode toggle keys
        if (currentKey == "123") {
            toggleMode();
            return;
        } else if (currentKey == "ABC") {
            toggleMode();
            return;
        }
        
        // Toggle caps lock if Shift key was selected (giống Keyboard gốc - check currentKey trước)
        if (!isNumericMode && currentKey == "shift") {
            capsLock = !capsLock;
            return;  // Don't call callback for shift
        }
        
        // Gọi callback nếu có để xử lý phím được chọn (giống Keyboard gốc)
        if (onKeySelected != nullptr) {
            onKeySelected(currentKey);
        }
    }
}

String MiniKeyboard::getCurrentChar() {
    // Nếu đang ở hàng Space (hàng 3) và không phải numeric mode - COPY TỪ Keyboard
    if (!isNumericMode && cursorRow == 3) {
        return " ";  // Trả về phím Space
    }
    
    String key = "";
    
    // Use active layout based on mode
    if (isNumericMode) {
        if (cursorRow < 3 && cursorCol < COLS) {
            key = numericLayout[cursorRow][cursorCol];
        }
    } else {
        if (cursorRow < 3 && cursorCol < COLS) {
            key = qwertyLayout[cursorRow][cursorCol];
        }
    }
    
    if (key.length() == 0) {
        return "";
    }
    
    // Handle special keys
    if (key == "|e") {
        return "|e";  // Enter key
    } else if (key == "<") {
        return "<";  // Backspace
    } else if (key == "shift") {
        return "shift";  // Shift key (toggles caps lock)
    } else if (key == "123") {
        return "123";  // Number mode toggle key
    } else if (key == "ABC") {
        return "ABC";  // QWERTY mode toggle key
    } else if (key == " ") {
        return " ";  // Space
    } else if (key.length() == 1 && key >= "a" && key <= "z") {
        // Apply caps lock to letters
        if (capsLock) {
            String upperKey = key;
            upperKey.toUpperCase();
            return upperKey;
        }
        return key;
    }
    
    return key;
}

void MiniKeyboard::reset() {
    cursorRow = 0;
    cursorCol = 0;
    capsLock = false;
    isNumericMode = false;
}

void MiniKeyboard::toggleMode() {
    isNumericMode = !isNumericMode;
    // Reset cursor to safe position when switching modes
    cursorRow = 0;
    cursorCol = 0;
    // Refresh the display immediately using last draw position
    draw(lastDrawX, lastDrawY);
}

void MiniKeyboard::moveCursorTo(uint16_t row, int8_t col) {
    // Giới hạn vị trí (giống Keyboard gốc)
    if (row > 3) row = 3;
    if (col < 0) col = 0;
    if (col > 9) col = 9;
    
    cursorRow = row;
    cursorCol = col;
    
    // Refresh display
    draw(lastDrawX, lastDrawY);
}

void MiniKeyboard::typeString(const String& text) {
    Serial.print("MiniKeyboard::typeString: Typing string '");
    Serial.print(text);
    Serial.print("' (length=");
    Serial.print(text.length());
    Serial.println(")");
    
    for (uint16_t i = 0; i < text.length(); i++) {
        char c = text.charAt(i);
        Serial.print("MiniKeyboard::typeString: [");
        Serial.print(i);
        Serial.print("/");
        Serial.print(text.length());
        Serial.print("] Typing '");
        Serial.print(c);
        Serial.println("'");
        
        // Phân loại ký tự
        bool isDigit = (c >= '0' && c <= '9');
        bool isLowercaseLetter = (c >= 'a' && c <= 'z');
        bool isUppercaseLetter = (c >= 'A' && c <= 'Z');
        bool isLetter = isLowercaseLetter || isUppercaseLetter;
        char normalizedChar = c;
        if (isUppercaseLetter) {
            normalizedChar = c - 'A' + 'a';  // Normalize to lowercase
        }
        
        // Nếu là số hoặc ký tự đặc biệt, cần chuyển sang chế độ số
        if ((isDigit || (!isLetter && c != ' ')) && !isNumericMode) {
            // Dùng moveCursorTo để di chuyển trực tiếp (giống Keyboard gốc)
            moveCursorTo(1, 0);  // Move directly to "123"
            delay(100);
            moveCursor("select");  // Select "123"
            delay(100);
        }
        // Nếu là chữ cái, cần chuyển về chế độ chữ cái
        else if (isLetter && isNumericMode) {
            // Dùng moveCursorTo để di chuyển trực tiếp (giống Keyboard gốc)
            moveCursorTo(1, 0);  // Move directly to "ABC"
            delay(100);
            moveCursor("select");  // Select "ABC"
            delay(100);
        }
        
        // Đặt chế độ chữ hoa/thường theo ký tự cần gõ
        if (isLetter && isUppercaseLetter && !capsLock) {
            // Dùng moveCursorTo để di chuyển trực tiếp (giống Keyboard gốc)
            moveCursorTo(2, 0);  // Move directly to "shift"
            delay(100);
            moveCursor("select");  // Toggle caps lock
            delay(100);
        } else if (isLetter && isLowercaseLetter && capsLock) {
            // Dùng moveCursorTo để di chuyển trực tiếp (giống Keyboard gốc)
            moveCursorTo(2, 0);  // Move directly to "shift"
            delay(100);
            moveCursor("select");  // Toggle caps lock
            delay(100);
        }
        
        // Tìm ký tự trong bảng phím hiện tại
        const String (*currentKeys)[10] = isNumericMode ? numericLayout : qwertyLayout;
        bool found = false;
        
        for (uint16_t row = 0; row < 3; row++) {
            for (uint16_t col = 0; col < 10; col++) {
                String key = currentKeys[row][col];
                // So sánh ký tự (bỏ qua các phím đặc biệt)
                if (key.length() == 1 && key.charAt(0) == normalizedChar) {
                    Serial.print("MiniKeyboard::typeString: Found '");
                    Serial.print(c);
                    Serial.print("' at row=");
                    Serial.print(row);
                    Serial.print(", col=");
                    Serial.print(col);
                    Serial.println(" in current layout");
                    
                    // Di chuyển trực tiếp đến ký tự (giống Keyboard gốc)
                    // Dùng moveCursorTo để tránh đi qua các phím khác và gọi callback nhầm
                    moveCursorTo(row, col);
                    delay(150);
                    // Select the character
                    moveCursor("select");
                    delay(150);
                    found = true;
                    break;
                }
            }
            if (found) break;
        }
        
        if (!found) {
            Serial.print("MiniKeyboard::typeString: WARNING - Character '");
            Serial.print(c);
            Serial.println("' not found in current layout!");
        }
    }
    
    Serial.println("MiniKeyboard::typeString: Finished typing string");
}

void MiniKeyboard::pressEnter() {
    Serial.println("MiniKeyboard::pressEnter: Navigating to Enter key and pressing it...");
    
    // Enter luôn ở hàng 2, cột 8 trong tất cả layout (chữ cái, số)
    Serial.println("MiniKeyboard::pressEnter: Moving to Enter key at row=2, col=8...");
    moveCursorTo(2, 8);
    delay(150);
    
    Serial.println("MiniKeyboard::pressEnter: Pressing Enter key...");
    moveCursor("select");
    delay(150);
    
    Serial.println("MiniKeyboard::pressEnter: Enter key pressed successfully");
}
