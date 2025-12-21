#include "nickname_screen.h"

// Midnight Blue Arcade Theme (matching Buddy List Screen)
#define NICKNAME_BG_DARK   0x0042  // Deep Midnight Blue #020817
#define NICKNAME_TEXT      0xFFFF  // White text
#define NICKNAME_MUTED     0xC618  // Muted gray text
#define NICKNAME_ERROR     0xF800  // Red for errors
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
    // Deep Midnight Blue background
    uint16_t width = 320;
    uint16_t height = 240;
    tft->fillRect(0, 0, width, height, NICKNAME_BG_DARK);
}

void NicknameScreen::drawInputBox(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const String& value) {
    tft->fillRoundRect(x, y, w, h, 6, ST77XX_BLACK);
    tft->drawRoundRect(x, y, w, h, 6, NICKNAME_TEXT);
    tft->setCursor(x + 8, y + (h / 2) - 6);
    tft->setTextSize(2);
    tft->setTextColor(NICKNAME_TEXT, ST77XX_BLACK);

    if (value.length() == 0) {
        tft->setTextColor(NICKNAME_MUTED, ST77XX_BLACK);
        tft->print("Enter your nickname");
    } else {
        tft->setTextColor(NICKNAME_TEXT, ST77XX_BLACK);
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
    // Clear input box region
    tft->fillRect(20, 30, 280, 34, NICKNAME_BG_DARK);
    drawInputBox(20, 30, 280, 34, nickname);

    // Clear error area
    tft->fillRect(20, 96, 280, 24, NICKNAME_BG_DARK);
    if (showErrorMsg) {
        if (showNameEmpty) {
            drawErrorMessage("Nickname is required", 100);
        } else if (showNameTooLong) {
            drawErrorMessage("Nickname too long (max 20 chars)", 100);
        }
    }
}

void NicknameScreen::draw() {
    drawBackground();

    // Title
    tft->setTextSize(2);
    tft->setTextColor(NICKNAME_TEXT, NICKNAME_BG_DARK);
    tft->setCursor(20, 12);
    tft->print("Create Nickname");

    drawInputBox(20, 50, 280, 34, nickname);

    // Instructions
    tft->setTextSize(1);
    tft->setTextColor(NICKNAME_TEXT, NICKNAME_BG_DARK);
    tft->setCursor(20, 100);
    tft->print("Press Enter to save");

    // Show current nickname if exists
    if (hasNickname()) {
        String currentNick = loadNickname();
        tft->setTextSize(1);
        tft->setTextColor(NICKNAME_MUTED, NICKNAME_BG_DARK);
        tft->setCursor(20, 120);
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

