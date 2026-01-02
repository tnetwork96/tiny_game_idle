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
#include "chat_screen.h"

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

    // Flag to track if auto-connect has been executed
bool autoConnectExecuted = false;
bool isChatScreenActive = false;
int currentChatFriendUserId = -1;  // ID của friend đang chat (nếu ChatScreen đang mở)
bool autoLoginExecuted = false;
bool autoNicknameExecuted = false;
bool isSocialScreenActive = false;

// Callback function for keyboard input - routes to appropriate screen
void onKeyboardKeySelected(String key) {
    // Route to ChatScreen if active
    if (isChatScreenActive && chatScreen != nullptr) {
        chatScreen->handleKeyPress(key);
    }
    // Route to SocialScreen if active
    else if (isSocialScreenActive && socialScreen != nullptr) {
        socialScreen->handleKeyPress(key);
    }
    // Route to LoginScreen if WiFi is connected and login screen is active
    else if (wifiManager != nullptr && wifiManager->isConnected() && loginScreen != nullptr && !isSocialScreenActive && !isChatScreenActive) {
        loginScreen->handleKeyPress(key);
    }
    // Otherwise route to WiFi manager
    else if (wifiManager != nullptr) {
        wifiManager->handleKeyboardInput(key);
    }
}

// Callback function for MiniKeyboard - routes to Add Friend screen (giống Keyboard gốc)
void onMiniKeyboardKeySelected(String key) {
    // Route to ChatScreen if active
    if (isChatScreenActive && chatScreen != nullptr) {
        chatScreen->handleKeyPress(key);
    }
    // Route to SocialScreen if active (Add Friend tab)
    else if (isSocialScreenActive && socialScreen != nullptr) {
        socialScreen->handleKeyPress(key);
    }
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
        
        // Update SocketManager state
        if (socketManager != nullptr) {
            socketManager->setChatScreen(nullptr);
            socketManager->setChatScreenActive(false);
            socketManager->setCurrentChatFriendUserId(-1);
        }
        
        isSocialScreenActive = true;
        
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
    isSocialScreenActive = false;
    
    // Update SocketManager state
    if (socketManager != nullptr) {
        socketManager->setSocialScreenActive(false);
    }
    
    // Disable main keyboard drawing when chat screen is active
    if (keyboard != nullptr) {
        keyboard->setDrawingEnabled(false);
    }
    
    // Force redraw chat screen
    chatScreen->forceRedraw();
}

// Callback function for when login is successful
void onLoginSuccess() {
    Serial.println("Main: Login successful, switching to Social Screen...");
    
    if (loginScreen != nullptr && socialScreen != nullptr) {
        // Set user ID and server info
        int userId = loginScreen->getUserId();
        socialScreen->setUserId(userId);
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
            Serial.println("Main: Set user status update callback");
            
            Serial.println("Main: Socket Manager initialized");
        }
        
        // Load data
        socialScreen->loadFriends();
        socialScreen->loadNotifications();
        
        // Activate social screen
        isSocialScreenActive = true;
        
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
        keyboard->typeString("player2");
        
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
        
        // Type PIN for player2: "2222"
        keyboard->typeString("2222");
        
        Serial.println("Auto-login: PIN typed, waiting before pressing Enter...");
        delay(1000);
        
        // Press Enter to login
        Serial.println("Auto-login: Pressing Enter to login...");
        keyboard->pressEnter();
        
        autoLoginExecuted = true;
        Serial.println("Auto-login: Login initiated!");
    }
}

