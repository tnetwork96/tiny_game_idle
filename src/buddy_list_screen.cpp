#include "buddy_list_screen.h"

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

BuddyListScreen::BuddyListScreen(Adafruit_ST7789* tft) {
    this->tft = tft;
    buddyCount = 0;
    selectedIndex = 0;
    scrollOffset = 0;
    notificationCount = 0;
    unreadCount = 0;
    currentTab = TAB_CHATS;
    isSidebarActive = false;  // Start with focus on list
    shouldShowNotifications = false;
    drawTime = 0;
    loadFriendsCallback = nullptr;
    loadNotificationsCallback = nullptr;
    userId = 0;
    notifications = nullptr;
    selectedNotificationIndex = 0;

    // Deep Space Arcade Theme - Midnight Blue
    // Background: Deep Midnight Blue #020817 (RGB: 2, 8, 23) -> RGB565: 0x0042
    bgColor = 0x0042;           // Deep Midnight Blue background
    // Header: Lighter Blue #0F172A (RGB: 15, 23, 42) -> RGB565: 0x08A5
    headerColor = 0x08A5;       // Lighter blue header
    headerTextColor = 0xFFFF;   // White text for header
    listTextColor = 0xFFFF;     // White text for names (crisp white)
    // Selection: Electric Blue tint #162845 (RGB: 22, 40, 69) -> RGB565: 0x1148
    highlightColor = 0x1148;    // Electric Blue tint for selection (Tron Glow)
    // Status dots: Minty Green #00FF99 for online, Bright Red #FF3333 for offline
    onlineColor = 0x07F3;       // Minty Neon Green #00FF99 (RGB: 0, 255, 153) -> RGB565: 0x07F3
    offlineColor = 0xF986;      // Bright Red #FF3333 (RGB: 255, 51, 51) -> RGB565: 0xF986
    // Avatar: Dark Slate Blue #1E293B (RGB: 30, 41, 59) -> RGB565: 0x1947
    avatarColor = 0x1947;       // Dark Slate Blue for avatar
    // Separator: Cyan #00FFFF (RGB: 0, 255, 255) -> RGB565: 0x07FF
    separatorColor = 0x07FF;     // Cyan separator lines
    // Action Button: Cyan #00FFFF (RGB: 0, 255, 255) -> RGB565: 0x07FF
    actionButtonColor = 0x07FF;  // Cyan for Add Friend button
}

void BuddyListScreen::setBuddies(const BuddyEntry* entries, uint8_t count) {
    buddyCount = min(count, MAX_BUDDIES);
    for (uint8_t i = 0; i < buddyCount; i++) {
        buddies[i] = entries[i];
    }
    // Reset to first buddy if index is invalid (but allow MAX_BUDDIES for header button)
    if (selectedIndex >= buddyCount && selectedIndex != MAX_BUDDIES) {
        selectedIndex = buddyCount > 0 ? buddyCount - 1 : 0;
    }
    ensureSelectionVisible();
}

void BuddyListScreen::updateStatus(uint8_t index, bool online) {
    if (index >= buddyCount) return;
    buddies[index].online = online;
    draw();
}

bool BuddyListScreen::addFriend(String name, bool online) {
    // Check if we've reached the maximum number of buddies
    if (buddyCount >= MAX_BUDDIES) {
        return false;  // Cannot add more friends
    }
    
    // Add the new friend to the end of the list
    buddies[buddyCount].name = name;
    buddies[buddyCount].online = online;
    buddyCount++;
    
    return true;
}

bool BuddyListScreen::removeFriend(uint8_t index) {
    if (index >= buddyCount) {
        return false;  // Invalid index
    }
    
    // Shift all buddies after the removed one to the left
    for (uint8_t i = index; i < buddyCount - 1; i++) {
        buddies[i] = buddies[i + 1];
    }
    buddyCount--;
    
    // Adjust selectedIndex if needed
    if (selectedIndex >= buddyCount && selectedIndex != MAX_BUDDIES) {
        selectedIndex = buddyCount > 0 ? buddyCount - 1 : 0;
    }
    
    // Ensure selection is visible
    ensureSelectionVisible();
    
    return true;
}

bool BuddyListScreen::removeFriendByName(const String& name) {
    // Find the friend by name
    for (uint8_t i = 0; i < buddyCount; i++) {
        if (buddies[i].name == name) {
            return removeFriend(i);
        }
    }
    return false;  // Friend not found
}

