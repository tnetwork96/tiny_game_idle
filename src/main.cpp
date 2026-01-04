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
ChatScreen* chatScreen = nullptr;
AutoNavigator* autoNavigator = nullptr;

bool isChatScreenActive = false;
int currentChatFriendUserId = -1;  // ID của friend đang chat (nếu ChatScreen đang mở)
bool isSocialScreenActive = false;
bool hasTransitionedToLogin = false;  // Track if we've already transitioned to login screen

// Forward declaration
void onKeyboardKeySelected(const String& key);

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
        
        // 2c. MiniAddFriendScreen (child)
        if (socialScreen->getCurrentTab() == SocialScreen::TAB_ADD_FRIEND) {
            if (socialScreen->getMiniAddFriend() != nullptr && 
                socialScreen->getMiniAddFriend()->isActive()) {
                Serial.println("Input: Routing to MiniAddFriendScreen");
                socialScreen->getMiniAddFriend()->handleKeyPress(key);
                return;
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
        // Check if on Add Friend tab and MiniAddFriend is active
        if (socialScreen->getCurrentTab() == SocialScreen::TAB_ADD_FRIEND) {
            if (socialScreen->getMiniAddFriend() != nullptr && 
                socialScreen->getMiniAddFriend()->isActive()) {
                Serial.println("MiniKeyboard Input: Routing to MiniAddFriendScreen");
                socialScreen->getMiniAddFriend()->handleKeyPress(key);
                return;
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
    
    // Write username and PIN to file (one per line)
    file.println(username);
    file.println(pin);
    file.close();
    
    Serial.print("Main: Login credentials saved successfully to: ");
    Serial.println(fileName);
    Serial.print("Main: Username: ");
    Serial.println(username);
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
        
        // Navigate to Notifications tab để test badge trên Friends icon
        // (Comment out navigateToFriends() để không clear badge khi vào Friends tab)
        // socialScreen->navigateToFriends();
        socialScreen->navigateToNotifications();
        
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
    
    delay(100);
}
