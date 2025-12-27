#ifndef LOGIN_SCREEN_H
#define LOGIN_SCREEN_H

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include "keyboard.h"
#include "pin_screen.h"
#include "confirmation_dialog.h"
#include "api_client.h"

// Two-step login screen: username then PIN (Windows 10–like)
class LoginScreen {
public:
    enum LoginState {
        LOGIN_USERNAME,
        LOGIN_PIN,
        LOGIN_SUCCESS,
        LOGIN_SHOWING_DIALOG  // Showing confirmation dialog for account creation
    };

    LoginScreen(Adafruit_ST7789* tft, Keyboard* keyboard);
    ~LoginScreen() = default;

    // Draw current state
    void draw();

    // Handle keyboard key (coming from onKeySelected)
    void handleKeyPress(const String& key);
    
    // Update state (called from main loop for auto-confirm)
    void update();

    // State helpers
    bool isAuthenticated() const { return state == LOGIN_SUCCESS; }
    bool isOnPinStep() const { return state == LOGIN_PIN; }
    
    // Check if confirmation dialog is showing (for auto-confirm in main loop)
    bool isShowingDialog() const { 
        return state == LOGIN_SHOWING_DIALOG && 
               confirmationDialog != nullptr && 
               confirmationDialog->isVisible(); 
    }
    
    // Get dialog show time for auto-confirm (for main loop)
    unsigned long getDialogShowTime() const { return dialogShowTime; }
    
    // Trigger confirm from outside (for auto-confirm in main loop)
    void triggerConfirm();
    
    // Trigger navigation for testing (for auto-navigation in main loop)
    void triggerNavigateLeft();
    void triggerNavigateRight();
    void triggerSelect();
    
    // Reset to username step (public method for external use)
    void resetToUsernameStep();
    
    // Get username and PIN for API calls
    String getUsername() const { return username; }
    String getPin() const { return pinScreen ? pinScreen->getPin() : ""; }
    
    // Get user ID after successful login
    int getUserId() const { return userId; }
    
    // Load friends list (called after successful login)
    void loadFriends();
    
    // Load notifications (called after successful login)
    void loadNotifications();
    
    // Callback for when friends are loaded
    typedef void (*FriendsLoadedCallback)(const ApiClient::FriendEntry* friends, int count);
    typedef void (*FriendsLoadedStringCallback)(const String& friendsString);  // Callback for string format
    typedef void (*NotificationsLoadedCallback)(uint8_t count);  // Callback for notification count
    void setFriendsLoadedCallback(FriendsLoadedCallback callback) { friendsLoadedCallback = callback; }
    void setFriendsLoadedStringCallback(FriendsLoadedStringCallback callback) { friendsLoadedStringCallback = callback; }
    void setNotificationsLoadedCallback(NotificationsLoadedCallback callback) { notificationsLoadedCallback = callback; }
 
    // Configuration
    void setExpectedPin(const String& pin) { if (pinScreen) pinScreen->setExpectedPin(pin); }

private:
    Adafruit_ST7789* tft;
    Keyboard* keyboard;
    PinScreen* pinScreen;
    ConfirmationDialog* confirmationDialog;

    LoginState state;
    String username;
    bool showUsernameEmpty;
    uint16_t usernameCursorRow;
    int8_t usernameCursorCol;
    
    // Account creation state
    String pendingUsername;
    String pendingPin;
    unsigned long dialogShowTime;  // Timestamp when dialog was shown
    static const unsigned long AUTO_CONFIRM_DELAY = 3000;  // Auto-confirm after 3 seconds
    
    // User ID and friends loading
    int userId;
    FriendsLoadedCallback friendsLoadedCallback;
    FriendsLoadedStringCallback friendsLoadedStringCallback;
    NotificationsLoadedCallback notificationsLoadedCallback;

    // Drawing helpers
    void drawBackground();
    void drawProfileCircle(uint16_t cx, uint16_t cy, uint16_t r, uint16_t color);
    void drawUsernameScreen();
    void drawSuccessScreen();
    void drawErrorMessage(const String& message, uint16_t y);
    void drawInputBox(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const String& value, bool masked);
    void updateUsernameInputArea(bool showErrorMsg);

    // Input helpers
    void ensureAlphabetMode();
    void handleUsernameKey(const String& key);
    void goToUsernameStep();
    void goToPinStep();
    
    // Static callbacks for ConfirmationDialog
    static void staticOnCreateAccountConfirm();
    static void staticOnCreateAccountCancel();
    
    // Instance methods for account creation
    void onCreateAccountConfirm();
    void onCreateAccountCancel();
    
    // Static instance pointer for callbacks
    static LoginScreen* instanceForCallback;
};

#endif

