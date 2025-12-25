#include "pin_screen.h"

// Deep Space Arcade Theme (matching Buddy List Screen)
#define WIN_BG_DARK   0x0042  // Deep Midnight Blue #020817
#define WIN_HEADER    0x08A5  // Header Blue #0F172A
#define WIN_ACCENT    0x07FF  // Cyan accent
#define WIN_TEXT      0xFFFF  // White text
#define WIN_MUTED     0x8410  // Muted gray text
#define WIN_ERROR     0xF986  // Bright Red #FF3333

PinScreen::PinScreen(Adafruit_ST7789* tft, Keyboard* keyboard) {
    this->tft = tft;
    this->keyboard = keyboard;
    this->username = "";
    this->pinInput = "";
    this->expectedPin = "1234";
    this->showError = false;
    this->pinAccepted = false;
    this->backToUsername = false;
    this->pinCursorRow = 0;
    this->pinCursorCol = 0;
}

void PinScreen::reset() {
    pinInput = "";
    showError = false;
    pinAccepted = false;
    backToUsername = false;
    pinCursorRow = 0;
    pinCursorCol = 0;
}

void PinScreen::drawBackground() {
    // Clear entire screen to prevent black stripe artifacts
    tft->fillScreen(WIN_BG_DARK);
}

void PinScreen::drawProfileCircle(uint16_t cx, uint16_t cy, uint16_t r, uint16_t color) {
    tft->fillCircle(cx, cy, r, color);
    tft->drawCircle(cx, cy, r, WIN_ACCENT);  // Cyan border
    tft->drawCircle(cx, cy, r - 2, WIN_ACCENT);
}

void PinScreen::drawInputBox(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const String& value, bool masked) {
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
            renderValue += "*";
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

void PinScreen::drawErrorMessage(const String& message, uint16_t y) {
    tft->setTextSize(1);
    tft->setTextColor(WIN_ERROR, WIN_BG_DARK);
    tft->setCursor(20, y);
    tft->print(message);
}

void PinScreen::updatePinInputArea(bool showErrorMsg) {
    // Clear input box region (accounting for header)
    const uint16_t headerHeight = 30;
    tft->fillRect(20, headerHeight + 10, 280, 34, WIN_BG_DARK);
    drawInputBox(20, headerHeight + 10, 280, 34, pinInput, true);

    // Clear error area
    tft->fillRect(20, headerHeight + 65, 280, 12, WIN_BG_DARK);
    if (showErrorMsg) {
        drawErrorMessage("PIN is required or incorrect", headerHeight + 69);
    }
}

void PinScreen::drawPinScreen() {
    drawBackground();

    // Header bar matching BuddyListScreen style
    const uint16_t headerHeight = 30;
    tft->fillRect(0, 0, 320, headerHeight, WIN_HEADER);
    tft->drawFastHLine(0, headerHeight - 1, 320, WIN_ACCENT);
    
    // Title in header
    tft->setTextSize(2);
    tft->setTextColor(WIN_TEXT, WIN_HEADER);
    tft->setCursor(10, 8);
    tft->print("PIN");

    // Label below header
    tft->setTextSize(1);
    tft->setTextColor(WIN_TEXT, WIN_BG_DARK);
    tft->setCursor(20, headerHeight + 12);
    tft->print("Enter your PIN");

    drawInputBox(20, headerHeight + 10, 280, 34, pinInput, true);

    tft->setTextSize(1);
    tft->setTextColor(WIN_TEXT, WIN_BG_DARK);
    tft->setCursor(20, headerHeight + 50);
    tft->print("Press Enter to continue");

    if (showError) {
        drawErrorMessage("PIN is required or incorrect", headerHeight + 69);
    }

    // Draw and focus keyboard for navigation across PIN buttons
    keyboard->draw();
    ensureNumericMode();
    keyboard->moveCursorTo(pinCursorRow, pinCursorCol);
}

void PinScreen::draw() {
    drawPinScreen();
}

void PinScreen::ensureNumericMode() {
    if (keyboard->getIsAlphabetMode()) {
        keyboard->moveCursorTo(1, 0);
        delay(120);
        keyboard->moveCursorByCommand("select", 0, 0);
        delay(120);
    }
}

void PinScreen::handleKeyPress(const String& key) {
    pinCursorRow = keyboard->getCursorRow();
    pinCursorCol = keyboard->getCursorCol();
    pinAccepted = false;
    backToUsername = false;

    if (key == "|e") {
        // Accept PIN immediately (task: assume PIN is valid)
        pinAccepted = true;
        showError = false;
        updatePinInputArea(false);
        return;
    }

    if (key == "<") {
        if (pinInput.length() > 0) {
            pinInput.remove(pinInput.length() - 1);
            updatePinInputArea(showError);
        } else {
            backToUsername = true;
        }
        return;
    }

    if (key == "ABC") {
        backToUsername = true;
        return;
    }

    if (key == "123" || key == Keyboard::KEY_ICON || key == "shift") {
        return;
    }

    if (key.length() == 1 && key.charAt(0) >= '0' && key.charAt(0) <= '9') {
        if (pinInput.length() < 6) {
            pinInput += key;
            showError = false;
            updatePinInputArea(false);
        }
    }
}


