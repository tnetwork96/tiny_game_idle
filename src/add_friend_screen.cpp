#include "add_friend_screen.h"

// Midnight Blue Arcade Theme (matching Buddy List Screen)
#define FRIEND_BG_DARK   0x0042  // Deep Midnight Blue #020817
#define FRIEND_TEXT      0xFFFF  // White text
#define FRIEND_MUTED     0xC618  // Muted gray text
#define FRIEND_ERROR     0xF800  // Red for errors

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
    // Deep Midnight Blue background
    uint16_t width = 320;
    uint16_t height = 240;
    tft->fillRect(0, 0, width, height, FRIEND_BG_DARK);
}

void AddFriendScreen::drawInputBox(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const String& value) {
    tft->fillRoundRect(x, y, w, h, 6, ST77XX_BLACK);
    tft->drawRoundRect(x, y, w, h, 6, FRIEND_TEXT);
    tft->setCursor(x + 8, y + (h / 2) - 6);
    tft->setTextSize(2);
    tft->setTextColor(FRIEND_TEXT, ST77XX_BLACK);

    if (value.length() == 0) {
        tft->setTextColor(FRIEND_MUTED, ST77XX_BLACK);
        tft->print("Enter friend name");
    } else {
        tft->setTextColor(FRIEND_TEXT, ST77XX_BLACK);
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
    // Clear input box region
    tft->fillRect(20, 30, 280, 34, FRIEND_BG_DARK);
    drawInputBox(20, 30, 280, 34, friendName);

    // Clear error area
    tft->fillRect(20, 96, 280, 12, FRIEND_BG_DARK);
    if (showErrorMsg) {
        drawErrorMessage("Friend name is required", 100);
    }
}

void AddFriendScreen::draw() {
    drawBackground();

    // Simple label above the input
    tft->setTextSize(1);
    tft->setTextColor(FRIEND_TEXT, FRIEND_BG_DARK);
    tft->setCursor(20, 12);
    tft->print("Add Friend");

    drawInputBox(20, 30, 280, 34, friendName);

    tft->setTextSize(1);
    tft->setTextColor(FRIEND_TEXT, FRIEND_BG_DARK);
    tft->setCursor(20, 80);
    tft->print("Press Enter to add friend");

    if (showNameEmpty) {
        drawErrorMessage("Friend name is required", 100);
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

