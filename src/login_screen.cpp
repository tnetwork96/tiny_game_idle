#include "login_screen.h"

// Deep Space Arcade Theme (matching Buddy List Screen)
#define WIN_BG_DARK   0x0042  // Deep Midnight Blue #020817
#define WIN_HEADER    0x08A5  // Header Blue #0F172A
#define WIN_ACCENT    0x07FF  // Cyan accent
#define WIN_TEXT      0xFFFF  // White text
#define WIN_MUTED     0x8410  // Muted gray text
#define WIN_ERROR     0xF986  // Bright Red #FF3333
#define WIN_HIGHLIGHT 0x1148  // Electric Blue tint
#define WIN_SUCCESS   0x07F3  // Minty Green for success

LoginScreen::LoginScreen(Adafruit_ST7789* tft, Keyboard* keyboard) {
    this->tft = tft;
    this->keyboard = keyboard;
    this->pinScreen = new PinScreen(tft, keyboard);
    this->state = LOGIN_USERNAME;
    this->username = "";
    this->showUsernameEmpty = false;
    this->usernameCursorRow = 0;
    this->usernameCursorCol = 0;
}

void LoginScreen::drawBackground() {
    // Clear entire screen to prevent black stripe artifacts
    tft->fillScreen(WIN_BG_DARK);
}

void LoginScreen::drawProfileCircle(uint16_t cx, uint16_t cy, uint16_t r, uint16_t color) {
    tft->fillCircle(cx, cy, r, color);
    tft->drawCircle(cx, cy, r, WIN_ACCENT);  // Cyan border
    tft->drawCircle(cx, cy, r - 2, WIN_ACCENT);
}

void LoginScreen::drawInputBox(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const String& value, bool masked) {
    // Background with Deep Midnight Blue
    tft->fillRoundRect(x, y, w, h, 6, WIN_BG_DARK);
    // Cyan border for accent
    tft->drawRoundRect(x, y, w, h, 6, WIN_ACCENT);
    tft->setCursor(x + 8, y + (h / 2) - 6);
    tft->setTextSize(2);
    tft->setTextColor(WIN_TEXT, WIN_BG_DARK);

    String renderValue = value;
    if (masked && value.length() > 0) {
        renderValue = "";
        for (uint16_t i = 0; i < value.length(); i++) {
            renderValue += "*";  // Use asterisk to keep ASCII while showing dots
        }
    } else if (value.length() == 0) {
        tft->setTextColor(WIN_MUTED, WIN_BG_DARK);
    }

    if (renderValue.length() == 0) {
        tft->print(masked ? "Enter PIN" : "Enter username");
    } else {
        tft->print(renderValue);
    }
}

void LoginScreen::drawErrorMessage(const String& message, uint16_t y) {
    tft->setTextSize(1);
    tft->setTextColor(WIN_ERROR, WIN_BG_DARK);
    tft->setCursor(20, y);
    tft->print(message);
}

void LoginScreen::updateUsernameInputArea(bool showErrorMsg) {
    // Clear input box region (accounting for header)
    const uint16_t headerHeight = 30;
    tft->fillRect(20, headerHeight + 30, 280, 34, WIN_BG_DARK);
    drawInputBox(20, headerHeight + 30, 280, 34, username, false);

    // Clear error area
    tft->fillRect(20, headerHeight + 96, 280, 12, WIN_BG_DARK);
    if (showErrorMsg) {
        drawErrorMessage("Username is required", headerHeight + 100);
    }
}

void LoginScreen::drawUsernameScreen() {
    drawBackground();

    // Header bar matching BuddyListScreen style
    const uint16_t headerHeight = 30;
    tft->fillRect(0, 0, 320, headerHeight, WIN_HEADER);
    tft->drawFastHLine(0, headerHeight - 1, 320, WIN_ACCENT);
    
    // Title in header
    tft->setTextSize(2);
    tft->setTextColor(WIN_TEXT, WIN_HEADER);
    tft->setCursor(10, 8);
    tft->print("LOGIN");

    // Label below header
    tft->setTextSize(1);
    tft->setTextColor(WIN_TEXT, WIN_BG_DARK);
    tft->setCursor(20, headerHeight + 12);
    tft->print("Username");

    drawInputBox(20, headerHeight + 30, 280, 34, username, false);

    tft->setTextSize(1);
    tft->setTextColor(WIN_TEXT, WIN_BG_DARK);
    tft->setCursor(20, headerHeight + 80);
    tft->print("Press Enter to continue");

    if (showUsernameEmpty) {
        drawErrorMessage("Username is required", headerHeight + 100);
    }

    ensureAlphabetMode();
    keyboard->moveCursorTo(usernameCursorRow, usernameCursorCol);
}

