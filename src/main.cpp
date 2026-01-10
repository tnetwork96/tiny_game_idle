#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>
#ifdef ESP32
#include <Preferences.h>
#include <SPIFFS.h>
#elif defined(ESP8266)
#include <EEPROM.h>
#include <FS.h>
#endif
#include "wifi_manager.h"
#include "login_screen.h"
#include "socket_manager.h"
#include "keyboard.h"
#include "keyboard_skins_wrapper.h"
#include "api_client.h"
#include "social_screen.h"
#include "chat_screen.h"
#include "caro_game_screen.h"
#include "game_lobby_screen.h"
#include "auto_navigator.h"

// ST7789 pins
#define TFT_CS    15   // CS pin
#define TFT_DC    2    // IO2 - LCD_DC
#define TFT_RST   4    // IO4 - LCD_RESET
#define TFT_MOSI  23   // IO23 - LCD_MOSI
#define TFT_SCLK  18   // IO18 - LCD_SCLK
#define TFT_BLK   27   // IO27 - LCD_BLK (Backlight control)

// Button pins (GPIO36 và GPIO39 là input-only, không có internal pull-up)
#define BTN_UP      39  // GPIO39 (SENSOR_VN) - Input only, no internal pull-up
#define BTN_DOWN    14  // GPIO14 - Input (supports pull-up), if wired as a button
#define BTN_SELECT  36  // GPIO36 (SENSOR_VP) - Input only, no internal pull-up

// Rotary encoder pins (GPIO number - không phải index)
#define ENCODER_CLK  14  // GPIO14 - Encoder CLK pin
#define ENCODER_DT   16  // GPIO16 - Encoder DT pin

// Analog thumbwheel navigation (VR1) - net "ADC" goes to GPIO32 in schematic
#define PIN_VR_NAV   32

// Set to 1 to print VR1 ADC readings to Serial (throttled)
#define VR_NAV_DEBUG 0

Adafruit_ST7789 tft = Adafruit_ST7789(&SPI, TFT_CS, TFT_DC, TFT_RST);
Keyboard* keyboard;
WiFiManager* wifiManager;
LoginScreen* loginScreen;
SocketManager* socketManager;
SocialScreen* socialScreen;
ChatScreen* chatScreen = nullptr;
AutoNavigator* autoNavigator = nullptr;

bool isChatScreenActive = false;
int currentChatFriendUserId = -1;  // ID của friend đang chat (nếu ChatScreen đang mở)
bool isSocialScreenActive = false;
bool hasTransitionedToLogin = false;  // Track if we've already transitioned to login screen

// Button state tracking (debounce + auto-detect active level by sampling idle state at boot)
struct ButtonDebounce {
    uint8_t pin;
    bool idleState;              // sampled at boot
    bool stableState;            // debounced current state
    bool lastReading;            // last raw reading
    unsigned long lastChangeMs;  // last time raw reading changed
};

static const unsigned long DEBOUNCE_DELAY = 50;  // 50ms debounce
ButtonDebounce btnUp   = { BTN_UP,   true, true, true, 0 };
ButtonDebounce btnDown = { BTN_DOWN, true, true, true, 0 };
ButtonDebounce btnSelect = { BTN_SELECT, true, true, true, 0 };

// Encoder state tracking
int lastEncoderCLK = HIGH;
int lastEncoderDT = HIGH;
unsigned long lastEncoderTime = 0;
const unsigned long ENCODER_DEBOUNCE = 5;  // 5ms debounce

// Forward declaration
void onKeyboardKeySelected(const String& key);

static void initButton(ButtonDebounce& b) {
    bool r = digitalRead(b.pin);
    b.idleState = r;
    b.stableState = r;
    b.lastReading = r;
    b.lastChangeMs = millis();
}

// Returns true only on a debounced PRESS (state deviates from idle)
static bool readButtonPressed(ButtonDebounce& b) {
    bool reading = digitalRead(b.pin);
    unsigned long now = millis();

    if (reading != b.lastReading) {
        b.lastReading = reading;
        b.lastChangeMs = now;
    }

    if ((now - b.lastChangeMs) < DEBOUNCE_DELAY) {
        return false;
    }

    if (reading != b.stableState) {
        b.stableState = reading;
        // Press = stable state differs from idle state
        return (b.stableState != b.idleState);
    }

    return false;
}