void BuddyListScreen::parseFriendsString(const String& friendsString) {
    if (friendsString.length() == 0) {
        Serial.println("BuddyListScreen: Empty friends string, setting empty list");
        setBuddies(nullptr, 0);
        return;
    }
    
    Serial.print("BuddyListScreen: Parsing friends string: ");
    Serial.println(friendsString);
    Serial.print("BuddyListScreen: String length: ");
    Serial.println(friendsString.length());
    
    // Count friends (by counting | separators)
    int count = 0;
    for (int i = 0; i < friendsString.length(); i++) {
        if (friendsString.charAt(i) == '|') {
            count++;
        }
    }
    
    Serial.print("BuddyListScreen: Found ");
    Serial.print(count);
    Serial.println(" friends (by | count)");
    
    if (count == 0) {
        setBuddies(nullptr, 0);
        return;
    }
    
    // Allocate buddies array
    BuddyEntry* buddies = new BuddyEntry[count];
    int buddyIdx = 0;
    
    // Parse each friend entry: "username,online|"
    int pos = 0;
    while (pos < friendsString.length() && buddyIdx < count) {
        // Find next | separator
        int pipePos = friendsString.indexOf('|', pos);
        if (pipePos < 0) break;
        
        // Extract entry: "username,online"
        String entry = friendsString.substring(pos, pipePos);
        entry.trim();
        
        if (entry.length() > 0) {
            // Find comma separator
            int commaPos = entry.indexOf(',');
            if (commaPos > 0) {
                // Extract username
                buddies[buddyIdx].name = entry.substring(0, commaPos);
                buddies[buddyIdx].name.trim();
                
                // Extract online status
                String onlineStr = entry.substring(commaPos + 1);
                onlineStr.trim();
                buddies[buddyIdx].online = (onlineStr == "1");
                
                Serial.print("BuddyListScreen: Parsed friend ");
                Serial.print(buddyIdx);
                Serial.print(": ");
                Serial.print(buddies[buddyIdx].name);
                Serial.print(" (online: ");
                Serial.print(buddies[buddyIdx].online ? "1" : "0");
                Serial.println(")");
                
                buddyIdx++;
            }
        }
        
        pos = pipePos + 1;
    }
    
    Serial.print("BuddyListScreen: Successfully parsed ");
    Serial.print(buddyIdx);
    Serial.println(" friends");
    
    // Set buddies
    setBuddies(buddies, buddyIdx);
    delete[] buddies;
}

uint8_t BuddyListScreen::getVisibleRows() const {
    const uint16_t rowHeight = 24;  // Updated to 24px as per requirements
    // Available space = full screen height (no header in sidebar layout)
    uint16_t available = SCREEN_HEIGHT;
    uint8_t rows = available / rowHeight;
    return rows < 1 ? 1 : rows;
}

void BuddyListScreen::ensureSelectionVisible() {
    // If sidebar is active, no need to scroll
    if (isSidebarActive) {
        return;
    }
    
    uint8_t visible = getVisibleRows();
    uint8_t currentIndex = shouldShowNotifications ? selectedNotificationIndex : selectedIndex;
    uint8_t currentCount = shouldShowNotifications ? notificationCount : buddyCount;
    
    if (currentIndex < scrollOffset) {
        scrollOffset = currentIndex;
    } else if (currentIndex >= scrollOffset + visible) {
        scrollOffset = currentIndex - visible + 1;
    }
}

