#ifndef MINI_ADD_FRIEND_SCREEN_H
#define MINI_ADD_FRIEND_SCREEN_H

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include "mini_keyboard.h"

// Mini Add Friend Screen - Terminal-style input form for adding friends
// Used within SocialScreen's Add Friend tab
class MiniAddFriendScreen {
public:
    MiniAddFriendScreen(Adafruit_ST7789* tft, MiniKeyboard* keyboard);
    ~MiniAddFriendScreen() = default;

    // Draw the input form at specified position
    void draw(uint16_t x, uint16_t y, uint16_t w, uint16_t h, bool hasFocus);

    // Handle keyboard input
    void handleKeyPress(const String& key);

    // Get the entered username
    String getEnteredName() const { return enteredName; }

    // Reset the input state
    void reset();

    // Get cursor position for keyboard navigation
    uint16_t getCursorRow() const { return cursorRow; }
    int8_t getCursorCol() const { return cursorCol; }

private:
    Adafruit_ST7789* tft;
    MiniKeyboard* keyboard;

    String enteredName;
    static const int MAX_NAME_LENGTH = 18;
    
    // Cursor position for keyboard navigation
    uint16_t cursorRow;
    int8_t cursorCol;
};

#endif

