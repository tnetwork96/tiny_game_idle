#ifndef NICKNAME_SCREEN_H
#define NICKNAME_SCREEN_H

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include "keyboard.h"
#include <FS.h>
#include <SPIFFS.h>

class NicknameScreen {
public:
    NicknameScreen(Adafruit_ST7789* tft, Keyboard* keyboard);
    ~NicknameScreen() = default;

    // Draw current state
    void draw();

    // Handle keyboard key (coming from onKeySelected)
    void handleKeyPress(const String& key);

    // State helpers
    String getNickname() const { return nickname; }
    bool isConfirmed() const { return confirmed; }
    bool isCancelled() const { return cancelled; }
    void reset();
    void setNickname(const String& name);  // Set nickname programmatically

    // File operations
    bool hasNickname() const;  // Check if nickname file exists
    String loadNickname();      // Load nickname from file
    bool saveNickname(const String& name);  // Save nickname to file

private:
    Adafruit_ST7789* tft;
    Keyboard* keyboard;

    String nickname;
    bool showNameEmpty;
    bool showNameTooLong;
    bool confirmed;
    bool cancelled;
    uint16_t cursorRow;
    int8_t cursorCol;

    static const char* NICKNAME_FILE_PATH;  // "/nickname.txt"

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

