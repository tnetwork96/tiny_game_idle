#include "social_screen.h"

// 16x16 "User List" (Friends/Buddy List)
const unsigned char PROGMEM iconUserList[] = {
    0x00, 0x00, 0x07, 0xe0, 0x18, 0x18, 0x20, 0x04, 
    0x20, 0x04, 0x18, 0x18, 0x07, 0xe0, 0x00, 0x00, 
    0x0f, 0xf0, 0x18, 0x18, 0x20, 0x04, 0x20, 0x04, 
    0x20, 0x04, 0x20, 0x04, 0x18, 0x18, 0x0f, 0xf0
};

// 16x16 "Bell" (Notifications)
const unsigned char PROGMEM iconBell[] = {
    0x00, 0x00, 0x01, 0x80, 0x02, 0x40, 0x04, 0x20, 
    0x08, 0x10, 0x08, 0x10, 0x08, 0x10, 0x08, 0x10, 
    0x10, 0x08, 0x10, 0x08, 0x20, 0x04, 0x3f, 0xfc, 
    0x40, 0x02, 0x40, 0x02, 0x3f, 0xfc, 0x00, 0x00
};

// 16x16 "Plus" (Add Friend)
const unsigned char PROGMEM iconPlus[] = {
    0x00, 0x00, 0x00, 0x00, 0x01, 0x80, 0x01, 0x80, 
    0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x7f, 0xfe, 
    0x7f, 0xfe, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 
    0x01, 0x80, 0x01, 0x80, 0x00, 0x00, 0x00, 0x00
};

// Deep Space Arcade Theme
#define SOCIAL_BG_DARK   0x0042  // Deep Midnight Blue #020817
#define SOCIAL_HEADER    0x08A5  // Header Blue #0F172A
#define SOCIAL_ACCENT    0x07FF  // Cyan accent
#define SOCIAL_TEXT      0xFFFF  // White text
#define SOCIAL_MUTED     0x8410  // Muted gray text
#define SOCIAL_HIGHLIGHT 0x1148  // Electric Blue tint
#define SOCIAL_SUCCESS   0x07F3  // Minty Green for online
#define SOCIAL_ERROR     0xF986  // Bright Red for offline

// Layout constants
#define SIDEBAR_WIDTH    45
#define CONTENT_X        45
#define CONTENT_WIDTH    275
#define SCREEN_HEIGHT    240
#define TAB_HEIGHT       60
#define TAB_PADDING      10

SocialScreen::SocialScreen(Adafruit_ST7789* tft, Keyboard* keyboard) {
    this->tft = tft;
    this->keyboard = keyboard;
    this->addFriendScreen = new AddFriendScreen(tft, keyboard);
    this->currentTab = TAB_FRIENDS;
    this->focusMode = FOCUS_SIDEBAR;
    this->userId = -1;
    this->serverHost = "";
    this->serverPort = 8080;
    
    this->friends = nullptr;
    this->friendsCount = 0;
    this->selectedFriendIndex = 0;
    this->friendsScrollOffset = 0;
    
    this->notifications = nullptr;
    this->notificationsCount = 0;
    this->selectedNotificationIndex = 0;
    this->notificationsScrollOffset = 0;
    
    this->onAddFriendSuccessCallback = nullptr;
}

SocialScreen::~SocialScreen() {
    clearFriends();
    clearNotifications();
    if (addFriendScreen != nullptr) {
        delete addFriendScreen;
    }
}

void SocialScreen::drawBackground() {
    tft->fillScreen(SOCIAL_BG_DARK);
}

