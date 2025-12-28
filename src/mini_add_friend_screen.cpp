#include "mini_add_friend_screen.h"

// Deep Space Arcade Theme (matching SocialScreen)
#define SOCIAL_BG_DARK   0x0042  // Deep Midnight Blue #020817
#define SOCIAL_HEADER    0x08A5  // Header Blue #0F172A
#define SOCIAL_ACCENT    0x07FF  // Cyan accent
#define SOCIAL_TEXT      0xFFFF  // White text
#define SOCIAL_MUTED     0x8410  // Muted gray text

MiniAddFriendScreen::MiniAddFriendScreen(Adafruit_ST7789* tft, MiniKeyboard* keyboard) {
    this->tft = tft;
    this->keyboard = keyboard;
    this->enteredName = "";
    this->cursorRow = 0;
    this->cursorCol = 0;
}

void MiniAddFriendScreen::draw(uint16_t x, uint16_t y, uint16_t w, uint16_t h, bool hasFocus) {
    // Draw terminal-style input box
    // Background (card style)
    tft->fillRoundRect(x, y, w, h, 4, SOCIAL_HEADER);
    
    // Border - bright cyan when focused, muted otherwise
    if (hasFocus) {
        tft->drawRoundRect(x, y, w, h, 4, SOCIAL_ACCENT);
    } else {
        tft->drawRoundRect(x, y, w, h, 4, SOCIAL_MUTED);
    }
    
    // Draw terminal prompt "> "
    tft->setTextSize(2);
    tft->setTextColor(SOCIAL_ACCENT, SOCIAL_HEADER);
    tft->setCursor(x + 8, y + (h / 2) - 6);
    tft->print("> ");
    
    // Draw input text or placeholder
    uint16_t textX = x + 24;  // After "> "
    tft->setCursor(textX, y + (h / 2) - 6);
    
    if (enteredName.length() == 0) {
        tft->setTextColor(SOCIAL_MUTED, SOCIAL_HEADER);
        tft->print("Enter Username");
    } else {
        tft->setTextColor(SOCIAL_TEXT, SOCIAL_HEADER);
        // Truncate if too long
        String displayName = enteredName;
        int maxChars = (w - 24 - 16) / 12;  // Approximate chars for text size 2
        if (displayName.length() > maxChars) {
            displayName = displayName.substring(0, maxChars - 3) + "...";
        }
        tft->print(displayName);
        
        // Draw blinking cursor (simple underscore)
        if ((millis() / 500) % 2 == 0) {  // Blink every 500ms
            tft->setTextColor(SOCIAL_ACCENT, SOCIAL_HEADER);
            tft->print("_");
        }
    }
}

void MiniAddFriendScreen::handleKeyPress(const String& key) {
    // Track cursor position
    cursorRow = keyboard->getCursorRow();
    cursorCol = keyboard->getCursorCol();
    
    if (key == "|e") {
        // Enter key - handled by parent (SocialScreen)
        return;
    }
    
    if (key == "<") {
        // Backspace - remove last character
        if (enteredName.length() > 0) {
            enteredName.remove(enteredName.length() - 1);
        }
        return;
    }
    
    // Ignore mode switch keys (not needed for MiniKeyboard)
    if (key == "123" || key == "ABC" || key == "shift") {
        return;
    }
    
    // Add character if under limit
    if (key.length() == 1 && enteredName.length() < MAX_NAME_LENGTH) {
        enteredName += key;
    }
}

void MiniAddFriendScreen::reset() {
    enteredName = "";
    cursorRow = 0;
    cursorCol = 0;
    if (keyboard != nullptr) {
        keyboard->resetCursor();
    }
}

