#ifndef MINI_KEYBOARD_H
#define MINI_KEYBOARD_H

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

// Mini Keyboard - Standalone keyboard component for Add Friend screen
// Self-contained with no dependencies on main Keyboard class
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

    // Partial redraw helper: redraw only the old/new cursor keys.
    // Call this AFTER cursorRow/cursorCol have been updated (e.g. after moveCursor()).
    void redrawAfterCursorMove(uint16_t oldRow, int8_t oldCol);

    // Move cursor to specific position (giống Keyboard gốc)
    void moveCursorTo(uint16_t row, int8_t col);

    // Toggle between QWERTY and Numeric mode
    void toggleMode();
    
    // Type a string by navigating to each character and selecting it
    void typeString(const String& text);
    
    // Press Enter key (giống Keyboard gốc)
    void pressEnter();
    
    // Set callback for when a key is selected (giống Keyboard gốc)
    void setOnKeySelectedCallback(void (*callback)(const String& key)) { 
        onKeySelected = callback; 
    }

private:
    Adafruit_ST7789* tft;

    // Cursor position
    uint16_t cursorRow;
    int8_t cursorCol;

    // Caps Lock state
    bool capsLock;

    // Numeric mode state
    bool isNumericMode;

    // Last draw position (for refresh after mode toggle)
    uint16_t lastDrawX;
    uint16_t lastDrawY;
    
    // Callback function pointer cho khi nhấn select (giống Keyboard gốc)
    void (*onKeySelected)(const String& key);

    // Local constants
    static const uint16_t KEY_WIDTH = 24;
    static const uint16_t KEY_HEIGHT = 28;
    static const uint16_t SPACING = 3;
    static const uint16_t ROWS = 3;  // 3 QWERTY rows (spacebar vẽ riêng, giống Keyboard gốc)
    static const uint16_t COLS = 10;

    // Layout definitions (3 rows x 10 cols for QWERTY, 3 rows for numeric)
    // Spacebar không có trong layout array, được vẽ riêng giống Keyboard gốc
    static const String qwertyLayout[3][10];
    static const String numericLayout[3][10];

    // Helper methods
    void moveCursorInternal(int8_t deltaRow, int8_t deltaCol);
    void drawKey(uint16_t x, uint16_t y, const String& key, bool isSelected);
    String getKeyDisplayText(const String& key) const;

    // Drawing helpers for partial redraw
    uint16_t computeXStart(uint16_t x) const;
    void redrawKeyAt(uint16_t row, int8_t col, bool isSelected);
    void redrawSpacebarAt(uint16_t xStart, uint16_t y, bool isSelected);
};

#endif