void BuddyListScreen::drawSidebar() {
    if (tft == nullptr) return;
    
    // Draw sidebar background
    tft->fillRect(0, 0, SIDEBAR_WIDTH, SCREEN_HEIGHT, headerColor);
    
    // Color constants - Strict coloring rules
    const uint16_t ICON_ACTIVE_COLOR = 0x07FF;    // Cyan - Active Tab
    const uint16_t ICON_INACTIVE_COLOR = 0x8410;  // Dark Grey - Inactive Tab
    const uint16_t FOCUS_BORDER_COLOR = 0xFFFF;   // White - Selection Box
    
    // Icon size constant
    const uint16_t iconSize = 16;
    
    // Position constants - Shared Y-coordinates for perfect alignment
    const uint16_t POS_X = 12;  // Center icon horizontally in 40px width: (40 - 16) / 2 = 12
    const uint16_t POS_Y_TAB0 = 30;  // User List (Tab 0)
    const uint16_t POS_Y_TAB1 = 80;  // Notifications (Tab 1)
    const uint16_t POS_Y_TAB2 = 130; // Add Friend (Tab 2)
    
    // Draw Tab 0 (User List)
    uint16_t tab0Color = (currentTab == TAB_CHATS) ? ICON_ACTIVE_COLOR : ICON_INACTIVE_COLOR;
    tft->drawBitmap(POS_X, POS_Y_TAB0, iconUserList, iconSize, iconSize, tab0Color);
    // Selection border if selected & active
    if (isSidebarActive && currentTab == TAB_CHATS) {
        tft->drawRect(POS_X - 4, POS_Y_TAB0 - 4, 24, 24, FOCUS_BORDER_COLOR);
    }
    
    // Draw Tab 1 (Bell/Notifications)
    uint16_t tab1Color = (currentTab == TAB_NOTIFICATIONS) ? ICON_ACTIVE_COLOR : ICON_INACTIVE_COLOR;
    tft->drawBitmap(POS_X, POS_Y_TAB1, iconBell, iconSize, iconSize, tab1Color);
    // Selection border if selected & active
    if (isSidebarActive && currentTab == TAB_NOTIFICATIONS) {
        tft->drawRect(POS_X - 4, POS_Y_TAB1 - 4, 24, 24, FOCUS_BORDER_COLOR);
    }
    // Red Dot Fix: Perfectly aligned with top-right of Bell icon
    if (unreadCount > 0) {
        const uint16_t badgeX = POS_X + 14;  // Top-right corner: 12 + 14 = 26 (right edge of 16px icon)
        const uint16_t badgeY = POS_Y_TAB1 + 2;  // Slightly below top edge
        const uint16_t badgeRadius = 2;  // Small dot
        const uint16_t badgeColor = 0xF800;  // Red
        tft->fillCircle(badgeX, badgeY, badgeRadius, badgeColor);
    }
    
    // Draw Tab 2 (Plus/Add Friend)
    uint16_t tab2Color = (currentTab == TAB_ADD_FRIEND) ? ICON_ACTIVE_COLOR : ICON_INACTIVE_COLOR;
    tft->drawBitmap(POS_X, POS_Y_TAB2, iconPlus, iconSize, iconSize, tab2Color);
    // Selection border if selected & active
    if (isSidebarActive && currentTab == TAB_ADD_FRIEND) {
        tft->drawRect(POS_X - 4, POS_Y_TAB2 - 4, 24, 24, FOCUS_BORDER_COLOR);
    }
}

void BuddyListScreen::drawHeader() {
    if (tft == nullptr) return;
    const uint16_t headerHeight = 30;
    tft->fillRect(0, 0, 320, headerHeight, headerColor);

    // Draw "CHATS" title on the left
    tft->setTextColor(headerTextColor, headerColor);
    tft->setTextSize(2);
    tft->setCursor(10, 8);
    tft->print("CHATS");

    // Draw Bell icon (notification icon) at x=80, y=7
    const uint16_t bellX = 80;
    const uint16_t bellY = 7;
    const uint16_t bellColor = 0x07FF;  // Cyan/Neon Blue
    tft->drawBitmap(bellX, bellY, iconBell, 16, 16, bellColor);

    // Draw Badge (if unreadCount > 0)
    if (unreadCount > 0) {
        const uint16_t badgeX = 92;  // Top-right corner of bell (80 + 16 - 4)
        const uint16_t badgeY = 7;
        const uint16_t badgeRadius = 5;
        const uint16_t badgeColor = 0xF800;  // Red
        
        // Draw filled red circle
        tft->fillCircle(badgeX, badgeY, badgeRadius, badgeColor);
        
        // Draw number text centered in the circle
        tft->setTextColor(0xFFFF, badgeColor);  // White text on red background
        tft->setTextSize(1);
        
        String countStr;
        if (unreadCount > 99) {
            countStr = "99+";
        } else {
            countStr = String(unreadCount);
        }
        
        // Center text in circle
        uint16_t textWidth = countStr.length() * 6;  // Approximate width per character
        uint16_t textX = badgeX - textWidth / 2;
        uint16_t textY = badgeY - 4;  // Vertically center (text size 1 is ~8px tall)
        tft->setCursor(textX, textY);
        tft->print(countStr);
    }

    // Draw "Add Friend" button on the right
    bool isSelected = isHeaderButtonSelected();
    uint16_t buttonBg = isSelected ? highlightColor : headerColor;
    
    // Button area (right side of header)
    const uint16_t buttonWidth = 120;
    const uint16_t buttonX = 320 - buttonWidth - 5;  // 5px margin from right
    const uint16_t buttonY = 5;
    const uint16_t buttonHeight = headerHeight - 10;  // 5px margin top and bottom
    
    // Draw button background (always draw, but different color when selected)
    tft->fillRect(buttonX, buttonY, buttonWidth, buttonHeight, buttonBg);
    
    // Draw Cyan border when selected (glow effect for better visibility)
    if (isSelected) {
        const uint16_t borderWidth = 2;
        // Top border
        tft->drawFastHLine(buttonX, buttonY, buttonWidth, separatorColor);
        // Bottom border
        tft->drawFastHLine(buttonX, buttonY + buttonHeight - 1, buttonWidth, separatorColor);
        // Left border
        tft->drawFastVLine(buttonX, buttonY, buttonHeight, separatorColor);
        // Right border
        tft->drawFastVLine(buttonX + buttonWidth - 1, buttonY, buttonHeight, separatorColor);
    }
    
    // Draw [+] icon (always Cyan for visibility)
    const uint16_t iconSize = 14;
    const uint16_t iconX = buttonX + 5;
    const uint16_t iconY = buttonY + (buttonHeight - iconSize) / 2;
    uint16_t iconColor = separatorColor;  // Always Cyan (#00FFFF) for visibility
    drawAddFriendIcon(iconX, iconY, iconSize, iconColor);
    
    // Draw "Add Friend" text (always Cyan for visibility)
    tft->setTextColor(iconColor, buttonBg);
    tft->setTextSize(1);
    uint16_t textX = iconX + iconSize + 4;
    uint16_t textY = buttonY + (buttonHeight - 8) / 2;  // Text size 1 is ~8px tall
    tft->setCursor(textX, textY);
    tft->print("Add Friend");
    
    // Draw separator line below header (Cyan or Dark Grey)
    tft->drawFastHLine(0, headerHeight - 1, 320, separatorColor);
}