// Function to read rotary encoder
void readEncoder() {
    int clkState = digitalRead(ENCODER_CLK);
    int dtState = digitalRead(ENCODER_DT);
    unsigned long now = millis();
    
    // Debounce
    if (now - lastEncoderTime < ENCODER_DEBOUNCE) {
        return;
    }
    
    if (clkState != lastEncoderCLK) {
        lastEncoderTime = now;
        if (dtState != clkState) {
            // Clockwise rotation -> Right
            onKeyboardKeySelected("right");
        } else {
            // Counter-clockwise rotation -> Left
            onKeyboardKeySelected("left");
        }
    }
    
    lastEncoderCLK = clkState;
    lastEncoderDT = dtState;
}

// Function to handle hardware buttons and encoder
void handleHardwareInputs() {
    // Read Up button (GPIO36) - map to "up"
    if (readButtonPressed(btnUp)) {
        onKeyboardKeySelected("up");
    }
    
    // Read Down button (GPIO39) - map to "down"
    if (readButtonPressed(btnDown)) {
        onKeyboardKeySelected("down");
    }

    // Read Select button - map to "select"
    if (readButtonPressed(btnSelect)) {
        onKeyboardKeySelected("select");
    }

    // Read analog value from GPIO 32 (VR1 net ADC, 0-4095)
    // Average more samples to reduce noise (helps consistent stepping)
    int vrValue = 0;
    for (int i = 0; i < 8; i++) {
        vrValue += analogRead(PIN_VR_NAV);
    }
    vrValue >>= 3;

    unsigned long nowMs = millis();
    
#if VR_NAV_DEBUG
    static unsigned long vrLastPrintMs = 0;
    static int vrMin = 4095;
    static int vrMax = 0;
    if (vrValue < vrMin) vrMin = vrValue;
    if (vrValue > vrMax) vrMax = vrValue;
    if (nowMs - vrLastPrintMs >= 200) {
        vrLastPrintMs = nowMs;
        Serial.print("VR1=");
        Serial.print(vrValue);
        Serial.print(" min=");
        Serial.print(vrMin);
        Serial.print(" max=");
        Serial.println(vrMax);
    }
#endif
    
    // VR1 thumbwheel: delta + accumulator stepping
    // Goal: nudge a little -> move 1 key; keep rolling -> move evenly; stop -> stop immediately.
    static int vrLastValue = -1;   // last ADC reading
    static int vrAcc = 0;          // accumulated delta

    // STEP: how many ADC counts equals 1 navigation step.
    // We use a smaller step near the ADC ends because readings can "stick" near min/max.
    const int STEP_BASE = 120;   // normal
    const int STEP_EDGE = 60;    // more sensitive near ends (prevents "not eating" at extremes)
    // Ignore tiny ADC jitter so it doesn't spam when your hand is steady.
    const int NOISE = 1;

    if (vrLastValue < 0) {
        vrLastValue = vrValue;
        vrAcc = 0;
    } else {
        int dv = vrValue - vrLastValue;
        vrLastValue = vrValue;

        // Clamp dv so occasional slow frames don't create huge jumps
        if (dv > 400) dv = 400;
        if (dv < -400) dv = -400;

        if (abs(dv) < NOISE) {
            dv = 0;
        }

        vrAcc += dv;

        const bool atEdge = (vrValue < 400) || (vrValue > 3800);
        const int step = atEdge ? STEP_EDGE : STEP_BASE;

        // Direction swap: ADC increases should navigate LEFT, decreases should navigate RIGHT
        while (vrAcc >= step) {
            onKeyboardKeySelected("left");
            vrAcc -= step;
        }
        while (vrAcc <= -step) {
            onKeyboardKeySelected("right");
            vrAcc += step;
        }
    }
    
    // Read rotary encoder
    // Avoid conflicts if encoder pins overlap with button pins
    const bool encoderEnabled =
        (ENCODER_CLK != BTN_UP) && (ENCODER_CLK != BTN_DOWN) && (ENCODER_CLK != BTN_SELECT) &&
        (ENCODER_DT != BTN_UP) && (ENCODER_DT != BTN_DOWN) && (ENCODER_DT != BTN_SELECT);
    if (encoderEnabled) {
        readEncoder();
    }
}

