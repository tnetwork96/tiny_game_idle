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
#define SIDEBAR_WIDTH    26
#define CONTENT_X        26
#define CONTENT_WIDTH    294
#define SCREEN_HEIGHT    240
#define TAB_HEIGHT       60
#define TAB_PADDING      10

// Static instance pointer for callbacks
static SocialScreen* s_socialScreenInstance = nullptr;

SocialScreen::SocialScreen(Adafruit_ST7789* tft, Keyboard* keyboard) {
    this->tft = tft;
    this->keyboard = keyboard;
    this->miniKeyboard = new MiniKeyboard(tft);
    this->miniAddFriend = new MiniAddFriendScreen(tft, miniKeyboard);
    this->confirmationDialog = new ConfirmationDialog(tft);
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
    this->pendingAcceptNotificationId = -1;
    
    // Notification popup
    this->popupMessage = "";
    this->popupShowTime = 0;
    this->popupVisible = false;
    
    // Red dot badge for unread notifications
    this->hasUnreadNotification = false;
    
    // Create semaphore for thread-safe notifications access
    notificationsMutex = xSemaphoreCreateMutex();
    if (notificationsMutex == NULL) {
        Serial.println("Social Screen: Failed to create notifications mutex!");
    }
    
    // Set static instance for callbacks
    s_socialScreenInstance = this;
}

