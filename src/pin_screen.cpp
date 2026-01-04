#include "pin_screen.h"

// Deep Space Arcade Theme (matching Buddy List Screen)
#define WIN_BG_DARK   0x0042  // Deep Midnight Blue #020817
#define WIN_HEADER    0x08A5  // Header Blue #0F172A
#define WIN_INPUT_BG  0x0021  // Darker Blue for input box fill
#define WIN_ACCENT    0x07FF  // Cyan accent
#define WIN_TEXT      0xFFFF  // White text
#define WIN_MUTED     0x8410  // Muted gray text
#define WIN_ERROR     0xF986  // Bright Red #FF3333

PinScreen::PinScreen(Adafruit_ST7789* tft, Keyboard* keyboard) {
    this->tft = tft;
    this->keyboard = keyboard;
    this->username = "";
    this->pinInput = "";
    this->expectedPin = "";  // No longer used - PIN verification is done via API against database
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
    // Draw filled rounded rectangle input card with Deep Space theme
    // Background: Darker Blue for input box
    tft->fillRoundRect(x, y, w, h, 6, WIN_INPUT_BG);
    // Cyan border for accent
    tft->drawRoundRect(x, y, w, h, 6, WIN_ACCENT);
    
    // Prepare text rendering
    tft->setTextSize(2);
    
    // Calculate text to display
    String displayText = value;
    bool showPlaceholder = (value.length() == 0);
    
    if (masked && value.length() > 0) {
        displayText = "";
        for (uint16_t i = 0; i < value.length(); i++) {
            displayText += "*";
        }
        showPlaceholder = false;
    } else if (value.length() == 0) {
        displayText = masked ? "Enter PIN" : "Enter username";
    }
    
    // Calculate text width for centering (approximate: text size 2 = ~12px per char)
    int textWidth = displayText.length() * 12;
    uint16_t textX = x + (w - textWidth) / 2;
    uint16_t textY = y + (h / 2) - 6;  // Center vertically (text size 2 is ~12px tall)
    
    // Set text color
    if (showPlaceholder) {
        tft->setTextColor(WIN_MUTED, WIN_INPUT_BG);
    } else {
        tft->setTextColor(WIN_TEXT, WIN_INPUT_BG);
    }
    
    // Draw text centered
    tft->setCursor(textX, textY);
    tft->print(displayText);
}

void PinScreen::drawErrorMessage(const String& message, uint16_t y) {
    tft->setTextSize(1);
    tft->setTextColor(WIN_ERROR, WIN_BG_DARK);
    // Center error message horizontally
    uint16_t msgWidth = message.length() * 6;  // Approximate width for text size 1
    uint16_t msgX = (320 - msgWidth) / 2;
    tft->setCursor(msgX, y);
    tft->print(message);
}

void PinScreen::updatePinInputArea(bool showErrorMsg) {
    // Clear input box region (using new positioning at Y = 45)
    const uint16_t inputBoxY = 45;
    const uint16_t inputBoxW = 280;
    const uint16_t inputBoxH = 40;
    uint16_t inputBoxX = (320 - inputBoxW) / 2;
    
    tft->fillRect(inputBoxX, inputBoxY, inputBoxW, inputBoxH, WIN_INPUT_BG);
    drawInputBox(inputBoxX, inputBoxY, inputBoxW, inputBoxH, pinInput, true);

    // Clear error area (positioned safely above keyboard)
    tft->fillRect(inputBoxX, inputBoxY + inputBoxH + 8, inputBoxW, 12, WIN_BG_DARK);
    if (showErrorMsg) {
        drawErrorMessage("PIN is required or incorrect", inputBoxY + inputBoxH + 8);
    }
}

void PinScreen::drawPinScreen() {
    drawBackground();

    // Header bar matching Deep Space theme
    const uint16_t headerHeight = 30;
    tft->fillRect(0, 0, 320, headerHeight, WIN_HEADER);
    tft->drawFastHLine(0, headerHeight - 1, 320, WIN_ACCENT);
    
    // Title in header (centered)
    tft->setTextSize(2);
    tft->setTextColor(WIN_TEXT, WIN_HEADER);
    String title = "PIN";
    uint16_t titleWidth = title.length() * 12;  // Approximate width for text size 2
    uint16_t titleX = (320 - titleWidth) / 2;   // Center horizontally
    tft->setCursor(titleX, 8);
    tft->print(title);

    // Position input box at Y = 45 (ensures no overlap with keyboard at Y ~ 115)
    // Input box ends at Y = 85, leaving 30px safe space before keyboard
    const uint16_t inputBoxY = 45;
    const uint16_t inputBoxW = 280;
    const uint16_t inputBoxH = 40;  // Chunky, touch-friendly height
    
    // Label above input box (muted color)
    tft->setTextSize(1);
    tft->setTextColor(WIN_MUTED, WIN_BG_DARK);
    String label = "Enter your PIN";
    uint16_t labelWidth = label.length() * 6;  // Approximate width for text size 1
    uint16_t labelX = (320 - labelWidth) / 2;   // Center horizontally
    tft->setCursor(labelX, inputBoxY - 12);
    tft->print(label);

    // Draw input box (centered horizontally)
    uint16_t inputBoxX = (320 - inputBoxW) / 2;
    drawInputBox(inputBoxX, inputBoxY, inputBoxW, inputBoxH, pinInput, true);

    // Error message if needed (positioned safely above keyboard)
    if (showError) {
        drawErrorMessage("PIN is required or incorrect", inputBoxY + inputBoxH + 8);
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

// Navigation handlers (delegate to keyboard for navigation)
void PinScreen::handleUp() {
    if (keyboard != nullptr) {
        keyboard->moveCursorByCommand("up", 0, 0);
    }
}

void PinScreen::handleDown() {
    if (keyboard != nullptr) {
        keyboard->moveCursorByCommand("down", 0, 0);
    }
}

void PinScreen::handleLeft() {
    if (keyboard != nullptr) {
        keyboard->moveCursorByCommand("left", 0, 0);
    }
}

void PinScreen::handleRight() {
    if (keyboard != nullptr) {
        keyboard->moveCursorByCommand("right", 0, 0);
    }
}

void PinScreen::handleSelect() {
    // Select means type the selected character from keyboard
    if (keyboard != nullptr) {
        String currentChar = keyboard->getCurrentChar();
        if (currentChar.length() > 0) {
            if (currentChar == "<") {
                // Delete key - handle as exit
                handleExit();
            } else {
                // Use moveCursorByCommand to trigger the key selection
                keyboard->moveCursorByCommand("select", 0, 0);
            }
        }
    }
}

void PinScreen::handleExit() {
    // Exit - delete last character (backspace behavior)
    if (pinInput.length() > 0) {
        pinInput.remove(pinInput.length() - 1);
        showError = false;
        updatePinInputArea(false);
    } else {
        backToUsername = true;
    }
}

void PinScreen::handleKeyPress(const String& key) {
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
    
    pinCursorRow = keyboard->getCursorRow();
    pinCursorCol = keyboard->getCursorCol();
    pinAccepted = false;
    backToUsername = false;

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
        // Enter key - accept PIN immediately
        pinAccepted = true;
        showError = false;
        updatePinInputArea(false);
        return;
    } else if (key == "<" || key == "|b") {
        handleExit();
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