// Function to handle Serial input for navigation
void handleSerialNavigation() {
    // First, check for auto navigator commands
    // if (autoNavigator != nullptr) {
    //     autoNavigator->processSerialInput();
    // }
    
    // Then handle regular navigation commands
    if (Serial.available() > 0) {
        String command = Serial.readStringUntil('\n');
        command.trim();  // Remove whitespace and newline
        
        // Debug: Print raw input
        Serial.print("Serial: Raw input received: [");
        Serial.print(command);
        Serial.println("]");
        
        // Skip if it's an auto navigator command (already processed)
        if (command.startsWith("auto:")) {
            Serial.println("Serial: Auto navigator command, skipping regular handling");
            return;
        }
        
        // Convert to lowercase for case-insensitive matching
        String originalCommand = command;  // Keep original for debug
        command.toLowerCase();
        Serial.print("Serial: Processed command: [");
        Serial.print(command);
        Serial.println("]");
        
        // Map serial commands to navigation keys
        if (command == "up" || command == "u") {
            Serial.println("Serial: Received UP command");
            onKeyboardKeySelected("up");
        } else if (command == "down" || command == "d") {
            Serial.println("Serial: Received DOWN command");
            onKeyboardKeySelected("down");
        } else if (command == "left" || command == "l") {
            Serial.println("Serial: Received LEFT command");
            onKeyboardKeySelected("left");
        } else if (command == "right" || command == "r") {
            Serial.println("Serial: Received RIGHT command");
            onKeyboardKeySelected("right");
        } else if (command == "select" || command == "s" || command == "enter" || command == "e") {
            Serial.println("Serial: Received SELECT command");
            onKeyboardKeySelected("select");
        } else if (command == "exit" || command == "x" || command == "back" || command == "b") {
            Serial.println("Serial: Received EXIT command");
            onKeyboardKeySelected("exit");
        } else if (command.length() > 0) {
            // Unknown command
            Serial.print("Serial: Unknown command: ");
            Serial.println(command);
            Serial.println("Serial: Available commands: up/u, down/d, left/l, right/r, select/s/enter/e, exit/x/back/b");
            Serial.println("Serial: Auto Navigator: auto:load:/path, auto:exec:commands, auto:start, auto:stop, auto:run, auto:status");
        }
    }
}

