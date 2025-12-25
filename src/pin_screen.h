#ifndef PIN_SCREEN_H
#define PIN_SCREEN_H

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include "keyboard.h"

// Dedicated PIN entry screen (mirrors username screen style)
class PinScreen {
public:
    PinScreen(Adafruit_ST7789* tft, Keyboard* keyboard);
    ~PinScreen() = default;

    void setUsername(const String& user) { username = user; }
    void setExpectedPin(const String& pin) { expectedPin = pin; }
    void reset();
    void draw();
    void handleKeyPress(const String& key);

    bool isPinAccepted() const { return pinAccepted; }
    bool wantsUsernameStep() const { return backToUsername; }
    String getPinInput() const { return pinInput; }

private:
    Adafruit_ST7789* tft;
    Keyboard* keyboard;

    String username;
    String pinInput;
    String expectedPin;
    bool showError;
    bool pinAccepted;
    bool backToUsername;
    uint16_t pinCursorRow;
    int8_t pinCursorCol;

    // Drawing helpers (kept consistent with username screen)
    void drawBackground();
    void drawProfileCircle(uint16_t cx, uint16_t cy, uint16_t r, uint16_t color);
    void drawInputBox(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const String& value, bool masked);
    void drawErrorMessage(const String& message, uint16_t y);
    void drawPinScreen();
    void updatePinInputArea(bool showErrorMsg);

    // Input helpers
    void ensureNumericMode();
};

#endif