uint16_t BuddyListScreen::statusColor(bool online) const {
    return online ? onlineColor : offlineColor;
}

void BuddyListScreen::drawBuddyRow(uint8_t visibleRow, uint8_t buddyIdx) {
    if (tft == nullptr || buddyIdx >= buddyCount) return;

    const uint16_t rowHeight = 24;  // Updated to 24px
    const uint16_t startX = SIDEBAR_WIDTH;  // Start after sidebar
    const uint16_t y = visibleRow * rowHeight;  // No header offset

    bool isSelected = (buddyIdx == selectedIndex && !isSidebarActive);
    uint16_t rowBg = isSelected ? highlightColor : bgColor;

    tft->fillRect(startX, y, CONTENT_WIDTH, rowHeight, rowBg);
    
    // Draw Cyan accent bar on left edge for selected row (Neon Glow effect)
    if (isSelected) {
        const uint16_t accentBarWidth = 2;  // Thin vertical bar
        const uint16_t accentBarColor = 0x07FF;  // Cyan #00FFFF (RGB: 0, 255, 255)
        tft->fillRect(startX, y, accentBarWidth, rowHeight, accentBarColor);
    }

    // Avatar: 20x20px circle, Dark Slate Blue color, no text
    const uint16_t avatarSize = 20;
    const uint16_t avatarX = startX + 2;  // Left margin (after sidebar)
    const uint16_t avatarY = y + (rowHeight - avatarSize) / 2;  // Vertically centered
    const uint16_t avatarCenterX = avatarX + avatarSize / 2;
    const uint16_t avatarCenterY = avatarY + avatarSize / 2;
    const uint16_t avatarRadius = avatarSize / 2;
    
    // Draw avatar circle (Dark Slate Blue)
    tft->fillCircle(avatarCenterX, avatarCenterY, avatarRadius, avatarColor);
    // Border: Deep Midnight Blue to match background (blends cleanly)
    tft->drawCircle(avatarCenterX, avatarCenterY, avatarRadius, bgColor);

    // Status dot: 4-5px maximum (using 4px diameter = 2px radius for subtlety)
    // Position: Bottom-right corner of avatar
    const uint16_t dotRadius = 2;  // 4px diameter (20-25% of 20px avatar)
    const uint16_t dotOffsetX = 6;  // Offset from avatar center to bottom-right
    const uint16_t dotOffsetY = 6;
    const uint16_t dotCenterX = avatarCenterX + dotOffsetX;
    const uint16_t dotCenterY = avatarCenterY + dotOffsetY;
    
    // Draw status dot with deep midnight blue stroke (1px border) to cut cleanly into avatar
    uint16_t statusDotColor = statusColor(buddies[buddyIdx].online);
    
    // Draw deep midnight blue stroke (outer circle) - matches background
    tft->fillCircle(dotCenterX, dotCenterY, dotRadius + 1, bgColor);
    // Draw status dot (inner circle) - Minty Green or Bright Red
    tft->fillCircle(dotCenterX, dotCenterY, dotRadius, statusDotColor);

    // Name: White text, vertically centered
    tft->setTextColor(listTextColor, rowBg);
    tft->setTextSize(2);
    uint16_t textX = avatarX + avatarSize + 4;  // 4px spacing after avatar
    uint16_t textY = y + (rowHeight - 16) / 2;  // Vertically centered (text size 2 is ~16px tall)
    tft->setCursor(textX, textY);
    String displayName = buddies[buddyIdx].name;
    const uint8_t charWidth = 12;  // Approx for text size 2
    const uint16_t rightMargin = 4;
    uint8_t maxChars = (CONTENT_WIDTH - (textX - startX) - rightMargin) / charWidth;
    if (displayName.length() > maxChars && maxChars > 3) {
        displayName = displayName.substring(0, maxChars - 3) + "...";
    }
    tft->print(displayName);
}

