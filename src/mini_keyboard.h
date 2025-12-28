#ifndef MINI_KEYBOARD_H
#define MINI_KEYBOARD_H

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

// Mini Keyboard - Simplified keyboard for Add Friend screen
// Compact layout with smaller keys, alphabet-only mode
class MiniKeyboard {
public:
    MiniKeyboard(Adafruit_ST7789* tft);
    ~MiniKeyboard() = default;

    // Draw the keyboard
    void draw();

    // Move cursor by command (up, down, left, right, select)
    void moveCursorByCommand(String command);

    // Get the current key at cursor position
    String getCurrentKey() const;

    // Reset cursor to initial position
    void resetCursor();

    // Set visibility (for future use)
    void setVisible(bool visible) { this->visible = visible; }
    bool isVisible() const { return visible; }

    // Get cursor position
    uint16_t getCursorRow() const { return cursorRow; }
    int8_t getCursorCol() const { return cursorCol; }
    
    // Move cursor to specific position
    void moveCursorTo(uint16_t row, int8_t col);

private:
    Adafruit_ST7789* tft;

    // Cursor position
    uint16_t cursorRow;
    int8_t cursorCol;

    // Visibility
    bool visible;

    // Key dimensions (smaller than main keyboard)
    static const uint16_t KEY_WIDTH = 18;
    static const uint16_t KEY_HEIGHT = 20;
    static const uint16_t SPACING = 2;

    // QWERTY layout (alphabet only, lowercase)
    static const String qwertyKeys[3][10];

    // Helper methods
    void moveCursor(int8_t deltaRow, int8_t deltaCol);
    void drawKey(uint16_t x, uint16_t y, const String& key, bool isSelected);
};

#endif

