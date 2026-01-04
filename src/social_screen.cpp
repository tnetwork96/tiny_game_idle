#include "social_screen.h"
#include "game_lobby_screen.h"
#include "caro_game_screen.h"

// 16x16 "User List" (Friends/Buddy List)
const unsigned char PROGMEM iconUserList[] = {
    0x00, 0x00, 0x07, 0xe0, 0x18, 0x18, 0x20, 0x04, 
    0x20, 0x04, 0x18, 0x18, 0x07, 0xe0, 0x00, 0x00, 0x0f, 0xf0, 0x18, 0x18, 0x20, 0x04, 0x20, 0x04, 
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

// 16x16 "Gamepad" (Game Hub)
const unsigned char PROGMEM iconGamepad[] = {
    0x00, 0x00, 0x0f, 0xf0, 0x18, 0x18, 0x20, 0x04,
    0x27, 0xE4, 0x2f, 0xf4, 0x2f, 0xf4, 0x27, 0xe4,
    0x20, 0x04, 0x18, 0x18, 0x0f, 0xf0, 0x00, 0x00,
    0x04, 0x20, 0x0e, 0x70, 0x04, 0x20, 0x00, 0x00
};

// Game list (shared)
static const char* GAME_NAMES[]  = { "Caro", "Seahorse Chess", "Chess", "Snake", "Tetris", "Pong" };
static const char* GAME_STATUS[] = { "Play", "Dev", "Dev", "Dev", "Dev", "Dev" };
static const int TOTAL_GAMES = 6;

// Theme Definition: Deep Space Arcade
// All visual configuration is centralized here for easy theme swapping
static const SocialTheme themeDeepSpace = {
    // Color Palette
    .colorBg = 0x0042,          // Deep Midnight Blue #020817
    .colorSidebarBg = 0x0021,   // Darker Blue for sidebar
    .colorCardBg = 0x08A5,      // Header Blue #0F172A
    .colorHighlight = 0x1148,   // Electric Blue tint
    .colorAccent = 0x07FF,      // Cyan accent
    .colorTextMain = 0xFFFF,    // White text
    .colorTextMuted = 0x8410,   // Muted gray text
    .colorSuccess = 0x07F3,     // Minty Green for online
    .colorError = 0xF986,       // Bright Red for offline
    
    // Layout Metrics
    .tabHeight = 40,            // Compact tabs (fit 6+)
    .rowHeight = 32,            // Friend list row height
    .sidebarWidth = 26,         // Sidebar width
    .cornerRadius = 3,          // Border radius
    .itemPadding = 10,          // General padding
    .headerHeight = 35,         // Content header height
    
    // Decor Flags
    .showTabBorders = false,    // No borders on tabs
    .useFloatingTabs = true     // Use floating pill style
};

// Keep layout constants for screen geometry
#define CONTENT_X        26
#define CONTENT_WIDTH    294
#define SCREEN_HEIGHT    240
#define TAB_PADDING      10


// Static instance pointer for callbacks
static SocialScreen* s_socialScreenInstance = nullptr;

SocialScreen::SocialScreen(Adafruit_ST7789* tft, Keyboard* keyboard) {
    this->tft = tft;
    this->keyboard = keyboard;
    this->miniKeyboard = new MiniKeyboard(tft);
    this->miniAddFriend = new MiniAddFriendScreen(tft, miniKeyboard);
    this->confirmationDialog = new ConfirmationDialog(tft);
    this->gameLobby = new GameLobbyScreen(tft, themeDeepSpace);
    this->caroGameScreen = nullptr;
    
    // Set up game lobby callbacks
    this->gameLobby->setOnStartGame([]() {
        if (s_socialScreenInstance != nullptr) {
            s_socialScreenInstance->startGame();
        }
    });
    this->gameLobby->setOnExit([]() {
        if (s_socialScreenInstance != nullptr) {
            s_socialScreenInstance->exitLobby();
        }
    });
    
    // Initialize theme (hot-swappable - change this line to switch skins)
    this->currentTheme = themeDeepSpace;
    
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
    this->gameInviteCount = 0;
    this->selectedGameInviteIndex = -1;
    for (int i = 0; i < MAX_GAME_INVITES; i++) {
        gameInvites[i].sessionId = -1;
        gameInvites[i].gameType = "";
        gameInvites[i].status = "";
        gameInvites[i].eventType = "";
        gameInvites[i].hostNickname = "";
    }
    this->selectedGameIndex = 0;
    this->pendingGameName = "";
    this->screenState = STATE_NORMAL;
    this->isActive = false;  // Initially inactive
    
    this->onAddFriendSuccessCallback = nullptr;
    this->onOpenChatCallback = nullptr;
    this->onGameSelectedCallback = nullptr;
    this->pendingAcceptNotificationId = -1;
    
    // Notification popup
    this->popupMessage = "";
    this->popupShowTime = 0;
    this->popupVisible = false;
    
    // Red dot badge for unread notifications
    this->hasUnreadNotification = false;
    
    // Red dot badge for unread chat messages
    this->hasUnreadChatFlag = false;
    
    
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
    if (gameLobby != nullptr) {
        delete gameLobby;
    }
    // Delete semaphore
    if (notificationsMutex != NULL) {
        vSemaphoreDelete(notificationsMutex);
        notificationsMutex = NULL;
    }
    s_socialScreenInstance = nullptr;
}

void SocialScreen::drawBackground() {
    tft->fillScreen(currentTheme.colorBg);
}

void SocialScreen::drawTab(int tabIndex, bool isSelected) {
    uint16_t tabY = tabIndex * currentTheme.tabHeight;
    
    // Draw default background (always use sidebar background)
    tft->fillRect(0, tabY, currentTheme.sidebarWidth, currentTheme.tabHeight, currentTheme.colorSidebarBg);
    
    // If selected, draw a "floating pill" highlight with margins
    if (isSelected) {
        const uint16_t margin = 2;
        const uint16_t pillX = margin;
        const uint16_t pillY = tabY + margin;
        const uint16_t pillW = currentTheme.sidebarWidth - (margin * 2);
        const uint16_t pillH = currentTheme.tabHeight - (margin * 2);
        const uint16_t cornerRadius = currentTheme.cornerRadius;
        
        // Draw rounded rectangle "pill" background
        tft->fillRoundRect(pillX, pillY, pillW, pillH, cornerRadius, currentTheme.colorHighlight);
        
        // Draw shorter vertical cyan accent bar (centered, 20px height)
        const uint16_t accentHeight = 20;
        const uint16_t accentY = tabY + (currentTheme.tabHeight - accentHeight) / 2;
        tft->fillRect(1, accentY, 2, accentHeight, currentTheme.colorAccent);
    }
    
    // Calculate icon position (centered in tab)
    uint16_t iconSize = 16;  // Bitmap is 16x16
    uint16_t iconX = (currentTheme.sidebarWidth - iconSize) / 2;
    uint16_t iconY = tabY + (currentTheme.tabHeight - iconSize) / 2;
    
    // Icon color: bright when selected, muted when inactive
    uint16_t iconColor;
    if (isSelected && focusMode == FOCUS_SIDEBAR) {
        iconColor = currentTheme.colorTextMain;  // White when active and focused
    } else if (isSelected) {
        iconColor = currentTheme.colorAccent;  // Cyan when active but not focused
    } else {
        iconColor = currentTheme.colorTextMuted;  // Muted when inactive
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
        case TAB_GAMES:
            drawGamepadIcon(iconX, iconY, iconColor);
            break;
    }
}

// Draw a "LoL-style" game lobby / matchmaking screen (Removed, now using GameLobbyScreen)

void SocialScreen::drawFriendsIcon(uint16_t x, uint16_t y, uint16_t color) {
    // Draw bitmap icon (16x16)
    tft->drawBitmap(x, y, iconUserList, 16, 16, color);
    
    // Chỉ vẽ badge khi màn social đang active
    // Draw red dot badge if there are unread chat messages
    if (hasUnreadChatFlag && isActive) {
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

void SocialScreen::drawNotificationsIcon(uint16_t x, uint16_t y, uint16_t color) {
    // Draw bitmap icon (16x16)
    tft->drawBitmap(x, y, iconBell, 16, 16, color);
    
    // Chỉ vẽ badge khi màn social đang active
    // Draw red dot badge if there are unread notifications
    if (hasUnreadNotification && isActive) {
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

void SocialScreen::drawGamepadIcon(uint16_t x, uint16_t y, uint16_t color) {
    // Draw bitmap icon (16x16)
    tft->drawBitmap(x, y, iconGamepad, 16, 16, color);
}

void SocialScreen::drawSidebar() {
    // Draw sidebar background with darker color
    tft->fillRect(0, 0, currentTheme.sidebarWidth, SCREEN_HEIGHT, currentTheme.colorSidebarBg);
    
    // Draw divider line (subtle)
    tft->drawFastVLine(currentTheme.sidebarWidth - 1, 0, SCREEN_HEIGHT, currentTheme.colorTextMuted);
    
    // Draw all tabs
    for (int i = 0; i < 4; i++) {
        drawTab(i, (i == currentTab));
    }
}

void SocialScreen::redrawSidebar() {
    // Redraw sidebar to update badges
    drawSidebar();
}

void SocialScreen::setHasUnreadChat(bool hasUnread) {
    hasUnreadChatFlag = hasUnread;
}

void SocialScreen::addUnreadChatForFriend(int friendUserId) {
    for (int i = 0; i < friendsCount; i++) {
        if (friends[i].userId == friendUserId) {
            friends[i].unreadCount++;
            Serial.print("Social Screen: Added unread chat for friend ");
            Serial.print(friends[i].nickname);
            Serial.print(" (userId: ");
            Serial.print(friendUserId);
            Serial.print("). Unread count: ");
            Serial.println(friends[i].unreadCount);
            
            // Redraw friend card nếu đang visible và đang ở Friends tab
            if (currentTab == TAB_FRIENDS) {
                redrawFriendCard(i, (i == selectedFriendIndex));
            }
            return;
        }
    }
    Serial.print("Social Screen: ⚠️  Friend with userId ");
    Serial.print(friendUserId);
    Serial.println(" not found in friends list");
}

void SocialScreen::clearUnreadChatForFriend(int friendUserId) {
    for (int i = 0; i < friendsCount; i++) {
        if (friends[i].userId == friendUserId) {
            if (friends[i].unreadCount > 0) {
                friends[i].unreadCount = 0;
                Serial.print("Social Screen: Cleared unread chat for friend ");
                Serial.print(friends[i].nickname);
                Serial.print(" (userId: ");
                Serial.println(friendUserId);
                
                // Redraw friend card
                if (currentTab == TAB_FRIENDS) {
                    redrawFriendCard(i, (i == selectedFriendIndex));
                }
            }
            return;
        }
    }
}

int SocialScreen::getUnreadCountForFriend(int friendUserId) const {
    for (int i = 0; i < friendsCount; i++) {
        if (friends[i].userId == friendUserId) {
            return friends[i].unreadCount;
        }
    }
    return 0;
}

void SocialScreen::drawStatusIndicator(uint16_t x, uint16_t y, bool isOnline) {
    const uint16_t radius = 4;  // 8px diameter
    
    if (isOnline) {
        // Online: Solid filled circle (Active/Present)
        tft->fillCircle(x, y, radius, currentTheme.colorSuccess);
    } else {
        // Offline: Hollow circle ring (Inactive/Away)
        tft->drawCircle(x, y, radius, currentTheme.colorTextMuted);
        tft->drawCircle(x, y, radius - 1, currentTheme.colorTextMuted);  // 2px thick ring for visibility
    }
}

void SocialScreen::drawFriendItem(uint16_t x, uint16_t y, uint16_t w, uint16_t h, FriendItem* friendItem, bool isSelected) {
    const uint16_t cornerRadius = currentTheme.cornerRadius;
    const uint16_t statusDotX = 12;  // Status dot at 12px from left
    const uint16_t nameStartX = 24;  // Text starts at 24px (12px gap from dot)
    
    // 1. Draw card background (flat color, no border)
    uint16_t cardBgColor = isSelected ? currentTheme.colorHighlight : currentTheme.colorCardBg;
    tft->fillRoundRect(x, y, w, h, cornerRadius, cardBgColor);
    
    // 2. Draw Messenger-style status indicator (centered vertically)
    uint16_t dotAbsX = x + statusDotX;
    uint16_t dotAbsY = y + h / 2;
    drawStatusIndicator(dotAbsX, dotAbsY, friendItem->online);
    
    // 3. Draw Nickname (white text, left aligned after status dot)
    tft->setTextSize(1);
    tft->setTextColor(currentTheme.colorTextMain, cardBgColor);
    uint16_t nicknameY = y + (h - 8) / 2;  // Vertically center text (font height ~8px)
    tft->setCursor(x + nameStartX, nicknameY);
    
    String nickname = friendItem->nickname;
    // Calculate max width for nickname (reserve space for unread badge)
    int availableWidth = w - nameStartX - 10;
    if (friendItem->unreadCount > 0) {
        availableWidth -= 24;  // Reserve space for badge
    }
    int maxChars = availableWidth / 6;  // Approximate 6px per char
    if (nickname.length() > maxChars) {
        nickname = nickname.substring(0, maxChars - 3) + "...";
    }
    tft->print(nickname);
    
    // 4. Draw unread badge (notification pill on right side) if unread count > 0
    if (friendItem->unreadCount > 0) {
        const uint16_t badgeRadius = 8;
        const uint16_t badgeX = x + w - badgeRadius - 8;
        const uint16_t badgeY = y + h / 2;  // Vertically centered
        
        // Draw red filled circle
        tft->fillCircle(badgeX, badgeY, badgeRadius, ST77XX_RED);
        // Draw white border for contrast
        tft->drawCircle(badgeX, badgeY, badgeRadius, ST77XX_WHITE);
        
        // Draw count text (centered)
        tft->setTextSize(1);
        tft->setTextColor(ST77XX_WHITE, ST77XX_RED);
        if (friendItem->unreadCount <= 9) {
            // Single digit - center it
            tft->setCursor(badgeX - 3, badgeY - 4);
            tft->print(friendItem->unreadCount);
        } else {
            // Display "9+"
            tft->setCursor(badgeX - 6, badgeY - 4);
            tft->print("9+");
        }
    }
}

void SocialScreen::drawFriendsList() {
    // Clear content area
    tft->fillRect(CONTENT_X, 0, CONTENT_WIDTH, SCREEN_HEIGHT, currentTheme.colorBg);
    
    // Draw vertical separator line (always visible)
    tft->drawFastVLine(CONTENT_X, 0, SCREEN_HEIGHT, currentTheme.colorAccent);
    tft->drawFastVLine(CONTENT_X + 1, 0, SCREEN_HEIGHT, currentTheme.colorAccent);
    
    // Draw modern header
    const uint16_t headerHeight = currentTheme.headerHeight;
    tft->setTextSize(1);
    tft->setTextColor(currentTheme.colorTextMain, currentTheme.colorBg);
    tft->setCursor(CONTENT_X + 10, 8);
    tft->print("FRIENDS");
    
    // Draw header line
    tft->drawFastHLine(CONTENT_X + 10, headerHeight - 2, CONTENT_WIDTH - 20, currentTheme.colorAccent);
    
    // Draw friends list as cards
    if (friendsCount == 0) {
        tft->setTextSize(1);
        tft->setTextColor(currentTheme.colorTextMuted, currentTheme.colorBg);
        tft->setCursor(CONTENT_X + 10, 60);
        tft->print("No friends yet");
    } else {
        const uint16_t cardHeight = currentTheme.rowHeight;
        const uint16_t cardSpacing = 4;
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
            
            // Use drawFriendItem helper method
            drawFriendItem(cardX, cardY, cardW, cardHeight, &friends[i], isSelected);
        }
    }
}

void SocialScreen::drawNotificationsList() {
    // Clear content area
    tft->fillRect(CONTENT_X, 0, CONTENT_WIDTH, SCREEN_HEIGHT, currentTheme.colorBg);
    
    // Draw vertical separator line (always visible)
    tft->drawFastVLine(CONTENT_X, 0, SCREEN_HEIGHT, currentTheme.colorAccent);
    tft->drawFastVLine(CONTENT_X + 1, 0, SCREEN_HEIGHT, currentTheme.colorAccent);
    
    // Draw modern header
    const uint16_t headerHeight = currentTheme.headerHeight;
    tft->setTextSize(1);
    tft->setTextColor(currentTheme.colorTextMain, currentTheme.colorBg);
    tft->setCursor(CONTENT_X + 10, 8);
    tft->print("THONG BAO");
    
    // Draw header line
    tft->drawFastHLine(CONTENT_X + 10, headerHeight - 2, CONTENT_WIDTH - 20, currentTheme.colorAccent);
    
    // Draw notifications list as cards
    if (notificationsCount == 0) {
        tft->setTextSize(1);
        tft->setTextColor(currentTheme.colorTextMuted, currentTheme.colorBg);
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
            uint16_t cardBgColor = isSelected ? currentTheme.colorHighlight : currentTheme.colorCardBg;
            tft->fillRoundRect(cardX, cardY, cardW, cardHeight, cardR, cardBgColor);
            
            // Draw notification type indicator (icon on left)
            // Use different color for friend requests to indicate they are actionable
            uint16_t typeColor = (notifications[i].type == "friend_request") ? currentTheme.colorSuccess : currentTheme.colorAccent;
            uint16_t iconX = cardX + 12;
            uint16_t iconY = cardY + cardHeight / 2;
            tft->fillCircle(iconX, iconY, 5, typeColor);
            tft->drawCircle(iconX, iconY, 5, typeColor);
            
            // Draw message (truncate if too long, can wrap to two lines)
            tft->setTextSize(1);
            tft->setTextColor(currentTheme.colorTextMain, cardBgColor);
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
                tft->setTextColor(currentTheme.colorTextMuted, cardBgColor);
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
    tft->fillRect(CONTENT_X, 0, CONTENT_WIDTH, SCREEN_HEIGHT, currentTheme.colorBg);
    
    // Draw vertical separator line (always visible)
    tft->drawFastVLine(CONTENT_X, 0, SCREEN_HEIGHT, currentTheme.colorAccent);
    tft->drawFastVLine(CONTENT_X + 1, 0, SCREEN_HEIGHT, currentTheme.colorAccent);
    
    // Draw header
    const uint16_t headerHeight = currentTheme.headerHeight;
    tft->setTextSize(1);
    tft->setTextColor(currentTheme.colorTextMain, currentTheme.colorBg);
    tft->setCursor(CONTENT_X + 10, 8);
    tft->print("ADD FRIEND");
    
    // Draw header line
    tft->drawFastHLine(CONTENT_X + 10, headerHeight - 2, CONTENT_WIDTH - 20, currentTheme.colorAccent);
    
    // Center the input box horizontally, position vertically to avoid keyboard overlap
    uint16_t inputW = CONTENT_WIDTH - 40;
    uint16_t inputH = 40;
    uint16_t inputX = CONTENT_X + (CONTENT_WIDTH - inputW) / 2;
    uint16_t inputY = 45;  // Moved up from 60 to avoid keyboard overlap (keyboard height: 121px, starts at Y=115)
    
    // Delegate drawing to MiniAddFriendScreen
    // Pass isActive state to show focus (cyan border when active)
    if (miniAddFriend != nullptr) {
        bool isActive = miniAddFriend->isActive() && (focusMode == FOCUS_CONTENT);
        miniAddFriend->draw(inputX, inputY, inputW, inputH, isActive);
    }
    
    // Keyboard is drawn by MiniAddFriendScreen::draw()
}

void SocialScreen::drawGameMenu() {
    // Clear content area
    tft->fillRect(CONTENT_X, 0, CONTENT_WIDTH, SCREEN_HEIGHT, currentTheme.colorBg);
    
    // Draw vertical separator line (always visible)
    tft->drawFastVLine(CONTENT_X, 0, SCREEN_HEIGHT, currentTheme.colorAccent);
    tft->drawFastVLine(CONTENT_X + 1, 0, SCREEN_HEIGHT, currentTheme.colorAccent);
    
    // Header
    const uint16_t headerHeight = currentTheme.headerHeight;
    tft->setTextSize(1);
    tft->setTextColor(currentTheme.colorTextMain, currentTheme.colorBg);
    tft->setCursor(CONTENT_X + 10, 8);
    tft->print("GAME CENTER");
    tft->drawFastHLine(CONTENT_X + 10, headerHeight - 2, CONTENT_WIDTH - 20, currentTheme.colorAccent);
    
    // Game list data (expanded)
    // Compact layout constants
    const uint16_t GAME_ROW_HEIGHT = 28;
    const uint16_t GAME_ROW_SPACING = 2;
    uint16_t startY = headerHeight + 4;
    
    // Show incoming game invites (compact list)
    if (gameInviteCount > 0) {
        tft->setTextSize(1);
        tft->setTextColor(currentTheme.colorAccent, currentTheme.colorBg);
        tft->setCursor(CONTENT_X + 10, startY);
        tft->print("INVITES");
        startY += 14;
        
        int invitesToShow = (gameInviteCount < 3) ? gameInviteCount : 3;
        for (int i = 0; i < invitesToShow; i++) {
            uint16_t y = startY + i * 14;
            bool highlighted = (i == selectedGameInviteIndex);
            if (highlighted) {
                tft->fillRoundRect(CONTENT_X + 8, y - 2, CONTENT_WIDTH - 16, 14, 3, currentTheme.colorHighlight);
                tft->setTextColor(currentTheme.colorTextMain, currentTheme.colorHighlight);
            } else {
                tft->setTextColor(currentTheme.colorTextMain, currentTheme.colorBg);
            }
            String line = "#" + String(gameInvites[i].sessionId) + " ";
            line += gameInvites[i].gameType;
            line += " (" + gameInvites[i].status + ")";
            tft->setCursor(CONTENT_X + 12, y);
            int maxWidth = (CONTENT_WIDTH - 24) / 6;
            if (line.length() > maxWidth) {
                line = line.substring(0, maxWidth - 3) + "...";
            }
            tft->print(line);
        }
        startY += invitesToShow * 14 + 6;
    }
    
    // Visible items (for potential scrolling)
    const int visibleItems = (SCREEN_HEIGHT - startY) / (GAME_ROW_HEIGHT + GAME_ROW_SPACING);
    // Optional scrollOffset could be added; for now assume selectedGameIndex is in range
    
    for (int i = 0; i < TOTAL_GAMES; i++) {
        uint16_t y = startY + i * (GAME_ROW_HEIGHT + GAME_ROW_SPACING);
        uint16_t pillX = CONTENT_X + 10;
        uint16_t pillW = CONTENT_WIDTH - 20;
        uint16_t pillH = 22;  // slim highlight height
        uint16_t pillY = y + (GAME_ROW_HEIGHT - pillH) / 2;
        bool isSelected = (i == selectedGameIndex);
        
        // Background: only draw pill when selected; else keep clean background
        if (isSelected) {
            tft->fillRoundRect(pillX, pillY, pillW, pillH, 4, currentTheme.colorHighlight);
            // Accent bar inside pill (left)
            tft->fillRect(pillX, pillY + 2, 2, pillH - 4, currentTheme.colorAccent);
        }
        
        // Text positions (text-only)
        uint16_t nameX = pillX + 8;
        uint16_t nameY = y + (GAME_ROW_HEIGHT - 8) / 2;  // center for size 1 font
        
        // Game name (left aligned, clean text)
        tft->setTextColor(currentTheme.colorTextMain, currentTheme.colorCardBg);
        tft->setTextSize(1);
        tft->setCursor(nameX, nameY);
        tft->print(GAME_NAMES[i]);
        
        // Status (right aligned)
        bool isReady = (i == GAME_CARO);
        uint16_t statusColor = isReady ? currentTheme.colorSuccess : currentTheme.colorTextMuted;
        const char* statusText = GAME_STATUS[i];
        int statusWidth = strlen(statusText) * 6; // approx width size1
        uint16_t statusX = pillX + pillW - statusWidth - 8;
        uint16_t statusY = nameY;
        tft->setTextColor(statusColor, currentTheme.colorCardBg);
        tft->setCursor(statusX, statusY);
        tft->print(statusText);
    }
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
        case TAB_GAMES:
            drawGameMenu();
            break;
    }
}

void SocialScreen::draw() {
    if (screenState == STATE_PLAYING_GAME) {
        if (caroGameScreen != nullptr) {
            caroGameScreen->draw();
        }
        return;
    }
    
    if (screenState == STATE_WAITING_GAME) {
        if (gameLobby != nullptr) {
            gameLobby->update();  // Check auto-start timer
            gameLobby->draw();
        }
        if (confirmationDialog != nullptr && confirmationDialog->isVisible()) {
            confirmationDialog->draw();
        }
        return;
    }
    
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
    // Handle new format first
    if (key == "up") {
        // Move to previous tab (circular: from TAB_FRIENDS, go to TAB_GAMES)
        if (currentTab > TAB_FRIENDS) {
            switchTab((Tab)(currentTab - 1));
        } else {
            // At first tab (TAB_FRIENDS), wrap around to last tab (TAB_GAMES)
            switchTab(TAB_GAMES);
        }
    } else if (key == "down") {
        // Move to next tab (circular: from TAB_GAMES, go to TAB_FRIENDS)
        if (currentTab < TAB_GAMES) {
            switchTab((Tab)(currentTab + 1));
        } else {
            // At last tab (TAB_GAMES), wrap around to first tab (TAB_FRIENDS)
            switchTab(TAB_FRIENDS);
        }
    }
    // Backward compatibility: handle old format
    else if (key == "|u") {
        // Move to previous tab (circular)
        if (currentTab > TAB_FRIENDS) {
            switchTab((Tab)(currentTab - 1));
        } else {
            switchTab(TAB_GAMES);
        }
    } else if (key == "|d") {
        // Move to next tab (circular)
        if (currentTab < TAB_GAMES) {
            switchTab((Tab)(currentTab + 1));
        } else {
            switchTab(TAB_FRIENDS);
        }
    }
}

void SocialScreen::redrawFriendCard(int index, bool isSelected) {
    if (index < 0 || index >= friendsCount) return;
    
    const uint16_t cardHeight = currentTheme.rowHeight;
    const uint16_t cardSpacing = 4;
    const uint16_t headerHeight = currentTheme.headerHeight;
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
    
    // Use drawFriendItem helper method for consistent rendering
    drawFriendItem(cardX, cardY, cardW, cardHeight, &friends[index], isSelected);
}

void SocialScreen::redrawNotificationCard(int index, bool isSelected) {
    if (index < 0 || index >= notificationsCount) return;
    
    const uint16_t cardHeight = 48;
    const uint16_t cardSpacing = 4;
    const uint16_t headerHeight = currentTheme.headerHeight;
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
    uint16_t cardBgColor = isSelected ? currentTheme.colorHighlight : currentTheme.colorCardBg;
    tft->fillRoundRect(cardX, cardY, cardW, cardHeight, cardR, cardBgColor);
    
            // Draw notification type indicator
            // Use different color for friend requests to indicate they are actionable
            uint16_t typeColor = (notifications[index].type == "friend_request") ? currentTheme.colorSuccess : currentTheme.colorAccent;
            uint16_t iconX = cardX + 12;
            uint16_t iconY = cardY + cardHeight / 2;
            tft->fillCircle(iconX, iconY, 5, typeColor);
            tft->drawCircle(iconX, iconY, 5, typeColor);
    
    // Draw message
    tft->setTextSize(1);
    tft->setTextColor(currentTheme.colorTextMain, cardBgColor);
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
        tft->setTextColor(currentTheme.colorTextMuted, cardBgColor);
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
            const uint16_t cardHeight = currentTheme.rowHeight;
            const uint16_t cardSpacing = 4;
            const uint16_t headerHeight = currentTheme.headerHeight;
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
            const uint16_t cardHeight = currentTheme.rowHeight;
            const uint16_t cardSpacing = 4;
            const uint16_t headerHeight = currentTheme.headerHeight;
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
        } else if (key == "|e") {
            // Enter: Mở chat với friend được chọn
            if (selectedFriendIndex >= 0 && selectedFriendIndex < friendsCount) {
                openChatWithFriend(selectedFriendIndex);
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
            const uint16_t headerHeight = currentTheme.headerHeight;
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
            const uint16_t headerHeight = currentTheme.headerHeight;
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
    } else if (currentTab == TAB_GAMES) {
        // Invite navigation and actions
        if (gameInviteCount > 0 && selectedGameInviteIndex < 0) {
            selectedGameInviteIndex = 0;
        }

        if (gameInviteCount > 0 && selectedGameInviteIndex >= 0) {
            if (key == "|u") {
                if (selectedGameInviteIndex > 0) {
                    selectedGameInviteIndex--;
                } else {
                    selectedGameInviteIndex = -1;  // Move focus to game list
                }
                drawContentArea();
                return;
            } else if (key == "|d") {
                if (selectedGameInviteIndex < gameInviteCount - 1) {
                    selectedGameInviteIndex++;
                } else {
                    selectedGameInviteIndex = -1;  // Move focus to game list
                }
                drawContentArea();
                return;
            } else if (key == "|e") {
                if (userId > 0 && serverHost.length() > 0) {
                    int sessionId = gameInvites[selectedGameInviteIndex].sessionId;
                    ApiClient::GameSessionResult result = ApiClient::respondGameInvite(sessionId, userId, true, serverHost, serverPort);
                    Serial.print("Social Screen: Accept invite result: ");
                    Serial.println(result.message);
                    for (int j = selectedGameInviteIndex; j < gameInviteCount - 1; j++) {
                        gameInvites[j] = gameInvites[j + 1];
                    }
                    gameInviteCount = (gameInviteCount > 0) ? gameInviteCount - 1 : 0;
                    if (gameInviteCount == 0) {
                        selectedGameInviteIndex = -1;
                    } else if (selectedGameInviteIndex >= gameInviteCount) {
                        selectedGameInviteIndex = gameInviteCount - 1;
                    }
                    drawContentArea();
                }
                return;
            } else if (key == "|l") {
                if (userId > 0 && serverHost.length() > 0) {
                    int sessionId = gameInvites[selectedGameInviteIndex].sessionId;
                    ApiClient::GameSessionResult result = ApiClient::respondGameInvite(sessionId, userId, false, serverHost, serverPort);
                    Serial.print("Social Screen: Decline invite result: ");
                    Serial.println(result.message);
                    for (int j = selectedGameInviteIndex; j < gameInviteCount - 1; j++) {
                        gameInvites[j] = gameInvites[j + 1];
                    }
                    gameInviteCount = (gameInviteCount > 0) ? gameInviteCount - 1 : 0;
                    if (gameInviteCount == 0) {
                        selectedGameInviteIndex = -1;
                    } else if (selectedGameInviteIndex >= gameInviteCount) {
                        selectedGameInviteIndex = gameInviteCount - 1;
                    }
                    drawContentArea();
                }
                return;
            }
        }

        // Game list navigation
        if (key == "|u") {
            if (gameInviteCount > 0 && selectedGameInviteIndex == -1) {
                // Jump back to invites
                selectedGameInviteIndex = gameInviteCount - 1;
                drawContentArea();
                return;
            }
            if (selectedGameInviteIndex < 0 && selectedGameIndex > 0) {
                selectedGameIndex--;
                drawContentArea();
                return;
            }
        } else if (key == "|d") {
            if (selectedGameInviteIndex < 0 && selectedGameIndex < TOTAL_GAMES - 1) {
                selectedGameIndex++;
                drawContentArea();
                return;
            }
        } else if (key == "|e") {
            if (selectedGameInviteIndex >= 0 && gameInviteCount > 0) {
                // Already handled above
                return;
            }
            // Enter game room lobby
            screenState = STATE_WAITING_GAME;
            pendingGameName = String(GAME_NAMES[selectedGameIndex]);
            
            // Get current user's name
            String hostName = (ownerNickname.length() > 0) ? ownerNickname : "Host";
            currentGameHostName = hostName;
            currentGameGuestName = "";  // Will be set when guest joins
            
            // Tạo game session trước khi vào lobby
            if (userId > 0 && serverHost.length() > 0) {
                String gameType = pendingGameName;
                gameType.toLowerCase();
                int maxPlayers = (gameType == "caro") ? 2 : 4;
                
                // Tìm online friends để invite
                int onlineFriendIds[8];
                int onlineFriendCount = 0;
                for (int i = 0; i < friendsCount && onlineFriendCount < maxPlayers - 1; i++) {
                    if (friends[i].online && friends[i].userId > 0) {
                        onlineFriendIds[onlineFriendCount] = friends[i].userId;
                        onlineFriendCount++;
                    }
                }
                
                Serial.print("Social Screen: Creating game session for ");
                Serial.print(pendingGameName);
                Serial.print(" with ");
                Serial.print(onlineFriendCount);
                Serial.println(" participant(s)");
                
                ApiClient::GameSessionResult result = ApiClient::createGameSession(
                    userId,
                    gameType,
                    maxPlayers,
                    onlineFriendIds,
                    onlineFriendCount,
                    serverHost,
                    serverPort
                );
                
                if (result.success) {
                    currentGameSessionId = result.sessionId;
                    Serial.print("Social Screen: ✅ Created game session ");
                    Serial.println(currentGameSessionId);
                } else {
                    Serial.print("Social Screen: ❌ Failed to create game session: ");
                    Serial.println(result.message);
                    currentGameSessionId = -1;
                }
            } else {
                Serial.println("Social Screen: ⚠️ Cannot create game session - invalid user ID or server info");
                currentGameSessionId = -1;
            }
            
            gameLobby->setup(pendingGameName, hostName);
            
            // Set gameLobby active khi vào STATE_WAITING_GAME
            if (gameLobby != nullptr) {
                gameLobby->setActive(true);
            }
            
            // Convert current friends to lobby friends
            if (friendsCount > 0) {
                GameLobbyScreen::MiniFriend* miniFriends = new GameLobbyScreen::MiniFriend[friendsCount];
                for (int i = 0; i < friendsCount; i++) {
                    miniFriends[i] = {friends[i].nickname, friends[i].online};
                }
                gameLobby->setFriends(miniFriends, friendsCount);
                // Note: In a real app, we'd need to manage the lifecycle of this array.
                // For this simple implementation, let's just pass it.
            }

            Serial.print("Social Screen: Entered room for game ");
            Serial.println(pendingGameName);
            
            // Tự động nhấn nút START khi vào lobby (sau khi đã tạo session)
            Serial.println("Social Screen: Auto-pressing START button");
            gameLobby->triggerStart();
            
            draw();
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
        
        // Clear red dot badge on Friends icon when switching to friends tab (chat)
        // Note: Unread count trên từng friend card vẫn giữ nguyên để user biết friend nào có tin nhắn
        if (newTab == TAB_FRIENDS && hasUnreadChatFlag) {
            hasUnreadChatFlag = false;
            Serial.println("Social Screen: Cleared hasUnreadChatFlag on Friends icon (switched to friends tab)");
            Serial.println("Social Screen: Note: Unread badges on friend cards are preserved");
        }
        
        currentTab = newTab;
        // Auto-focus input when switching to Add Friend tab
        if (newTab == TAB_ADD_FRIEND) {
            focusMode = FOCUS_CONTENT;
            // Reset miniAddFriend to clear any previous input (also resets keyboard)
            if (miniAddFriend != nullptr) {
                miniAddFriend->reset();
                // Set miniAddFriend active if SocialScreen is active
                miniAddFriend->setActive(isActive);
            }
        } else {
            // Reset focus to sidebar when switching to other tabs
            // IMPORTANT: Always set to SIDEBAR for non-Add-Friend tabs to allow tab navigation
            focusMode = FOCUS_SIDEBAR;
            // Set miniAddFriend inactive when leaving Add Friend tab
            if (miniAddFriend != nullptr) {
                miniAddFriend->setActive(false);
            }
        }
        // Reset selection indices when switching tabs
        selectedFriendIndex = 0;
        friendsScrollOffset = 0;
        selectedNotificationIndex = 0;
        notificationsScrollOffset = 0;
        selectedGameIndex = 0;
        selectedGameInviteIndex = (gameInviteCount > 0) ? 0 : -1;
        draw();
    }
}

void SocialScreen::navigateToAddFriend() {
    // Switch to Add Friend tab
    switchTab(TAB_ADD_FRIEND);
    // Sync navigation state
    syncNavigation();
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
    // Sync navigation state (will set focusMode = FOCUS_SIDEBAR for tab navigation)
    syncNavigation();
    // Don't force FOCUS_CONTENT - let user navigate tabs first, then press RIGHT/SELECT to focus content
    // Draw the screen
    draw();
}

void SocialScreen::navigateToGames() {
    // Switch to Games tab
    switchTab(TAB_GAMES);
    // Sync navigation state (will set focusMode = FOCUS_SIDEBAR for tab navigation)
    syncNavigation();
    // Don't force FOCUS_CONTENT - let user navigate tabs first, then press RIGHT/SELECT to focus content
    // Draw the screen
    draw();
}

void SocialScreen::navigateToFriends() {
    // Switch to Friends tab (chat)
    switchTab(TAB_FRIENDS);
    // Sync navigation state (will set focusMode = FOCUS_SIDEBAR for tab navigation)
    syncNavigation();
    // Don't force FOCUS_CONTENT - let user navigate tabs first, then press RIGHT/SELECT to focus content
    // Draw the screen
    draw();
}

void SocialScreen::openChatWithFriend(int friendIndex) {
    if (friendIndex < 0 || friendIndex >= friendsCount) {
        Serial.println("Social Screen: Invalid friend index");
        return;
    }
    
    FriendItem* friendItem = &friends[friendIndex];
    if (friendItem->userId <= 0) {
        Serial.println("Social Screen: Invalid friend userId");
        return;
    }
    
    Serial.print("Social Screen: Opening chat with friend: ");
    Serial.print(friendItem->nickname);
    Serial.print(" (ID: ");
    Serial.print(friendItem->userId);
    Serial.println(")");
    
    // Clear unread count cho friend này khi mở chat
    clearUnreadChatForFriend(friendItem->userId);
    
    // Redraw friend card to remove badge
    redrawFriendCard(friendIndex, true);
    
    // Parent vẫn active, chỉ mở ChatScreen (child)
    // Không set inactive - parent vẫn active
    // setActive(false);  // ❌ REMOVED - parent stays active
    
    // Call callback để main.cpp mở ChatScreen
    if (onOpenChatCallback != nullptr) {
        onOpenChatCallback(friendItem->userId, friendItem->nickname);
    } else {
        Serial.println("Social Screen: ⚠️  onOpenChatCallback is null!");
    }
}

// Navigation handlers
void SocialScreen::handleUp() {
    if (focusMode == FOCUS_SIDEBAR) {
        // Focus on sidebar: navigate between tabs
        handleTabNavigation("up");
    } else if (focusMode == FOCUS_CONTENT) {
        // Focus on content: navigate within content (scroll lists)
        // Exception: If on Add Friend tab, UP/DOWN should navigate tabs, not content
        if (currentTab == TAB_ADD_FRIEND) {
            handleTabNavigation("up");
        } else {
            handleContentNavigation("up");
        }
    }
}

void SocialScreen::handleDown() {
    if (focusMode == FOCUS_SIDEBAR) {
        // Focus on sidebar: navigate between tabs
        handleTabNavigation("down");
    } else if (focusMode == FOCUS_CONTENT) {
        // Focus on content: navigate within content (scroll lists)
        // Exception: If on Add Friend tab, UP/DOWN should navigate tabs, not content
        if (currentTab == TAB_ADD_FRIEND) {
            handleTabNavigation("down");
        } else {
            handleContentNavigation("down");
        }
    }
}

void SocialScreen::handleLeft() {
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
}

void SocialScreen::handleRight() {
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
}

void SocialScreen::handleSelect() {
    if (focusMode == FOCUS_CONTENT) {
        handleContentNavigation("select");
    }
    // If focus is on sidebar, Enter could select the tab (but tab is already selected)
    // So we move focus to content when Enter is pressed on sidebar
    else if (focusMode == FOCUS_SIDEBAR) {
        focusMode = FOCUS_CONTENT;
        draw();
    }
}

void SocialScreen::handleExit() {
    // Exit/back - no specific action needed at SocialScreen level
    // Child screens will handle their own exit logic
}

void SocialScreen::handleKeyPress(const String& key) {
    // Handle new navigation key format first (similar to WiFi password screen)
    if (key == "up") {
        handleUp();
        return;
    } else if (key == "down") {
        handleDown();
        return;
    } else if (key == "left") {
        handleLeft();
        return;
    } else if (key == "right") {
        handleRight();
        return;
    } else if (key == "select") {
        handleSelect();
        return;
    } else if (key == "exit") {
        handleExit();
        return;
    }
    
    // If in game playing state: delegate to game screen
    if (screenState == STATE_PLAYING_GAME) {
        if (caroGameScreen != nullptr) {
            caroGameScreen->handleKeyPress(key);
        }
        return;
    }
    
    // If in game waiting state: delegate to lobby screen
    if (screenState == STATE_WAITING_GAME) {
        if (gameLobby != nullptr) {
            // Add a special case for exiting the lobby
            if (key == "<" || key == "|b" || key == "exit") {
                Serial.println("Social Screen: Leaving game room, returning to games tab");
                screenState = STATE_NORMAL;
                pendingGameName = "";
                // Ensure we are on Games tab to show menu again
                switchTab(TAB_GAMES);
                focusMode = FOCUS_CONTENT;
                draw();
                return;
            }
            gameLobby->handleKeyPress(key);
        }
        return;
    }
    
    // Popup notification disabled - no need to check or hide popup
    
    // If on Add Friend tab and typing, forward to MiniAddFriendScreen
    if (currentTab == TAB_ADD_FRIEND && 
        (key.length() == 1 || key == "|e" || key == "select" || key == "<" || key == "exit" || key == "123" || key == "ABC")) {
        // When typing in Add Friend, focus should be on content
        focusMode = FOCUS_CONTENT;
        handleContentNavigation(key);
        return;
    }
    
    // Backward compatibility: handle old key format
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
    
    // Backward compatibility: Up/Down: Navigate based on focus mode
    if (key == "|u") {
        handleUp();
        return;
    } else if (key == "|d") {
        handleDown();
        return;
    }
    
    // Backward compatibility: Enter key: handle based on current tab and focus
    if (key == "|e") {
        handleSelect();
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
    
    // Parse format: "user1,userId1,0|user2,userId2,1|..."
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
            if (entry.length() > 0) {
                // Parse: "nickname,userId,online"
                int comma1 = entry.indexOf(',');
                if (comma1 > 0) {
                    friends[entryIndex].nickname = entry.substring(0, comma1);
                    
                    int comma2 = entry.indexOf(',', comma1 + 1);
                    if (comma2 > comma1) {
                        // Parse userId
                        String userIdStr = entry.substring(comma1 + 1, comma2);
                        friends[entryIndex].userId = userIdStr.toInt();
                        
                        // Parse online status
                        String onlineStr = entry.substring(comma2 + 1);
                        friends[entryIndex].online = (onlineStr == "1");
                    } else {
                        // Fallback: old format "nickname,online" (no userId)
                        String onlineStr = entry.substring(comma1 + 1);
                        friends[entryIndex].userId = -1;  // Unknown userId
                        friends[entryIndex].online = (onlineStr == "1");
                    }
                    
                    // Initialize unread count
                    friends[entryIndex].unreadCount = 0;
                    entryIndex++;
                }
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

void SocialScreen::updateFriendStatus(int friendUserId, bool isOnline) {
    Serial.print("Social Screen: Updating friend status - userId: ");
    Serial.print(friendUserId);
    Serial.print(", isOnline: ");
    Serial.println(isOnline ? "true" : "false");
    
    // Check child screen: if STATE_PLAYING_GAME and caroGameScreen is active, don't draw
    if (screenState == STATE_PLAYING_GAME && caroGameScreen != nullptr && caroGameScreen->isActive()) {
        // Just update data, don't draw anything (focus on caro game)
        for (int i = 0; i < friendsCount; i++) {
            if (friends[i].userId == friendUserId) {
                friends[i].online = isOnline;
                Serial.println("Social Screen: Status updated (data only) - caro game is active (focused)");
                return;
            }
        }
        Serial.println("Social Screen: Friend not found, but skipping UI update (caro game focused)");
        return;
    }
    
    // Check parent active: if not active, don't draw
    if (!isActive) {
        // Just update data, don't draw anything
        for (int i = 0; i < friendsCount; i++) {
            if (friends[i].userId == friendUserId) {
                friends[i].online = isOnline;
                Serial.println("Social Screen: Status updated (data only) - screen not active");
                return;
            }
        }
        return;
    }
    
    // Find friend in friends array
    for (int i = 0; i < friendsCount; i++) {
        if (friends[i].userId == friendUserId) {
            // Update status
            bool statusChanged = (friends[i].online != isOnline);
            friends[i].online = isOnline;
            
            Serial.print("Social Screen: Updated friend at index ");
            Serial.print(i);
            Serial.print(" (");
            Serial.print(friends[i].nickname);
            Serial.print(") to ");
            Serial.println(isOnline ? "online" : "offline");
            
            // Redraw friend card if we're on the friends tab and status changed
            // Only draw if parent active AND not in game playing state
            if (statusChanged && screenState == STATE_NORMAL && currentTab == TAB_FRIENDS) {
                bool isSelected = (i == selectedFriendIndex);
                Serial.print("Social Screen: Redrawing friend card at index ");
                Serial.println(i);
                redrawFriendCard(i, isSelected);
            } else if (statusChanged) {
                // If not on friends tab, just log (will update when user navigates to friends tab)
                Serial.println("Social Screen: Status changed but not on friends tab - will update when navigated");
            }

            // Nếu đang ở lobby chờ, cập nhật danh sách bạn trong lobby với online flag mới
            // Check parent active AND lobby active
            if (statusChanged && screenState == STATE_WAITING_GAME && gameLobby != nullptr && gameLobby->isActive() && friendsCount > 0) {
                GameLobbyScreen::MiniFriend* miniFriends = new GameLobbyScreen::MiniFriend[friendsCount];
                for (int j = 0; j < friendsCount; j++) {
                    miniFriends[j] = {friends[j].nickname, friends[j].online};
                }
                gameLobby->setFriends(miniFriends, friendsCount);
                gameLobby->draw();  // refresh lobby list with status dots
            }
            
            return;
        }
    }
    
    Serial.print("Social Screen: ⚠️ Friend with userId ");
    Serial.print(friendUserId);
    Serial.println(" not found in friends list");
}

// Static callback wrapper
void SocialScreen::onUserStatusUpdate(int userId, const String& status) {
    if (s_socialScreenInstance == nullptr) {
        Serial.println("Social Screen: ⚠️ onUserStatusUpdate called but instance not set");
        return;
    }
    
    // Convert status string to bool
    bool isOnline = (status == "online");
    s_socialScreenInstance->updateFriendStatus(userId, isOnline);
}

void SocialScreen::onGameEvent(const String& eventType, int sessionId, const String& gameType, const String& status, int userId, bool accepted, bool ready, const String& userNickname) {
    if (s_socialScreenInstance == nullptr) {
        Serial.println("Social Screen: ⚠️ onGameEvent called but instance not set");
        return;
    }

    SocialScreen* self = s_socialScreenInstance;

    Serial.print("Social Screen: onGameEvent type=");
    Serial.print(eventType);
    Serial.print(" session=");
    Serial.print(sessionId);
    Serial.print(" status=");
    Serial.print(status);
    Serial.print(" user=");
    Serial.print(userId);
    Serial.print(" accepted=");
    Serial.print(accepted ? "true" : "false");
    Serial.print(" ready=");
    Serial.print(ready ? "true" : "false");
    Serial.print(" nickname=");
    Serial.println(userNickname);

    // Chỉ quan tâm các event liên quan game (invite/respond/ready)
    bool isGameEventType = (eventType == "respond") || (eventType == "ready") || (eventType == "invite");
    // Fallback: nếu eventType unknown nhưng có accepted/ready, vẫn xử lý như join event
    bool isJoinEvent = (eventType == "respond" && accepted) || (eventType == "ready" && ready) || 
                       (eventType == "unknown" && (accepted || ready));
    if (!isGameEventType && !isJoinEvent) {
        return;
    }

    // Chỉ cập nhật guest khi đúng loại event (join/ready) và gameLobby đang active
    if (self->screenState == STATE_WAITING_GAME && self->gameLobby != nullptr && self->gameLobby->isActive()) {
        if (isJoinEvent) {
            // Use nickname from server, fallback to "User<id>" if empty
            String guestName = userNickname.length() > 0 ? userNickname : ("User" + String(userId));
            self->currentGameGuestName = guestName;
            self->gameLobby->setGuest(guestName);
            self->gameLobby->setGuestReady(ready);
            self->gameLobby->draw();
        }
    }

    // Không tự chuyển tab hay thay đổi screenState ở đây để tránh "nhảy màn hình"
}

void SocialScreen::startGame() {
    if (pendingGameName != "Caro" && pendingGameName != "caro") {
        Serial.println("Social Screen: Only Caro game is supported for now");
        return;
    }
    
    if (currentGameSessionId <= 0) {
        Serial.println("Social Screen: No active game session");
        return;
    }
    
    // Create CaroGameScreen if not exists
    if (caroGameScreen == nullptr) {
        caroGameScreen = new CaroGameScreen(tft, currentTheme);
    }
    
    // Setup game screen
    caroGameScreen->setup(
        currentGameSessionId,
        currentGameHostName,
        currentGameGuestName,
        userId,
        true,  // isHost - TODO: determine from session
        serverHost,
        serverPort
    );
    
    // Set exit callback
    caroGameScreen->setOnExitGame([]() {
        if (s_socialScreenInstance != nullptr) {
            s_socialScreenInstance->exitGame();
        }
    });
    
    // Parent vẫn active, chỉ set child screen active
    // isActive vẫn = true (không set false)
    // Chỉ set caroGameScreen active
    if (caroGameScreen != nullptr) {
        caroGameScreen->setActive(true);
    }
    
    screenState = STATE_PLAYING_GAME;
    draw();
}

void SocialScreen::exitLobby() {
    screenState = STATE_NORMAL;
    pendingGameName = "";
    currentTab = TAB_GAMES;
    
    // Set active khi về game menu
    // setActive() will automatically manage child screens
    setActive(true);
    
    // Set gameLobby inactive khi rời lobby (explicit since we're leaving lobby)
    if (gameLobby != nullptr) {
        gameLobby->setActive(false);
    }
    
    draw();
}

void SocialScreen::exitGame() {
    screenState = STATE_WAITING_GAME;
    
    // Set active lại khi về lobby (có thể hiển thị badge)
    // setActive() will automatically set gameLobby active if in STATE_WAITING_GAME
    setActive(true);
    
    if (caroGameScreen != nullptr) {
        caroGameScreen->setActive(false);
    }
    // Ensure gameLobby is active and draw
    if (gameLobby != nullptr) {
        gameLobby->setActive(true);
        gameLobby->draw();
    }
}

void SocialScreen::onGameMoveReceived(int sessionId, int userId, int row, int col, const String& gameStatus, int winnerId, int currentTurn) {
    if (screenState == STATE_PLAYING_GAME && caroGameScreen != nullptr && caroGameScreen->isActive()) {
        if (caroGameScreen->getSessionId() == sessionId) {
            caroGameScreen->onMoveReceived(row, col, userId, gameStatus, winnerId, currentTurn);
        }
    }
}

void SocialScreen::setActive(bool active) {
    bool wasActive = this->isActive;
    this->isActive = active;
    
    // When parent becomes inactive, set all child screens inactive
    if (!active) {
        if (gameLobby != nullptr) {
            gameLobby->setActive(false);
        }
        if (miniAddFriend != nullptr) {
            miniAddFriend->setActive(false);
        }
        if (caroGameScreen != nullptr) {
            caroGameScreen->setActive(false);
        }
    } else {
        // When parent becomes active (transitioning from inactive to active), sync navigation state
        if (!wasActive) {
            syncNavigation();
        }
        
        // When parent becomes active, set child screens active based on current state/tab
        if (screenState == STATE_WAITING_GAME && gameLobby != nullptr) {
            gameLobby->setActive(true);
        }
        if (screenState == STATE_NORMAL && currentTab == TAB_ADD_FRIEND && miniAddFriend != nullptr) {
            miniAddFriend->setActive(true);
        }
    }
}

void SocialScreen::update() {
    // Update lobby if in waiting state (check auto-start timer)
    if (screenState == STATE_WAITING_GAME && gameLobby != nullptr) {
        gameLobby->update();
    }
    
    // Update game screen if playing
    if (screenState == STATE_PLAYING_GAME && caroGameScreen != nullptr && caroGameScreen->isActive()) {
        caroGameScreen->update();
    }
    
}

void SocialScreen::onGameComingSoonConfirm() {
    if (s_socialScreenInstance != nullptr && s_socialScreenInstance->confirmationDialog != nullptr) {
        s_socialScreenInstance->confirmationDialog->hide();
        s_socialScreenInstance->draw();
    }
}

void SocialScreen::onGameComingSoonCancel() {
    if (s_socialScreenInstance != nullptr && s_socialScreenInstance->confirmationDialog != nullptr) {
        s_socialScreenInstance->confirmationDialog->hide();
        s_socialScreenInstance->draw();
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

void SocialScreen::syncNavigation() {
    // Reset navigation state based on current tab
    switch (currentTab) {
        case TAB_FRIENDS:
            selectedFriendIndex = 0;
            friendsScrollOffset = 0;
            // Focus mode: sidebar for tab navigation, but can be overridden by navigateTo* functions
            if (focusMode != FOCUS_CONTENT) {
                focusMode = FOCUS_SIDEBAR;
            }
            break;
            
        case TAB_NOTIFICATIONS:
            selectedNotificationIndex = (notificationsCount > 0) ? 0 : -1;
            notificationsScrollOffset = 0;
            // Focus mode: sidebar for tab navigation, but can be overridden by navigateTo* functions
            if (focusMode != FOCUS_CONTENT) {
                focusMode = FOCUS_SIDEBAR;
            }
            break;
            
        case TAB_ADD_FRIEND:
            // Focus mode should be CONTENT for keyboard input
            focusMode = FOCUS_CONTENT;
            // Reset miniAddFriend if needed
            if (miniAddFriend != nullptr) {
                miniAddFriend->reset();
            }
            break;
            
        case TAB_GAMES:
            selectedGameIndex = 0;
            selectedGameInviteIndex = (gameInviteCount > 0) ? 0 : -1;
            // Focus mode: sidebar for tab navigation, but can be overridden by navigateTo* functions
            if (focusMode != FOCUS_CONTENT) {
                focusMode = FOCUS_SIDEBAR;
            }
            break;
    }
    
    Serial.print("Social Screen: Navigation synced - Tab: ");
    Serial.print(currentTab);
    Serial.print(", Focus: ");
    Serial.print(focusMode == FOCUS_SIDEBAR ? "SIDEBAR" : "CONTENT");
    Serial.println();
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

int SocialScreen::getFirstFriendWithUnreadIndex() const {
    if (friends == nullptr || friendsCount == 0) {
        return -1;
    }
    
    // Find first friend with unread messages
    for (int i = 0; i < friendsCount; i++) {
        if (friends[i].unreadCount > 0) {
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
        const uint16_t headerHeight = currentTheme.headerHeight;
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

void SocialScreen::selectFriend(int index) {
    if (index < 0 || index >= friendsCount) {
        Serial.println("Social Screen: Invalid friend index for selection");
        return;
    }
    
    int oldIndex = selectedFriendIndex;
    selectedFriendIndex = index;
    
    // Adjust scroll if needed
    const uint16_t cardHeight = currentTheme.rowHeight;
    const uint16_t cardSpacing = 4;
    const uint16_t headerHeight = currentTheme.headerHeight;
    const uint16_t startY = headerHeight + 4;
    const int visibleItems = (SCREEN_HEIGHT - startY) / (cardHeight + cardSpacing);
    
    if (selectedFriendIndex < friendsScrollOffset) {
        friendsScrollOffset = selectedFriendIndex;
        drawContentArea();  // Full redraw if scroll changed
    } else if (selectedFriendIndex >= friendsScrollOffset + visibleItems) {
        friendsScrollOffset = selectedFriendIndex - visibleItems + 1;
        drawContentArea();  // Full redraw if scroll changed
    } else {
        // Partial redraw: unselect old, select new
        redrawFriendCard(oldIndex, false);
        redrawFriendCard(selectedFriendIndex, true);
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

void SocialScreen::addGameInviteFromSocket(int sessionId, const String& gameType, const String& status, const String& eventType, const String& hostNickname) {
    if (sessionId <= 0) {
        return;
    }

    int targetIndex = -1;
    for (int i = 0; i < gameInviteCount; i++) {
        if (gameInvites[i].sessionId == sessionId) {
            targetIndex = i;
            break;
        }
    }

    // Remove invite if cancelled
    if (status == "cancelled") {
        if (targetIndex >= 0) {
            for (int j = targetIndex; j < gameInviteCount - 1; j++) {
                gameInvites[j] = gameInvites[j + 1];
            }
            gameInviteCount = (gameInviteCount > 0) ? gameInviteCount - 1 : 0;
            Serial.print("Social Screen: Removed cancelled game invite ");
            Serial.println(sessionId);
            if (currentTab == TAB_GAMES) {
                drawContentArea();
            }
        }
        return;
    }

    if (targetIndex < 0) {
        if (gameInviteCount < MAX_GAME_INVITES) {
            targetIndex = gameInviteCount;
            gameInviteCount++;
        } else {
            // Shift to make room and overwrite the last slot
            for (int j = 0; j < MAX_GAME_INVITES - 1; j++) {
                gameInvites[j] = gameInvites[j + 1];
            }
            targetIndex = MAX_GAME_INVITES - 1;
        }
    }

    gameInvites[targetIndex].sessionId = sessionId;
    gameInvites[targetIndex].gameType = gameType;
    gameInvites[targetIndex].status = status;
    gameInvites[targetIndex].eventType = eventType;
    gameInvites[targetIndex].hostNickname = hostNickname;
    if (gameInviteCount > 0 && selectedGameInviteIndex < 0) {
        selectedGameInviteIndex = 0;
    }

    Serial.print("Social Screen: Added/updated game invite ");
    Serial.print(sessionId);
    Serial.print(" status: ");
    Serial.println(status);

    // Nếu đang ở lobby chờ, không vẽ lại Game Center để tránh bị "văng" khỏi lobby
    if (screenState == STATE_WAITING_GAME) {
        return;
    }

    if (currentTab == TAB_GAMES) {
        drawContentArea();
    } else {
        // Reuse notification badge to signal new game invites
        if (!hasUnreadNotification) {
            hasUnreadNotification = true;
            drawSidebar();
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
    tft->fillRoundRect(popupX, popupY, popupWidth, popupHeight, cornerRadius, currentTheme.colorCardBg);
    tft->drawRoundRect(popupX, popupY, popupWidth, popupHeight, cornerRadius, currentTheme.colorAccent);
    
    // Draw accent line at top
    tft->drawFastHLine(popupX + 4, popupY + 2, popupWidth - 8, currentTheme.colorAccent);
    
    // Draw notification icon (bell) on left
    const uint16_t iconSize = 16;
    const uint16_t iconX = popupX + padding;
    const uint16_t iconY = popupY + (popupHeight - iconSize) / 2;
    drawNotificationsIcon(iconX, iconY, currentTheme.colorSuccess);
    
    // Draw message text
    tft->setTextSize(1);
    tft->setTextColor(currentTheme.colorTextMain, currentTheme.colorCardBg);
    
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

