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
#define SOCIAL_SIDEBAR_BG 0x0021  // Darker Blue for sidebar
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
    this->miniKeyboard = new MiniKeyboard(tft);
    this->miniAddFriend = new MiniAddFriendScreen(tft, miniKeyboard);
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
    if (miniAddFriend != nullptr) {
        delete miniAddFriend;
    }
}

void SocialScreen::drawBackground() {
    tft->fillScreen(SOCIAL_BG_DARK);
}

void SocialScreen::drawTab(int tabIndex, bool isSelected) {
    uint16_t y = tabIndex * TAB_HEIGHT;
    
    // Draw tab background - inactive tabs use darker background
    uint16_t bgColor = isSelected ? SOCIAL_HIGHLIGHT : SOCIAL_SIDEBAR_BG;
    tft->fillRect(0, y, SIDEBAR_WIDTH, TAB_HEIGHT, bgColor);
    
    // Draw glowing left border for selected tab (3px wide cyan bar)
    if (isSelected) {
        tft->fillRect(0, y, 3, TAB_HEIGHT, SOCIAL_ACCENT);
        // Add subtle glow effect with a lighter line
        tft->drawFastVLine(3, y, TAB_HEIGHT, SOCIAL_HIGHLIGHT);
    }
    
    // Draw focus indicator: right border when sidebar has focus and this tab is selected
    if (isSelected && focusMode == FOCUS_SIDEBAR) {
        // Draw right border to indicate sidebar focus (bright)
        tft->drawFastVLine(SIDEBAR_WIDTH - 2, y, TAB_HEIGHT, SOCIAL_ACCENT);
        tft->drawFastVLine(SIDEBAR_WIDTH - 1, y, TAB_HEIGHT, SOCIAL_ACCENT);
    } else if (isSelected && focusMode == FOCUS_CONTENT) {
        // Dimmed right border when content has focus
        tft->drawFastVLine(SIDEBAR_WIDTH - 1, y, TAB_HEIGHT, SOCIAL_MUTED);
    }
    
    // Calculate icon position (centered in tab)
    uint16_t iconSize = 16;  // Bitmap is 16x16
    uint16_t iconX = (SIDEBAR_WIDTH - iconSize) / 2;
    uint16_t iconY = y + (TAB_HEIGHT - iconSize) / 2;
    
    // Icon color: bright when selected and sidebar focused, muted when inactive
    uint16_t iconColor;
    if (isSelected && focusMode == FOCUS_SIDEBAR) {
        iconColor = SOCIAL_TEXT;  // White when active and focused
    } else if (isSelected) {
        iconColor = SOCIAL_ACCENT;  // Cyan when active but not focused
    } else {
        iconColor = SOCIAL_MUTED;  // Muted when inactive
    }
    
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
    // Draw sidebar background with darker color
    tft->fillRect(0, 0, SIDEBAR_WIDTH, SCREEN_HEIGHT, SOCIAL_SIDEBAR_BG);
    
    // Draw divider line (subtle)
    tft->drawFastVLine(SIDEBAR_WIDTH - 1, 0, SCREEN_HEIGHT, SOCIAL_MUTED);
    
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
    
    // Draw modern header with count
    const uint16_t headerHeight = 35;
    tft->setTextSize(2);
    tft->setTextColor(SOCIAL_TEXT, SOCIAL_BG_DARK);
    tft->setCursor(CONTENT_X + 10, 8);
    tft->print("FRIENDS");
    
    // Draw count if available
    if (friendsCount > 0) {
        tft->setTextSize(1);
        tft->setTextColor(SOCIAL_MUTED, SOCIAL_BG_DARK);
        tft->setCursor(CONTENT_X + 10, 24);
        tft->print("[ ");
        tft->print(friendsCount);
        tft->print(" ]");
    }
    
    // Draw header line
    tft->drawFastHLine(CONTENT_X + 10, headerHeight - 2, CONTENT_WIDTH - 20, SOCIAL_ACCENT);
    
    // Draw friends list as cards
    if (friendsCount == 0) {
        tft->setTextSize(1);
        tft->setTextColor(SOCIAL_MUTED, SOCIAL_BG_DARK);
        tft->setCursor(CONTENT_X + 10, 60);
        tft->print("No friends yet");
    } else {
        const uint16_t cardHeight = 32;
        const uint16_t cardSpacing = 4;  // 2px padding on each side = 4px total
        const uint16_t startY = headerHeight + 4;
        const int visibleItems = (SCREEN_HEIGHT - startY) / (cardHeight + cardSpacing);
        
        // Calculate which items to show
        int startIndex = friendsScrollOffset;
        int endIndex = (startIndex + visibleItems < friendsCount) ? (startIndex + visibleItems) : friendsCount;
        
        for (int i = startIndex; i < endIndex; i++) {
            uint16_t y = startY + (i - startIndex) * (cardHeight + cardSpacing);
            bool isSelected = (i == selectedFriendIndex);
            
            // Card dimensions
            uint16_t cardX = CONTENT_X + 8;
            uint16_t cardY = y;
            uint16_t cardW = CONTENT_WIDTH - 16;
            uint16_t cardR = 4;  // Rounded corner radius
            
            // Draw card background
            uint16_t cardBgColor = isSelected ? SOCIAL_HIGHLIGHT : SOCIAL_HEADER;
            tft->fillRoundRect(cardX, cardY, cardW, cardHeight, cardR, cardBgColor);
            
            // Draw card border based on selection and focus
            if (isSelected && focusMode == FOCUS_CONTENT) {
                // Bright cyan border when selected and content focused
                tft->drawRoundRect(cardX, cardY, cardW, cardHeight, cardR, SOCIAL_ACCENT);
            } else if (isSelected && focusMode == FOCUS_SIDEBAR) {
                // Dimmed border when selected but sidebar focused
                tft->drawRoundRect(cardX, cardY, cardW, cardHeight, cardR, SOCIAL_MUTED);
            }
            // No border for unselected cards
            
            // Draw online/offline indicator (larger, 6px radius)
            uint16_t statusColor = friends[i].online ? SOCIAL_SUCCESS : SOCIAL_ERROR;
            uint16_t dotX = cardX + 12;
            uint16_t dotY = cardY + cardHeight / 2;
            tft->fillCircle(dotX, dotY, 6, statusColor);
            tft->drawCircle(dotX, dotY, 6, statusColor);
            
            // Draw username (truncate if too long)
            tft->setTextSize(1);
            tft->setTextColor(SOCIAL_TEXT, cardBgColor);
            tft->setCursor(cardX + 25, cardY + 8);
            String username = friends[i].username;
            // Calculate max width (card width - indicator - padding)
            int maxWidth = (cardW - 25 - 8) / 6;  // Approximate chars (6px per char)
            if (username.length() > maxWidth) {
                username = username.substring(0, maxWidth - 3) + "...";
            }
            tft->print(username);
            
            // Draw online status text (right-aligned)
            tft->setTextColor(SOCIAL_MUTED, cardBgColor);
            String statusText = friends[i].online ? "Online" : "Offline";
            int statusWidth = statusText.length() * 6;  // Approximate width
            tft->setCursor(cardX + cardW - statusWidth - 8, cardY + 20);
            tft->print(statusText);
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
    
    // Draw modern header with count
    const uint16_t headerHeight = 35;
    tft->setTextSize(2);
    tft->setTextColor(SOCIAL_TEXT, SOCIAL_BG_DARK);
    tft->setCursor(CONTENT_X + 10, 8);
    tft->print("THONG BAO");
    
    // Draw count if available
    if (notificationsCount > 0) {
        tft->setTextSize(1);
        tft->setTextColor(SOCIAL_MUTED, SOCIAL_BG_DARK);
        tft->setCursor(CONTENT_X + 10, 24);
        tft->print("[ ");
        tft->print(notificationsCount);
        tft->print(" ]");
    }
    
    // Draw header line
    tft->drawFastHLine(CONTENT_X + 10, headerHeight - 2, CONTENT_WIDTH - 20, SOCIAL_ACCENT);
    
    // Draw notifications list as cards
    if (notificationsCount == 0) {
        tft->setTextSize(1);
        tft->setTextColor(SOCIAL_MUTED, SOCIAL_BG_DARK);
        tft->setCursor(CONTENT_X + 10, 60);
        tft->print("No notifications");
    } else {
        const uint16_t cardHeight = 48;
        const uint16_t cardSpacing = 4;
        const uint16_t startY = headerHeight + 4;
        const int visibleItems = (SCREEN_HEIGHT - startY) / (cardHeight + cardSpacing);
        
        // Calculate which items to show
        int startIndex = notificationsScrollOffset;
        int endIndex = (startIndex + visibleItems < notificationsCount) ? (startIndex + visibleItems) : notificationsCount;
        
        for (int i = startIndex; i < endIndex; i++) {
            uint16_t y = startY + (i - startIndex) * (cardHeight + cardSpacing);
            bool isSelected = (i == selectedNotificationIndex);
            
            // Card dimensions
            uint16_t cardX = CONTENT_X + 8;
            uint16_t cardY = y;
            uint16_t cardW = CONTENT_WIDTH - 16;
            uint16_t cardR = 4;  // Rounded corner radius
            
            // Draw card background
            uint16_t cardBgColor = isSelected ? SOCIAL_HIGHLIGHT : SOCIAL_HEADER;
            tft->fillRoundRect(cardX, cardY, cardW, cardHeight, cardR, cardBgColor);
            
            // Draw card border based on selection and focus
            if (isSelected && focusMode == FOCUS_CONTENT) {
                // Bright cyan border when selected and content focused
                tft->drawRoundRect(cardX, cardY, cardW, cardHeight, cardR, SOCIAL_ACCENT);
            } else if (isSelected && focusMode == FOCUS_SIDEBAR) {
                // Dimmed border when selected but sidebar focused
                tft->drawRoundRect(cardX, cardY, cardW, cardHeight, cardR, SOCIAL_MUTED);
            }
            
            // Draw notification type indicator (icon on left)
            // Use different color for friend requests to indicate they are actionable
            uint16_t typeColor = (notifications[i].type == "friend_request") ? SOCIAL_SUCCESS : SOCIAL_ACCENT;
            uint16_t iconX = cardX + 12;
            uint16_t iconY = cardY + cardHeight / 2;
            tft->fillCircle(iconX, iconY, 5, typeColor);
            tft->drawCircle(iconX, iconY, 5, typeColor);
            
            // Draw message (truncate if too long, can wrap to two lines)
            tft->setTextSize(1);
            tft->setTextColor(SOCIAL_TEXT, cardBgColor);
            tft->setCursor(cardX + 25, cardY + 8);
            
            String message = notifications[i].message;
            // Calculate max width for message
            int maxWidth = (cardW - 25 - 8) / 6;  // Approximate chars
            if (message.length() > maxWidth * 2) {
                // Truncate if too long for two lines
                message = message.substring(0, maxWidth * 2 - 3) + "...";
            } else if (message.length() > maxWidth) {
                // Try to wrap at space if possible
                int spacePos = message.lastIndexOf(' ', maxWidth);
                if (spacePos > 0) {
                    // Draw first line
                    tft->print(message.substring(0, spacePos));
                    // Draw second line
                    tft->setCursor(cardX + 25, cardY + 20);
                    String secondLine = message.substring(spacePos + 1);
                    if (secondLine.length() > maxWidth) {
                        secondLine = secondLine.substring(0, maxWidth - 3) + "...";
                    }
                    tft->print(secondLine);
                    message = "";  // Already printed
                }
            }
            if (message.length() > 0) {
                tft->print(message);
            }
            
            // Draw timestamp (right-aligned, bottom corner)
            if (notifications[i].timestamp.length() > 0) {
                tft->setTextColor(SOCIAL_MUTED, cardBgColor);
                String timestamp = notifications[i].timestamp;
                // Truncate timestamp if needed
                int maxTimestampWidth = (cardW - 25) / 6;
                if (timestamp.length() > maxTimestampWidth) {
                    timestamp = timestamp.substring(0, maxTimestampWidth - 3) + "...";
                }
                int timestampWidth = timestamp.length() * 6;
                tft->setCursor(cardX + cardW - timestampWidth - 8, cardY + cardHeight - 10);
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
    
    // Draw header
    const uint16_t headerHeight = 35;
    tft->setTextSize(2);
    tft->setTextColor(SOCIAL_TEXT, SOCIAL_BG_DARK);
    tft->setCursor(CONTENT_X + 10, 8);
    tft->print("ADD FRIEND");
    
    // Draw header line
    tft->drawFastHLine(CONTENT_X + 10, headerHeight - 2, CONTENT_WIDTH - 20, SOCIAL_ACCENT);
    
    // Center the input box horizontally, position vertically to avoid keyboard overlap
    uint16_t inputW = CONTENT_WIDTH - 40;
    uint16_t inputH = 40;
    uint16_t inputX = CONTENT_X + (CONTENT_WIDTH - inputW) / 2;
    uint16_t inputY = 60;  // Fixed position to avoid keyboard overlap (keyboard occupies bottom ~85px)
    
    // Delegate drawing to MiniAddFriendScreen
    if (miniAddFriend != nullptr) {
        miniAddFriend->draw(inputX, inputY, inputW, inputH, focusMode == FOCUS_CONTENT);
    }
    
    // Instructions (centered below input)
    tft->setTextSize(1);
    tft->setTextColor(SOCIAL_MUTED, SOCIAL_BG_DARK);
    tft->setCursor(inputX, inputY + inputH + 12);
    tft->print("Press Enter to add friend");
    
    // Keyboard is drawn by MiniAddFriendScreen::draw()
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

void SocialScreen::redrawFriendCard(int index, bool isSelected) {
    if (index < 0 || index >= friendsCount) return;
    
    const uint16_t cardHeight = 32;
    const uint16_t cardSpacing = 4;
    const uint16_t headerHeight = 35;
    const uint16_t startY = headerHeight + 4;
    const int visibleItems = (SCREEN_HEIGHT - startY) / (cardHeight + cardSpacing);
    
    // Check if item is visible
    if (index < friendsScrollOffset || index >= friendsScrollOffset + visibleItems) {
        return;  // Not visible, no need to redraw
    }
    
    uint16_t y = startY + (index - friendsScrollOffset) * (cardHeight + cardSpacing);
    
    // Card dimensions
    uint16_t cardX = CONTENT_X + 8;
    uint16_t cardY = y;
    uint16_t cardW = CONTENT_WIDTH - 16;
    uint16_t cardR = 4;
    
    // Draw card background
    uint16_t cardBgColor = isSelected ? SOCIAL_HIGHLIGHT : SOCIAL_HEADER;
    tft->fillRoundRect(cardX, cardY, cardW, cardHeight, cardR, cardBgColor);
    
    // Draw card border based on selection and focus
    if (isSelected && focusMode == FOCUS_CONTENT) {
        tft->drawRoundRect(cardX, cardY, cardW, cardHeight, cardR, SOCIAL_ACCENT);
    } else if (isSelected && focusMode == FOCUS_SIDEBAR) {
        tft->drawRoundRect(cardX, cardY, cardW, cardHeight, cardR, SOCIAL_MUTED);
    }
    
    // Draw online/offline indicator
    uint16_t statusColor = friends[index].online ? SOCIAL_SUCCESS : SOCIAL_ERROR;
    uint16_t dotX = cardX + 12;
    uint16_t dotY = cardY + cardHeight / 2;
    tft->fillCircle(dotX, dotY, 6, statusColor);
    tft->drawCircle(dotX, dotY, 6, statusColor);
    
    // Draw username
    tft->setTextSize(1);
    tft->setTextColor(SOCIAL_TEXT, cardBgColor);
    tft->setCursor(cardX + 25, cardY + 8);
    String username = friends[index].username;
    int maxWidth = (cardW - 25 - 8) / 6;
    if (username.length() > maxWidth) {
        username = username.substring(0, maxWidth - 3) + "...";
    }
    tft->print(username);
    
    // Draw online status text
    tft->setTextColor(SOCIAL_MUTED, cardBgColor);
    String statusText = friends[index].online ? "Online" : "Offline";
    int statusWidth = statusText.length() * 6;
    tft->setCursor(cardX + cardW - statusWidth - 8, cardY + 20);
    tft->print(statusText);
}

void SocialScreen::redrawNotificationCard(int index, bool isSelected) {
    if (index < 0 || index >= notificationsCount) return;
    
    const uint16_t cardHeight = 48;
    const uint16_t cardSpacing = 4;
    const uint16_t headerHeight = 35;
    const uint16_t startY = headerHeight + 4;
    const int visibleItems = (SCREEN_HEIGHT - startY) / (cardHeight + cardSpacing);
    
    // Check if item is visible
    if (index < notificationsScrollOffset || index >= notificationsScrollOffset + visibleItems) {
        return;  // Not visible, no need to redraw
    }
    
    uint16_t y = startY + (index - notificationsScrollOffset) * (cardHeight + cardSpacing);
    
    // Card dimensions
    uint16_t cardX = CONTENT_X + 8;
    uint16_t cardY = y;
    uint16_t cardW = CONTENT_WIDTH - 16;
    uint16_t cardR = 4;
    
    // Draw card background
    uint16_t cardBgColor = isSelected ? SOCIAL_HIGHLIGHT : SOCIAL_HEADER;
    tft->fillRoundRect(cardX, cardY, cardW, cardHeight, cardR, cardBgColor);
    
    // Draw card border based on selection and focus
    if (isSelected && focusMode == FOCUS_CONTENT) {
        tft->drawRoundRect(cardX, cardY, cardW, cardHeight, cardR, SOCIAL_ACCENT);
    } else if (isSelected && focusMode == FOCUS_SIDEBAR) {
        tft->drawRoundRect(cardX, cardY, cardW, cardHeight, cardR, SOCIAL_MUTED);
    }
    
            // Draw notification type indicator
            // Use different color for friend requests to indicate they are actionable
            uint16_t typeColor = (notifications[index].type == "friend_request") ? SOCIAL_SUCCESS : SOCIAL_ACCENT;
            uint16_t iconX = cardX + 12;
            uint16_t iconY = cardY + cardHeight / 2;
            tft->fillCircle(iconX, iconY, 5, typeColor);
            tft->drawCircle(iconX, iconY, 5, typeColor);
    
    // Draw message
    tft->setTextSize(1);
    tft->setTextColor(SOCIAL_TEXT, cardBgColor);
    tft->setCursor(cardX + 25, cardY + 8);
    
    String message = notifications[index].message;
    int maxWidth = (cardW - 25 - 8) / 6;
    if (message.length() > maxWidth * 2) {
        message = message.substring(0, maxWidth * 2 - 3) + "...";
    } else if (message.length() > maxWidth) {
        int spacePos = message.lastIndexOf(' ', maxWidth);
        if (spacePos > 0) {
            tft->print(message.substring(0, spacePos));
            tft->setCursor(cardX + 25, cardY + 20);
            String secondLine = message.substring(spacePos + 1);
            if (secondLine.length() > maxWidth) {
                secondLine = secondLine.substring(0, maxWidth - 3) + "...";
            }
            tft->print(secondLine);
            message = "";
        }
    }
    if (message.length() > 0) {
        tft->print(message);
    }
    
    // Draw timestamp
    if (notifications[index].timestamp.length() > 0) {
        tft->setTextColor(SOCIAL_MUTED, cardBgColor);
        String timestamp = notifications[index].timestamp;
        int maxTimestampWidth = (cardW - 25) / 6;
        if (timestamp.length() > maxTimestampWidth) {
            timestamp = timestamp.substring(0, maxTimestampWidth - 3) + "...";
        }
        int timestampWidth = timestamp.length() * 6;
        tft->setCursor(cardX + cardW - timestampWidth - 8, cardY + cardHeight - 10);
        tft->print(timestamp);
    }
}

void SocialScreen::handleContentNavigation(const String& key) {
    if (currentTab == TAB_ADD_FRIEND) {
        // Forward to MiniAddFriendScreen
        if (miniAddFriend != nullptr) {
            miniAddFriend->handleKeyPress(key);
            
            // Handle Enter key to trigger Add Friend API call
            if (key == "|e") {
                String friendName = miniAddFriend->getEnteredName();
                if (friendName.length() > 0) {
                    Serial.print("Social Screen: Adding friend: ");
                    Serial.println(friendName);
                    
                    // Call API to add friend (send friend request)
                    if (userId > 0 && serverHost.length() > 0) {
                        bool success = ApiClient::sendFriendRequest(userId, friendName, serverHost, serverPort);
                        if (success) {
                            Serial.println("Social Screen: Friend request sent successfully");
                            miniAddFriend->reset();
                            
                            // Refresh friends list
                            loadFriends();
                            
                            // Call callback if set
                            if (onAddFriendSuccessCallback != nullptr) {
                                onAddFriendSuccessCallback();
                            }
                        } else {
                            Serial.println("Social Screen: Failed to send friend request");
                        }
                    }
                }
            }
        }
        
        // Redraw content area
        drawContentArea();
    } else if (currentTab == TAB_FRIENDS) {
        // Navigate friends list with partial redraw
        int oldIndex = selectedFriendIndex;
        if (key == "|u" && selectedFriendIndex > 0) {
            selectedFriendIndex--;
            // Adjust scroll if needed
            const uint16_t cardHeight = 32;
            const uint16_t cardSpacing = 4;
            const uint16_t headerHeight = 35;
            const uint16_t startY = headerHeight + 4;
            const int visibleItems = (SCREEN_HEIGHT - startY) / (cardHeight + cardSpacing);
            
            if (selectedFriendIndex < friendsScrollOffset) {
                friendsScrollOffset = selectedFriendIndex;
                // Need full redraw if scroll changed
                drawContentArea();
            } else {
                // Partial redraw: unselect old, select new
                redrawFriendCard(oldIndex, false);
                redrawFriendCard(selectedFriendIndex, true);
            }
        } else if (key == "|d" && selectedFriendIndex < friendsCount - 1) {
            selectedFriendIndex++;
            // Adjust scroll if needed
            const uint16_t cardHeight = 32;
            const uint16_t cardSpacing = 4;
            const uint16_t headerHeight = 35;
            const uint16_t startY = headerHeight + 4;
            const int visibleItems = (SCREEN_HEIGHT - startY) / (cardHeight + cardSpacing);
            
            if (selectedFriendIndex >= friendsScrollOffset + visibleItems) {
                friendsScrollOffset = selectedFriendIndex - visibleItems + 1;
                // Need full redraw if scroll changed
                drawContentArea();
            } else {
                // Partial redraw: unselect old, select new
                redrawFriendCard(oldIndex, false);
                redrawFriendCard(selectedFriendIndex, true);
            }
        }
    } else if (currentTab == TAB_NOTIFICATIONS) {
        // Handle Enter key for accepting friend requests
        if (key == "|e") {
            if (selectedNotificationIndex >= 0 && selectedNotificationIndex < notificationsCount) {
                ApiClient::NotificationEntry* notification = &notifications[selectedNotificationIndex];
                
                // Check if this is a friend request notification
                if (notification->type == "friend_request") {
                    Serial.print("Social Screen: Accepting friend request from notification ID: ");
                    Serial.println(notification->id);
                    
                    if (userId > 0 && serverHost.length() > 0) {
                        bool success = ApiClient::acceptFriendRequest(userId, notification->id, serverHost, serverPort);
                        if (success) {
                            Serial.println("Social Screen: Friend request accepted successfully");
                            
                            // Refresh friends list
                            loadFriends();
                            
                            // Reload notifications to get updated list from server
                            loadNotifications();
                        } else {
                            Serial.println("Social Screen: Failed to accept friend request");
                        }
                    }
                }
            }
            return;
        }
        
        // Navigate notifications list with partial redraw
        int oldIndex = selectedNotificationIndex;
        if (key == "|u" && selectedNotificationIndex > 0) {
            selectedNotificationIndex--;
            // Adjust scroll if needed
            const uint16_t cardHeight = 48;
            const uint16_t cardSpacing = 4;
            const uint16_t headerHeight = 35;
            const uint16_t startY = headerHeight + 4;
            const int visibleItems = (SCREEN_HEIGHT - startY) / (cardHeight + cardSpacing);
            
            if (selectedNotificationIndex < notificationsScrollOffset) {
                notificationsScrollOffset = selectedNotificationIndex;
                // Need full redraw if scroll changed
                drawContentArea();
            } else {
                // Partial redraw: unselect old, select new
                redrawNotificationCard(oldIndex, false);
                redrawNotificationCard(selectedNotificationIndex, true);
            }
        } else if (key == "|d" && selectedNotificationIndex < notificationsCount - 1) {
            selectedNotificationIndex++;
            // Adjust scroll if needed
            const uint16_t cardHeight = 48;
            const uint16_t cardSpacing = 4;
            const uint16_t headerHeight = 35;
            const uint16_t startY = headerHeight + 4;
            const int visibleItems = (SCREEN_HEIGHT - startY) / (cardHeight + cardSpacing);
            
            if (selectedNotificationIndex >= notificationsScrollOffset + visibleItems) {
                notificationsScrollOffset = selectedNotificationIndex - visibleItems + 1;
                // Need full redraw if scroll changed
                drawContentArea();
            } else {
                // Partial redraw: unselect old, select new
                redrawNotificationCard(oldIndex, false);
                redrawNotificationCard(selectedNotificationIndex, true);
            }
        }
    }
}

void SocialScreen::switchTab(Tab newTab) {
    if (currentTab != newTab) {
        currentTab = newTab;
        // Auto-focus input when switching to Add Friend tab
        if (newTab == TAB_ADD_FRIEND) {
            focusMode = FOCUS_CONTENT;
            // Reset miniAddFriend to clear any previous input (also resets keyboard)
            if (miniAddFriend != nullptr) {
                miniAddFriend->reset();
            }
        } else {
            // Reset focus to sidebar when switching to other tabs
            focusMode = FOCUS_SIDEBAR;
        }
        // Reset selection indices when switching tabs
        selectedFriendIndex = 0;
        friendsScrollOffset = 0;
        selectedNotificationIndex = 0;
        notificationsScrollOffset = 0;
        draw();
    }
}

void SocialScreen::navigateToAddFriend() {
    // Switch to Add Friend tab
    switchTab(TAB_ADD_FRIEND);
    // Ensure focus is on content (keyboard)
    focusMode = FOCUS_CONTENT;
    // Reset keyboard and input
    if (miniAddFriend != nullptr) {
        miniAddFriend->reset();
    }
    // Draw the screen
    draw();
}

void SocialScreen::handleKeyPress(const String& key) {
    // If on Add Friend tab and typing, forward to MiniAddFriendScreen
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
            // Redraw sidebar tabs to show focus change
            drawSidebar();
            // Redraw selected card with dimmed border
            if (currentTab == TAB_FRIENDS && selectedFriendIndex >= 0 && selectedFriendIndex < friendsCount) {
                redrawFriendCard(selectedFriendIndex, true);
            } else if (currentTab == TAB_NOTIFICATIONS && selectedNotificationIndex >= 0 && selectedNotificationIndex < notificationsCount) {
                redrawNotificationCard(selectedNotificationIndex, true);
            }
        }
        return;
    } else if (key == "|r") {
        // Right: Move focus to content
        if (focusMode == FOCUS_SIDEBAR) {
            focusMode = FOCUS_CONTENT;
            // Redraw sidebar tabs to show focus change
            drawSidebar();
            // Redraw selected card with bright border
            if (currentTab == TAB_FRIENDS && selectedFriendIndex >= 0 && selectedFriendIndex < friendsCount) {
                redrawFriendCard(selectedFriendIndex, true);
            } else if (currentTab == TAB_NOTIFICATIONS && selectedNotificationIndex >= 0 && selectedNotificationIndex < notificationsCount) {
                redrawNotificationCard(selectedNotificationIndex, true);
            } else {
                // Redraw content area if no selection
                drawContentArea();
            }
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
    if (miniAddFriend != nullptr) {
        miniAddFriend->reset();
    }
}