// Callback function for keyboard input - routes to appropriate screen
// RULE: Check parent active TRƯỚC, sau đó check child active
void onKeyboardKeySelected(const String& key) {
    // Debug: Print input key
    Serial.print("Input: Key selected: [");
    Serial.print(key);
    Serial.println("]");
    
    // 1. ChatScreen (child of SocialScreen)
    bool isSocialParentActive = isSocialScreenActive && socialScreen != nullptr && socialScreen->getActive();
    if (isSocialParentActive && isChatScreenActive && chatScreen != nullptr && chatScreen->isActive()) {
        Serial.println("Input: Routing to ChatScreen");
        chatScreen->handleKeyPress(key);
        return;
    }
    
    // 2. SocialScreen and its children
    if (isSocialParentActive) {
        SocialScreen::ScreenState screenState = socialScreen->getScreenState();
        
        // 2a. CaroGameScreen (child)
        if (screenState == SocialScreen::STATE_PLAYING_GAME) {
            if (socialScreen->getCaroGameScreen() != nullptr && 
                socialScreen->getCaroGameScreen()->isActive()) {
                Serial.println("Input: Routing to CaroGameScreen");
                socialScreen->getCaroGameScreen()->handleKeyPress(key);
                return;
            }
        }
        
        // 2b. GameLobbyScreen (child)
        if (screenState == SocialScreen::STATE_WAITING_GAME) {
            if (socialScreen->getGameLobby() != nullptr && 
                socialScreen->getGameLobby()->isActive()) {
                Serial.println("Input: Routing to GameLobbyScreen");
                socialScreen->getGameLobby()->handleKeyPress(key);
                return;
            }
        }
        
        // 2c. MiniAddFriendScreen (child) - BUT navigation keys (up/down/left/right/select/exit) should go to parent
        // Navigation keys should always route to SocialScreen for tab navigation
        bool isNavigationKey = (key == "up" || key == "down" || key == "left" || key == "right" || key == "select" || key == "exit");
        
        if (socialScreen->getCurrentTab() == SocialScreen::TAB_ADD_FRIEND) {
            if (socialScreen->getMiniAddFriend() != nullptr && 
                socialScreen->getMiniAddFriend()->isActive()) {
                // If it's a navigation key, route to parent (SocialScreen) for tab navigation
                if (isNavigationKey) {
                    Serial.println("Input: Navigation key on Add Friend tab - routing to SocialScreen for tab navigation");
                    socialScreen->handleKeyPress(key);
                    return;
                } else {
                    // Text input keys go to MiniAddFriendScreen
                    Serial.println("Input: Routing to MiniAddFriendScreen");
                    socialScreen->getMiniAddFriend()->handleKeyPress(key);
                    return;
                }
            }
        }
        
        // 2d. Parent (SocialScreen) - no child active
        Serial.println("Input: Routing to SocialScreen (parent)");
        socialScreen->handleKeyPress(key);
        return;
    }
    
    // 3. WiFi Manager - Check BEFORE LoginScreen because WiFi selection happens first
    // WiFi Manager handles navigation when WiFi is not connected (SELECT, PASSWORD, CONNECTING, ERROR states)
    if (wifiManager != nullptr) {
        WiFiState wifiState = wifiManager->getState();
        // If WiFi is not connected, WiFi Manager should handle all input
        if (wifiState != WIFI_STATE_CONNECTED) {
            // Route all keys to WiFi Manager (navigation keys and password input)
            Serial.println("Input: Routing to WiFiManager (not connected)");
            wifiManager->handleKeyboardInput(key);
            return;
        }
    }
    
    // 4. LoginScreen - Only when WiFi is connected
    if (wifiManager != nullptr && wifiManager->isConnected() && 
        loginScreen != nullptr && !isSocialScreenActive && !isChatScreenActive) {
        Serial.println("Input: Routing to LoginScreen");
        loginScreen->handleKeyPress(key);
        return;
    }
    
    // 5. WiFi Manager fallback (for connected state if needed)
    if (wifiManager != nullptr) {
        Serial.println("Input: Routing to WiFiManager (fallback)");
        wifiManager->handleKeyboardInput(key);
        return;
    }
    
    // Debug: No handler found
    Serial.println("Input: WARNING - No handler found for key!");
}

// Callback function for MiniKeyboard - routes to Add Friend screen
// RULE: Check parent active TRƯỚC, sau đó check child active
void onMiniKeyboardKeySelected(const String& key) {
    // Debug: Print input key
    Serial.print("MiniKeyboard Input: Key selected: [");
    Serial.print(key);
    Serial.println("]");
    
    // 1. ChatScreen (child of SocialScreen)
    bool isSocialParentActive = isSocialScreenActive && socialScreen != nullptr && socialScreen->getActive();
    if (isSocialParentActive && isChatScreenActive && chatScreen != nullptr && chatScreen->isActive()) {
        Serial.println("MiniKeyboard Input: Routing to ChatScreen");
        chatScreen->handleKeyPress(key);
        return;
    }
    
    // 2. SocialScreen and its children
    if (isSocialParentActive) {
        // Navigation keys should always route to SocialScreen for tab navigation
        bool isNavigationKey = (key == "up" || key == "down" || key == "left" || key == "right" || key == "select" || key == "exit");
        
        // Check if on Add Friend tab and MiniAddFriend is active
        if (socialScreen->getCurrentTab() == SocialScreen::TAB_ADD_FRIEND) {
            if (socialScreen->getMiniAddFriend() != nullptr && 
                socialScreen->getMiniAddFriend()->isActive()) {
                // If it's a navigation key, route to parent (SocialScreen) for tab navigation
                if (isNavigationKey) {
                    Serial.println("MiniKeyboard Input: Navigation key on Add Friend tab - routing to SocialScreen for tab navigation");
                    socialScreen->handleKeyPress(key);
                    return;
                } else {
                    // Text input keys go to MiniAddFriendScreen
                    Serial.println("MiniKeyboard Input: Routing to MiniAddFriendScreen");
                    socialScreen->getMiniAddFriend()->handleKeyPress(key);
                    return;
                }
            }
        }
        
        // Otherwise route to parent
        Serial.println("MiniKeyboard Input: Routing to SocialScreen (parent)");
        socialScreen->handleKeyPress(key);
        return;
    }
    
    // Debug: No handler found
    Serial.println("MiniKeyboard Input: WARNING - No handler found for key!");
}