void SocialScreen::drawTab(int tabIndex, bool isSelected) {
    uint16_t y = tabIndex * TAB_HEIGHT;
    uint16_t bgColor = isSelected ? SOCIAL_HIGHLIGHT : SOCIAL_HEADER;
    
    // Draw tab background
    tft->fillRect(0, y, SIDEBAR_WIDTH, TAB_HEIGHT, bgColor);
    
    // Draw left border if selected
    if (isSelected) {
        tft->fillRect(0, y, 3, TAB_HEIGHT, SOCIAL_ACCENT);
    }
    
    // Draw focus indicator: right border when sidebar has focus and this tab is selected
    if (isSelected && focusMode == FOCUS_SIDEBAR) {
        // Draw right border to indicate sidebar focus
        tft->drawFastVLine(SIDEBAR_WIDTH - 2, y, TAB_HEIGHT, SOCIAL_ACCENT);
        tft->drawFastVLine(SIDEBAR_WIDTH - 1, y, TAB_HEIGHT, SOCIAL_ACCENT);
    }
    
    // Calculate icon position (centered in tab)
    uint16_t iconSize = 16;  // Bitmap is 16x16
    uint16_t iconX = (SIDEBAR_WIDTH - iconSize) / 2;
    uint16_t iconY = y + (TAB_HEIGHT - iconSize) / 2;
    uint16_t iconColor = isSelected ? SOCIAL_ACCENT : SOCIAL_TEXT;
    
    // Draw icon based on tab using bitmap
    switch (tabIndex) {
        case TAB_FRIENDS:
            drawFriendsIcon(iconX, iconY, iconColor);
            break;
        case TAB_NOTIFICATIONS:
            drawNotificationsIcon(iconX, iconY, iconColor);
            break;
        case TAB_ADD_FRIEND:
            drawAddFriendIcon(iconX, iconY, iconColor);
            break;
    }
}

void SocialScreen::drawFriendsIcon(uint16_t x, uint16_t y, uint16_t color) {
    // Draw bitmap icon (16x16)
    tft->drawBitmap(x, y, iconUserList, 16, 16, color);
}

void SocialScreen::drawNotificationsIcon(uint16_t x, uint16_t y, uint16_t color) {
    // Draw bitmap icon (16x16)
    tft->drawBitmap(x, y, iconBell, 16, 16, color);
}

void SocialScreen::drawAddFriendIcon(uint16_t x, uint16_t y, uint16_t color) {
    // Draw bitmap icon (16x16)
    tft->drawBitmap(x, y, iconPlus, 16, 16, color);
}

void SocialScreen::drawSidebar() {
    // Draw sidebar background
    tft->fillRect(0, 0, SIDEBAR_WIDTH, SCREEN_HEIGHT, SOCIAL_HEADER);
    
    // Draw divider line
    tft->drawFastVLine(SIDEBAR_WIDTH - 1, 0, SCREEN_HEIGHT, SOCIAL_ACCENT);
    
    // Draw all tabs
    for (int i = 0; i < 3; i++) {
        drawTab(i, (i == currentTab));
    }
}

void SocialScreen::drawFriendsList() {
    // Clear content area
    tft->fillRect(CONTENT_X, 0, CONTENT_WIDTH, SCREEN_HEIGHT, SOCIAL_BG_DARK);
    
    // Draw focus indicator: left border when content has focus
    if (focusMode == FOCUS_CONTENT) {
        tft->drawFastVLine(CONTENT_X, 0, SCREEN_HEIGHT, SOCIAL_ACCENT);
        tft->drawFastVLine(CONTENT_X + 1, 0, SCREEN_HEIGHT, SOCIAL_ACCENT);
    }
    
    // Draw header
    tft->setTextSize(2);
    tft->setTextColor(SOCIAL_TEXT, SOCIAL_BG_DARK);
    tft->setCursor(CONTENT_X + 10, 10);
    tft->print("FRIENDS");
    
    // Draw friends list
    if (friendsCount == 0) {
        tft->setTextSize(1);
        tft->setTextColor(SOCIAL_MUTED, SOCIAL_BG_DARK);
        tft->setCursor(CONTENT_X + 10, 50);
        tft->print("No friends yet");
    } else {
        const uint16_t itemHeight = 24;
        const uint16_t startY = 40;
        const int visibleItems = (SCREEN_HEIGHT - startY) / itemHeight;
        
        // Calculate which items to show
        int startIndex = friendsScrollOffset;
        int endIndex = (startIndex + visibleItems < friendsCount) ? (startIndex + visibleItems) : friendsCount;
        
        for (int i = startIndex; i < endIndex; i++) {
            uint16_t y = startY + (i - startIndex) * itemHeight;
            bool isSelected = (i == selectedFriendIndex);
            
            // Draw item background if selected
            if (isSelected) {
                tft->fillRect(CONTENT_X, y, CONTENT_WIDTH, itemHeight, SOCIAL_HIGHLIGHT);
                tft->fillRect(CONTENT_X, y, 2, itemHeight, SOCIAL_ACCENT);
            }
            
            // Draw online/offline indicator
            uint16_t statusColor = friends[i].online ? SOCIAL_SUCCESS : SOCIAL_ERROR;
            tft->fillCircle(CONTENT_X + 15, y + itemHeight / 2, 4, statusColor);
            
            // Draw username
            tft->setTextSize(1);
            tft->setTextColor(SOCIAL_TEXT, isSelected ? SOCIAL_HIGHLIGHT : SOCIAL_BG_DARK);
            tft->setCursor(CONTENT_X + 25, y + 8);
            tft->print(friends[i].username);
            
            // Draw online status text
            tft->setTextColor(SOCIAL_MUTED, isSelected ? SOCIAL_HIGHLIGHT : SOCIAL_BG_DARK);
            tft->setCursor(CONTENT_X + 25, y + 16);
            tft->print(friends[i].online ? "Online" : "Offline");
        }
    }
}

