#include "mini_keyboard.h"

// Cyberpunk theme colors (matching main Keyboard)
#define BG_COLOR        0x8010  // Dark Purple background
#define KEY_BG          0x0000  // Black key background
#define KEY_BORDER      0x07FF  // Neon Cyan border
#define KEY_TEXT        0x07E0  // Neon Green text
#define HIGHLIGHT_COLOR 0xFE20  // Yellow/Orange for selected key
#define BLACK           0x0000  // Black for text on highlighted key

// QWERTY layout (4 rows: 3 QWERTY + 1 Spacebar)
const String MiniKeyboard::qwertyKeys[4][10] = {
    { "q", "w", "e", "r", "t", "y", "u", "i", "o", "p" },
    { "a", "s", "d", "f", "g", "h", "j", "k", "l", "|e" },  // Enter at end
    { "shift", "z", "x", "c", "v", "b", "n", "m", "<", "" }, // Shift, Backspace, empty
    { " ", " ", " ", " ", " ", " ", " ", " ", " ", "" }      // Spacebar row
};

MiniKeyboard::MiniKeyboard(Adafruit_ST7789* tft) {
    this->tft = tft;
    this->cursorRow = 0;
    this->cursorCol = 0;
    this->capsLock = false;
}

void MiniKeyboard::draw(uint16_t x, uint16_t y) {
    // Draw all keys
    for (uint16_t row = 0; row < 4; row++) {
        for (uint16_t col = 0; col < 10; col++) {
            // Skip empty keys
            if (qwertyKeys[row][col].length() == 0) {
                continue;
            }

            // Handle Spacebar row (spans multiple columns)
            if (row == 3) {
                // Spacebar spans columns 0-5 (6 keys wide, centered)
                if (col == 0) {
                    // Calculate spacebar position (centered in row)
                    uint16_t rowWidth = 10 * (KEY_WIDTH + SPACING) - SPACING;
                    uint16_t spacebarWidth = 6 * (KEY_WIDTH + SPACING) - SPACING;
                    uint16_t xPos = x + (rowWidth - spacebarWidth) / 2;  // Center spacebar
                    uint16_t yPos = y + row * (KEY_HEIGHT + SPACING);
                    bool isSelected = (cursorRow == 3 && cursorCol >= 0 && cursorCol < 6);
                    drawKey(xPos, yPos, " ", isSelected);
                    // Skip drawing other spacebar columns
                    col = 5;
                }
                continue;
            }

            uint16_t xPos = x + col * (KEY_WIDTH + SPACING);
            uint16_t yPos = y + row * (KEY_HEIGHT + SPACING);

            bool isSelected = (row == cursorRow && col == cursorCol);
            drawKey(xPos, yPos, qwertyKeys[row][col], isSelected);
        }
    }
}

void MiniKeyboard::drawKey(uint16_t x, uint16_t y, const String& key, bool isSelected) {
    // Determine key width (normal or spacebar)
    uint16_t keyWidth = KEY_WIDTH;
    if (key == " ") {
        keyWidth = 6 * (KEY_WIDTH + SPACING) - SPACING;  // Spacebar width
    }

    // Background color
    uint16_t bgColor = isSelected ? HIGHLIGHT_COLOR : KEY_BG;
    
    // Draw key background with rounded corners
    tft->fillRoundRect(x, y, keyWidth, KEY_HEIGHT, 3, bgColor);
    
    // Draw border (cyan for cyberpunk effect)
    tft->drawRoundRect(x, y, keyWidth, KEY_HEIGHT, 3, KEY_BORDER);

    // Draw cyberpunk grid pattern (matching main keyboard)
    if (!isSelected) {
        // Draw grid lines in cyan
        uint16_t gridColor = KEY_BORDER;
        // Horizontal line
        tft->drawFastHLine(x + 2, y + KEY_HEIGHT / 2, keyWidth - 4, gridColor);
        // Vertical line
        tft->drawFastVLine(x + keyWidth / 2, y + 2, KEY_HEIGHT - 4, gridColor);
    }

    // Draw key text
    String displayText = getKeyDisplayText(key);
    
    // Text color: black on highlighted key, neon green otherwise
    uint16_t textColor = isSelected ? BLACK : KEY_TEXT;
    
    tft->setTextSize(1);
    tft->setTextColor(textColor, bgColor);
    
    // Center text in key
    uint16_t textWidth = displayText.length() * 6;  // Approximate width
    uint16_t textX = x + (keyWidth - textWidth) / 2;
    uint16_t textY = y + (KEY_HEIGHT - 8) / 2;  // Center vertically
    
    tft->setCursor(textX, textY);
    tft->print(displayText);
}

String MiniKeyboard::getKeyDisplayText(const String& key) const {
    if (key == "|e") {
        return "Ent";
    } else if (key == "<") {
        return "Del";
    } else if (key == "shift") {
        return capsLock ? "CAPS" : "Shft";
    } else if (key == " ") {
        return "Space";
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

void MiniKeyboard::moveCursorInternal(int8_t deltaRow, int8_t deltaCol) {
    int16_t newRow = (int16_t)cursorRow + deltaRow;
    int16_t newCol = (int16_t)cursorCol + deltaCol;
    
    // Handle row wrapping
    if (newRow < 0) {
        newRow = 3;  // Wrap to spacebar row
    } else if (newRow > 3) {
        newRow = 0;  // Wrap to top row
    }
    
    // Handle column wrapping
    if (newCol < 0) {
        newCol = 9;  // Wrap to right
    } else if (newCol > 9) {
        newCol = 0;  // Wrap to left
    }
    
    // Special handling for spacebar row
    if (newRow == 3) {
        // Spacebar spans columns 0-5, so clamp to that range
        if (newCol < 0 || newCol > 5) {
            newCol = 2;  // Center of spacebar
        }
    } else {
        // Skip empty keys in other rows
        if (qwertyKeys[newRow][newCol].length() == 0) {
            // Find nearest valid key
            if (deltaCol > 0) {
                // Moving right, find next valid key
                for (int8_t c = newCol + 1; c < 10; c++) {
                    if (qwertyKeys[newRow][c].length() > 0) {
                        newCol = c;
                        break;
                    }
                }
            } else if (deltaCol < 0) {
                // Moving left, find previous valid key
                for (int8_t c = newCol - 1; c >= 0; c--) {
                    if (qwertyKeys[newRow][c].length() > 0) {
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
        // Toggle caps lock if Shift key was selected
        if (cursorRow < 4 && cursorCol < 10) {
            String currentKey = qwertyKeys[cursorRow][cursorCol];
            if (currentKey == "shift") {
                capsLock = !capsLock;
            }
        }
        // Selection result is returned via getCurrentChar()
    }
}

String MiniKeyboard::getCurrentChar() {
    if (cursorRow < 4 && cursorCol < 10) {
        String key = qwertyKeys[cursorRow][cursorCol];
        
        // Handle special keys
        if (key == "|e") {
            return "|e";  // Enter key
        } else if (key == "<") {
            return "<";  // Backspace
        } else if (key == "shift") {
            return "shift";  // Shift key
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
    return "";
}

void MiniKeyboard::reset() {
    cursorRow = 0;
    cursorCol = 0;
    capsLock = false;
}