// Callback function to open chat with a friend
void onOpenChat(int friendUserId, const String& friendNickname) {
    Serial.print("Main: Opening chat with friend ID: ");
    Serial.print(friendUserId);
    Serial.print(", nickname: ");
    Serial.println(friendNickname);
    
    if (chatScreen == nullptr) {
        chatScreen = new ChatScreen(&tft, keyboard);
    }
    
    // Set owner user ID (lấy từ loginScreen)
    if (loginScreen != nullptr) {
        int ownerUserId = loginScreen->getUserId();
        chatScreen->setOwnerUserId(ownerUserId);
        Serial.print("Main: Set owner user ID: ");
        Serial.println(ownerUserId);
        
        String ownerNickname = loginScreen->getUsername();
        chatScreen->setOwnerNickname(ownerNickname);
    }
    
    // Set friend info
    chatScreen->setFriendUserId(friendUserId);
    chatScreen->setFriendNickname(friendNickname);
    
    // Update global variable để track friend đang chat
    currentChatFriendUserId = friendUserId;
    
    // Set friend status (có thể lấy từ friends list sau)
    chatScreen->setFriendStatus(0);  // Default offline, có thể update sau
    
    // Load messages từ file (sau khi đã set friend info để tạo đúng tên file)
    Serial.println("Main: Loading messages from file...");
    chatScreen->loadMessagesFromFile();
    
    // Update SocketManager state để socket_manager có thể xử lý messages
    // setChatScreen() sẽ tự động set SocketManager cho ChatScreen
    if (socketManager != nullptr) {
        Serial.print("Main: Setting ChatScreen for SocketManager, friendUserId: ");
        Serial.println(friendUserId);
        socketManager->setChatScreen(chatScreen);
        socketManager->setChatScreenActive(true);
        socketManager->setCurrentChatFriendUserId(friendUserId);
        Serial.println("Main: Updated SocketManager state for chat");
    } else {
        Serial.println("Main: ⚠️  socketManager is NULL in onOpenChat!");
    }
    
    // Set exit callback để quay lại SocialScreen
    chatScreen->setOnExitCallback([]() {
        Serial.println("Main: Chat screen exit callback - returning to Social Screen");
        isChatScreenActive = false;
        currentChatFriendUserId = -1;  // Reset khi đóng chat
        
        // Set ChatScreen inactive
        if (chatScreen != nullptr) {
            chatScreen->setActive(false);
        }
        
        // Update SocketManager state
        if (socketManager != nullptr) {
            socketManager->setChatScreen(nullptr);
            socketManager->setChatScreenActive(false);
            socketManager->setCurrentChatFriendUserId(-1);
        }
        
        isSocialScreenActive = true;
        
        // Set SocialScreen active
        if (socialScreen != nullptr) {
            socialScreen->setActive(true);
        }
        
        // Update SocketManager state
        if (socketManager != nullptr) {
            socketManager->setSocialScreenActive(true);
        }
        
        // Redraw social screen
        if (socialScreen != nullptr) {
            socialScreen->draw();
        }
    });
    
    // Activate chat screen
    isChatScreenActive = true;
    // isSocialScreenActive = false;  // ❌ REMOVED - parent stays active
    // Keep isSocialScreenActive = true (parent still active)
    
    // Set ChatScreen active
    if (chatScreen != nullptr) {
        chatScreen->setActive(true);
    }
    
    // Set SocialScreen inactive  // ❌ REMOVED - parent stays active
    // if (socialScreen != nullptr) {
    //     socialScreen->setActive(false);
    // }
    
    // Update SocketManager state
    // socketManager->setSocialScreenActive(false);  // ❌ REMOVED - keep social screen active
    // Keep social screen active in socket manager
    
    // Disable main keyboard drawing when chat screen is active
    if (keyboard != nullptr) {
        keyboard->setDrawingEnabled(false);
    }
    
    // Force redraw chat screen
    chatScreen->forceRedraw();
}

