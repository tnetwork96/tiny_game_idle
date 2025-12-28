#include "mini_keyboard.h"

// Deep Space Arcade Theme (matching SocialScreen)
#define SOCIAL_BG_DARK   0x0042  // Deep Midnight Blue #020817
#define SOCIAL_HEADER    0x08A5  // Header Blue #0F172A
#define SOCIAL_ACCENT    0x07FF  // Cyan accent
#define SOCIAL_TEXT      0xFFFF  // White text
#define SOCIAL_MUTED     0x8410  // Muted gray text

// QWERTY layout (alphabet only, lowercase)
const String MiniKeyboard::qwertyKeys[3][10] = {
    { "q", "w", "e", "r", "t", "y", "u", "i", "o", "p" },
    { "a", "s", "d", "f", "g", "h", "j", "k", "l", "|e" },  // Enter at end of row 2
    { "z", "x", "c", "v", "b", "n", "m", "<", " ", " " }   // Backspace, Space, empty
};

MiniKeyboard::MiniKeyboard(Adafruit_ST7789* tft) {
    this->tft = tft;
    this->cursorRow = 0;
    this->cursorCol = 0;
    this->visible = true;
}

void MiniKeyboard::draw() {
    if (!visible) return;

    // Calculate keyboard dimensions
    uint16_t keyboardWidth = 10 * (KEY_WIDTH + SPACING) - SPACING;
    uint16_t keyboardHeight = 3 * (KEY_HEIGHT + SPACING) - SPACING;
    
    // Position at bottom of content area (240px screen height)
    // Content area starts at CONTENT_X (45), so we position relative to that
    uint16_t screenHeight = 240;
    uint16_t yStart = screenHeight - keyboardHeight - 5;  // 5px margin from bottom
    uint16_t xStart = 45;  // Start at content area (CONTENT_X)

    // Draw all keys
    for (uint16_t row = 0; row < 3; row++) {
        for (uint16_t col = 0; col < 10; col++) {
            uint16_t xPos = xStart + col * (KEY_WIDTH + SPACING);
            uint16_t yPos = yStart + row * (KEY_HEIGHT + SPACING);

            // Skip empty keys
            if (qwertyKeys[row][col].length() == 0 || qwertyKeys[row][col] == " ") {
                continue;
            }

            bool isSelected = (row == cursorRow && col == cursorCol);
            drawKey(xPos, yPos, qwertyKeys[row][col], isSelected);
        }
    }
}

void MiniKeyboard::drawKey(uint16_t x, uint16_t y, const String& key, bool isSelected) {
    // Background color
    uint16_t bgColor = isSelected ? SOCIAL_ACCENT : SOCIAL_HEADER;
    
    // Draw key background
    tft->fillRoundRect(x, y, KEY_WIDTH, KEY_HEIGHT, 2, bgColor);
    
    // Draw border for selected key
    if (isSelected) {
        tft->drawRoundRect(x, y, KEY_WIDTH, KEY_HEIGHT, 2, SOCIAL_TEXT);
    }

    // Draw key text
    tft->setTextSize(1);
    tft->setTextColor(SOCIAL_TEXT, bgColor);
    
    String displayText = key;
    
    // Special handling for special keys
    if (key == "|e") {
        displayText = "Ent";
    } else if (key == "<") {
        displayText = "Del";
    } else if (key == " ") {
        displayText = "Sp";
    }
    
    // Truncate if too long
    if (displayText.length() > 3) {
        displayText = displayText.substring(0, 3);
    }
    
    // Center text in key
    uint16_t textWidth = displayText.length() * 6;  // Approximate width
    uint16_t textX = x + (KEY_WIDTH - textWidth) / 2;
    uint16_t textY = y + (KEY_HEIGHT - 8) / 2;  // Center vertically
    
    tft->setCursor(textX, textY);
    tft->print(displayText);
}

void MiniKeyboard::moveCursor(int8_t deltaRow, int8_t deltaCol) {
    int16_t newRow = (int16_t)cursorRow + deltaRow;
    int16_t newCol = (int16_t)cursorCol + deltaCol;
    
    // Clamp to valid range
    if (newRow < 0) newRow = 0;
    if (newRow >= 3) newRow = 2;
    if (newCol < 0) newCol = 0;
    if (newCol >= 10) newCol = 9;
    
    // Skip empty keys
    if (qwertyKeys[newRow][newCol].length() == 0 || qwertyKeys[newRow][newCol] == " ") {
        // Try to find next valid key
        if (deltaCol > 0) {
            // Moving right, find next non-empty key
            for (int8_t c = newCol + 1; c < 10; c++) {
                if (qwertyKeys[newRow][c].length() > 0 && qwertyKeys[newRow][c] != " ") {
                    newCol = c;
                    break;
                }
            }
        } else if (deltaCol < 0) {
            // Moving left, find previous non-empty key
            for (int8_t c = newCol - 1; c >= 0; c--) {
                if (qwertyKeys[newRow][c].length() > 0 && qwertyKeys[newRow][c] != " ") {
                    newCol = c;
                    break;
                }
            }
        }
    }
    
    cursorRow = (uint16_t)newRow;
    cursorCol = (int8_t)newCol;
}

void MiniKeyboard::moveCursorByCommand(String command) {
    if (command == "up") {
        moveCursor(-1, 0);
    } else if (command == "down") {
        moveCursor(1, 0);
    } else if (command == "left") {
        moveCursor(0, -1);
    } else if (command == "right") {
        moveCursor(0, 1);
    } else if (command == "select") {
        // Selection is handled by getCurrentKey() - caller should handle the key
        // No action needed here
    }
}

String MiniKeyboard::getCurrentKey() const {
    if (cursorRow < 3 && cursorCol < 10) {
        return qwertyKeys[cursorRow][cursorCol];
    }
    return "";
}

void MiniKeyboard::resetCursor() {
    cursorRow = 0;
    cursorCol = 0;
}

void MiniKeyboard::moveCursorTo(uint16_t row, int8_t col) {
    if (row < 3 && col >= 0 && col < 10) {
        // Check if target key is valid (not empty)
        if (qwertyKeys[row][col].length() > 0 && qwertyKeys[row][col] != " ") {
            cursorRow = row;
            cursorCol = col;
        }
    }
}

