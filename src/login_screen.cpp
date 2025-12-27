#include "login_screen.h"
#include "api_client.h"

// Static instance pointer for callbacks
LoginScreen* LoginScreen::instanceForCallback = nullptr;

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
    this->confirmationDialog = new ConfirmationDialog(tft);
    this->state = LOGIN_USERNAME;
    this->username = "";
    this->showUsernameEmpty = false;
    this->usernameCursorRow = 0;
    this->usernameCursorCol = 0;
    this->pendingUsername = "";
    this->pendingPin = "";
    this->dialogShowTime = 0;
    this->userId = -1;
    this->friendsLoadedCallback = nullptr;
    this->friendsLoadedStringCallback = nullptr;
    
    // Set static instance for callbacks
    instanceForCallback = this;
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
    tft->fillRect(20, headerHeight + 10, 280, 34, WIN_BG_DARK);
    drawInputBox(20, headerHeight + 10, 280, 34, username, false);

    // Clear error area
    tft->fillRect(20, headerHeight + 65, 280, 12, WIN_BG_DARK);
    if (showErrorMsg) {
        drawErrorMessage("Username is required", headerHeight + 69);
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

    drawInputBox(20, headerHeight + 10, 280, 34, username, false);

    tft->setTextSize(1);
    tft->setTextColor(WIN_TEXT, WIN_BG_DARK);
    tft->setCursor(20, headerHeight + 50);
    tft->print("Press Enter to continue");

    if (showUsernameEmpty) {
        drawErrorMessage("Username is required", headerHeight + 69);
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
        case LOGIN_SHOWING_DIALOG:
            // Draw the underlying screen first
            if (pinScreen) {
                pinScreen->draw();
            }
            break;
    }
    
    // Always draw dialog if visible (on top of everything)
    if (confirmationDialog && confirmationDialog->isVisible()) {
        confirmationDialog->draw();
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

void LoginScreen::resetToUsernameStep() {
    goToUsernameStep();
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
            // PIN entered, try to login via API
            String userPin = pinScreen->getPin();
            Serial.println("Login Screen: PIN entered, verifying with API...");
            
            ApiClient::LoginResult loginResult = ApiClient::checkLogin(username, userPin, "192.168.1.7", 8080);
            
            if (loginResult.success) {
                // Login successful
                userId = loginResult.user_id;
                state = LOGIN_SUCCESS;
                drawSuccessScreen();
                Serial.println("Login Screen: API verification successful!");
                
                // Load friends list
                loadFriends();
            } else {
                // Login failed
                if (!loginResult.accountExists) {
                    // Account doesn't exist - show confirmation dialog
                    Serial.println("Login Screen: Account not found, showing create account dialog...");
                    pendingUsername = username;
                    pendingPin = userPin;
                    
                    String dialogMessage = "Account not found.\nCreate new account with\nusername '" + username + "'\nand PIN '" + userPin + "'?";
                    confirmationDialog->show(
                        dialogMessage,
                        "YES",
                        "NO",
                        staticOnCreateAccountConfirm,
                        staticOnCreateAccountCancel,
                        WIN_ACCENT
                    );
                    state = LOGIN_SHOWING_DIALOG;
                    dialogShowTime = millis();  // Record when dialog was shown
                    draw();  // Redraw to show dialog
                } else {
                    // Account exists but PIN is wrong
                    Serial.println("Login Screen: Wrong PIN!");
                    pinScreen->reset();
                    // Show error and go back to PIN step
                    goToPinStep();
                }
            }
        } else if (pinScreen->wantsUsernameStep()) {
            goToUsernameStep();
        }
    } else if (state == LOGIN_SHOWING_DIALOG) {
        // Handle dialog navigation
        if (confirmationDialog && confirmationDialog->isVisible()) {
            if (key == "<") {
                // Back key - cancel
                confirmationDialog->handleCancel();
            } else if (key == "|e") {
                // Enter - select current button
                confirmationDialog->handleSelect();
            } else if (key == "|l") {
                // Left arrow - move to YES (Confirm)
                confirmationDialog->handleLeft();
            } else if (key == "|r") {
                // Right arrow - move to NO (Cancel)
                confirmationDialog->handleRight();
            }
        }
    }
}