void SocialScreen::drawNotificationsList() {
    // Clear content area
    tft->fillRect(CONTENT_X, 0, CONTENT_WIDTH, SCREEN_HEIGHT, SOCIAL_BG_DARK);
    
    // Draw focus indicator: left border when content has focus
    if (focusMode == FOCUS_CONTENT) {
        tft->drawFastVLine(CONTENT_X, 0, SCREEN_HEIGHT, SOCIAL_ACCENT);
        tft->drawFastVLine(CONTENT_X + 1, 0, SCREEN_HEIGHT, SOCIAL_ACCENT);
    }
    
    // Draw header
    tft->setTextSize(2);
    tft->setTextColor(SOCIAL_TEXT, SOCIAL_BG_DARK);
    tft->setCursor(CONTENT_X + 10, 10);
    tft->print("THONG BAO");
    
    // Draw notifications list
    if (notificationsCount == 0) {
        tft->setTextSize(1);
        tft->setTextColor(SOCIAL_MUTED, SOCIAL_BG_DARK);
        tft->setCursor(CONTENT_X + 10, 50);
        tft->print("No notifications");
    } else {
        const uint16_t itemHeight = 40;
        const uint16_t startY = 40;
        const int visibleItems = (SCREEN_HEIGHT - startY) / itemHeight;
        
        // Calculate which items to show
        int startIndex = notificationsScrollOffset;
        int endIndex = (startIndex + visibleItems < notificationsCount) ? (startIndex + visibleItems) : notificationsCount;
        
        for (int i = startIndex; i < endIndex; i++) {
            uint16_t y = startY + (i - startIndex) * itemHeight;
            bool isSelected = (i == selectedNotificationIndex);
            
            // Draw item background if selected
            if (isSelected) {
                tft->fillRect(CONTENT_X, y, CONTENT_WIDTH, itemHeight, SOCIAL_HIGHLIGHT);
                tft->fillRect(CONTENT_X, y, 2, itemHeight, SOCIAL_ACCENT);
            }
            
            // Draw notification type indicator
            uint16_t typeColor = SOCIAL_ACCENT;
            tft->fillCircle(CONTENT_X + 15, y + itemHeight / 2, 4, typeColor);
            
            // Draw message
            tft->setTextSize(1);
            tft->setTextColor(SOCIAL_TEXT, isSelected ? SOCIAL_HIGHLIGHT : SOCIAL_BG_DARK);
            tft->setCursor(CONTENT_X + 25, y + 6);
            
            // Truncate message if too long
            String message = notifications[i].message;
            if (message.length() > 25) {
                message = message.substring(0, 22) + "...";
            }
            tft->print(message);
            
            // Draw timestamp if available
            if (notifications[i].timestamp.length() > 0) {
                tft->setTextColor(SOCIAL_MUTED, isSelected ? SOCIAL_HIGHLIGHT : SOCIAL_BG_DARK);
                tft->setCursor(CONTENT_X + 25, y + 18);
                String timestamp = notifications[i].timestamp;
                if (timestamp.length() > 20) {
                    timestamp = timestamp.substring(0, 17) + "...";
                }
                tft->print(timestamp);
            }
        }
    }
}

