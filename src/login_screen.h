#ifndef LOGIN_SCREEN_H
#define LOGIN_SCREEN_H

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include "keyboard.h"
#include "pin_screen.h"

// Two-step login screen: username then PIN (Windows 10–like)
class LoginScreen {
public:
    enum LoginState {
        LOGIN_USERNAME,
        LOGIN_USERNAME_VALIDATING,  // Waiting for server validation response
        LOGIN_PIN,
        LOGIN_SUCCESS
    };

    LoginScreen(Adafruit_ST7789* tft, Keyboard* keyboard);
    ~LoginScreen() = default;

    // Draw current state
    void draw();

    // Handle keyboard key (coming from onKeySelected)
    void handleKeyPress(const String& key);

    // State helpers
    bool isAuthenticated() const { return state == LOGIN_SUCCESS; }
    bool isOnPinStep() const { return state == LOGIN_PIN; }
    bool isOnUsernameStep() const { return state == LOGIN_USERNAME || state == LOGIN_USERNAME_VALIDATING; }
    bool needsUsernameValidation() const { 
        return state == LOGIN_USERNAME_VALIDATING && !usernameValidationSent; 
    }
    void markUsernameValidationSent() { usernameValidationSent = true; }
    String getUsername() const { return username; }

    // Configuration
    void setExpectedPin(const String& pin) { if (pinScreen) pinScreen->setExpectedPin(pin); }
    
    // Get login credentials in server-parseable format
    String getLoginCredentials() const;
    
    // Username validation
    void validateUsernameWithServer();
    void onUsernameValidationResponse(bool isValid, const String& message);
    bool isUsernameValidated() const { return usernameValidated; }
    bool hasUsernameValidationError() const { return usernameValidationError; }
    String getUsernameValidationError() const { return usernameValidationErrorMsg; }

private:
    Adafruit_ST7789* tft;
    Keyboard* keyboard;
    PinScreen* pinScreen;

    LoginState state;
    String username;
    bool showUsernameEmpty;
    uint16_t usernameCursorRow;
    int8_t usernameCursorCol;
    
    // Username validation state
    bool usernameValidated;
    bool usernameValidationError;
    String usernameValidationErrorMsg;
    bool usernameValidationSent;  // Track if validation request already sent

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
};

#endif