// Function to load login credentials from file
bool loadLoginCredentials() {
    if (loginScreen == nullptr) {
        return false;
    }
    
    // Initialize SPIFFS if not already initialized
    if (!SPIFFS.begin(true)) {
        Serial.println("Main: SPIFFS initialization failed for loading login credentials!");
        return false;
    }
    
    String fileName = "/login_credentials.txt";
    
    Serial.print("Main: Loading login credentials from file: ");
    Serial.println(fileName);
    
    // Open file for reading
    File file = SPIFFS.open(fileName, "r");
    if (!file) {
        Serial.print("Main: File not found or cannot open: ");
        Serial.println(fileName);
        return false;
    }
    
    String username = "";
    String pin = "";
    bool foundUsername = false;
    bool foundPin = false;
    
    // Read file line by line
    while (file.available()) {
        String line = file.readStringUntil('\n');
        line.trim();
        
        // Skip empty lines and separators
        if (line.length() == 0 || line.startsWith("===") || line.startsWith("====")) {
            continue;
        }
        
        // Parse username
        if (line.startsWith("Username: ")) {
            username = line.substring(10);  // Remove "Username: " prefix
            username.trim();
            foundUsername = true;
            Serial.print("Main: Loaded username: ");
            Serial.println(username);
        }
        // Parse PIN
        else if (line.startsWith("PIN: ")) {
            pin = line.substring(5);  // Remove "PIN: " prefix
            pin.trim();
            foundPin = true;
            Serial.print("Main: Loaded PIN (length): ");
            Serial.println(pin.length());
        }
    }
    
    file.close();
    
    // Set credentials if found
    if (foundUsername && username.length() > 0) {
        loginScreen->setUsername(username);
        Serial.print("Main: Username set to: ");
        Serial.println(username);
    }
    
    if (foundPin && pin.length() > 0) {
        loginScreen->setPin(pin);
        Serial.print("Main: PIN set (length: ");
        Serial.print(pin.length());
        Serial.println(")");
    }
    
    if (foundUsername && foundPin) {
        Serial.println("Main: Login credentials loaded successfully!");
        return true;
    } else {
        Serial.println("Main: Failed to load complete credentials");
        return false;
    }
}

// Function to save login credentials to file
void saveLoginCredentials() {
    if (loginScreen == nullptr) {
        return;
    }
    
    // Initialize SPIFFS if not already initialized
    if (!SPIFFS.begin(true)) {
        Serial.println("Main: SPIFFS initialization failed for saving login credentials!");
        return;
    }
    
    String username = loginScreen->getUsername();
    String pin = loginScreen->getPin();
    int userId = loginScreen->getUserId();
    
    if (username.length() == 0 || pin.length() == 0) {
        Serial.println("Main: Cannot save login credentials - username or PIN is empty");
        return;
    }
    
    String fileName = "/login_credentials.txt";
    
    Serial.print("Main: Saving login credentials to file: ");
    Serial.println(fileName);
    
    // Open file for writing
    File file = SPIFFS.open(fileName, "w");
    if (!file) {
        Serial.print("Main: Failed to open file for writing: ");
        Serial.println(fileName);
        return;
    }
    
    // Write login information to file
    file.println("=== Login Information ===");
    file.print("Username: ");
    file.println(username);
    file.print("User ID: ");
    file.println(userId);
    file.print("PIN: ");
    file.println(pin);
    file.print("Login Time: ");
    file.println(millis() / 1000);  // Time in seconds since boot
    file.println("========================");
    file.close();
    
    Serial.print("Main: Login credentials saved successfully to: ");
    Serial.println(fileName);
    Serial.print("Main: Username: ");
    Serial.println(username);
    Serial.print("Main: User ID: ");
    Serial.println(userId);
    Serial.print("Main: PIN length: ");
    Serial.println(pin.length());
}