// Function to automatically type a random nickname
void autoNickname() {
    if (loginScreen == nullptr || keyboard == nullptr) return;
    if (autoNicknameExecuted) return;
    
    // Check if we're on nickname screen
    if (loginScreen->isOnNicknameStep()) {
        Serial.println("Auto-nickname: Nickname screen ready, typing random nickname...");
        
        // Đợi một chút để màn hình sẵn sàng
        delay(1000);
        
        // List of random nicknames
        const char* nicknames[] = {
            "CoolPlayer",
            "GameMaster",
            "ProGamer",
            "StarPlayer",
            "Champion",
            "Hero",
            "Warrior",
            "Legend",
            "Ace",
            "Elite"
        };
        
        // Select a random nickname (using millis() for pseudo-random)
        int nicknameCount = sizeof(nicknames) / sizeof(nicknames[0]);
        int randomIndex = millis() % nicknameCount;
        String selectedNickname = String(nicknames[randomIndex]);
        
        Serial.print("Auto-nickname: Selected nickname: ");
        Serial.println(selectedNickname);
        
        // Type the nickname
        keyboard->typeString(selectedNickname);
        
        Serial.println("Auto-nickname: Nickname typed, waiting before pressing Enter...");
        delay(1000);
        
        // Press Enter to confirm
        Serial.println("Auto-nickname: Pressing Enter to confirm...");
        keyboard->pressEnter();
        
        autoNicknameExecuted = true;
        Serial.println("Auto-nickname: Nickname confirmed!");
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
    
    // Auto-nickname when nickname screen appears (only once)
    if (loginScreen != nullptr && loginScreen->isOnNicknameStep()) {
        if (!autoNicknameExecuted) {
            // Đợi một chút để màn hình nickname sẵn sàng
            delay(1000);
            
            // Bắt đầu auto nickname
            autoNickname();
        }
    } else {
        // Reset flag when nickname screen is no longer active
        if (autoNicknameExecuted) {
            autoNicknameExecuted = false;
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
    
    // Auto-navigation flow for Friends (chat) - Đợi 10 giây rồi chuyển sang Notifications
    if (isSocialScreenActive && socialScreen != nullptr && !isChatScreenActive) {
        static unsigned long friendsTabStartTime = 0;
        static bool hasSwitchedToNotifications = false;
        static bool notificationProcessed = false;  // Track if notification was processed
        static bool hasCheckedUnreadFriend = false;  // Track if đã check unread friend
        
        // Nếu đang ở Friends tab
        if (socialScreen->getCurrentTab() == SocialScreen::TAB_FRIENDS) {
            // Ghi lại thời gian khi vào Friends tab (chỉ lần đầu)
            if (friendsTabStartTime == 0) {
                friendsTabStartTime = millis();
                hasSwitchedToNotifications = false;
                notificationProcessed = false;  // Reset flag
                hasCheckedUnreadFriend = false;  // Reset flag
                Serial.println("Friends: Started timer - will switch to Notifications after 10 seconds");
            }
            
            // Tự động chọn và mở chat với friend có tin nhắn chưa đọc (chỉ một lần khi vào tab)
            if (!hasCheckedUnreadFriend) {
                int unreadFriendIndex = socialScreen->getFirstFriendWithUnreadIndex();
                if (unreadFriendIndex >= 0) {
                    Serial.print("Friends: Found friend with unread messages at index: ");
                    Serial.println(unreadFriendIndex);
                    
                    // Chọn friend có unread
                    socialScreen->selectFriend(unreadFriendIndex);
                    delay(500);  // Đợi UI update
                    
                    // Tự động mở chat
                    Serial.println("Friends: Auto-opening chat with friend who has unread messages");
                    socialScreen->openChatWithFriend(unreadFriendIndex);
                    
                    hasCheckedUnreadFriend = true;
                } else {
                    Serial.println("Friends: No friends with unread messages");
                    hasCheckedUnreadFriend = true;  // Đánh dấu đã check, không có unread
                }
            }
            
            // Kiểm tra nếu đã qua 10 giây (10000 milliseconds)
            unsigned long currentTime = millis();
            unsigned long elapsedTime = currentTime - friendsTabStartTime;
            
            if (!hasSwitchedToNotifications && elapsedTime >= 10000) {
                Serial.println("Friends: 10 seconds elapsed, switching to Notifications tab");
                socialScreen->navigateToNotifications();
                hasSwitchedToNotifications = true;
                notificationProcessed = false;  // Reset để xử lý notification
                friendsTabStartTime = 0;  // Reset timer
                hasCheckedUnreadFriend = false;  // Reset để check lại lần sau
            }
        } 
        // Nếu đang ở Notifications tab
        else if (socialScreen->getCurrentTab() == SocialScreen::TAB_NOTIFICATIONS) {
            // Reset Friends tab timer nếu đang ở Notifications
            if (friendsTabStartTime != 0) {
                friendsTabStartTime = 0;
                hasSwitchedToNotifications = false;
            }
            
            // Timer để đợi 10 giây rồi chuyển về Friends tab
            static unsigned long notificationsTabStartTime = 0;
            static bool hasSwitchedToFriends = false;
            
            // Ghi lại thời gian khi vào Notifications tab (chỉ lần đầu)
            if (notificationsTabStartTime == 0) {
                notificationsTabStartTime = millis();
                hasSwitchedToFriends = false;
                Serial.println("Notifications: Started timer - will switch to Friends after 10 seconds");
            }
            
            // Kiểm tra xem có notification không
            int notificationsCount = socialScreen->getNotificationsCount();
            
            // Nếu có notification và chưa xử lý hết
            if (notificationsCount > 0 && !notificationProcessed) {
                // Track state để xử lý từng notification một
                static bool waitingForDialogClose = false;
                static unsigned long dialogCloseWaitTime = 0;
                
                // Nếu confirmation dialog đang hiển thị, tự động điều hướng sang NO và select
                if (socialScreen->isConfirmationDialogVisible()) {
                    static unsigned long dialogShowTime = 0;
                    static bool navRightDone = false;
                    static bool selectDone = false;
                    
                    if (dialogShowTime == 0) {
                        dialogShowTime = millis();
                        navRightDone = false;
                        selectDone = false;
                        Serial.println("Notifications: Confirmation dialog detected, will navigate to NO and select");
                    }
                    
                    unsigned long currentTime = millis();
                    unsigned long elapsedTime = currentTime - dialogShowTime;
                    
                    // Sau 0.5 giây, điều hướng sang NO (right)
                    if (!navRightDone && elapsedTime >= 500) {
                        Serial.println("Notifications: Auto-navigation - Moving to NO (right)");
                        socialScreen->handleKeyPress("|r");  // Move right to NO button
                        navRightDone = true;
                    }
                    
                    // Sau 1 giây, tự động chọn NO
                    if (!selectDone && navRightDone && elapsedTime >= 1000) {
                        Serial.println("Notifications: Auto-selecting NO in confirmation dialog");
                        socialScreen->handleKeyPress("|e");  // Select NO (now selected)
                        selectDone = true;
                        dialogShowTime = 0;  // Reset for next time
                        navRightDone = false;  // Reset flags
                        waitingForDialogClose = true;  // Đợi dialog đóng
                        dialogCloseWaitTime = millis();  // Ghi lại thời gian bắt đầu đợi
                        Serial.println("Notifications: Notification processed, waiting for dialog to close");
                    }
                }
                // Nếu đang đợi dialog đóng
                else if (waitingForDialogClose) {
                    unsigned long currentTime = millis();
                    unsigned long waitTime = currentTime - dialogCloseWaitTime;
                    
                    // Đợi 1 giây sau khi dialog đóng để UI update và notification được xóa
                    if (waitTime >= 1000) {
                        waitingForDialogClose = false;
                        dialogCloseWaitTime = 0;
                        
                        // Kiểm tra lại số lượng notifications
                        int newCount = socialScreen->getNotificationsCount();
                        Serial.print("Notifications: Dialog closed. Remaining notifications: ");
                        Serial.println(newCount);
                        
                        // Nếu vẫn còn notifications, tiếp tục xử lý (reset để tìm notification đầu tiên)
                        if (newCount > 0) {
                            Serial.println("Notifications: Continuing to process next notification");
                            // Reset để tiếp tục xử lý notification tiếp theo
                            // (firstNotificationSelected sẽ được reset trong else block)
                        } else {
                            // Không còn notification nào, đánh dấu đã xử lý hết
                            Serial.println("Notifications: All notifications processed");
                            notificationProcessed = true;
                        }
                    }
                }
                // Nếu không có dialog và không đang đợi, tìm và chọn thông báo đầu tiên
                else {
                    static bool firstNotificationSelected = false;
                    
                    // Tìm và chọn thông báo đầu tiên (friend request)
                    if (!firstNotificationSelected) {
                        int firstRequestIndex = socialScreen->getFirstFriendRequestIndex();
                        if (firstRequestIndex >= 0) {
                            Serial.print("Notifications: Found friend request notification at index: ");
                            Serial.println(firstRequestIndex);
                            
                            // Chọn notification đầu tiên
                            socialScreen->selectNotification(firstRequestIndex);
                            delay(300);  // Đợi một chút để UI update
                            
                            // Nhấn Enter để mở confirmation dialog
                            Serial.println("Notifications: Pressing Enter to open confirmation dialog");
                            socialScreen->handleKeyPress("|e");
                            
                            firstNotificationSelected = true;
                        } else {
                            // Không có friend request, đánh dấu đã xử lý
                            Serial.println("Notifications: No unread friend requests found");
                            firstNotificationSelected = true;
                            notificationProcessed = true;  // No notifications to process
                        }
                    }
                    // Nếu đã chọn notification nhưng dialog chưa mở, reset để thử lại
                    else if (!socialScreen->isConfirmationDialogVisible() && !waitingForDialogClose) {
                        // Reset để tìm notification tiếp theo
                        firstNotificationSelected = false;
                    }
                }
            }
            // Nếu không có notification hoặc đã xử lý xong, đợi 10 giây rồi chuyển về Friends tab
            else if (notificationsCount == 0 || notificationProcessed) {
                unsigned long currentTime = millis();
                unsigned long elapsedTime = currentTime - notificationsTabStartTime;
                
                // Đợi 10 giây rồi chuyển về Friends tab
                if (!hasSwitchedToFriends && elapsedTime >= 10000) {
                    Serial.println("Notifications: 10 seconds elapsed, switching to Friends tab");
                    socialScreen->navigateToFriends();
                    hasSwitchedToFriends = true;
                    notificationsTabStartTime = 0;  // Reset timer
                    notificationProcessed = false;  // Reset for next cycle
                }
            }
        } 
        // Nếu ở tab khác, đảm bảo chuyển về Friends
        else {
            // Reset timer
            if (friendsTabStartTime != 0) {
                friendsTabStartTime = 0;
                hasSwitchedToNotifications = false;
                notificationProcessed = false;
            }
            
            // Reset Notifications tab timer variables (không cần làm gì vì static variables sẽ tự reset khi vào Notifications tab)
        }
    }
    
    // Không cần gọi socketManager->update() nữa
    // Task đã chạy while(true) tự động sau khi begin()
    // Task xử lý tất cả: webSocket.loop(), keep-alive ping, etc.
    
    // Update ChatScreen decor animation if active
    if (isChatScreenActive && chatScreen != nullptr) {
        chatScreen->updateDecorAnimation();
    }
    
    // Popup notification disabled - no need to update popup
    
    delay(100);
}