void SocialScreen::drawAddFriendContent() {
    // Clear content area
    tft->fillRect(CONTENT_X, 0, CONTENT_WIDTH, SCREEN_HEIGHT, SOCIAL_BG_DARK);
    
    // Draw focus indicator: left border when content has focus
    if (focusMode == FOCUS_CONTENT) {
        tft->drawFastVLine(CONTENT_X, 0, SCREEN_HEIGHT, SOCIAL_ACCENT);
        tft->drawFastVLine(CONTENT_X + 1, 0, SCREEN_HEIGHT, SOCIAL_ACCENT);
    }
    
    // Draw AddFriendScreen content manually (adapted for smaller area)
    const uint16_t headerHeight = 30;
    
    // Draw header
    tft->fillRect(CONTENT_X, 0, CONTENT_WIDTH, headerHeight, SOCIAL_HEADER);
    tft->drawFastHLine(CONTENT_X, headerHeight - 1, CONTENT_WIDTH, SOCIAL_ACCENT);
    
    // Title in header
    tft->setTextSize(2);
    tft->setTextColor(SOCIAL_TEXT, SOCIAL_HEADER);
    tft->setCursor(CONTENT_X + 10, 8);
    tft->print("ADD FRIEND");
    
    // Draw input box (scaled for content area)
    uint16_t inputX = CONTENT_X + 10;
    uint16_t inputY = headerHeight + 10;
    uint16_t inputW = CONTENT_WIDTH - 20;
    uint16_t inputH = 34;
    
    String friendName = addFriendScreen->getFriendName();
    
    // Background with Deep Midnight Blue
    tft->fillRoundRect(inputX, inputY, inputW, inputH, 6, SOCIAL_BG_DARK);
    // Cyan border for accent
    tft->drawRoundRect(inputX, inputY, inputW, inputH, 6, SOCIAL_ACCENT);
    tft->setCursor(inputX + 8, inputY + (inputH / 2) - 6);
    tft->setTextSize(2);
    
    if (friendName.length() == 0) {
        tft->setTextColor(SOCIAL_MUTED, SOCIAL_BG_DARK);
        tft->print("Enter name");
    } else {
        tft->setTextColor(SOCIAL_TEXT, SOCIAL_BG_DARK);
        tft->print(friendName);
    }
    
    // Instructions
    tft->setTextSize(1);
    tft->setTextColor(SOCIAL_TEXT, SOCIAL_BG_DARK);
    tft->setCursor(inputX, inputY + inputH + 10);
    tft->print("Press Enter to add");
}

void SocialScreen::drawContentArea() {
    switch (currentTab) {
        case TAB_FRIENDS:
            drawFriendsList();
            break;
        case TAB_NOTIFICATIONS:
            drawNotificationsList();
            break;
        case TAB_ADD_FRIEND:
            drawAddFriendContent();
            break;
    }
}

void SocialScreen::draw() {
    drawBackground();
    drawSidebar();
    drawContentArea();
}

void SocialScreen::handleTabNavigation(const String& key) {
    if (key == "|u") {
        // Move to previous tab
        if (currentTab > TAB_FRIENDS) {
            switchTab((Tab)(currentTab - 1));
        }
    } else if (key == "|d") {
        // Move to next tab
        if (currentTab < TAB_ADD_FRIEND) {
            switchTab((Tab)(currentTab + 1));
        }
    }
}