void BuddyListScreen::drawNotificationsList(bool clearBackground) {
    if (tft == nullptr) return;
    
    const uint16_t rowHeight = 24;
    const uint16_t startX = SIDEBAR_WIDTH;
    const uint16_t startY = 0;
    const uint8_t visibleRows = getVisibleRows();
    const uint16_t listWidth = CONTENT_WIDTH;
    
    if (clearBackground) {
        tft->fillRect(startX, startY, listWidth, SCREEN_HEIGHT, bgColor);
    }
    
    if (notificationCount == 0 || notifications == nullptr) {
        // Empty state: Show "No notifications" message
        tft->setTextColor(listTextColor, bgColor);
        tft->setTextSize(2);
        String emptyMsg = "No notifications";
        uint16_t textX = startX + (listWidth - emptyMsg.length() * 12) / 2;
        uint16_t textY = SCREEN_HEIGHT / 2 - 8;
        tft->setCursor(textX, textY);
        tft->print(emptyMsg);
        return;
    }
    
    // Draw all visible notification rows
    for (uint8_t row = 0; row < visibleRows; row++) {
        uint8_t notificationIdx = scrollOffset + row;
        if (notificationIdx >= notificationCount) {
            if (clearBackground) {
                tft->fillRect(startX, startY + row * rowHeight, listWidth, 
                             (visibleRows - row) * rowHeight, bgColor);
            }
            break;
        }
        drawNotificationRow(row, notificationIdx);
    }
    
    // Draw scrollbar
    drawScrollbar();
}

void BuddyListScreen::drawNotificationRow(uint8_t visibleRow, uint8_t notificationIdx) {
    if (tft == nullptr || notifications == nullptr || notificationIdx >= notificationCount) return;
    
    const uint16_t rowHeight = 24;
    const uint16_t startX = SIDEBAR_WIDTH;
    const uint16_t y = visibleRow * rowHeight;
    
    bool isSelected = (notificationIdx == selectedNotificationIndex && !isSidebarActive);
    uint16_t rowBg = isSelected ? highlightColor : bgColor;
    
    tft->fillRect(startX, y, CONTENT_WIDTH, rowHeight, rowBg);
    
    if (isSelected) {
        const uint16_t accentBarWidth = 2;
        const uint16_t accentBarColor = 0x07FF;
        tft->fillRect(startX, y, accentBarWidth, rowHeight, accentBarColor);
    }
    
    // Draw notification icon (bell) - small icon on left
    const uint16_t iconSize = 16;
    const uint16_t iconX = startX + 2;
    const uint16_t iconY = y + (rowHeight - iconSize) / 2;
    uint16_t iconColor = notifications[notificationIdx].read ? 0x8410 : 0x07FF;  // Grey if read, Cyan if unread
    tft->drawBitmap(iconX, iconY, iconBell, iconSize, iconSize, iconColor);
    
    // Draw notification message
    tft->setTextColor(listTextColor, rowBg);
    tft->setTextSize(1);  // Smaller text to fit more
    uint16_t textX = iconX + iconSize + 4;
    uint16_t textY = y + (rowHeight - 8) / 2;
    tft->setCursor(textX, textY);
    
    String displayMsg = notifications[notificationIdx].message;
    const uint8_t charWidth = 6;  // Approx for text size 1
    const uint16_t rightMargin = 4;
    uint8_t maxChars = (CONTENT_WIDTH - (textX - startX) - rightMargin) / charWidth;
    if (displayMsg.length() > maxChars && maxChars > 3) {
        displayMsg = displayMsg.substring(0, maxChars - 3) + "...";
    }
    tft->print(displayMsg);
}

void BuddyListScreen::drawScrollbar() {
    if (tft == nullptr) return;
    
    const uint16_t startY = 0;  // No header in sidebar layout
    const uint16_t startX = SIDEBAR_WIDTH + CONTENT_WIDTH;  // Right edge of content area (320px)
    const uint8_t visibleRows = getVisibleRows();
    
    // Get current count based on view mode
    uint8_t currentCount = shouldShowNotifications ? notificationCount : buddyCount;
    
    // Only draw scrollbar if there are more items than visible rows
    if (currentCount <= visibleRows) {
        // Clear scrollbar area if it was previously visible
        uint16_t barWidth = 4;
        uint16_t barX = startX;
        uint16_t scrollbarHeight = SCREEN_HEIGHT;
        tft->fillRect(barX, startY, barWidth, scrollbarHeight, bgColor);
        return;
    }
    
    uint16_t barWidth = 4;
    uint16_t barX = startX;
    uint16_t scrollbarHeight = SCREEN_HEIGHT;  // Full height
    
    // Dark track (darker than background) - use slightly darker midnight blue
    tft->fillRect(barX, startY, barWidth, scrollbarHeight, 0x0021);  // Very dark midnight blue track

    float ratio = (float)visibleRows / (float)currentCount;
    uint16_t thumbHeight = scrollbarHeight * ratio;
    if (thumbHeight < 8) thumbHeight = 8;

    float scrollPercent = (float)scrollOffset / (float)(currentCount - visibleRows);
    if (scrollPercent < 0.0f) scrollPercent = 0.0f;
    if (scrollPercent > 1.0f) scrollPercent = 1.0f;
    uint16_t thumbY = startY + (scrollbarHeight - thumbHeight) * scrollPercent;

    // Cyan thumb for scrollbar (Tron-like accent)
    tft->fillRect(barX, thumbY, barWidth, thumbHeight, 0x07FF);  // Cyan #00FFFF
}

