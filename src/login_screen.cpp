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
    this->nicknameScreen = new NicknameScreen(tft, keyboard);
    this->state = LOGIN_USERNAME;
    this->username = "";
    this->showUsernameEmpty = false;
    this->usernameCursorRow = 0;
    this->usernameCursorCol = 0;
    this->pendingUsername = "";
    this->pendingPin = "";
    this->dialogShowTime = 0;
    this->nicknameAutoSubmitTime = 0;
    this->userId = -1;
    this->friendsLoadedCallback = nullptr;
    this->friendsLoadedStringCallback = nullptr;
    this->notificationsLoadedCallback = nullptr;
    this->onLoginSuccessCallback = nullptr;
    
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
        case LOGIN_NICKNAME:
            if (nicknameScreen) {
                nicknameScreen->draw();
            }
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
                Serial.print("Login Screen: User ID saved: ");
                Serial.println(userId);
                Serial.print("Login Screen: Username: ");
                Serial.println(username);
                
                // Load friends list
                loadFriends();
                
                // Load notifications
                loadNotifications();
                
                // Call login success callback
                if (onLoginSuccessCallback != nullptr) {
                    onLoginSuccessCallback();
                }
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
    } else if (state == LOGIN_NICKNAME) {
        // Forward to nickname screen
        if (nicknameScreen != nullptr) {
            nicknameScreen->handleKeyPress(key);
            
            // Check if nickname was confirmed
            if (nicknameScreen->isConfirmed()) {
                String enteredNickname = nicknameScreen->getNickname();
                Serial.println("Login Screen: Nickname confirmed, creating account...");
                Serial.print("Login Screen: Nickname: ");
                Serial.println(enteredNickname);
                
                // Create account with pending username and PIN
                if (pendingUsername.length() > 0 && pendingPin.length() > 0) {
                    if (ApiClient::createAccount(pendingUsername, pendingPin, enteredNickname, "192.168.1.7", 8080)) {
                        Serial.println("Login Screen: Account created successfully!");
                        
                        // Save nickname
                        if (nicknameScreen->saveNickname(enteredNickname)) {
                            Serial.println("Login Screen: Nickname saved successfully!");
                        } else {
                            Serial.println("Login Screen: Failed to save nickname!");
                        }
                        
                        // Login to get user ID
                        ApiClient::LoginResult loginResult = ApiClient::checkLogin(pendingUsername, pendingPin, "192.168.1.7", 8080);
                        if (loginResult.success) {
                            userId = loginResult.user_id;
                            username = pendingUsername;
                            state = LOGIN_SUCCESS;
                            drawSuccessScreen();
                            Serial.println("Login Screen: Auto-login successful after account creation!");
                            Serial.print("Login Screen: User ID: ");
                            Serial.println(userId);
                            
                            // Load friends list
                            loadFriends();
                            
                            // Load notifications
                            loadNotifications();
                            
                            // Call login success callback
                            if (onLoginSuccessCallback != nullptr) {
                                onLoginSuccessCallback();
                            }
                        } else {
                            Serial.println("Login Screen: Failed to login after account creation!");
                            // Still show success screen even if login fails
                            username = pendingUsername;
                            state = LOGIN_SUCCESS;
                            drawSuccessScreen();
                        }
                    } else {
                        Serial.println("Login Screen: Account creation failed!");
                        // Reset nickname screen and go back to username step
                        nicknameScreen->reset();
                        goToUsernameStep();
                    }
                } else {
                    Serial.println("Login Screen: No pending account info!");
                    nicknameScreen->reset();
                    goToUsernameStep();
                }
            }
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

// Helper function to generate nickname from username
String LoginScreen::generateNicknameFromUsername(const String& username) {
    if (username.length() == 0) {
        return "Player";
    }
    
    String nickname = "";
    // Capitalize first letter
    if (username.length() > 0) {
        char firstChar = username.charAt(0);
        if (firstChar >= 'a' && firstChar <= 'z') {
            nickname += (char)(firstChar - 32);  // Convert to uppercase
        } else {
            nickname += firstChar;
        }
        
        // Add rest of username
        for (int i = 1; i < username.length(); i++) {
            nickname += username.charAt(i);
        }
    }
    
    return nickname;
}

// Instance callback methods for account creation
void LoginScreen::onCreateAccountConfirm() {
    Serial.println("Login Screen: User confirmed account creation");
    
    if (pendingUsername.length() > 0 && pendingPin.length() > 0) {
        // Hide dialog
        confirmationDialog->hide();
        
        // Reset nickname screen and transition to nickname input
        Serial.println("Login Screen: Showing nickname screen for new account...");
        if (nicknameScreen != nullptr) {
            nicknameScreen->reset();
            
            // Don't auto-fill nickname - let user enter it themselves
            state = LOGIN_NICKNAME;
            nicknameScreen->draw();
            
            // Set timestamp for auto-submit (after 1.5 seconds)
            nicknameAutoSubmitTime = millis() + 1500;
        } else {
            Serial.println("Login Screen: NicknameScreen not initialized!");
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
        Serial.print("Login Screen: Current userId: ");
        Serial.println(userId);
        return;
    }
    
    Serial.println("Login Screen: Loading friends list from database (string format)...");
    Serial.print("Login Screen: Using user_id: ");
    Serial.println(userId);
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

// Load notifications after successful login
void LoginScreen::loadNotifications() {
    if (userId <= 0) {
        Serial.println("Login Screen: Cannot load notifications - invalid user ID");
        return;
    }
    
    Serial.println("Login Screen: Loading notifications from API...");
    Serial.print("Login Screen: Using user_id: ");
    Serial.println(userId);
    
    ApiClient::NotificationsResult notificationsResult = ApiClient::getNotifications(userId, "192.168.1.7", 8080);
    
    if (notificationsResult.success) {
        Serial.print("Login Screen: Received ");
        Serial.print(notificationsResult.count);
        Serial.println(" notifications");
        
        // Log notification details
        for (int i = 0; i < notificationsResult.count; i++) {
            Serial.print("  Notification ");
            Serial.print(i);
            Serial.print(": ");
            Serial.println(notificationsResult.notifications[i].message);
        }
        
        // Call callback with notification count
        if (notificationsLoadedCallback != nullptr) {
            notificationsLoadedCallback(notificationsResult.count);
        }
        
        // Clean up memory
        if (notificationsResult.notifications != nullptr) {
            delete[] notificationsResult.notifications;
        }
    } else {
        Serial.print("Login Screen: Failed to load notifications: ");
        Serial.println(notificationsResult.message);
    }
}

