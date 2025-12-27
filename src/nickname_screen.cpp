#include "nickname_screen.h"

// Deep Space Arcade Theme (matching Buddy List Screen)
#define NICKNAME_BG_DARK   0x0042  // Deep Midnight Blue #020817
#define NICKNAME_HEADER    0x08A5  // Header Blue #0F172A
#define NICKNAME_TEXT      0xFFFF  // White text
#define NICKNAME_CYAN      0x07FF  // Cyan accent
#define NICKNAME_MUTED     0x8410  // Muted gray text (lighter for visibility)
#define NICKNAME_ERROR     0xF986  // Bright Red #FF3333
#define NICKNAME_SUCCESS   0x07F3  // Minty Green for success

const char* NicknameScreen::NICKNAME_FILE_PATH = "/nickname.txt";

NicknameScreen::NicknameScreen(Adafruit_ST7789* tft, Keyboard* keyboard) {
    this->tft = tft;
    this->keyboard = keyboard;
    this->nickname = "";
    this->showNameEmpty = false;
    this->showNameTooLong = false;
    this->confirmed = false;
    this->cancelled = false;
    this->cursorRow = 0;
    this->cursorCol = 0;

    // Initialize SPIFFS if not already initialized
    if (!SPIFFS.begin(true)) {
        Serial.println("Nickname: SPIFFS initialization failed!");
    } else {
        Serial.println("Nickname: SPIFFS initialized successfully");
    }
}

void NicknameScreen::drawBackground() {
    // Clear entire screen to prevent black stripe artifacts
    tft->fillScreen(NICKNAME_BG_DARK);
}

void NicknameScreen::drawInputBox(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const String& value) {
    // Background with Deep Midnight Blue
    tft->fillRoundRect(x, y, w, h, 6, NICKNAME_BG_DARK);
    // Cyan border for accent
    tft->drawRoundRect(x, y, w, h, 6, NICKNAME_CYAN);
    tft->setCursor(x + 8, y + (h / 2) - 6);
    tft->setTextSize(2);
    tft->setTextColor(NICKNAME_TEXT, NICKNAME_BG_DARK);

    if (value.length() == 0) {
        tft->setTextColor(NICKNAME_MUTED, NICKNAME_BG_DARK);
        tft->print("Enter your nickname");
    } else {
        tft->setTextColor(NICKNAME_TEXT, NICKNAME_BG_DARK);
        tft->print(value);
    }
}

void NicknameScreen::drawErrorMessage(const String& message, uint16_t y) {
    tft->setTextSize(1);
    tft->setTextColor(NICKNAME_ERROR, NICKNAME_BG_DARK);
    tft->setCursor(20, y);
    tft->print(message);
}

void NicknameScreen::updateInputArea(bool showErrorMsg) {
    // Clear input box region (accounting for header)
    const uint16_t headerHeight = 30;
    tft->fillRect(20, headerHeight + 10, 280, 34, NICKNAME_BG_DARK);
    drawInputBox(20, headerHeight + 10, 280, 34, nickname);

    // Clear error area
    tft->fillRect(20, headerHeight + 65, 280, 24, NICKNAME_BG_DARK);
    if (showErrorMsg) {
        if (showNameEmpty) {
            drawErrorMessage("Nickname is required", headerHeight + 69);
        } else if (showNameTooLong) {
            drawErrorMessage("Nickname too long (max 20 chars)", headerHeight + 69);
        }
    }
}

void NicknameScreen::draw() {
    drawBackground();

    // Header bar matching BuddyListScreen style
    const uint16_t headerHeight = 30;
    tft->fillRect(0, 0, 320, headerHeight, NICKNAME_HEADER);
    tft->drawFastHLine(0, headerHeight - 1, 320, NICKNAME_CYAN);
    
    // Title in header
    tft->setTextSize(2);
    tft->setTextColor(NICKNAME_TEXT, NICKNAME_HEADER);
    tft->setCursor(10, 8);
    tft->print("NICKNAME");

    drawInputBox(20, headerHeight + 10, 280, 34, nickname);

    // Instructions
    tft->setTextSize(1);
    tft->setTextColor(NICKNAME_TEXT, NICKNAME_BG_DARK);
    tft->setCursor(20, headerHeight + 50);
    tft->print("Press Enter to save");

    // Show current nickname if exists
    if (hasNickname()) {
        String currentNick = loadNickname();
        tft->setTextSize(1);
        tft->setTextColor(NICKNAME_MUTED, NICKNAME_BG_DARK);
        tft->setCursor(20, headerHeight + 65);
        tft->print("Current: ");
        tft->setTextColor(NICKNAME_SUCCESS, NICKNAME_BG_DARK);
        tft->print(currentNick);
    }

    if (showNameEmpty || showNameTooLong) {
        updateInputArea(true);
    }

    ensureAlphabetMode();
    keyboard->moveCursorTo(cursorRow, cursorCol);
}

