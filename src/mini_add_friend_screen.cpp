#include "mini_add_friend_screen.h"

// Deep Space Arcade Theme (matching SocialScreen)
#define SOCIAL_BG_DARK   0x0042  // Deep Midnight Blue #020817
#define SOCIAL_INPUT_BG  0x0021  // Dark Blue for input box fill
#define SOCIAL_HEADER    0x08A5  // Header Blue #0F172A
#define SOCIAL_ACCENT    0x07FF  // Cyan accent
#define SOCIAL_TEXT       0xFFFF  // White text
#define SOCIAL_MUTED      0x8410  // Muted gray text
#define SOCIAL_ERROR      0xF800  // Red for errors
#define SOCIAL_SUCCESS    0x07E0  // Green for success messages

MiniAddFriendScreen::MiniAddFriendScreen(Adafruit_ST7789* tft, MiniKeyboard* keyboard) {
    this->tft = tft;
    this->keyboard = keyboard;
    this->enteredName = "";
    this->cursorRow = 0;
    this->cursorCol = 0;
    this->submitRequested = false;
    this->errorMessage = "";
    this->active = false;  // Initially inactive
}

void MiniAddFriendScreen::draw(uint16_t x, uint16_t y, uint16_t w, uint16_t h, bool hasFocus) {
    // Draw modern input box with Deep Space theme
    // Fill with dark blue background
    tft->fillRoundRect(x, y, w, h, 4, SOCIAL_INPUT_BG);
    
    // Border - cyan when focused/active, muted grey otherwise
    uint16_t borderColor = hasFocus ? SOCIAL_ACCENT : SOCIAL_MUTED;
    tft->drawRoundRect(x, y, w, h, 4, borderColor);
    
    // Prepare text rendering
    tft->setTextSize(2);
    
    // Calculate text position (centered both horizontally and vertically)
    // CRITICAL: Remove ALL pipe characters from enteredName before displaying
    String displayText = enteredName;
    displayText.replace("|", "");  // Remove all pipe characters
    bool showPlaceholder = (displayText.length() == 0);
    
    if (showPlaceholder) {
        displayText = "Enter nickname...";
    }
    
    // Calculate text width for centering (approximate: text size 2 = ~12px per char)
    int textWidth = displayText.length() * 12;
    uint16_t textX = x + (w - textWidth) / 2;
    uint16_t textY = y + (h / 2) - 6;  // Center vertically (text size 2 is ~12px tall)
    
    // Draw placeholder or input text
    if (showPlaceholder) {
        tft->setTextColor(SOCIAL_MUTED, SOCIAL_INPUT_BG);
    } else {
        tft->setTextColor(SOCIAL_TEXT, SOCIAL_INPUT_BG);
        // Truncate if too long to fit
        int maxChars = (w - 16) / 12;  // Leave padding on sides
        if (displayText.length() > maxChars) {
            displayText = displayText.substring(0, maxChars - 3) + "...";
            textWidth = displayText.length() * 12;
            textX = x + (w - textWidth) / 2;  // Recalculate for truncated text
        }
    }
    
    tft->setCursor(textX, textY);
    tft->print(displayText);
    
    // Cursor blink removed - no need for visual cursor indicator
    
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
    
    // Draw error/status message below input box if present
    if (errorMessage.length() > 0) {
        tft->setTextSize(1);
        
        // Determine if it's a success message (starts with "SUCCESS:" or contains success keywords)
        bool isSuccess = errorMessage.startsWith("SUCCESS:") || 
                         errorMessage.indexOf("success") >= 0 ||
                         errorMessage.indexOf("added") >= 0 ||
                         errorMessage.indexOf("sent") >= 0;
        
        uint16_t messageColor = isSuccess ? SOCIAL_SUCCESS : SOCIAL_ERROR;
        tft->setTextColor(messageColor, SOCIAL_BG_DARK);
        
        // Position message below input box with spacing
        uint16_t messageY = y + h + 5;
        
        // Calculate text width for centering
        String displayMessage = errorMessage;
        // Remove "SUCCESS:" prefix if present for display
        if (displayMessage.startsWith("SUCCESS:")) {
            displayMessage = displayMessage.substring(8);
            displayMessage.trim();
        }
        
        // Truncate if too long
        int maxChars = w / 6;  // Approximate chars for text size 1
        if (displayMessage.length() > maxChars) {
            displayMessage = displayMessage.substring(0, maxChars - 3) + "...";
        }
        
        // Center message horizontally
        int messageWidth = displayMessage.length() * 6;  // Approximate width for text size 1
        uint16_t messageX = x + (w - messageWidth) / 2;
        
        tft->setCursor(messageX, messageY);
        tft->print(displayMessage);
    }
}

