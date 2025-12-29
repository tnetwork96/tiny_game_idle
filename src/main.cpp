#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>
#ifdef ESP32
#include <Preferences.h>
#elif defined(ESP8266)
#include <EEPROM.h>
#endif
#include "wifi_manager.h"
#include "login_screen.h"
#include "socket_manager.h"
#include "keyboard.h"
#include "keyboard_skins_wrapper.h"
#include "api_client.h"
#include "social_screen.h"

// ST7789 pins
#define TFT_CS    15
#define TFT_DC    2
#define TFT_RST   4
#define TFT_MOSI  19
#define TFT_SCLK  18

Adafruit_ST7789 tft = Adafruit_ST7789(&SPI, TFT_CS, TFT_DC, TFT_RST);
Keyboard* keyboard;
WiFiManager* wifiManager;
LoginScreen* loginScreen;
SocketManager* socketManager;
SocialScreen* socialScreen;

bool isSocialScreenActive = false;

// Normalize and validate navigation command
String normalizeNavCommand(String cmd) {
    cmd.trim();
    cmd.toLowerCase();
    if (cmd == "up" || cmd == "down" || cmd == "left" || cmd == "right" || cmd == "select" || cmd == "exit") {
        return cmd;
    }
    return "";
}

// Central input dispatcher: route nav commands to active screen
void dispatchNavCommand(String command) {
    String cmd = normalizeNavCommand(command);
    if (cmd.length() == 0) return;

    // Social screen has highest priority when active
    if (isSocialScreenActive && socialScreen != nullptr) {
        socialScreen->handleNavCommand(cmd);
        return;
    }

    // Login screen when WiFi already connected
    if (wifiManager != nullptr && wifiManager->isConnected() && loginScreen != nullptr) {
        loginScreen->handleNavCommand(cmd);
        return;
    }

    // WiFi selection/password flow
    if (wifiManager != nullptr) {
        // If in WiFi list/pwd flow, handle navigation and propagate to keyboard for typing/select
        wifiManager->handleNavCommand(cmd);
        if (keyboard != nullptr) {
            keyboard->moveCursorByCommand(cmd, 0, 0);
        }
        return;
    }

    // Fallback: move keyboard only
    if (keyboard != nullptr) {
        keyboard->moveCursorByCommand(cmd, 0, 0);
    }
}

// Callback function for keyboard input - routes to appropriate screen
void onKeyboardKeySelected(String key) {
    // Route to SocialScreen if active
    if (isSocialScreenActive && socialScreen != nullptr) {
        socialScreen->handleKeyPress(key);
    }
    // Route to LoginScreen if WiFi is connected and login screen is active
    else if (wifiManager != nullptr && wifiManager->isConnected() && loginScreen != nullptr && !isSocialScreenActive) {
        loginScreen->handleKeyPress(key);
    }
    // Otherwise route to WiFi manager
    else if (wifiManager != nullptr) {
        wifiManager->handleKeyboardInput(key);
    }
}

// Callback function for MiniKeyboard - routes to Add Friend screen (giống Keyboard gốc)
void onMiniKeyboardKeySelected(String key) {
    // Route to SocialScreen if active (Add Friend tab)
    if (isSocialScreenActive && socialScreen != nullptr) {
        socialScreen->handleKeyPress(key);
    }
}

// Callback function for when login is successful
void onLoginSuccess() {
    Serial.println("Main: Login successful, switching to Social Screen...");
    
    if (loginScreen != nullptr && socialScreen != nullptr) {
        // Set user ID and server info
        int userId = loginScreen->getUserId();
        socialScreen->setUserId(userId);
        socialScreen->setServerInfo("192.168.1.7", 8080);
        
        // Load data
        socialScreen->loadFriends();
        socialScreen->loadNotifications();
        
        // Activate social screen
        isSocialScreenActive = true;
        
        // Disable main keyboard drawing when social screen is active
        if (keyboard != nullptr) {
            keyboard->setDrawingEnabled(false);
        }
        
        // Auto-navigate to Add Friend tab and focus keyboard
        socialScreen->navigateToAddFriend();
        
        // Set callback for MiniKeyboard to route to Add Friend screen (giống Keyboard gốc)
        if (socialScreen->getMiniKeyboard() != nullptr) {
            socialScreen->getMiniKeyboard()->setOnKeySelectedCallback(onMiniKeyboardKeySelected);
            Serial.println("Main: MiniKeyboard callback set");
        }
        
        Serial.println("Main: Social Screen activated - Auto-navigated to Add Friend tab");
    }
}