void BuddyListScreen::drawList(bool clearBackground) {
    if (tft == nullptr) return;

    const uint16_t rowHeight = 24;  // Updated to 24px
    const uint16_t startX = SIDEBAR_WIDTH;  // Start after sidebar
    const uint16_t startY = 0;  // Start at top (no header)
    const uint8_t visibleRows = getVisibleRows();
    const uint16_t listHeight = visibleRows * rowHeight;
    const uint16_t listWidth = CONTENT_WIDTH;  // 280px width

    // Only clear list area if explicitly requested (for initial draw or scroll changes)
    if (clearBackground) {
        tft->fillRect(startX, startY, listWidth, SCREEN_HEIGHT, bgColor);
    }

    // Draw all visible rows
    for (uint8_t row = 0; row < visibleRows; row++) {
        uint8_t buddyIdx = scrollOffset + row;
        if (buddyIdx >= buddyCount) {
            // Clear remaining rows if there are fewer buddies than visible rows
            if (clearBackground) {
                tft->fillRect(startX, startY + row * rowHeight, listWidth, (visibleRows - row) * rowHeight, bgColor);
            }
            break;
        }
        drawBuddyRow(row, buddyIdx);
    }

    // Draw scrollbar (always redraw when list is redrawn)
    drawScrollbar();
}

bool BuddyListScreen::isHeaderButtonSelected() const {
    // Header button is selected when selectedIndex equals MAX_BUDDIES (special value)
    return (selectedIndex == MAX_BUDDIES);
}

void BuddyListScreen::drawAddFriendIcon(uint16_t x, uint16_t y, uint16_t size, uint16_t color) {
    // Draw a simple [+] icon (plus sign)
    // Center of icon
    uint16_t centerX = x + size / 2;
    uint16_t centerY = y + size / 2;
    uint16_t lineWidth = 2;  // Thickness of plus lines
    uint16_t lineLength = size / 2 - 2;  // Length of plus lines
    
    // Horizontal line
    tft->fillRect(centerX - lineLength, centerY - lineWidth / 2, lineLength * 2, lineWidth, color);
    // Vertical line
    tft->fillRect(centerX - lineWidth / 2, centerY - lineLength, lineWidth, lineLength * 2, color);
}

void BuddyListScreen::setNotificationCount(uint8_t count) {
    notificationCount = count;
    unreadCount = count;  // Keep unreadCount in sync with notificationCount
    Serial.print("Buddy List Screen: Notification count set to: ");
    Serial.println(count);
    // Redraw header to update badge
    drawHeader();
}

void BuddyListScreen::setUnreadCount(uint8_t count) {
    unreadCount = count;
    Serial.print("Buddy List Screen: Unread count set to: ");
    Serial.println(count);
    // Redraw sidebar to update badge
    drawSidebar();
}

void BuddyListScreen::setLoadFriendsCallback(LoadFriendsCallback callback) {
    loadFriendsCallback = callback;
}

void BuddyListScreen::setLoadNotificationsCallback(LoadNotificationsCallback callback) {
    loadNotificationsCallback = callback;
}

void BuddyListScreen::setUserId(int userId) {
    this->userId = userId;
}

void BuddyListScreen::setNotifications(const ApiClient::NotificationEntry* entries, uint8_t count) {
    // Free old notifications if any
    if (notifications != nullptr) {
        delete[] notifications;
        notifications = nullptr;
    }
    
    notificationCount = min(count, (uint8_t)50);  // Max 50 notifications
    if (notificationCount > 0 && entries != nullptr) {
        notifications = new ApiClient::NotificationEntry[notificationCount];
        for (uint8_t i = 0; i < notificationCount; i++) {
            notifications[i] = entries[i];
        }
    } else {
        notifications = nullptr;
        notificationCount = 0;
    }
    
    selectedNotificationIndex = 0;
    scrollOffset = 0;
    
    // Redraw notifications list if we're currently showing notifications
    if (shouldShowNotifications) {
        drawNotificationsList(true);
    }
}