void SocialScreen::handleContentNavigation(const String& key) {
    if (currentTab == TAB_ADD_FRIEND) {
        // Forward to AddFriendScreen
        addFriendScreen->handleKeyPress(key);
        
        // Check if friend was confirmed
        if (addFriendScreen->isConfirmed()) {
            String friendName = addFriendScreen->getFriendName();
            Serial.print("Social Screen: Friend name confirmed: ");
            Serial.println(friendName);
            
            // TODO: Call API to add friend here
            // For now, just reset and call callback
            addFriendScreen->reset();
            
            // Refresh friends list
            loadFriends();
            
            // Call callback if set
            if (onAddFriendSuccessCallback != nullptr) {
                onAddFriendSuccessCallback();
            }
        }
        
        // Redraw content area
        drawContentArea();
    } else if (currentTab == TAB_FRIENDS) {
        // Navigate friends list
        if (key == "|u" && selectedFriendIndex > 0) {
            selectedFriendIndex--;
            // Adjust scroll if needed
            if (selectedFriendIndex < friendsScrollOffset) {
                friendsScrollOffset = selectedFriendIndex;
            }
            drawContentArea();
        } else if (key == "|d" && selectedFriendIndex < friendsCount - 1) {
            selectedFriendIndex++;
            // Adjust scroll if needed
            const int visibleItems = (SCREEN_HEIGHT - 40) / 24;
            if (selectedFriendIndex >= friendsScrollOffset + visibleItems) {
                friendsScrollOffset = selectedFriendIndex - visibleItems + 1;
            }
            drawContentArea();
        }
    } else if (currentTab == TAB_NOTIFICATIONS) {
        // Navigate notifications list
        if (key == "|u" && selectedNotificationIndex > 0) {
            selectedNotificationIndex--;
            // Adjust scroll if needed
            if (selectedNotificationIndex < notificationsScrollOffset) {
                notificationsScrollOffset = selectedNotificationIndex;
            }
            drawContentArea();
        } else if (key == "|d" && selectedNotificationIndex < notificationsCount - 1) {
            selectedNotificationIndex++;
            // Adjust scroll if needed
            const int visibleItems = (SCREEN_HEIGHT - 40) / 40;
            if (selectedNotificationIndex >= notificationsScrollOffset + visibleItems) {
                notificationsScrollOffset = selectedNotificationIndex - visibleItems + 1;
            }
            drawContentArea();
        }
    }
}

void SocialScreen::switchTab(Tab newTab) {
    if (currentTab != newTab) {
        currentTab = newTab;
        // Reset focus to sidebar when switching tabs
        focusMode = FOCUS_SIDEBAR;
        // Reset selection indices when switching tabs
        selectedFriendIndex = 0;
        friendsScrollOffset = 0;
        selectedNotificationIndex = 0;
        notificationsScrollOffset = 0;
        draw();
    }
}

void SocialScreen::handleKeyPress(const String& key) {
    // If on Add Friend tab and typing, forward to AddFriendScreen
    if (currentTab == TAB_ADD_FRIEND && 
        (key.length() == 1 || key == "|e" || key == "<" || key == "123" || key == "ABC")) {
        // When typing in Add Friend, focus should be on content
        focusMode = FOCUS_CONTENT;
        handleContentNavigation(key);
        return;
    }
    
    // Left/Right: Switch focus between sidebar and content
    if (key == "|l") {
        // Left: Move focus to sidebar
        if (focusMode == FOCUS_CONTENT) {
            focusMode = FOCUS_SIDEBAR;
            draw();  // Redraw to show focus change
        }
        return;
    } else if (key == "|r") {
        // Right: Move focus to content
        if (focusMode == FOCUS_SIDEBAR) {
            focusMode = FOCUS_CONTENT;
            draw();  // Redraw to show focus change
        }
        return;
    }
    
    // Up/Down: Navigate based on focus mode
    if (key == "|u" || key == "|d") {
        if (focusMode == FOCUS_SIDEBAR) {
            // Focus on sidebar: navigate between tabs
            handleTabNavigation(key);
        } else if (focusMode == FOCUS_CONTENT) {
            // Focus on content: navigate within content (scroll lists)
            handleContentNavigation(key);
        }
        return;
    }
    
    // Enter key: handle based on current tab and focus
    if (key == "|e") {
        if (focusMode == FOCUS_CONTENT) {
            handleContentNavigation(key);
        }
        // If focus is on sidebar, Enter could select the tab (but tab is already selected)
        // So we move focus to content when Enter is pressed on sidebar
        else if (focusMode == FOCUS_SIDEBAR) {
            focusMode = FOCUS_CONTENT;
            draw();
        }
        return;
    }
    
    // Forward other keys to content navigation if on Add Friend tab
    if (currentTab == TAB_ADD_FRIEND) {
        handleContentNavigation(key);
    }
}

