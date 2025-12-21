#ifndef ADD_FRIEND_SCREEN_H
#define ADD_FRIEND_SCREEN_H

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include "keyboard.h"

class AddFriendScreen {
public:
    AddFriendScreen(Adafruit_ST7789* tft, Keyboard* keyboard);
    ~AddFriendScreen() = default;

    // Draw current state
    void draw();

    // Handle keyboard key (coming from onKeySelected)
    void handleKeyPress(const String& key);

    // State helpers
    String getFriendName() const { return friendName; }
    bool isConfirmed() const { return confirmed; }
    bool isCancelled() const { return cancelled; }
    void reset();

private:
    Adafruit_ST7789* tft;
    Keyboard* keyboard;

    String friendName;
    bool showNameEmpty;
    bool confirmed;
    bool cancelled;
    uint16_t cursorRow;
    int8_t cursorCol;

    // Drawing helpers
    void drawBackground();
    void drawInputBox(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const String& value);
    void drawErrorMessage(const String& message, uint16_t y);
    void updateInputArea(bool showErrorMsg);

    // Input helpers
    void ensureAlphabetMode();
    void handleNameKey(const String& key);
};

#endif

