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
    
    // Draw keyboard at bottom of content area
    if (keyboard != nullptr) {
        // Calculate keyboard dimensions
        const uint16_t KEY_WIDTH = 22;
        const uint16_t KEY_HEIGHT = 22;
        const uint16_t SPACING = 2;
        const uint16_t keyboardWidth = 10 * (KEY_WIDTH + SPACING) - SPACING;
        const uint16_t keyboardHeight = 4 * (KEY_HEIGHT + SPACING) - SPACING;
        
        // Position keyboard at bottom of screen
        const uint16_t screenHeight = 240;
        uint16_t keyboardY = screenHeight - keyboardHeight - 10;  // 10px margin from bottom
        
        // Center keyboard in content area (x is content area X, w is content area width)
        uint16_t keyboardX = x + (w - keyboardWidth) / 2;
        
        keyboard->draw(keyboardX, keyboardY);
    }
}

void MiniAddFriendScreen::handleKeyPress(const String& key) {
    if (keyboard == nullptr) return;
    
    // Handle navigation keys - delegate to keyboard
    if (key == "|u") {
        keyboard->moveCursor("up");
        return;
    } else if (key == "|d") {
        keyboard->moveCursor("down");
        return;
    } else if (key == "|l") {
        keyboard->moveCursor("left");
        return;
    } else if (key == "|r") {
        keyboard->moveCursor("right");
        return;
    } else if (key == "|e") {
        // Enter/Select key
        keyboard->moveCursor("select");
        String currentChar = keyboard->getCurrentChar();
        
        // Handle the selected character
        if (currentChar == "|e") {
            // Enter key - handled by parent (SocialScreen)
            return;
        } else if (currentChar == "<") {
            // Backspace - remove last character
            if (enteredName.length() > 0) {
                enteredName.remove(enteredName.length() - 1);
            }
            return;
        } else if (currentChar == "shift") {
            // Shift key - caps lock toggled in keyboard
            return;
        } else if (currentChar == " ") {
            // Space
            if (enteredName.length() < MAX_NAME_LENGTH) {
                enteredName += " ";
            }
            return;
        } else if (currentChar.length() == 1) {
            // Regular character
            if (enteredName.length() < MAX_NAME_LENGTH) {
                enteredName += currentChar;
            }
            return;
        }
    }
    
    // Track cursor position for display
    cursorRow = keyboard->getCursorRow();
    cursorCol = keyboard->getCursorCol();
}

void MiniAddFriendScreen::reset() {
    enteredName = "";
    cursorRow = 0;
    cursorCol = 0;
    if (keyboard != nullptr) {
        keyboard->reset();
    }
}

