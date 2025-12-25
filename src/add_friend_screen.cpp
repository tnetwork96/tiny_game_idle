#include "add_friend_screen.h"

// Deep Space Arcade Theme (matching Buddy List Screen)
#define FRIEND_BG_DARK   0x0042  // Deep Midnight Blue #020817
#define FRIEND_HEADER    0x08A5  // Header Blue #0F172A
#define FRIEND_TEXT      0xFFFF  // White text
#define FRIEND_CYAN      0x07FF  // Cyan accent
#define FRIEND_MUTED     0x8410  // Muted gray text (lighter for visibility)
#define FRIEND_ERROR     0xF986  // Bright Red #FF3333
#define FRIEND_HIGHLIGHT 0x1148  // Electric Blue tint for selection

AddFriendScreen::AddFriendScreen(Adafruit_ST7789* tft, Keyboard* keyboard) {
    this->tft = tft;
    this->keyboard = keyboard;
    this->friendName = "";
    this->showNameEmpty = false;
    this->confirmed = false;
    this->cancelled = false;
    this->cursorRow = 0;
    this->cursorCol = 0;
}

void AddFriendScreen::drawBackground() {
    // Clear entire screen to prevent black stripe artifacts
    tft->fillScreen(FRIEND_BG_DARK);
}

void AddFriendScreen::drawInputBox(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const String& value) {
    // Background with Deep Midnight Blue
    tft->fillRoundRect(x, y, w, h, 6, FRIEND_BG_DARK);
    // Cyan border for accent
    tft->drawRoundRect(x, y, w, h, 6, FRIEND_CYAN);
    tft->setCursor(x + 8, y + (h / 2) - 6);
    tft->setTextSize(2);
    tft->setTextColor(FRIEND_TEXT, FRIEND_BG_DARK);

    if (value.length() == 0) {
        tft->setTextColor(FRIEND_MUTED, FRIEND_BG_DARK);
        tft->print("Enter friend name");
    } else {
        tft->setTextColor(FRIEND_TEXT, FRIEND_BG_DARK);
        tft->print(value);
    }
}

void AddFriendScreen::drawErrorMessage(const String& message, uint16_t y) {
    tft->setTextSize(1);
    tft->setTextColor(FRIEND_ERROR, FRIEND_BG_DARK);
    tft->setCursor(20, y);
    tft->print(message);
}

void AddFriendScreen::updateInputArea(bool showErrorMsg) {
    // Clear input box region (accounting for header)
    const uint16_t headerHeight = 30;
    tft->fillRect(20, headerHeight + 20, 280, 34, FRIEND_BG_DARK);
    drawInputBox(20, headerHeight + 20, 280, 34, friendName);

    // Clear error area
    tft->fillRect(20, headerHeight + 86, 280, 12, FRIEND_BG_DARK);
    if (showErrorMsg) {
        drawErrorMessage("Friend name is required", headerHeight + 90);
    }
}

void AddFriendScreen::draw() {
    drawBackground();

    // Header bar matching BuddyListScreen style
    const uint16_t headerHeight = 30;
    tft->fillRect(0, 0, 320, headerHeight, FRIEND_HEADER);
    tft->drawFastHLine(0, headerHeight - 1, 320, FRIEND_CYAN);
    
    // Title in header
    tft->setTextSize(2);
    tft->setTextColor(FRIEND_TEXT, FRIEND_HEADER);
    tft->setCursor(10, 8);
    tft->print("ADD FRIEND");

    drawInputBox(20, headerHeight + 20, 280, 34, friendName);

    tft->setTextSize(1);
    tft->setTextColor(FRIEND_TEXT, FRIEND_BG_DARK);
    tft->setCursor(20, headerHeight + 70);
    tft->print("Press Enter to add friend");

    if (showNameEmpty) {
        drawErrorMessage("Friend name is required", headerHeight + 90);
    }

    ensureAlphabetMode();
    keyboard->moveCursorTo(cursorRow, cursorCol);
}

void AddFriendScreen::ensureAlphabetMode() {
    if (!keyboard->getIsAlphabetMode()) {
        keyboard->moveCursorTo(1, 0);  // "ABC" in numeric layout
        delay(120);
        keyboard->moveCursorByCommand("select", 0, 0);
        delay(120);
    }
}

void AddFriendScreen::handleNameKey(const String& key) {
    cursorRow = keyboard->getCursorRow();
    cursorCol = keyboard->getCursorCol();
    
    if (key == "|e") {
        // Enter key - confirm if valid, show error if empty
        if (friendName.length() == 0) {
            showNameEmpty = true;
            updateInputArea(true);
        } else {
            confirmed = true;
        }
        return;
    }

    if (key == "<") {
        // Delete key - remove last character
        if (friendName.length() > 0) {
            friendName.remove(friendName.length() - 1);
            showNameEmpty = false;
            updateInputArea(false);
        }
        return;
    }

    // Ignore mode switch keys
    if (key == "123" || key == "ABC" || key == Keyboard::KEY_ICON || key == "shift") {
        return;
    }

    // Add character if under limit
    if (friendName.length() < 18) {
        friendName += key;
        showNameEmpty = false;
        updateInputArea(false);
    }
}

void AddFriendScreen::handleKeyPress(const String& key) {
    handleNameKey(key);
}

void AddFriendScreen::reset() {
    friendName = "";
    showNameEmpty = false;
    confirmed = false;
    cancelled = false;
    cursorRow = 0;
    cursorCol = 0;
}