// Callback function for when login is successful
void onLoginSuccess() {
    Serial.println("Main: Login successful, switching to Social Screen...");
    
    // Save login credentials to file
    saveLoginCredentials();
    
    if (loginScreen != nullptr && socialScreen != nullptr) {
        // Set user ID and server info
        int userId = loginScreen->getUserId();
        String username = loginScreen->getUsername();
        socialScreen->setUserId(userId);
        socialScreen->setOwnerNickname(username);
        socialScreen->setServerInfo("192.168.1.7", 8080);
        
        // Initialize and configure Socket Manager
        if (socketManager != nullptr) {
            // Set user ID for socket init message
            socketManager->setUserId(userId);
            
            // Initialize socket connection
            socketManager->begin("192.168.1.7", 8080, "/ws");
            
            // Set SocialScreen state để socket_manager có thể xử lý notification và badge
            if (socketManager != nullptr && socialScreen != nullptr) {
                socketManager->setSocialScreen(socialScreen);
                socketManager->setSocialScreenActive(true);
                Serial.println("Main: Set SocialScreen state for SocketManager");
            }
            
            // Set callback for user status updates
            socketManager->setOnUserStatusUpdateCallback(SocialScreen::onUserStatusUpdate);
            // Set callback for game events (invites/respond/ready)
            socketManager->setOnGameEventCallback(SocialScreen::onGameEvent);
            socketManager->setOnGameMoveCallback([](int sessionId, int userId, int row, int col, const String& gameStatus, int winnerId, int currentTurn) {
                if (socialScreen != nullptr) {
                    socialScreen->onGameMoveReceived(sessionId, userId, row, col, gameStatus, winnerId, currentTurn);
                }
            });
            Serial.println("Main: Set user status update callback");
            
            Serial.println("Main: Socket Manager initialized");
        }
        
        // Load data
        socialScreen->loadFriends();
        socialScreen->loadNotifications();
        
        // Activate social screen
        isSocialScreenActive = true;
        
        // Set SocialScreen active
        if (socialScreen != nullptr) {
            socialScreen->setActive(true);
        }
        
        // Update SocketManager state
        if (socketManager != nullptr) {
            socketManager->setSocialScreenActive(true);
        }
        
        // Disable main keyboard drawing when social screen is active
        if (keyboard != nullptr) {
            keyboard->setDrawingEnabled(false);
        }
        
        // Sync navigation state before navigating (ensures clean state when switching to social screen)
        if (socialScreen != nullptr) {
            socialScreen->syncNavigation();
        }
        
        // Navigate to Friends tab (default tab when entering social screen)
        socialScreen->navigateToFriends();
        
        // Set callback để mở chat từ SocialScreen
        socialScreen->setOnOpenChatCallback(onOpenChat);
        
        // Set callback for MiniKeyboard to route to Add Friend screen (giống Keyboard gốc)
        if (socialScreen->getMiniKeyboard() != nullptr) {
            socialScreen->getMiniKeyboard()->setOnKeySelectedCallback(onMiniKeyboardKeySelected);
            Serial.println("Main: MiniKeyboard callback set");
        }
        
        Serial.println("Main: Social Screen activated - Navigated to Notifications tab (for badge testing)");
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

    // Initialize backlight (IO27) - must be HIGH to turn on
    pinMode(TFT_BLK, OUTPUT);
    digitalWrite(TFT_BLK, HIGH);
    Serial.println("Backlight enabled on IO27");
    delay(50);

    // Initialize button pins
    // GPIO36/39 are input-only (no internal pull-up). GPIO14 supports pull-up if your wiring uses active-low.
    pinMode(BTN_UP, INPUT);      // GPIO39 - Input only, no pull-up
    pinMode(BTN_DOWN, INPUT);    // GPIO14 - Input (change to INPUT_PULLUP if needed)
    pinMode(BTN_SELECT, INPUT);  // GPIO36 - Input only, no pull-up
    pinMode(PIN_VR_NAV, INPUT);  // GPIO34 - Analog thumbwheel (VR1), input-only

    // Configure ADC for GPIO34 (ADC1) to better cover 0..3.3V range
    analogReadResolution(12);  // 0..4095
    analogSetPinAttenuation(PIN_VR_NAV, ADC_11db);

    Serial.println("Buttons initialized (Up: GPIO" + String(BTN_UP) + ", Down: GPIO" + String(BTN_DOWN) + ", Select: GPIO" + String(BTN_SELECT) + ")");
    // Sample idle state after pinMode so we can auto-detect active level
    initButton(btnUp);
    initButton(btnDown);
    initButton(btnSelect);
    
    // Initialize rotary encoder pins (INPUT_PULLUP)
    pinMode(ENCODER_CLK, INPUT_PULLUP);
    pinMode(ENCODER_DT, INPUT_PULLUP);
    lastEncoderCLK = digitalRead(ENCODER_CLK);
    lastEncoderDT = digitalRead(ENCODER_DT);
    Serial.println("Rotary encoder initialized (CLK: GPIO" + String(ENCODER_CLK) + ", DT: GPIO" + String(ENCODER_DT) + ")");

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
    
    // Try to load saved login credentials
    Serial.println("Main: Attempting to load saved login credentials...");
    if (loadLoginCredentials()) {
        Serial.println("Main: Saved credentials loaded - username and PIN will be pre-filled");
    } else {
        Serial.println("Main: No saved credentials found - user will need to enter manually");
    }
    
    // Initialize SocialScreen
    socialScreen = new SocialScreen(&tft, keyboard);
    
    // Register login success callback
    loginScreen->setOnLoginSuccessCallback(onLoginSuccess);
    
    // Initialize Socket Manager
    socketManager = new SocketManager();
    
    // Initialize Auto Navigator
    autoNavigator = new AutoNavigator();
    autoNavigator->setCommandCallback(onKeyboardKeySelected);
    autoNavigator->setCommandDelay(200);  // 200ms delay between commands
    Serial.println("Auto Navigator initialized!");
    
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
    // Handle hardware buttons and encoder
    handleHardwareInputs();
    
    // Handle Serial navigation input
    handleSerialNavigation();
    
    // Execute next auto navigation command if enabled
    if (autoNavigator != nullptr && autoNavigator->getEnabled()) {
        autoNavigator->executeNext();
    }
    
    // Update WiFi Manager state (check connection status)
    if (wifiManager != nullptr) {
        wifiManager->update();
    }
    
    // Auto-transition to login screen when WiFi is connected
    if (wifiManager != nullptr && wifiManager->isConnected() && 
        loginScreen != nullptr && !hasTransitionedToLogin &&
        !isSocialScreenActive && !isChatScreenActive) {
        // Check if user hasn't logged in yet
        if (!loginScreen->isAuthenticated()) {
            Serial.println("Main: WiFi connected, transitioning to login screen...");
            loginScreen->resetToUsernameStep();
            loginScreen->draw();
            hasTransitionedToLogin = true;
        }
    }
    
    // Reset transition flag if WiFi disconnects
    if (wifiManager != nullptr && !wifiManager->isConnected()) {
        hasTransitionedToLogin = false;
    }
    
    // Không cần gọi socketManager->update() nữa
    // Task đã chạy while(true) tự động sau khi begin()
    // Task xử lý tất cả: webSocket.loop(), keep-alive ping, etc.
    
    // Update ChatScreen decor animation if active
    if (isChatScreenActive && chatScreen != nullptr) {
        chatScreen->updateDecorAnimation();
    }
    
    // Update SocialScreen
    if (isSocialScreenActive && socialScreen != nullptr) {
        socialScreen->update();
    }
    
    delay(10);  // Fast polling for ultra real-time VR1 response
}
