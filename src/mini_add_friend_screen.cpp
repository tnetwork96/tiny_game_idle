#include "mini_add_friend_screen.h"

// Deep Space Arcade Theme (matching SocialScreen)
#define SOCIAL_BG_DARK   0x0042  // Deep Midnight Blue #020817
#define SOCIAL_HEADER    0x08A5  // Header Blue #0F172A
#define SOCIAL_ACCENT    0x07FF  // Cyan accent
#define SOCIAL_TEXT      0xFFFF  // White text
#define SOCIAL_MUTED     0x8410  // Muted gray text
#define SOCIAL_ERROR     0xF986  // Bright Red for errors

MiniAddFriendScreen::MiniAddFriendScreen(Adafruit_ST7789* tft, MiniKeyboard* keyboard) {
    this->tft = tft;
    this->keyboard = keyboard;
    this->enteredName = "";
    this->cursorRow = 0;
    this->cursorCol = 0;
    this->submitRequested = false;
    this->errorMessage = "";
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
        tft->print("Enter Nickname");
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
        // Calculate keyboard dimensions (matching MiniKeyboard constants)
        const uint16_t KEY_WIDTH = 24;
        const uint16_t KEY_HEIGHT = 28;  // Updated to match MiniKeyboard
        const uint16_t SPACING = 3;
        const uint16_t keyboardWidth = 10 * KEY_WIDTH + 9 * SPACING;  // 267px
        const uint16_t keyboardHeight = 4 * KEY_HEIGHT + 3 * SPACING;  // 4 rows (3 QWERTY + 1 Spacebar)
        
        // Position keyboard at bottom of screen
        const uint16_t screenHeight = 240;
        uint16_t keyboardY = screenHeight - keyboardHeight - 4;  // 4px margin from bottom
        
        // Center keyboard in content area (x is content area X, w is content area width)
        // Content area is 275px wide, keyboard is 267px, leaving 4px padding on each side
        uint16_t keyboardX = x + (w - keyboardWidth) / 2;
        
        keyboard->draw(keyboardX, keyboardY);
    }
    
    // Draw error message below input box if present
    if (errorMessage.length() > 0) {
        tft->setTextSize(1);
        tft->setTextColor(SOCIAL_ERROR, SOCIAL_BG_DARK);
        // Position error message below input box with some spacing
        uint16_t errorY = y + h + 8;
        tft->setCursor(x, errorY);
        
        // Truncate error message if too long
        String displayError = errorMessage;
        int maxChars = (w) / 6;  // Approximate chars for text size 1
        if (displayError.length() > maxChars) {
            displayError = displayError.substring(0, maxChars - 3) + "...";
        }
        tft->print(displayError);
    }
}

void MiniAddFriendScreen::setErrorMessage(const String& message) {
    errorMessage = message;
}

void MiniAddFriendScreen::clearError() {
    errorMessage = "";
}

void MiniAddFriendScreen::handleKeyPress(const String& key) {
    if (keyboard == nullptr) return;
    
    // Reset submit flag at start of each key press
    submitRequested = false;
    
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
        // Physical Enter key pressed
        // If there's text in input, submit form immediately
        // Otherwise, type the selected character from keyboard
        if (enteredName.length() > 0) {
            // Submit form if there's text
            submitRequested = true;
            return;
        } else {
            // No text - type the selected character from keyboard
            keyboard->moveCursor("select");
            String currentChar = keyboard->getCurrentChar();
            
            // Handle the selected character
            if (currentChar == "|e") {
                // Enter key on keyboard was selected - but no text, do nothing
                return;
            } else if (currentChar == "<") {
                // Backspace - nothing to remove
                return;
            } else if (currentChar == "shift") {
                // Shift key - caps lock toggled in keyboard
                return;
            } else if (currentChar == "123" || currentChar == "ABC") {
                // Mode toggle keys - already handled in keyboard
                return;
            } else if (currentChar == " ") {
                // Space
                if (enteredName.length() < MAX_NAME_LENGTH) {
                    enteredName += " ";
                }
                return;
            } else if (currentChar.length() == 1) {
                // Regular character (letter, number, or symbol)
                if (enteredName.length() < MAX_NAME_LENGTH) {
                    enteredName += currentChar;
                }
                return;
            }
        }
    }
    
    // Handle direct character input from callback (giống LoginScreen)
    // Keyboard gốc gọi callback với currentKey (ký tự), screen xử lý trực tiếp
    if (key.length() == 1) {
        // Ignore special keys
        if (key == "123" || key == "ABC" || key == "shift") {
            return;
        }
        
        // Add character directly (giống LoginScreen::handleUsernameKey)
        // Validate length before adding
        if (enteredName.length() < MAX_NAME_LENGTH) {
            enteredName += key;
            // Clear error when user starts typing
            if (errorMessage.length() > 0) {
                errorMessage = "";
            }
        }
        return;
    }
    
    // Handle backspace from callback
    if (key == "<") {
        if (enteredName.length() > 0) {
            enteredName.remove(enteredName.length() - 1);
            // Clear error when user starts editing
            if (errorMessage.length() > 0) {
                errorMessage = "";
            }
        }
        return;
    }
    
    // Track cursor position for display
    cursorRow = keyboard->getCursorRow();
    cursorCol = keyboard->getCursorCol();
}

void MiniAddFriendScreen::reset() {
    enteredName = "";
    cursorRow = 0;
    cursorCol = 0;
    submitRequested = false;
    errorMessage = "";
    if (keyboard != nullptr) {
        keyboard->reset();
    }
}

