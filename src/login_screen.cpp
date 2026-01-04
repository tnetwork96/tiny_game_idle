#include "login_screen.h"
#include "api_client.h"

// Static instance pointer for callbacks
LoginScreen* LoginScreen::instanceForCallback = nullptr;

// Deep Space Arcade Theme (matching Buddy List Screen)
#define WIN_BG_DARK   0x0042  // Deep Midnight Blue #020817
#define WIN_HEADER    0x08A5  // Header Blue #0F172A
#define WIN_INPUT_BG  0x0021  // Darker Blue for input box fill
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
    this->nickname = "";
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
            displayText += "*";  // Use asterisk to keep ASCII while showing dots
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

void LoginScreen::drawErrorMessage(const String& message, uint16_t y) {
    tft->setTextSize(1);
    tft->setTextColor(WIN_ERROR, WIN_BG_DARK);
    // Center error message horizontally
    uint16_t msgWidth = message.length() * 6;  // Approximate width for text size 1
    uint16_t msgX = (320 - msgWidth) / 2;
    tft->setCursor(msgX, y);
    tft->print(message);
}

void LoginScreen::updateUsernameInputArea(bool showErrorMsg) {
    // Clear input box region (using new positioning at Y = 45)
    const uint16_t inputBoxY = 45;
    const uint16_t inputBoxW = 280;
    const uint16_t inputBoxH = 40;
    uint16_t inputBoxX = (320 - inputBoxW) / 2;
    
    tft->fillRect(inputBoxX, inputBoxY, inputBoxW, inputBoxH, WIN_INPUT_BG);
    drawInputBox(inputBoxX, inputBoxY, inputBoxW, inputBoxH, username, false);

    // Clear error area (positioned safely above keyboard)
    tft->fillRect(inputBoxX, inputBoxY + inputBoxH + 8, inputBoxW, 12, WIN_BG_DARK);
    if (showErrorMsg) {
        drawErrorMessage("Username is required", inputBoxY + inputBoxH + 8);
    }
}

void LoginScreen::drawUsernameScreen() {
    drawBackground();

    // Header bar matching Deep Space theme
    const uint16_t headerHeight = 30;
    tft->fillRect(0, 0, 320, headerHeight, WIN_HEADER);
    tft->drawFastHLine(0, headerHeight - 1, 320, WIN_ACCENT);
    
    // Title in header (centered)
    tft->setTextSize(2);
    tft->setTextColor(WIN_TEXT, WIN_HEADER);
    String title = "LOGIN";
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
    String label = "Username";
    uint16_t labelWidth = label.length() * 6;  // Approximate width for text size 1
    uint16_t labelX = (320 - labelWidth) / 2;   // Center horizontally
    tft->setCursor(labelX, inputBoxY - 12);
    tft->print(label);

    // Draw input box (centered horizontally)
    uint16_t inputBoxX = (320 - inputBoxW) / 2;
    drawInputBox(inputBoxX, inputBoxY, inputBoxW, inputBoxH, username, false);

    // Error message if needed (positioned safely above keyboard)
    if (showUsernameEmpty) {
        drawErrorMessage("Username is required", inputBoxY + inputBoxH + 8);
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
    // Display nickname if available, otherwise fallback to username
    String displayName = nickname.length() > 0 ? nickname : username;
    tft->print(displayName);

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
            // Success screen removed - callback transitions immediately to SocialScreen
            // This case should not be reached, but kept for safety
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

// Navigation handlers (delegate to keyboard for navigation)
void LoginScreen::handleUp() {
    if (keyboard != nullptr) {
        keyboard->moveCursorByCommand("up", 0, 0);
    }
}

void LoginScreen::handleDown() {
    if (keyboard != nullptr) {
        keyboard->moveCursorByCommand("down", 0, 0);
    }
}

void LoginScreen::handleLeft() {
    if (keyboard != nullptr) {
        keyboard->moveCursorByCommand("left", 0, 0);
    }
}

void LoginScreen::handleRight() {
    if (keyboard != nullptr) {
        keyboard->moveCursorByCommand("right", 0, 0);
    }
}

void LoginScreen::handleSelect() {
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

void LoginScreen::handleExit() {
    // Exit - delete last character (backspace behavior)
    if (state == LOGIN_USERNAME) {
        if (username.length() > 0) {
            username.remove(username.length() - 1);
            showUsernameEmpty = false;
            updateUsernameInputArea(false);
        }
    }
}

void LoginScreen::handleUsernameKey(const String& key) {
    usernameCursorRow = keyboard->getCursorRow();
    usernameCursorCol = keyboard->getCursorCol();
    
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
        // Enter key - proceed to PIN step
        if (username.length() == 0) {
            showUsernameEmpty = true;
            updateUsernameInputArea(true);
        } else {
            goToPinStep();
        }
        return;
    } else if (key == "<" || key == "|b") {
        handleExit();
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
                // Login successful - skip success screen, proceed immediately
                userId = loginResult.user_id;
                nickname = loginResult.nickname.length() > 0 ? loginResult.nickname : username;  // Use nickname, fallback to username
                Serial.println("Login Screen: API verification successful!");
                Serial.print("Login Screen: User ID saved: ");
                Serial.println(userId);
                Serial.print("Login Screen: Username: ");
                Serial.println(username);
                Serial.print("Login Screen: Nickname: ");
                Serial.println(nickname);
                
                // Load friends list
                loadFriends();
                
                // Load notifications
                loadNotifications();
                
                // Call login success callback immediately (transitions to SocialScreen)
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
                            nickname = loginResult.nickname.length() > 0 ? loginResult.nickname : username;  // Use nickname, fallback to username
                            Serial.println("Login Screen: Auto-login successful after account creation!");
                            Serial.print("Login Screen: User ID: ");
                            Serial.println(userId);
                            
                            // Load friends list
                            loadFriends();
                            
                            // Load notifications
                            loadNotifications();
                            
                            // Call login success callback immediately (transitions to SocialScreen)
                            if (onLoginSuccessCallback != nullptr) {
                                onLoginSuccessCallback();
                            }
                        } else {
                            Serial.println("Login Screen: Failed to login after account creation!");
                            // Go back to username step if auto-login fails
                            username = pendingUsername;
                            goToUsernameStep();
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

