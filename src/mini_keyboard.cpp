#include "mini_keyboard.h"

// QWERTY layout (4 rows: 3 QWERTY + 1 Spacebar) - Matching main Keyboard layout
const String MiniKeyboard::qwertyKeys[4][10] = {
    { "q", "w", "e", "r", "t", "y", "u", "i", "o", "p" },
    { "123", "a", "s", "d", "f", "g", "h", "j", "k", "l" },  // Added "123" at start, removed "|e" at end
    { "shift", "z", "x", "c", "v", "b", "n", "m", "|e", "<" }, // Added "|e" at position 8
    { " ", " ", " ", " ", " ", " ", " ", " ", " ", "" }      // Spacebar row
};

MiniKeyboard::MiniKeyboard(Adafruit_ST7789* tft) {
    this->tft = tft;
    this->cursorRow = 0;
    this->cursorCol = 0;
    this->capsLock = false;
    // Initialize with same skin as main Keyboard (FeminineLilac)
    this->currentSkin = KeyboardSkins::getFeminineLilac();
}

void MiniKeyboard::setSkin(KeyboardSkin skin) {
    this->currentSkin = skin;
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
                    uint16_t rowWidth = 10 * KEY_WIDTH + 9 * SPACING;  // Total row width: 267px
                    uint16_t spacebarWidth = 6 * KEY_WIDTH + 5 * SPACING;  // Spacebar width: 159px
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
        keyWidth = 6 * KEY_WIDTH + 5 * SPACING;  // Spacebar width
    }

    // Background color from skin
    uint16_t bgColor = isSelected ? currentSkin.keySelectedColor : currentSkin.keyBgColor;
    
    // Draw key background with rounded corners or square based on skin
    if (currentSkin.roundedCorners && currentSkin.cornerRadius > 0) {
        tft->fillRoundRect(x, y, keyWidth, KEY_HEIGHT, currentSkin.cornerRadius, bgColor);
        if (currentSkin.hasBorder) {
            tft->drawRoundRect(x, y, keyWidth, KEY_HEIGHT, currentSkin.cornerRadius, currentSkin.keyBorderColor);
        }
    } else {
        tft->fillRect(x, y, keyWidth, KEY_HEIGHT, bgColor);
        if (currentSkin.hasBorder) {
            tft->drawRect(x, y, keyWidth, KEY_HEIGHT, currentSkin.keyBorderColor);
        }
    }

    // Draw pattern if skin has pattern (feminine pattern for lilac skin)
    if (currentSkin.hasPattern && !isSelected) {
        // Check for feminine skin (pink/lilac colors)
        if (currentSkin.keyBgColor == 0xF81F || currentSkin.keyBgColor == 0xFE1F || 
            currentSkin.keyBgColor == 0xA2B5 || currentSkin.keyBgColor == 0xC618) {
            drawFemininePattern(x, y, keyWidth, KEY_HEIGHT);
        }
        // Check for cyberpunk skin (black bg, cyan border)
        else if (currentSkin.keyBgColor == 0x0000 && currentSkin.keyBorderColor == 0x07FF) {
            // Draw grid pattern
            uint16_t gridColor = 0x07FF;  // Neon cyan
            tft->drawFastHLine(x + 2, y + KEY_HEIGHT / 2, keyWidth - 4, gridColor);
            tft->drawFastVLine(x + keyWidth / 2, y + 2, KEY_HEIGHT - 4, gridColor);
        }
    }

    // Draw key text (matching main Keyboard logic)
    String displayText = getKeyDisplayText(key);
    
    // Skip drawing if text is empty (e.g., spacebar)
    if (displayText.length() == 0) {
        return;
    }
    
    // Truncate text if too long (matching main Keyboard - max 2 chars)
    if (displayText.length() > 2) {
        displayText = displayText.substring(0, 2);
    }
    
    // Text color from skin
    uint16_t textColor = isSelected ? currentSkin.keySelectedTextColor : currentSkin.keyTextColor;
    
    tft->setTextSize(currentSkin.textSize);
    tft->setTextColor(textColor, bgColor);
    
    // Calculate text position (matching main Keyboard logic)
    uint16_t textWidth = displayText.length() * 6 * currentSkin.textSize;  // Approximate width
    uint16_t marginX = 2;  // Margin to avoid border
    uint16_t textX = x + marginX;  // Start from left with margin
    if (textWidth < keyWidth - (marginX * 2)) {
        textX = x + (keyWidth - textWidth) / 2;  // Center if enough space
    }
    uint16_t textY = y + (KEY_HEIGHT - 8 * currentSkin.textSize) / 2;  // Center vertically
    
    tft->setCursor(textX, textY);
    tft->print(displayText);
}

void MiniKeyboard::drawFemininePattern(uint16_t x, uint16_t y, uint16_t width, uint16_t height) {
    // Draw feminine pattern (hearts, flowers) matching main Keyboard
    uint16_t patternColor = 0xFFFF;  // White color for pattern
    
    // Draw small heart at top-right corner
    uint16_t heartX = x + width - 6;
    uint16_t heartY = y + 2;
    
    // Draw simple heart (3 pixels)
    tft->drawPixel(heartX, heartY, patternColor);
    tft->drawPixel(heartX + 1, heartY, patternColor);
    tft->drawPixel(heartX, heartY + 1, patternColor);
    
    // Draw small flower at bottom-left corner
    uint16_t flowerX = x + 2;
    uint16_t flowerY = y + height - 4;
    
    // Draw simple 5-petal flower
    tft->drawPixel(flowerX, flowerY, patternColor);      // Center petal
    tft->drawPixel(flowerX - 1, flowerY - 1, patternColor);  // Top-left petal
    tft->drawPixel(flowerX + 1, flowerY - 1, patternColor);  // Top-right petal
    tft->drawPixel(flowerX - 1, flowerY + 1, patternColor);  // Bottom-left petal
    tft->drawPixel(flowerX + 1, flowerY + 1, patternColor);  // Bottom-right petal
    
    // Draw small dot in center
    if (width > 15 && height > 15) {
        uint16_t dotX = x + width / 2;
        uint16_t dotY = y + height / 2;
        tft->drawPixel(dotX, dotY, patternColor);
    }
}

String MiniKeyboard::getKeyDisplayText(const String& key) const {
    // Handle shift key - show "Aa" or "AA" based on capsLock (matching main Keyboard)
    if (key == "shift") {
        return capsLock ? "AA" : "Aa";
    }
    // Handle Enter key
    if (key == "|e") {
        return "Ent";  // Could draw symbol, but text is simpler for mini keyboard
    }
    // Handle Backspace
    if (key == "<") {
        return "<";  // Keep simple, matching main keyboard format
    }
    // Handle spacebar
    if (key == " ") {
        return "";  // Spacebar usually has no text, or could show "Space"
    }
    // Handle "123" key
    if (key == "123") {
        return "123";
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
            return "shift";  // Shift key (toggles caps lock)
        } else if (key == "123") {
            return "123";  // Number mode key (not used in mini keyboard, but return for compatibility)
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