void BuddyListScreen::draw() {
    if (tft == nullptr) return;
    tft->fillScreen(bgColor);
    
    // Ensure currentTab matches the current screen view
    // When drawing buddy list, always default to TAB_CHATS
    // This ensures the User List icon is always highlighted when showing the buddy list
    currentTab = TAB_CHATS;
    shouldShowNotifications = false;  // Default to buddy list
    
    drawSidebar();   // Draw vertical sidebar on the left
    drawList(true);  // Full draw with background clear for initial screen entry
    drawTime = millis();  // Record draw time for auto-navigation
}

void BuddyListScreen::handleUp() {
    if (isSidebarActive) {
        // Cycle through sidebar tabs (up = previous tab)
        if (currentTab == TAB_CHATS) {
            currentTab = TAB_ADD_FRIEND;  // Wrap to bottom
        } else if (currentTab == TAB_NOTIFICATIONS) {
            currentTab = TAB_CHATS;
        } else {  // TAB_ADD_FRIEND
            currentTab = TAB_NOTIFICATIONS;
        }
        // Change currentTab and immediately drawSidebar() to update the Cyan color
        drawSidebar();  // Redraw sidebar to update Cyan color for new active tab
    } else {
        // Normal navigation: move up in list
        // Icon remains Cyan, but no border
        if (selectedIndex == 0) return;  // Can't go up from first buddy
        
        uint8_t prevIndex = selectedIndex;
        uint8_t prevScroll = scrollOffset;
        selectedIndex--;
        ensureSelectionVisible();
        redrawSelectionChange(prevIndex, prevScroll);
    }
}

void BuddyListScreen::handleDown() {
    if (isSidebarActive) {
        SidebarTab prevTab = currentTab;
        
        // Cycle through sidebar tabs (down = next tab)
        if (currentTab == TAB_CHATS) {
            currentTab = TAB_NOTIFICATIONS;
        } else if (currentTab == TAB_NOTIFICATIONS) {
            currentTab = TAB_ADD_FRIEND;
        } else {  // TAB_ADD_FRIEND
            currentTab = TAB_CHATS;  // Wrap to top
        }
        
        // If switching to a list tab, trigger load
        if (currentTab == TAB_CHATS && prevTab != TAB_CHATS) {
            shouldShowNotifications = false;
            if (loadFriendsCallback != nullptr && userId > 0) {
                Serial.println("Buddy List: Switching to Chats - loading friends...");
                loadFriendsCallback(userId);
            }
            drawList(true);
        } else if (currentTab == TAB_NOTIFICATIONS && prevTab != TAB_NOTIFICATIONS) {
            shouldShowNotifications = true;
            if (loadNotificationsCallback != nullptr && userId > 0) {
                Serial.println("Buddy List: Switching to Notifications - loading notifications...");
                loadNotificationsCallback(userId);
            }
            drawNotificationsList(true);
        }
        
        drawSidebar();
    } else {
        // Normal navigation: move down in list
        if (shouldShowNotifications) {
            if (selectedNotificationIndex + 1 >= notificationCount) return;
            uint8_t prevIndex = selectedNotificationIndex;
            uint8_t prevScroll = scrollOffset;
            selectedNotificationIndex++;
            ensureSelectionVisible();
            redrawSelectionChange(prevIndex, prevScroll);
        } else {
            if (selectedIndex + 1 >= buddyCount) return;
            uint8_t prevIndex = selectedIndex;
            uint8_t prevScroll = scrollOffset;
            selectedIndex++;
            ensureSelectionVisible();
            redrawSelectionChange(prevIndex, prevScroll);
        }
    }
}

void BuddyListScreen::handleLeft() {
    // If in List: Move focus to Sidebar
    if (!isSidebarActive) {
        isSidebarActive = true;
        // Redraw Sidebar (Border appears on the active Cyan icon)
        drawSidebar();  // Redraw sidebar - Border appears on active Cyan icon
        drawList(false);  // Redraw list to remove selection highlight
    }
    // If already in Sidebar: Do nothing (already at edge)
}

void BuddyListScreen::handleRight() {
    // If in Sidebar: Move focus to List
    if (isSidebarActive) {
        isSidebarActive = false;
        // When moving focus to list, ensure we're on the correct tab (TAB_CHATS for buddy list)
        currentTab = TAB_CHATS;
        // Redraw Sidebar (Border disappears, Icon stays Cyan)
        drawSidebar();  // Redraw sidebar - Border disappears, Icon stays Cyan
        drawList(false);  // Redraw list to show selection highlight
    }
    // If already in List: Do nothing (already at edge)
}