void SocialScreen::parseFriendsString(const String& friendsString) {
    clearFriends();
    
    if (friendsString.length() == 0) {
        friendsCount = 0;
        return;
    }
    
    // Parse format: "user1,0|user2,1|..."
    // Count entries first
    int count = 1;
    for (int i = 0; i < friendsString.length(); i++) {
        if (friendsString.charAt(i) == '|') {
            count++;
        }
    }
    
    friendsCount = count;
    friends = new FriendItem[friendsCount];
    
    // Parse each entry
    int startPos = 0;
    int entryIndex = 0;
    for (int i = 0; i <= friendsString.length(); i++) {
        if (i == friendsString.length() || friendsString.charAt(i) == '|') {
            String entry = friendsString.substring(startPos, i);
            int commaPos = entry.indexOf(',');
            
            if (commaPos > 0 && entryIndex < friendsCount) {
                friends[entryIndex].username = entry.substring(0, commaPos);
                String onlineStr = entry.substring(commaPos + 1);
                friends[entryIndex].online = (onlineStr == "1");
                entryIndex++;
            }
            
            startPos = i + 1;
        }
    }
    
    friendsCount = entryIndex;  // Update actual count
}

void SocialScreen::clearFriends() {
    if (friends != nullptr) {
        delete[] friends;
        friends = nullptr;
    }
    friendsCount = 0;
    selectedFriendIndex = 0;
    friendsScrollOffset = 0;
}

void SocialScreen::clearNotifications() {
    if (notifications != nullptr) {
        delete[] notifications;
        notifications = nullptr;
    }
    notificationsCount = 0;
    selectedNotificationIndex = 0;
    notificationsScrollOffset = 0;
}

void SocialScreen::loadFriends() {
    if (userId <= 0 || serverHost.length() == 0) {
        Serial.println("Social Screen: Cannot load friends - invalid user ID or server info");
        return;
    }
    
    Serial.println("Social Screen: Loading friends list...");
    String friendsString = ApiClient::getFriendsList(userId, serverHost, serverPort);
    
    if (friendsString.length() > 0) {
        Serial.print("Social Screen: Received friends string: ");
        Serial.println(friendsString);
        parseFriendsString(friendsString);
        Serial.print("Social Screen: Parsed ");
        Serial.print(friendsCount);
        Serial.println(" friends");
    } else {
        Serial.println("Social Screen: No friends or empty string");
        clearFriends();
    }
    
    // Redraw if currently showing friends tab
    if (currentTab == TAB_FRIENDS) {
        drawContentArea();
    }
}

void SocialScreen::loadNotifications() {
    if (userId <= 0 || serverHost.length() == 0) {
        Serial.println("Social Screen: Cannot load notifications - invalid user ID or server info");
        return;
    }
    
    Serial.println("Social Screen: Loading notifications...");
    ApiClient::NotificationsResult result = ApiClient::getNotifications(userId, serverHost, serverPort);
    
    if (result.success) {
        Serial.print("Social Screen: Received ");
        Serial.print(result.count);
        Serial.println(" notifications");
        
        clearNotifications();
        notificationsCount = result.count;
        if (notificationsCount > 0) {
            notifications = new ApiClient::NotificationEntry[notificationsCount];
            for (int i = 0; i < notificationsCount; i++) {
                notifications[i] = result.notifications[i];
            }
        }
        
        // Clean up result memory
        if (result.notifications != nullptr) {
            delete[] result.notifications;
        }
    } else {
        Serial.print("Social Screen: Failed to load notifications: ");
        Serial.println(result.message);
        clearNotifications();
    }
    
    // Redraw if currently showing notifications tab
    if (currentTab == TAB_NOTIFICATIONS) {
        drawContentArea();
    }
}

void SocialScreen::reset() {
    currentTab = TAB_FRIENDS;
    focusMode = FOCUS_SIDEBAR;
    selectedFriendIndex = 0;
    friendsScrollOffset = 0;
    selectedNotificationIndex = 0;
    notificationsScrollOffset = 0;
    if (addFriendScreen != nullptr) {
        addFriendScreen->reset();
    }
}

