#ifndef MINI_KEYBOARD_H
#define MINI_KEYBOARD_H

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

// Mini Keyboard - Simplified keyboard for Add Friend screen
// Cyberpunk theme matching main Keyboard class
class MiniKeyboard {
public:
    MiniKeyboard(Adafruit_ST7789* tft);
    ~MiniKeyboard() = default;

    // Draw the keyboard at specified position (top-left origin)
    void draw(uint16_t x, uint16_t y);

    // Move cursor by direction ("up", "down", "left", "right", "select")
    void moveCursor(String direction);

    // Get the current character at cursor position
    String getCurrentChar();

    // Reset cursor and caps lock
    void reset();

    // Get cursor position (for compatibility)
    uint16_t getCursorRow() const { return cursorRow; }
    int8_t getCursorCol() const { return cursorCol; }

private:
    Adafruit_ST7789* tft;

    // Cursor position
    uint16_t cursorRow;
    int8_t cursorCol;

    // Caps Lock state
    bool capsLock;

    // Key dimensions (fits in 275px content width)
    static const uint16_t KEY_WIDTH = 22;
    static const uint16_t KEY_HEIGHT = 22;
    static const uint16_t SPACING = 2;

    // QWERTY layout (4 rows: 3 QWERTY + 1 Spacebar)
    static const String qwertyKeys[4][10];

    // Helper methods
    void moveCursorInternal(int8_t deltaRow, int8_t deltaCol);
    void drawKey(uint16_t x, uint16_t y, const String& key, bool isSelected);
    String getKeyDisplayText(const String& key) const;
};

#endif