void BuddyListScreen::handleSelect() {
    if (isSidebarActive) {
        // Execute action for current tab
        if (currentTab == TAB_ADD_FRIEND) {
            // Signal that Add Friend screen should be shown
            // The actual screen display is handled in main.cpp
            // (main.cpp will check shouldShowAddFriendScreen())
            Serial.println("Buddy List: Add Friend tab selected");
        } else if (currentTab == TAB_NOTIFICATIONS) {
            Serial.println("Buddy List: Notifications tab selected - loading notifications...");
            shouldShowNotifications = true;
            
            // Call API to load notifications
            if (loadNotificationsCallback != nullptr && userId > 0) {
                loadNotificationsCallback(userId);
            }
            
            // Redraw to show notifications list (or empty state)
            drawSidebar();
            drawNotificationsList(true);
        } else {  // TAB_CHATS
            Serial.println("Buddy List: Chats tab selected - loading friends...");
            shouldShowNotifications = false;
            
            // Call API to reload friends
            if (loadFriendsCallback != nullptr && userId > 0) {
                loadFriendsCallback(userId);
            }
            
            // Redraw to show buddy list
            drawSidebar();
            drawList(true);
        }
    } else {
        // List is active: Select the buddy or notification
        if (shouldShowNotifications) {
            // Handle notification selection
            Serial.print("Notification selected: ");
            Serial.println(selectedNotificationIndex);
        } else {
            // Handle buddy selection (existing logic)
        }
    }
}

bool BuddyListScreen::shouldShowAddFriendScreen() const {
    return isSidebarActive && currentTab == TAB_ADD_FRIEND;
}

void BuddyListScreen::handleSelectRandom() {
    if (buddyCount == 0) {
        // If no buddies, activate sidebar and select Add Friend tab
        if (!isSidebarActive || currentTab != TAB_ADD_FRIEND) {
            isSidebarActive = true;
            currentTab = TAB_ADD_FRIEND;
            drawSidebar();
            drawList(false);
        }
        return;
    }
    // Move focus to list if in sidebar
    if (isSidebarActive) {
        isSidebarActive = false;
        drawSidebar();
    }
    uint8_t prevIndex = selectedIndex;
    uint8_t prevScroll = scrollOffset;
    uint8_t idx = random(0, buddyCount);
    selectedIndex = idx;
    ensureSelectionVisible();
    redrawSelectionChange(prevIndex, prevScroll);
}

BuddyEntry BuddyListScreen::getSelectedBuddy() const {
    BuddyEntry empty = { "", false };
    // If sidebar is active or no buddies, return empty
    if (buddyCount == 0 || isSidebarActive) return empty;
    if (selectedIndex >= buddyCount) return empty;
    return buddies[selectedIndex];
}

void BuddyListScreen::redrawSelectionChange(uint8_t previousIndex, uint8_t previousScroll) {
    // Handle sidebar selection changes - redraw sidebar if needed
    // (Sidebar redraw is handled in navigation methods, so we don't need to do it here)
    // Just redraw the list to update selection highlight

    // If scroll offset changed, need full redraw
    if (previousScroll != scrollOffset) {
        drawList(true);  // Full redraw with background clear
        return;
    }

    // Normal selection change without scrolling - only redraw affected rows
    // Only redraw if list is active (not sidebar)
    if (isSidebarActive) {
        return;  // Don't redraw list rows when sidebar is active
    }

    uint8_t visibleRows = getVisibleRows();

    // Redraw previous selected row to remove highlight (only if it was a buddy)
    if (previousIndex < buddyCount &&
        previousIndex >= scrollOffset &&
        previousIndex < scrollOffset + visibleRows) {
        uint8_t row = previousIndex - scrollOffset;
        drawBuddyRow(row, previousIndex);
    }

    // Redraw new selected row to show highlight (only if it's a buddy)
    if (selectedIndex < buddyCount &&
        selectedIndex >= scrollOffset &&
        selectedIndex < scrollOffset + visibleRows) {
        uint8_t row = selectedIndex - scrollOffset;
        drawBuddyRow(row, selectedIndex);
    }
    
    // Note: Scrollbar doesn't need redraw during normal selection changes
    // (it only changes when scrollOffset changes, which triggers full drawList())
}

// Auto-navigation methods (for demo/testing)
void BuddyListScreen::triggerNavigateUp() {
    handleUp();
}

void BuddyListScreen::triggerNavigateDown() {
    handleDown();
}

void BuddyListScreen::triggerNavigateLeft() {
    handleLeft();
}

void BuddyListScreen::triggerNavigateRight() {
    handleRight();
}

void BuddyListScreen::triggerSelect() {
    handleSelect();
}

unsigned long BuddyListScreen::getDrawTime() const {
    return drawTime;
}