void NicknameScreen::ensureAlphabetMode() {
    if (!keyboard->getIsAlphabetMode()) {
        keyboard->moveCursorTo(1, 0);  // "ABC" in numeric layout
        delay(120);
        keyboard->moveCursorByCommand("select", 0, 0);
        delay(120);
    }
}

void NicknameScreen::handleNameKey(const String& key) {
    cursorRow = keyboard->getCursorRow();
    cursorCol = keyboard->getCursorCol();
    
    if (key == "|e") {
        // Enter key - confirm if valid, show error if empty
        if (nickname.length() == 0) {
            showNameEmpty = true;
            showNameTooLong = false;
            updateInputArea(true);
        } else if (nickname.length() > 20) {
            showNameEmpty = false;
            showNameTooLong = true;
            updateInputArea(true);
        } else {
            // Save nickname to file
            if (saveNickname(nickname)) {
                confirmed = true;
                Serial.print("Nickname: Saved nickname: ");
                Serial.println(nickname);
            } else {
                Serial.println("Nickname: Failed to save nickname!");
                // Still confirm even if save failed (user can retry)
                confirmed = true;
            }
        }
        return;
    }

    if (key == "<") {
        // Delete key - remove last character
        if (nickname.length() > 0) {
            nickname.remove(nickname.length() - 1);
            showNameEmpty = false;
            showNameTooLong = false;
            updateInputArea(false);
        }
        return;
    }

    // Ignore mode switch keys
    if (key == "123" || key == "ABC" || key == Keyboard::KEY_ICON || key == "shift") {
        return;
    }

    // Add character if under limit
    if (nickname.length() < 20) {
        nickname += key;
        showNameEmpty = false;
        showNameTooLong = false;
        updateInputArea(false);
    } else {
        // Show error if trying to type beyond limit
        showNameTooLong = true;
        updateInputArea(true);
    }
}

void NicknameScreen::handleKeyPress(const String& key) {
    handleNameKey(key);
}

void NicknameScreen::reset() {
    nickname = "";
    showNameEmpty = false;
    showNameTooLong = false;
    confirmed = false;
    cancelled = false;
    cursorRow = 0;
    cursorCol = 0;
}

void NicknameScreen::setNickname(const String& name) {
    // Limit to 20 characters
    if (name.length() > 20) {
        nickname = name.substring(0, 20);
    } else {
        nickname = name;
    }
    showNameEmpty = false;
    showNameTooLong = false;
    // Update display
    updateInputArea(false);
}

bool NicknameScreen::hasNickname() const {
    return SPIFFS.exists(NICKNAME_FILE_PATH);
}

String NicknameScreen::loadNickname() {
    if (!SPIFFS.exists(NICKNAME_FILE_PATH)) {
        return "";
    }

    File file = SPIFFS.open(NICKNAME_FILE_PATH, "r");
    if (!file) {
        Serial.println("Nickname: Failed to open file for reading");
        return "";
    }

    String loadedNickname = file.readStringUntil('\n');
    loadedNickname.trim();
    file.close();

    Serial.print("Nickname: Loaded nickname: ");
    Serial.println(loadedNickname);
    return loadedNickname;
}

bool NicknameScreen::saveNickname(const String& name) {
    File file = SPIFFS.open(NICKNAME_FILE_PATH, "w");
    if (!file) {
        Serial.println("Nickname: Failed to open file for writing");
        return false;
    }

    file.println(name);
    file.close();

    Serial.print("Nickname: Saved nickname to file: ");
    Serial.println(name);
    return true;
}