// Function to clear WiFi credentials cache
void clearWiFiCredentials() {
    Serial.println("WiFi: Clearing all cached credentials...");
    
    // Clear credentials from Preferences (ESP32) or EEPROM (ESP8266)
    #ifdef ESP32
        Preferences preferences;
        preferences.begin("wifi", false);
        preferences.clear();
        preferences.end();
        Serial.println("WiFi: ESP32 Preferences cleared");
    #elif defined(ESP8266)
        EEPROM.begin(512);
        for (int i = 0; i < 512; i++) {
            EEPROM.write(i, 0);
        }
        EEPROM.commit();
        EEPROM.end();
        Serial.println("WiFi: ESP8266 EEPROM cleared");
    #endif
    
    // Clear credentials saved in WiFi stack
    WiFi.disconnect(true);  // true = clear credentials saved in flash
    delay(500);
    
    // Reset WiFi mode to Station mode
    WiFi.mode(WIFI_STA);
    delay(200);
    
    Serial.println("WiFi: All cached credentials cleared successfully");
}

void setup() {
    Serial.begin(115200);
    delay(200);

    SPI.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);
    SPI.setFrequency(27000000);

    tft.init(240, 320);     // Official Adafruit init
    tft.setRotation(3);  // Rotate -90 degrees (rotation 3: 320x240, origin at bottom left)
    tft.fillScreen(ST77XX_BLACK);

    Serial.println("Display initialized: 240x320");
    Serial.println("WiFi Manager: Starting...");
    
    // Clear WiFi cache before initializing WiFi Manager
    clearWiFiCredentials();
    
    // Initialize Keyboard (draws directly to tft, no canvas)
    keyboard = new Keyboard(&tft);
    
    // Apply keyboard skin
    Serial.println("Applying keyboard skin...");
    keyboard->setSkin(KeyboardSkins::getFeminineLilac());
    Serial.println("Keyboard skin applied!");
    
    // Set keyboard callback
    keyboard->setOnKeySelectedCallback(onKeyboardKeySelected);

    // Initialize WiFi Manager
    wifiManager = new WiFiManager(&tft, keyboard);
    
    // Initialize LoginScreen (sẽ được hiển thị sau khi WiFi kết nối)
    loginScreen = new LoginScreen(&tft, keyboard);
    // Note: setExpectedPin() is no longer used - all PIN verification is done via API against database
    
    // Initialize SocialScreen
    socialScreen = new SocialScreen(&tft, keyboard);
    
    // Register login success callback
    loginScreen->setOnLoginSuccessCallback(onLoginSuccess);
    
    // Initialize Socket Manager
    socketManager = new SocketManager();
    
    Serial.println("WiFi Manager initialized!");
    Serial.println("Login Screen initialized!");
    Serial.println("Social Screen initialized!");
    Serial.println("Socket Manager initialized!");
    
    // Start WiFi scanning process (automatic)
    if (wifiManager != nullptr) {
        wifiManager->begin();
        Serial.println("WiFi Manager: Scan started automatically");
    }
}

void loop() {
    // Read single navigation command from Serial (left/right/up/down/select/exit)
    if (Serial.available()) {
        String nav = Serial.readStringUntil('\n');
        nav.trim();
        if (nav.length() > 0) {
            dispatchNavCommand(nav);
        }
    }

    // Update WiFi Manager state (check connection status)
    if (wifiManager != nullptr) {
        wifiManager->update();
    }

    delay(100);
}