void LoginScreen::drawSuccessScreen() {
    drawBackground();
    
    // Header bar
    const uint16_t headerHeight = 30;
    tft->fillRect(0, 0, 320, headerHeight, WIN_HEADER);
    tft->drawFastHLine(0, headerHeight - 1, 320, WIN_ACCENT);
    
    // Title in header
    tft->setTextSize(2);
    tft->setTextColor(WIN_TEXT, WIN_HEADER);
    tft->setCursor(10, 8);
    tft->print("WELCOME");
    
    drawProfileCircle(60, headerHeight + 30, 24, WIN_SUCCESS);  // Minty Green for success

    tft->setTextColor(WIN_TEXT, WIN_BG_DARK);
    tft->setTextSize(2);
    tft->setCursor(100, headerHeight + 10);
    tft->print("Welcome,");
    tft->setCursor(100, headerHeight + 32);
    tft->print(username);

    tft->setTextSize(1);
    tft->setTextColor(WIN_SUCCESS, WIN_BG_DARK);
    tft->setCursor(20, headerHeight + 70);
    tft->print("Signed in successfully.");
    tft->setTextColor(WIN_TEXT, WIN_BG_DARK);
    tft->setCursor(20, headerHeight + 90);
    tft->print("Loading experience...");
}

void LoginScreen::draw() {
    switch (state) {
        case LOGIN_USERNAME:
            drawUsernameScreen();
            break;
        case LOGIN_PIN:
            pinScreen->draw();
            break;
        case LOGIN_SUCCESS:
            drawSuccessScreen();
            break;
    }
}

void LoginScreen::ensureAlphabetMode() {
    if (!keyboard->getIsAlphabetMode()) {
        keyboard->moveCursorTo(1, 0);  // "ABC" in numeric layout
        delay(120);
        keyboard->moveCursorByCommand("select", 0, 0);
        delay(120);
    }
}

void LoginScreen::goToUsernameStep() {
    state = LOGIN_USERNAME;
    showUsernameEmpty = false;
    drawUsernameScreen();
}

void LoginScreen::goToPinStep() {
    state = LOGIN_PIN;
    pinScreen->setUsername(username);
    pinScreen->reset();
    pinScreen->draw();
}

void LoginScreen::handleUsernameKey(const String& key) {
    usernameCursorRow = keyboard->getCursorRow();
    usernameCursorCol = keyboard->getCursorCol();
    if (key == "|e") {
        if (username.length() == 0) {
            showUsernameEmpty = true;
            updateUsernameInputArea(true);
        } else {
            goToPinStep();
        }
        return;
    }

    if (key == "<") {
        if (username.length() > 0) {
            username.remove(username.length() - 1);
            showUsernameEmpty = false;
            updateUsernameInputArea(false);
        }
        return;
    }

    if (key == "123" || key == "ABC" || key == Keyboard::KEY_ICON || key == "shift") {
        return;
    }

    if (username.length() < 18) {
        username += key;
        showUsernameEmpty = false;
        updateUsernameInputArea(false);
    }
}

void LoginScreen::handleKeyPress(const String& key) {
    if (state == LOGIN_USERNAME) {
        handleUsernameKey(key);
    } else if (state == LOGIN_PIN) {
        // Forward to PIN screen; PIN is assumed valid on Enter
        pinScreen->handleKeyPress(key);
        if (pinScreen->isPinAccepted()) {
            state = LOGIN_SUCCESS;
            drawSuccessScreen();
        } else if (pinScreen->wantsUsernameStep()) {
            goToUsernameStep();
        }
    }
}