void MiniAddFriendScreen::setErrorMessage(const String& message) {
    errorMessage = message;
}

void MiniAddFriendScreen::clearError() {
    errorMessage = "";
}

// Navigation handlers
void MiniAddFriendScreen::handleUp() {
    if (keyboard != nullptr) {
        keyboard->moveCursor("up");
    }
}

void MiniAddFriendScreen::handleDown() {
    if (keyboard != nullptr) {
        keyboard->moveCursor("down");
    }
}

void MiniAddFriendScreen::handleLeft() {
    if (keyboard != nullptr) {
        keyboard->moveCursor("left");
    }
}

void MiniAddFriendScreen::handleRight() {
    if (keyboard != nullptr) {
        keyboard->moveCursor("right");
    }
}

void MiniAddFriendScreen::handleSelect() {
    if (keyboard == nullptr) return;
    
    // Physical Enter key pressed - process the selected character from keyboard
    keyboard->moveCursor("select");
    // Note: moveCursor("select") will call onKeySelected callback if set
    // The callback routes to handleKeyPress(key) which handles regular character input
    // So we only need to handle special keys here (Enter) that need special behavior
    
    String currentChar = keyboard->getCurrentChar();
    
    // Handle Enter key for form submission
    if (currentChar == "|e") {
        // Enter key on keyboard was selected
        // If there's text, submit form; otherwise do nothing
        if (enteredName.length() > 0) {
            submitRequested = true;
        }
        return;
    }
    
    // For all other keys (regular characters, backspace, shift, mode toggles):
    // The callback (onKeySelected) will route to handleKeyPress(key) which handles them
    // So we don't need to handle them here to avoid duplicate processing
    // Only Enter key (|e) needs special handling here for form submission
}

void MiniAddFriendScreen::handleExit() {
    // Exit - delete last character (backspace behavior)
    if (enteredName.length() > 0) {
        enteredName.remove(enteredName.length() - 1);
        // Clear error when user starts editing
        if (errorMessage.length() > 0) {
            errorMessage = "";
        }
    }
}

void MiniAddFriendScreen::handleKeyPress(const String& key) {
    if (keyboard == nullptr) return;
    
    // Reset submit flag at start of each key press
    submitRequested = false;
    
    // Handle new navigation key format first (similar to WiFi password screen)
    if (key == "up") {
        handleUp();
        return;
    } else if (key == "down") {
        handleDown();
        return;
    } else if (key == "left") {
        handleLeft();
        return;
    } else if (key == "right") {
        handleRight();
        return;
    } else if (key == "select") {
        handleSelect();
        return;
    } else if (key == "exit") {
        handleExit();
        return;
    }
    
    // Backward compatibility: handle old key format
    if (key == "|u") {
        handleUp();
        return;
    } else if (key == "|d") {
        handleDown();
        return;
    } else if (key == "|l") {
        handleLeft();
        return;
    } else if (key == "|r") {
        handleRight();
        return;
    } else if (key == "|e") {
        handleSelect();
        return;
    } else if (key == "<" || key == "|b") {
        handleExit();
        return;
    }
    
    // CRITICAL: Block ALL keys containing pipe character from being added to text
    // This includes "|e", "|u", "|d", "|l", "|r", "|b", and any single "|"
    if (key.indexOf("|") >= 0) {
        return;  // Reject any key containing pipe character
    }
    
    // Handle direct character input from callback (giống LoginScreen)
    // Keyboard gốc gọi callback với currentKey (ký tự), screen xử lý trực tiếp
    if (key.length() == 1) {
        // Ignore special keys and pipe character
        if (key == "123" || key == "ABC" || key == "shift" || key == "|") {
            return;
        }
        
        // Add character directly (giống LoginScreen::handleUsernameKey)
        // Validate length before adding
        if (enteredName.length() < MAX_NAME_LENGTH) {
            enteredName += key;
            // Remove any pipe characters that might have been added (safety check)
            enteredName.replace("|", "");
            // Clear error when user starts typing
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
    enteredName.replace("|", "");  // Safety check: remove any pipe characters
    cursorRow = 0;
    cursorCol = 0;
    submitRequested = false;
    errorMessage = "";
    if (keyboard != nullptr) {
        keyboard->reset();
    }
}