SocialScreen::~SocialScreen() {
    clearFriends();
    clearNotifications();
    if (miniAddFriend != nullptr) {
        delete miniAddFriend;
    }
    if (confirmationDialog != nullptr) {
        delete confirmationDialog;
    }
    // Delete semaphore
    if (notificationsMutex != NULL) {
        vSemaphoreDelete(notificationsMutex);
        notificationsMutex = NULL;
    }
    s_socialScreenInstance = nullptr;
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
    
    // Draw red dot badge if there are unread notifications
    if (hasUnreadNotification) {
        // Draw small red circle at top-right corner of icon (16x16 icon)
        const uint16_t badgeSize = 6;
        const uint16_t badgeX = x + 16 - badgeSize;  // Top-right corner
        const uint16_t badgeY = y;
        const uint16_t badgeCenterX = badgeX + badgeSize/2;
        const uint16_t badgeCenterY = badgeY + badgeSize/2;
        const uint16_t badgeRadius = badgeSize/2;
        
        // Draw red filled circle
        tft->fillCircle(badgeCenterX, badgeCenterY, badgeRadius, ST77XX_RED);
        // Draw white border for better visibility
        tft->drawCircle(badgeCenterX, badgeCenterY, badgeRadius, ST77XX_WHITE);
    }
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
    
    // Draw modern header
    const uint16_t headerHeight = 35;
    tft->setTextSize(1);
    tft->setTextColor(SOCIAL_TEXT, SOCIAL_BG_DARK);
    tft->setCursor(CONTENT_X + 10, 8);
    tft->print("FRIENDS");
    
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
            
            // Draw online/offline indicator (larger, 6px radius)
            uint16_t statusColor = friends[i].online ? SOCIAL_SUCCESS : SOCIAL_ERROR;
            uint16_t dotX = cardX + 12;
            uint16_t dotY = cardY + cardHeight / 2;
            tft->fillCircle(dotX, dotY, 6, statusColor);
            tft->drawCircle(dotX, dotY, 6, statusColor);
            
            // Draw nickname (truncate if too long)
            tft->setTextSize(1);
            tft->setTextColor(SOCIAL_TEXT, cardBgColor);
            tft->setCursor(cardX + 25, cardY + 8);
            String nickname = friends[i].nickname;
            // Calculate max width (card width - indicator - padding)
            int maxWidth = (cardW - 25 - 8) / 6;  // Approximate chars (6px per char)
            if (nickname.length() > maxWidth) {
                nickname = nickname.substring(0, maxWidth - 3) + "...";
            }
            tft->print(nickname);
            
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
    
    // Draw modern header
    const uint16_t headerHeight = 35;
    tft->setTextSize(1);
    tft->setTextColor(SOCIAL_TEXT, SOCIAL_BG_DARK);
    tft->setCursor(CONTENT_X + 10, 8);
    tft->print("THONG BAO");
    
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
    tft->setTextSize(1);
    tft->setTextColor(SOCIAL_TEXT, SOCIAL_BG_DARK);
    tft->setCursor(CONTENT_X + 10, 8);
    tft->print("ADD FRIEND");
    
    // Draw header line
    tft->drawFastHLine(CONTENT_X + 10, headerHeight - 2, CONTENT_WIDTH - 20, SOCIAL_ACCENT);
    
    // Center the input box horizontally, position vertically to avoid keyboard overlap
    uint16_t inputW = CONTENT_WIDTH - 40;
    uint16_t inputH = 40;
    uint16_t inputX = CONTENT_X + (CONTENT_WIDTH - inputW) / 2;
    uint16_t inputY = 45;  // Moved up from 60 to avoid keyboard overlap (keyboard height: 121px, starts at Y=115)
    
    // Delegate drawing to MiniAddFriendScreen
    if (miniAddFriend != nullptr) {
        miniAddFriend->draw(inputX, inputY, inputW, inputH, focusMode == FOCUS_CONTENT);
    }
    
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
    
    // Draw confirmation dialog if visible (draw last so it appears on top)
    if (confirmationDialog != nullptr && confirmationDialog->isVisible()) {
        confirmationDialog->draw();
    }
    
    // Popup notification disabled - notifications will only show in notifications tab
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
    
    // Draw online/offline indicator
    uint16_t statusColor = friends[index].online ? SOCIAL_SUCCESS : SOCIAL_ERROR;
    uint16_t dotX = cardX + 12;
    uint16_t dotY = cardY + cardHeight / 2;
    tft->fillCircle(dotX, dotY, 6, statusColor);
    tft->drawCircle(dotX, dotY, 6, statusColor);
    
    // Draw nickname
    tft->setTextSize(1);
    tft->setTextColor(SOCIAL_TEXT, cardBgColor);
    tft->setCursor(cardX + 25, cardY + 8);
    String nickname = friends[index].nickname;
    int maxWidth = (cardW - 25 - 8) / 6;
    if (nickname.length() > maxWidth) {
        nickname = nickname.substring(0, maxWidth - 3) + "...";
    }
    tft->print(nickname);
    
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
        // Forward to MiniAddFriendScreen - it will handle Enter key appropriately
        // (typing selected character, or submitting if Enter key on keyboard is selected)
        if (miniAddFriend != nullptr) {
            miniAddFriend->handleKeyPress(key);
            
            // Redraw content area to show updated text (giống LoginScreen redraw sau khi update)
            // Chỉ redraw nếu là ký tự hoặc backspace để hiển thị text mới
            if (key.length() == 1 || key == "<") {
                drawContentArea();
            }
            
            // Check if form should be submitted (Enter key on keyboard was selected and pressed)
            if (miniAddFriend->shouldSubmitForm()) {
                String friendName = miniAddFriend->getEnteredName();
                
                // Client-side validation before sending
                friendName.trim();  // Trim whitespace
                
                if (friendName.length() == 0) {
                    miniAddFriend->setErrorMessage("Please enter a nickname");
                    drawContentArea();  // Redraw to show error
                    return;
                }
                
                if (friendName.length() > 255) {  // Match server limit
                    miniAddFriend->setErrorMessage("Nickname is too long (max 255 characters)");
                    drawContentArea();  // Redraw to show error
                    return;
                }
                
                // Prevent sending to self (basic check - server will also validate)
                // Note: We don't have current user's nickname here, so we skip this check
                // Server will handle it properly
                
                Serial.print("Social Screen: Adding friend: ");
                Serial.println(friendName);
                
                // Call API to add friend (send friend request)
                if (userId > 0 && serverHost.length() > 0) {
                    // Clear any previous error before sending
                    miniAddFriend->clearError();
                    drawContentArea();  // Redraw to clear error
                    
                    // Send friend request
                    ApiClient::FriendRequestResult result = ApiClient::sendFriendRequest(userId, friendName, serverHost, serverPort);
                    
                    if (result.success) {
                        Serial.print("Social Screen: Friend request sent successfully: ");
                        Serial.println(result.message);
                        
                        // Clear input and reset form
                        miniAddFriend->clearError();
                        miniAddFriend->reset();
                        
                        // Refresh friends list to show any updates
                        loadFriends();
                        
                        // Redraw to show cleared input
                        drawContentArea();
                        
                        // Call callback if set
                        if (onAddFriendSuccessCallback != nullptr) {
                            onAddFriendSuccessCallback();
                        }
                    } else {
                        Serial.print("Social Screen: Failed to send friend request: ");
                        Serial.println(result.message);
                        
                        // Display user-friendly error message
                        String errorMsg = result.message;
                        if (errorMsg.length() == 0) {
                            errorMsg = "Failed to send friend request. Please try again.";
                        }
                        miniAddFriend->setErrorMessage(errorMsg);
                        
                        // Redraw to show error
                        drawContentArea();
                    }
                } else {
                    // Invalid configuration
                    miniAddFriend->setErrorMessage("Server connection error. Please check settings.");
                    drawContentArea();
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
        } else if (key == "<") {
            // Handle Backspace key for removing friend
            if (selectedFriendIndex >= 0 && selectedFriendIndex < friendsCount && userId > 0 && serverHost.length() > 0) {
                FriendItem* friendItem = &friends[selectedFriendIndex];
                Serial.print("Social Screen: Attempting to remove friend: ");
                Serial.println(friendItem->nickname);
                
                // NOTE: removeFriend API requires friendId, but current FriendItem only has username
                // This is a limitation - the API endpoint DELETE /api/friends/{userId}/{friendId} needs friendId
                // TODO: Either:
                // 1. Modify API to support removal by username: DELETE /api/friends/{userId}/by-username/{username}
                // 2. Add friendId to FriendItem struct and update parseFriendsString to include friendId
                // 3. Lookup friendId from username via a separate API call
                
                // For now, we'll log the attempt but cannot complete without friendId
                Serial.println("Social Screen: Remove friend requires friendId - feature not fully implemented");
                Serial.println("TODO: Add friendId support to FriendItem or modify API to support username-based removal");
                
                // Uncomment below when friendId is available:
                // int friendId = friend->friendId; // Would need to add this field
                // ApiClient::FriendRequestResult result = ApiClient::removeFriend(userId, friendId, serverHost, serverPort);
                // if (result.success) {
                //     Serial.print("Social Screen: Friend removed successfully: ");
                //     Serial.println(result.message);
                //     loadFriends(); // Refresh list
                // } else {
                //     Serial.print("Social Screen: Failed to remove friend: ");
                //     Serial.println(result.message);
                // }
            }
        }
    } else if (currentTab == TAB_NOTIFICATIONS) {
        // If confirmation dialog is showing, handle dialog navigation
        if (confirmationDialog != nullptr && confirmationDialog->isVisible()) {
            if (key == "|l") {
                confirmationDialog->handleLeft();
            } else if (key == "|r") {
                confirmationDialog->handleRight();
            } else if (key == "|e") {
                confirmationDialog->handleSelect();
            } else if (key == "<" || key == "|b") {
                confirmationDialog->handleCancel();
            }
            return;
        }
        
        // Handle Enter key for accepting friend requests
        if (key == "|e") {
            if (selectedNotificationIndex >= 0 && selectedNotificationIndex < notificationsCount) {
                ApiClient::NotificationEntry* notification = &notifications[selectedNotificationIndex];
                
                // Check if this is a friend request notification
                if (notification->type == "friend_request") {
                    // Validate inputs before showing confirmation
                    if (userId <= 0) {
                        Serial.println("Social Screen: Invalid user ID for accepting friend request");
                        return;
                    }
                    
                    if (serverHost.length() == 0) {
                        Serial.println("Social Screen: Server host not configured");
                        return;
                    }
                    
                    // Check if notification is already read (may have been processed)
                    if (notification->read) {
                        Serial.println("Social Screen: Notification already processed, refreshing...");
                        loadNotifications();
                        return;
                    }
                    
                    // Extract sender name from notification message
                    String message = notification->message;
                    String senderName = message;
                    // Message format: "{sender_name} sent you a friend request"
                    int sentPos = message.indexOf(" sent you a friend request");
                    if (sentPos > 0) {
                        senderName = message.substring(0, sentPos);
                    }
                    
                    // Store notification ID for accept action
                    pendingAcceptNotificationId = notification->id;
                    
                    // Show confirmation dialog
                    String confirmMessage = "Accept friend request\nfrom " + senderName + "?";
                    confirmationDialog->show(
                        confirmMessage,
                        "YES",
                        "NO",
                        onConfirmAcceptFriendRequest,
                        onCancelAcceptFriendRequest
                    );
                    
                    // Redraw to show dialog
                    draw();
                } else {
                    Serial.print("Social Screen: Notification type is not friend_request: ");
                    Serial.println(notification->type);
                }
            }
            return;
        }
        
        // Handle Backspace key for rejecting friend requests
        if (key == "<") {
            if (selectedNotificationIndex >= 0 && selectedNotificationIndex < notificationsCount) {
                ApiClient::NotificationEntry* notification = &notifications[selectedNotificationIndex];
                
                // Check if this is a friend request notification
                if (notification->type == "friend_request") {
                    Serial.print("Social Screen: Rejecting friend request from notification ID: ");
                    Serial.println(notification->id);
                    
                    if (userId > 0 && serverHost.length() > 0) {
                        ApiClient::FriendRequestResult result = ApiClient::rejectFriendRequest(userId, notification->id, serverHost, serverPort);
                        if (result.success) {
                            Serial.print("Social Screen: Friend request rejected successfully: ");
                            Serial.println(result.message);
                            
                            // Reload notifications to get updated list from server
                            loadNotifications();
                        } else {
                            Serial.print("Social Screen: Failed to reject friend request: ");
                            Serial.println(result.message);
                            // TODO: Display error message to user
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
        // Clear red dot badge when switching to notifications tab
        if (newTab == TAB_NOTIFICATIONS && hasUnreadNotification) {
            hasUnreadNotification = false;
            Serial.println("Social Screen: Cleared hasUnreadNotification (switched to notifications tab)");
        }
        
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

void SocialScreen::navigateToNotifications() {
    // Switch to Notifications tab
    switchTab(TAB_NOTIFICATIONS);
    // Ensure focus is on content (notifications list)
    focusMode = FOCUS_CONTENT;
    // Draw the screen
    draw();
}

void SocialScreen::navigateToFriends() {
    // Switch to Friends tab (chat)
    switchTab(TAB_FRIENDS);
    // Ensure focus is on content (friends list)
    focusMode = FOCUS_CONTENT;
    // Draw the screen
    draw();
}

void SocialScreen::handleKeyPress(const String& key) {
    // Popup notification disabled - no need to check or hide popup
    
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
            // Redraw selected card
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
            // Redraw selected card
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
                friends[entryIndex].nickname = entry.substring(0, commaPos);
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
    // Lock semaphore for thread-safe access
    if (notificationsMutex != NULL && xSemaphoreTake(notificationsMutex, portMAX_DELAY) == pdTRUE) {
        if (notifications != nullptr) {
            delete[] notifications;
            notifications = nullptr;
        }
        notificationsCount = 0;
        selectedNotificationIndex = 0;
        notificationsScrollOffset = 0;
        xSemaphoreGive(notificationsMutex);
    } else {
        // Fallback if mutex not available
        if (notifications != nullptr) {
            delete[] notifications;
            notifications = nullptr;
        }
        notificationsCount = 0;
        selectedNotificationIndex = 0;
        notificationsScrollOffset = 0;
    }
}

void SocialScreen::removeNotificationById(int notificationId) {
    // Lock semaphore for thread-safe access
    if (notificationsMutex == NULL) {
        Serial.println("Social Screen: Notifications mutex not initialized!");
        return;
    }
    
    if (xSemaphoreTake(notificationsMutex, portMAX_DELAY) != pdTRUE) {
        Serial.println("Social Screen: Failed to take notifications mutex!");
        return;
    }
    
    if (notifications == nullptr || notificationsCount == 0) {
        xSemaphoreGive(notificationsMutex);
        return;
    }
    
    // Find and remove the notification with matching ID
    int foundIndex = -1;
    for (int i = 0; i < notificationsCount; i++) {
        if (notifications[i].id == notificationId) {
            foundIndex = i;
            break;
        }
    }
    
    if (foundIndex < 0) {
        Serial.print("Social Screen: Notification ID ");
        Serial.print(notificationId);
        Serial.println(" not found in current list");
        xSemaphoreGive(notificationsMutex);
        return;
    }
    
    Serial.print("Social Screen: Removing notification ID ");
    Serial.print(notificationId);
    Serial.print(" at index ");
    Serial.println(foundIndex);
    
    // Create new array without the removed notification
    if (notificationsCount == 1) {
        // Last notification, just clear
        delete[] notifications;
        notifications = nullptr;
        notificationsCount = 0;
        selectedNotificationIndex = -1;
        notificationsScrollOffset = 0;
    } else {
        // Create new array
        ApiClient::NotificationEntry* newNotifications = new ApiClient::NotificationEntry[notificationsCount - 1];
        int newIndex = 0;
        for (int i = 0; i < notificationsCount; i++) {
            if (i != foundIndex) {
                newNotifications[newIndex] = notifications[i];
                newIndex++;
            }
        }
        
        // Update count and swap arrays
        int oldCount = notificationsCount;
        delete[] notifications;
        notifications = newNotifications;
        notificationsCount = oldCount - 1;
        
        // Reset selection if needed
        if (selectedNotificationIndex == foundIndex) {
            // Selected item was removed
            selectedNotificationIndex = (notificationsCount > 0) ? 0 : -1;
            notificationsScrollOffset = 0;
        } else if (selectedNotificationIndex > foundIndex) {
            // Selected item index needs to shift down
            selectedNotificationIndex--;
        }
    }
    
    Serial.print("Social Screen: After removal, notifications count: ");
    Serial.println(notificationsCount);
    
    xSemaphoreGive(notificationsMutex);
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
    
    // Lock semaphore for thread-safe access
    if (notificationsMutex == NULL) {
        Serial.println("Social Screen: Notifications mutex not initialized!");
        return;
    }
    
    if (xSemaphoreTake(notificationsMutex, portMAX_DELAY) != pdTRUE) {
        Serial.println("Social Screen: Failed to take notifications mutex!");
        return;
    }
    
    if (result.success) {
        Serial.print("Social Screen: Received ");
        Serial.print(result.count);
        Serial.println(" notifications from server");
        
        // Clear existing notifications
        if (notifications != nullptr) {
            delete[] notifications;
            notifications = nullptr;
        }
        notificationsCount = 0;
        selectedNotificationIndex = 0;
        notificationsScrollOffset = 0;
        
        // Filter out read notifications - only keep unread ones
        int unreadCount = 0;
        for (int i = 0; i < result.count; i++) {
            if (!result.notifications[i].read) {
                unreadCount++;
            }
        }
        
        Serial.print("Social Screen: Filtered to ");
        Serial.print(unreadCount);
        Serial.println(" unread notifications");
        
        notificationsCount = unreadCount;
        if (notificationsCount > 0) {
            notifications = new ApiClient::NotificationEntry[notificationsCount];
            int unreadIndex = 0;
            for (int i = 0; i < result.count; i++) {
                if (!result.notifications[i].read) {
                    notifications[unreadIndex] = result.notifications[i];
                    unreadIndex++;
                }
            }
        }
        
        // Reset selected index if it's now out of bounds or if count changed significantly
        if (selectedNotificationIndex >= notificationsCount || selectedNotificationIndex < 0) {
            int oldIndex = selectedNotificationIndex;
            selectedNotificationIndex = (notificationsCount > 0) ? 0 : -1;
            notificationsScrollOffset = 0;
            Serial.print("Social Screen: Reset selected notification index from ");
            Serial.print(oldIndex);
            Serial.print(" to ");
            Serial.println(selectedNotificationIndex);
        }
        
        // Clean up result memory
        if (result.notifications != nullptr) {
            delete[] result.notifications;
        }
    } else {
        Serial.print("Social Screen: Failed to load notifications: ");
        Serial.println(result.message);
        // Clear existing notifications
        if (notifications != nullptr) {
            delete[] notifications;
            notifications = nullptr;
        }
        notificationsCount = 0;
        selectedNotificationIndex = 0;
        notificationsScrollOffset = 0;
    }
    
    // Unlock semaphore before drawing (drawing reads the array, but we unlock first)
    xSemaphoreGive(notificationsMutex);
    
    // Redraw if currently showing notifications tab
    if (currentTab == TAB_NOTIFICATIONS) {
        // Lock again for reading during draw
        if (xSemaphoreTake(notificationsMutex, portMAX_DELAY) == pdTRUE) {
            drawContentArea();
            xSemaphoreGive(notificationsMutex);
        }
    }
}

void SocialScreen::reset() {
    currentTab = TAB_FRIENDS;
    focusMode = FOCUS_SIDEBAR;
    selectedFriendIndex = 0;
    friendsScrollOffset = 0;
    selectedNotificationIndex = 0;
    notificationsScrollOffset = 0;
    pendingAcceptNotificationId = -1;
    if (miniAddFriend != nullptr) {
        miniAddFriend->reset();
    }
    if (confirmationDialog != nullptr) {
        confirmationDialog->hide();
    }
}

// Static callback wrappers for ConfirmationDialog
void SocialScreen::onConfirmAcceptFriendRequest() {
    if (s_socialScreenInstance != nullptr) {
        s_socialScreenInstance->doAcceptFriendRequest();
    }
}

void SocialScreen::onCancelAcceptFriendRequest() {
    if (s_socialScreenInstance != nullptr) {
        s_socialScreenInstance->doCancelAcceptFriendRequest();
    }
}

// Instance callback methods for confirmation dialog
void SocialScreen::doAcceptFriendRequest() {
    if (pendingAcceptNotificationId <= 0) {
        Serial.println("Social Screen: No pending notification ID for accept");
        pendingAcceptNotificationId = -1;
        if (confirmationDialog != nullptr) {
            confirmationDialog->hide();
        }
        draw();
        return;
    }
    
    Serial.print("Social Screen: Accepting friend request from notification ID: ");
    Serial.println(pendingAcceptNotificationId);
    
    // Call API to accept friend request
    ApiClient::FriendRequestResult result = ApiClient::acceptFriendRequest(userId, pendingAcceptNotificationId, serverHost, serverPort);
    
    // Reset pending notification ID first
    int processedNotificationId = pendingAcceptNotificationId;
    pendingAcceptNotificationId = -1;
    
    if (result.success) {
        Serial.print("Social Screen: Friend request accepted successfully: ");
        Serial.println(result.message);
        
        // Hide dialog first
        if (confirmationDialog != nullptr) {
            confirmationDialog->hide();
        }
        
        // Optimistic UI update: remove notification immediately
        removeNotificationById(processedNotificationId);
        
        // Redraw immediately to show updated list
        draw();
        
        // Then reload from server to ensure consistency
        delay(200);  // Small delay to ensure server has committed the transaction
        
        // Reload friends list first
        loadFriends();
        
        // Reload notifications from server (this will filter out the read notification)
        loadNotifications();
        
        // Reset selection since notification was removed
        if (selectedNotificationIndex >= notificationsCount || selectedNotificationIndex < 0) {
            selectedNotificationIndex = (notificationsCount > 0) ? 0 : -1;
            notificationsScrollOffset = 0;
        }
        
        Serial.print("Social Screen: After reload, notifications count: ");
        Serial.println(notificationsCount);
        
        // Ensure we're on notifications tab and redraw notifications list
        if (currentTab == TAB_NOTIFICATIONS) {
            drawContentArea();  // Redraw notifications list with updated data
        } else {
            draw();  // Full redraw if on different tab
        }
    } else {
        Serial.print("Social Screen: Failed to accept friend request: ");
        Serial.println(result.message);
        
        // Hide dialog
        if (confirmationDialog != nullptr) {
            confirmationDialog->hide();
        }
        
        // Reload notifications to get latest state from server
        // (in case the request was processed by another client)
        loadNotifications();
        
        // Reset selection if needed
        if (selectedNotificationIndex >= notificationsCount || selectedNotificationIndex < 0) {
            selectedNotificationIndex = (notificationsCount > 0) ? 0 : -1;
            notificationsScrollOffset = 0;
        }
        
        // Redraw notifications list with updated data
        if (currentTab == TAB_NOTIFICATIONS) {
            drawContentArea();  // Redraw notifications list
        } else {
            draw();  // Full redraw if on different tab
        }
    }
}

int SocialScreen::getFirstFriendRequestIndex() const {
    if (notifications == nullptr || notificationsCount == 0) {
        return -1;
    }
    
    // Find first unread friend_request notification
    for (int i = 0; i < notificationsCount; i++) {
        if (notifications[i].type == "friend_request" && !notifications[i].read) {
            return i;
        }
    }
    
    return -1;
}

void SocialScreen::selectNotification(int index) {
    if (index >= 0 && index < notificationsCount) {
        selectedNotificationIndex = index;
        // Update scroll offset if needed to keep selected item visible
        const uint16_t cardHeight = 48;
        const uint16_t cardSpacing = 4;
        const uint16_t headerHeight = 35;
        const uint16_t startY = headerHeight + 4;
        const int visibleItems = (SCREEN_HEIGHT - startY) / (cardHeight + cardSpacing);
        
        if (selectedNotificationIndex < notificationsScrollOffset) {
            notificationsScrollOffset = selectedNotificationIndex;
        } else if (selectedNotificationIndex >= notificationsScrollOffset + visibleItems) {
            notificationsScrollOffset = selectedNotificationIndex - visibleItems + 1;
        }
        
        // Redraw to show selection
        drawContentArea();
    }
}

void SocialScreen::doCancelAcceptFriendRequest() {
    if (pendingAcceptNotificationId <= 0) {
        Serial.println("Social Screen: No pending notification ID for reject");
        pendingAcceptNotificationId = -1;
        // Ensure dialog is hidden
        if (confirmationDialog != nullptr) {
            confirmationDialog->hide();
        }
        draw();
        return;
    }
    
    Serial.print("Social Screen: Rejecting friend request from notification ID: ");
    Serial.println(pendingAcceptNotificationId);
    
    // Validate inputs before making request
    if (userId <= 0) {
        Serial.println("Social Screen: Invalid user ID for rejecting friend request");
        pendingAcceptNotificationId = -1;
        if (confirmationDialog != nullptr) {
            confirmationDialog->hide();
        }
        draw();
        return;
    }
    
    if (serverHost.length() == 0) {
        Serial.println("Social Screen: Server host not configured");
        pendingAcceptNotificationId = -1;
        if (confirmationDialog != nullptr) {
            confirmationDialog->hide();
        }
        draw();
        return;
    }
    
    // Call API to reject friend request
    ApiClient::FriendRequestResult result = ApiClient::rejectFriendRequest(userId, pendingAcceptNotificationId, serverHost, serverPort);
    
    // Store notification ID before resetting
    int processedNotificationId = pendingAcceptNotificationId;
    
    // Reset pending notification ID first
    pendingAcceptNotificationId = -1;
    
    if (result.success) {
        Serial.print("Social Screen: Friend request rejected successfully: ");
        Serial.println(result.message);
        
        // Hide dialog first
        if (confirmationDialog != nullptr) {
            confirmationDialog->hide();
        }
        
        // Optimistic UI update: remove notification immediately
        removeNotificationById(processedNotificationId);
        
        // Redraw immediately to show updated list
        draw();
        
        // Then reload from server to ensure consistency
        delay(200);  // Small delay to ensure server has committed the transaction
        
        // Reload notifications from server (this will filter out the read notification)
        loadNotifications();
        
        // Reset selection since notification was removed
        if (selectedNotificationIndex >= notificationsCount || selectedNotificationIndex < 0) {
            selectedNotificationIndex = (notificationsCount > 0) ? 0 : -1;
            notificationsScrollOffset = 0;
        }
        
        Serial.print("Social Screen: After reload, notifications count: ");
        Serial.println(notificationsCount);
        
        // Ensure we're on notifications tab and redraw notifications list
        if (currentTab == TAB_NOTIFICATIONS) {
            drawContentArea();  // Redraw notifications list with updated data
        } else {
            draw();  // Full redraw if on different tab
        }
    } else {
        Serial.print("Social Screen: Failed to reject friend request: ");
        Serial.println(result.message);
        
        // Hide dialog
        if (confirmationDialog != nullptr) {
            confirmationDialog->hide();
        }
        
        // Reload notifications to get latest state from server
        // (in case the request was processed by another client)
        loadNotifications();
        
        // Reset selection if needed
        if (selectedNotificationIndex >= notificationsCount || selectedNotificationIndex < 0) {
            selectedNotificationIndex = (notificationsCount > 0) ? 0 : -1;
            notificationsScrollOffset = 0;
        }
        
        // Redraw notifications list with updated data
        if (currentTab == TAB_NOTIFICATIONS) {
            drawContentArea();  // Redraw notifications list
        } else {
            draw();  // Full redraw if on different tab
        }
    }
}

void SocialScreen::addNotificationFromSocket(int id, const String& type, const String& message, const String& timestamp, bool read) {
    // Lock semaphore for thread-safe access
    if (notificationsMutex == NULL) {
        Serial.println("Social Screen: Notifications mutex not initialized!");
        return;
    }
    
    if (xSemaphoreTake(notificationsMutex, portMAX_DELAY) != pdTRUE) {
        Serial.println("Social Screen: Failed to take notifications mutex!");
        return;
    }
    
    // Check if notification already exists (avoid duplicates)
    // Check by both ID and message to handle test notifications with same ID but different messages
    bool notificationExists = false;
    if (notifications != nullptr && notificationsCount > 0) {
        for (int i = 0; i < notificationsCount; i++) {
            // Check if same ID AND same message (to allow same ID with different messages for testing)
            if (notifications[i].id == id && notifications[i].message == message) {
                notificationExists = true;
                Serial.print("Social Screen: Notification ID ");
                Serial.print(id);
                Serial.print(" with message \"");
                Serial.print(message);
                Serial.println("\" already exists, skipping");
                break;
            }
        }
    }
    
    // Only add if it doesn't exist
    if (!notificationExists) {
        Serial.print("Social Screen: Adding notification from socket - id: ");
        Serial.print(id);
        Serial.print(", type: ");
        Serial.print(type);
        Serial.print(", message: ");
        Serial.println(message);
        
        // Create new array with one more element
        ApiClient::NotificationEntry* newNotifications = new ApiClient::NotificationEntry[notificationsCount + 1];
        
        // Copy existing notifications
        for (int i = 0; i < notificationsCount; i++) {
            newNotifications[i] = notifications[i];
        }
        
        // Add new notification at the beginning (most recent first)
        newNotifications[notificationsCount].id = id;
        newNotifications[notificationsCount].type = type;
        newNotifications[notificationsCount].message = message;
        newNotifications[notificationsCount].timestamp = timestamp;
        newNotifications[notificationsCount].read = read;
        
        // Only keep unread notifications
        if (read) {
            // If notification is read, don't add it to the list
            delete[] newNotifications;
            Serial.println("Social Screen: Notification is read, not adding to list");
        } else {
            // Delete old array and update
            if (notifications != nullptr) {
                delete[] notifications;
            }
            notifications = newNotifications;
            notificationsCount++;
            
            // Update selection if needed
            if (selectedNotificationIndex < 0 && notificationsCount > 0) {
                selectedNotificationIndex = 0;
            }
            
            Serial.print("Social Screen: Notification added. Total count: ");
            Serial.println(notificationsCount);
        }
    }
    
    // Unlock semaphore
    xSemaphoreGive(notificationsMutex);
    
    // Update UI if notification was added successfully
    if (!notificationExists && !read) {
        Serial.println("Social Screen: Notification added successfully");
        
        // Set red dot badge if NOT on notifications tab
        if (currentTab != TAB_NOTIFICATIONS) {
            if (!hasUnreadNotification) {
                hasUnreadNotification = true;
                Serial.println("Social Screen: Set hasUnreadNotification = true (not on notifications tab), redrawing sidebar to show badge");
                // Redraw sidebar to show red dot badge
                drawSidebar();
            }
        }
        
        // Only redraw content if we're on notifications tab
        if (currentTab == TAB_NOTIFICATIONS) {
            Serial.println("Social Screen: On notifications tab, redrawing content area...");
            // Redraw notifications list (this is called from main task, so it's safe)
            // Note: drawContentArea() reads notifications array, but we've already unlocked
            // We need to lock again when reading, but since we're in main task context,
            // we can safely redraw. However to be safe, let's lock during redraw too.
            if (xSemaphoreTake(notificationsMutex, portMAX_DELAY) == pdTRUE) {
                drawContentArea();
                xSemaphoreGive(notificationsMutex);
            }
        } else {
            // If not on notifications tab, notification will be visible when user switches to notifications tab
            Serial.println("Social Screen: Not on notifications tab, badge shown. Notification will be visible when user switches to notifications tab");
        }
    }
}

void SocialScreen::drawNotificationPopup() {
    if (!popupVisible || popupMessage.length() == 0) {
        Serial.print("Social Screen: drawNotificationPopup skipped - visible=");
        Serial.print(popupVisible);
        Serial.print(", messageLen=");
        Serial.println(popupMessage.length());
        return;
    }
    
    Serial.println("Social Screen: Drawing notification popup...");
    
    // Popup dimensions
    const uint16_t popupWidth = 280;
    const uint16_t popupHeight = 80;
    const uint16_t popupX = (320 - popupWidth) / 2;  // Center horizontally
    const uint16_t popupY = 20;  // Top of screen
    const uint16_t cornerRadius = 8;
    const uint16_t padding = 12;
    
    // Draw popup background with rounded corners
    tft->fillRoundRect(popupX, popupY, popupWidth, popupHeight, cornerRadius, SOCIAL_HEADER);
    tft->drawRoundRect(popupX, popupY, popupWidth, popupHeight, cornerRadius, SOCIAL_ACCENT);
    
    // Draw accent line at top
    tft->drawFastHLine(popupX + 4, popupY + 2, popupWidth - 8, SOCIAL_ACCENT);
    
    // Draw notification icon (bell) on left
    const uint16_t iconSize = 16;
    const uint16_t iconX = popupX + padding;
    const uint16_t iconY = popupY + (popupHeight - iconSize) / 2;
    drawNotificationsIcon(iconX, iconY, SOCIAL_SUCCESS);
    
    // Draw message text
    tft->setTextSize(1);
    tft->setTextColor(SOCIAL_TEXT, SOCIAL_HEADER);
    
    // Calculate text area
    const uint16_t textX = popupX + padding + iconSize + 8;
    const uint16_t textY = popupY + padding;
    const uint16_t textWidth = popupWidth - (textX - popupX) - padding;
    const uint16_t textHeight = popupHeight - padding * 2;
    
    // Truncate message if too long
    String displayMessage = popupMessage;
    int maxChars = textWidth / 6;  // Approximate chars per line
    int maxLines = textHeight / 10;  // Approximate lines
    
    if (displayMessage.length() > maxChars * maxLines) {
        displayMessage = displayMessage.substring(0, maxChars * maxLines - 3) + "...";
    }
    
    // Draw message (can wrap to 2 lines)
    tft->setCursor(textX, textY);
    if (displayMessage.length() > maxChars) {
        // Try to wrap at space
        int spacePos = displayMessage.lastIndexOf(' ', maxChars);
        if (spacePos > 0) {
            tft->print(displayMessage.substring(0, spacePos));
            tft->setCursor(textX, textY + 12);
            String secondLine = displayMessage.substring(spacePos + 1);
            if (secondLine.length() > maxChars) {
                secondLine = secondLine.substring(0, maxChars - 3) + "...";
            }
            tft->print(secondLine);
        } else {
            // No space found, just truncate
            tft->print(displayMessage.substring(0, maxChars - 3) + "...");
        }
    } else {
        tft->print(displayMessage);
    }
}

void SocialScreen::showNotificationPopup(const String& message) {
    popupMessage = message;
    popupShowTime = millis();
    popupVisible = true;
    Serial.print("Social Screen: Showing notification popup: ");
    Serial.println(message);
}

void SocialScreen::hideNotificationPopup() {
    if (popupVisible) {
        popupVisible = false;
        popupMessage = "";
        popupShowTime = 0;
        // Redraw to remove popup
        draw();
    }
}

void SocialScreen::updateNotificationPopup() {
    if (popupVisible && popupShowTime > 0) {
        unsigned long currentTime = millis();
        unsigned long elapsed = currentTime - popupShowTime;
        
        if (elapsed >= POPUP_DURATION) {
            hideNotificationPopup();
        }
    }
}

