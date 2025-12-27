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
#include "buddy_list_screen.h"
#include "api_client.h"

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
BuddyListScreen* buddyListScreen;

    // Flag to track if auto-connect has been executed
bool autoConnectExecuted = false;
bool autoLoginExecuted = false;

// Callback when friends are loaded from database (string format)
void onFriendsLoadedString(const String& friendsString) {
    if (buddyListScreen != nullptr) {
        Serial.print("Main: Received friends string: ");
        Serial.println(friendsString);
        Serial.print("Main: String length: ");
        Serial.println(friendsString.length());
        
        // Parse string in BuddyListScreen
        buddyListScreen->parseFriendsString(friendsString);
        
        // Switch to buddy list screen (always show, even if empty)
        Serial.println("Main: Drawing buddy list screen after parsing string...");
        delay(500);  // Small delay to ensure success screen is visible briefly
        buddyListScreen->draw();
    } else {
        Serial.println("Main: BuddyListScreen not initialized!");
    }
}

// Callback function for keyboard input - routes to appropriate screen
void onKeyboardKeySelected(String key) {
    // Route to LoginScreen if WiFi is connected and login screen is active
    if (wifiManager != nullptr && wifiManager->isConnected() && loginScreen != nullptr) {
        loginScreen->handleKeyPress(key);
    }
    // Otherwise route to WiFi manager
    else if (wifiManager != nullptr) {
        wifiManager->handleKeyboardInput(key);
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

// Function to automatically select WiFi "Van Ninh" and enter password
void autoConnectToVanNinh() {
    if (wifiManager == nullptr || keyboard == nullptr) return;
    if (autoConnectExecuted) return;
    
    // Wait for WiFi scan to complete and list to be ready
    if (wifiManager->getState() != WIFI_STATE_SELECT) return;
    
    WiFiListScreen* wifiList = wifiManager->getListScreen();
    if (wifiList == nullptr) return;
    
    Serial.println("Auto-connect: Starting to find 'Van Ninh' WiFi...");
    
    // Ensure we start from the beginning of the list
    while (wifiList->getSelectedIndex() != 0) {
        wifiList->selectPrevious();
        delay(100);
    }
    
    // Search for "Van Ninh" by navigating through the list
    bool foundVanNinh = false;
    uint16_t networkCount = wifiList->getNetworkCount();
    
    for (uint16_t i = 0; i < networkCount; i++) {
        String ssid = wifiList->getSelectedSSID();
        Serial.print("Auto-connect: Checking [");
        Serial.print(wifiList->getSelectedIndex());
        Serial.print("]: ");
        Serial.println(ssid);
        
        if (ssid == "Van Ninh") {
            foundVanNinh = true;
            Serial.println("========================================");
            Serial.println("Auto-connect: Found 'Van Ninh'! Selecting...");
            Serial.println("========================================");
            break;
        }
        
        // Move to next item if not the last one
        if (i < networkCount - 1) {
            wifiList->selectNext();
            delay(200);
        }
    }
    
    if (foundVanNinh) {
        // Select the WiFi network
        delay(1000);
        wifiManager->handleSelect();  // This will move to password screen
        Serial.println("Auto-connect: WiFi selected, waiting for password screen...");
        
        // Wait for password screen to appear
        delay(1500);
        
        // Check if we're now on password screen
        if (wifiManager->getState() == WIFI_STATE_PASSWORD) {
            Serial.println("Auto-connect: Password screen ready, typing password '123456a@'...");
            
            // Type password using keyboard navigation
            keyboard->typeString("123456a@");
            
            Serial.println("Auto-connect: Password typed, waiting before pressing Enter...");
            delay(1000);  // Wait to ensure all characters are entered
            
            // Press Enter to connect using keyboard navigation
            Serial.println("Auto-connect: Navigating to Enter key and pressing it...");
            keyboard->pressEnter();
            
            autoConnectExecuted = true;
            Serial.println("Auto-connect: Connection initiated!");
        } else {
            Serial.println("Auto-connect: Error - password screen not ready");
        }
    } else {
        Serial.println("Auto-connect: 'Van Ninh' not found in WiFi list");
    }
}

// Function to automatically login with username and PIN
void autoLogin() {
    if (loginScreen == nullptr || keyboard == nullptr) return;
    if (autoLoginExecuted) return;
    
    // Check if we're on username screen (not on PIN step and not authenticated)
    if (!loginScreen->isOnPinStep() && !loginScreen->isAuthenticated()) {
        // We're on username screen, type username
        Serial.println("Auto-login: Username screen ready, typing username...");
        
        // Đợi một chút để màn hình sẵn sàng
        delay(1000);
        
        // Type username (không tồn tại để test flow tạo account mới)
        keyboard->typeString("newuser1222");
        
        Serial.println("Auto-login: Username typed, waiting before pressing Enter...");
        delay(1000);
        
        // Press Enter to go to PIN screen
        Serial.println("Auto-login: Pressing Enter to go to PIN screen...");
        keyboard->pressEnter();
        
        // Đợi màn hình PIN xuất hiện
        delay(1500);
    }
    
    // Check if we're now on PIN screen
    if (loginScreen->isOnPinStep()) {
        Serial.println("Auto-login: PIN screen ready, typing PIN...");
        
        // Type PIN (ví dụ: "1234")
        keyboard->typeString("1234");
        
        Serial.println("Auto-login: PIN typed, waiting before pressing Enter...");
        delay(1000);
        
        // Press Enter to login
        Serial.println("Auto-login: Pressing Enter to login...");
        keyboard->pressEnter();
        
        autoLoginExecuted = true;
        Serial.println("Auto-login: Login initiated!");
    }
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
    
    // Initialize Socket Manager
    socketManager = new SocketManager();
    
    // Initialize BuddyListScreen
    buddyListScreen = new BuddyListScreen(&tft);
    
    // Set callback for friends loaded (string format)
    loginScreen->setFriendsLoadedStringCallback(onFriendsLoadedString);
    
    Serial.println("WiFi Manager initialized!");
    Serial.println("Login Screen initialized!");
    Serial.println("Socket Manager initialized!");
    Serial.println("Buddy List Screen initialized!");
    
    // Start WiFi scanning process (automatic)
    if (wifiManager != nullptr) {
        wifiManager->begin();
        Serial.println("WiFi Manager: Scan started automatically");
    }
}

void loop() {
    // Update WiFi Manager state (check connection status)
    if (wifiManager != nullptr) {
        wifiManager->update();
        
        // Auto-connect to "Van Ninh" when WiFi list is ready (only once)
        if (!autoConnectExecuted && wifiManager->getState() == WIFI_STATE_SELECT) {
            autoConnectToVanNinh();
        }
        
        // Auto-login when WiFi is connected (only once)
        if (!autoLoginExecuted && wifiManager->isConnected()) {
            // Đợi một chút để màn hình connection info hiển thị
            delay(2000);
            
            // Chuyển sang màn hình đăng nhập
            Serial.println("Auto-login: WiFi connected, switching to login screen...");
            loginScreen->resetToUsernameStep();
            
            // Đợi màn hình sẵn sàng
            delay(1000);
            
            // Bắt đầu auto login
            autoLogin();
        }
    }
    
    // Auto-navigation and confirm logic in main loop (outside LoginScreen)
    if (loginScreen != nullptr && loginScreen->isShowingDialog()) {
        unsigned long dialogShowTime = loginScreen->getDialogShowTime();
        if (dialogShowTime > 0) {
            unsigned long currentTime = millis();
            unsigned long elapsedTime = currentTime - dialogShowTime;
            
            // Auto-navigation flow: Move right (YES -> NO), then left (NO -> YES), then select YES
            static bool navRightDone = false;
            static bool navLeftDone = false;
            static bool selectDone = false;
            
            // After 0.5s: Move to NO (right)
            if (!navRightDone && elapsedTime >= 500) {
                Serial.println("Main Loop: Auto-navigation - Moving to NO (right)...");
                loginScreen->triggerNavigateRight();
                navRightDone = true;
            }
            
            // After 1.0s: Move back to YES (left)
            if (!navLeftDone && elapsedTime >= 1000) {
                Serial.println("Main Loop: Auto-navigation - Moving back to YES (left)...");
                loginScreen->triggerNavigateLeft();
                navLeftDone = true;
            }
            
            // After 1.5s: Select YES
            if (!selectDone && elapsedTime >= 1500) {
                Serial.println("Main Loop: Auto-navigation - Selecting YES...");
                loginScreen->triggerSelect();
                selectDone = true;
            }
            
            // Reset flags when dialog is hidden
            if (!loginScreen->isShowingDialog()) {
                navRightDone = false;
                navLeftDone = false;
                selectDone = false;
            }
        }
    }
    
    // Ensure buddy list is displayed after successful login and friends loaded
    static bool buddyListDisplayed = false;
    if (loginScreen != nullptr && loginScreen->isAuthenticated() && buddyListScreen != nullptr) {
        if (!buddyListDisplayed) {
            // Friends should already be loaded via callback
            // Just ensure buddy list is drawn if it has been set
            if (buddyListScreen->getBuddyCount() >= 0) {
                Serial.println("Main Loop: Ensuring buddy list screen is displayed...");
                buddyListScreen->draw();
                buddyListDisplayed = true;
            }
        }
    } else {
        buddyListDisplayed = false;
    }
    
    // Không cần gọi socketManager->update() nữa
    // Task đã chạy while(true) tự động sau khi begin()
    // Task xử lý tất cả: webSocket.loop(), keep-alive ping, etc.
    
    delay(100);
}