// Static callback wrappers for ConfirmationDialog
void LoginScreen::staticOnCreateAccountConfirm() {
    if (instanceForCallback != nullptr) {
        instanceForCallback->onCreateAccountConfirm();
    }
}

void LoginScreen::staticOnCreateAccountCancel() {
    if (instanceForCallback != nullptr) {
        instanceForCallback->onCreateAccountCancel();
    }
}

// Instance callback methods for account creation
void LoginScreen::onCreateAccountConfirm() {
    Serial.println("Login Screen: User confirmed account creation");
    
    if (pendingUsername.length() > 0 && pendingPin.length() > 0) {
        // Call API to create account
        Serial.println("Login Screen: Creating account...");
        Serial.print("Login Screen: Username: ");
        Serial.println(pendingUsername);
        Serial.print("Login Screen: PIN: ");
        Serial.println(pendingPin);
        
        if (ApiClient::createAccount(pendingUsername, pendingPin, "192.168.1.7", 8080)) {
            Serial.println("Login Screen: Account created successfully!");
            // Update username to the newly created account
            username = pendingUsername;
            // Hide dialog
            confirmationDialog->hide();
            // Auto-login with new account
            state = LOGIN_SUCCESS;
            drawSuccessScreen();
            Serial.println("Login Screen: Showing success screen for new account!");
        } else {
            Serial.println("Login Screen: Account creation failed!");
            // Show error and go back to username step
            confirmationDialog->hide();
            goToUsernameStep();
        }
    } else {
        Serial.println("Login Screen: No pending account info!");
        confirmationDialog->hide();
        goToUsernameStep();
    }
}

void LoginScreen::onCreateAccountCancel() {
    Serial.println("Login Screen: User cancelled account creation");
    confirmationDialog->hide();
    dialogShowTime = 0;  // Reset timer
    // Go back to username step
    goToUsernameStep();
}

// Trigger confirm from outside (for auto-confirm in main loop)
void LoginScreen::triggerConfirm() {
    if (state == LOGIN_SHOWING_DIALOG && 
        confirmationDialog != nullptr && 
        confirmationDialog->isVisible()) {
        Serial.println("Login Screen: Triggering confirm from main loop...");
        onCreateAccountConfirm();
    }
}

// Trigger left navigation (for auto-navigation testing)
void LoginScreen::triggerNavigateLeft() {
    if (state == LOGIN_SHOWING_DIALOG && 
        confirmationDialog != nullptr && 
        confirmationDialog->isVisible()) {
        Serial.println("Login Screen: Triggering left navigation (NO -> YES)...");
        confirmationDialog->handleLeft();
    }
}

// Trigger right navigation (for auto-navigation testing)
void LoginScreen::triggerNavigateRight() {
    if (state == LOGIN_SHOWING_DIALOG && 
        confirmationDialog != nullptr && 
        confirmationDialog->isVisible()) {
        Serial.println("Login Screen: Triggering right navigation (YES -> NO)...");
        confirmationDialog->handleRight();
    }
}

// Trigger select (for auto-navigation testing)
void LoginScreen::triggerSelect() {
    if (state == LOGIN_SHOWING_DIALOG && 
        confirmationDialog != nullptr && 
        confirmationDialog->isVisible()) {
        Serial.println("Login Screen: Triggering select...");
        confirmationDialog->handleSelect();
    }
}

// Update method - now empty, logic moved to main loop
void LoginScreen::update() {
    // Auto-confirm logic moved to main loop
}

// Load friends list after successful login
void LoginScreen::loadFriends() {
    if (userId <= 0) {
        Serial.println("Login Screen: Cannot load friends - invalid user ID");
        return;
    }
    
    Serial.println("Login Screen: Loading friends list from database (string format)...");
    String friendsString = ApiClient::getFriendsList(userId, "192.168.1.7", 8080);
    
    if (friendsString.length() > 0) {
        Serial.print("Login Screen: Received friends string: ");
        Serial.println(friendsString);
        
        // Call callback with string - BuddyListScreen will parse it
        if (friendsLoadedStringCallback != nullptr) {
            friendsLoadedStringCallback(friendsString);
        }
    } else {
        Serial.println("Login Screen: Failed to load friends or empty string");
        // Still call callback with empty string to show buddy list screen
        if (friendsLoadedStringCallback != nullptr) {
            friendsLoadedStringCallback("");
        }
    }
}

