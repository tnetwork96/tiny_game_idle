#include "buddy_list_screen.h"

BuddyListScreen::BuddyListScreen(Adafruit_ST7789* tft) {
    this->tft = tft;
    buddyCount = 0;
    selectedIndex = 0;
    scrollOffset = 0;

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

uint8_t BuddyListScreen::getVisibleRows() const {
    const uint16_t headerHeight = 30;
    const uint16_t rowHeight = 24;  // Updated to 24px as per requirements
    // Available space = screen height - header (no footer)
    uint16_t available = (240 > headerHeight) ? (240 - headerHeight) : 0;
    uint8_t rows = available / rowHeight;
    return rows < 1 ? 1 : rows;
}

void BuddyListScreen::ensureSelectionVisible() {
    // If header button is selected, no need to scroll
    if (isHeaderButtonSelected()) {
        return;
    }
    
    uint8_t visible = getVisibleRows();
    if (selectedIndex < scrollOffset) {
        scrollOffset = selectedIndex;
    } else if (selectedIndex >= scrollOffset + visible) {
        scrollOffset = selectedIndex - visible + 1;
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

    // Draw "Add Friend" button on the right
    bool isSelected = isHeaderButtonSelected();
    uint16_t buttonBg = isSelected ? highlightColor : headerColor;
    
    // Button area (right side of header)
    const uint16_t buttonWidth = 120;
    const uint16_t buttonX = 320 - buttonWidth - 5;  // 5px margin from right
    const uint16_t buttonY = 5;
    const uint16_t buttonHeight = headerHeight - 10;  // 5px margin top and bottom
    
    // Draw button background if selected
    if (isSelected) {
        tft->fillRect(buttonX, buttonY, buttonWidth, buttonHeight, buttonBg);
    }
    
    // Draw [+] icon
    const uint16_t iconSize = 14;
    const uint16_t iconX = buttonX + 5;
    const uint16_t iconY = buttonY + (buttonHeight - iconSize) / 2;
    uint16_t iconColor = isSelected ? separatorColor : actionButtonColor;  // Cyan
    drawAddFriendIcon(iconX, iconY, iconSize, iconColor);
    
    // Draw "Add Friend" text
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

    const uint16_t headerHeight = 30;
    const uint16_t rowHeight = 24;  // Updated to 24px
    const uint16_t y = headerHeight + visibleRow * rowHeight;

    bool isSelected = (buddyIdx == selectedIndex);
    uint16_t rowBg = isSelected ? highlightColor : bgColor;

    tft->fillRect(0, y, 320, rowHeight, rowBg);
    
    // Draw Cyan accent bar on left edge for selected row (Neon Glow effect)
    if (isSelected) {
        const uint16_t accentBarWidth = 2;  // Thin vertical bar
        const uint16_t accentBarColor = 0x07FF;  // Cyan #00FFFF (RGB: 0, 255, 255)
        tft->fillRect(0, y, accentBarWidth, rowHeight, accentBarColor);
    }

    // Avatar: 20x20px circle, Dark Slate Blue color, no text
    const uint16_t avatarSize = 20;
    const uint16_t avatarX = 2;  // Left margin
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
    uint8_t maxChars = (320 - textX - rightMargin) / charWidth;
    if (displayName.length() > maxChars && maxChars > 3) {
        displayName = displayName.substring(0, maxChars - 3) + "...";
    }
    tft->print(displayName);
}

void BuddyListScreen::drawList() {
    if (tft == nullptr) return;

    const uint16_t headerHeight = 30;
    const uint16_t rowHeight = 24;  // Updated to 24px
    const uint16_t startY = headerHeight;
    const uint8_t visibleRows = getVisibleRows();
    const uint16_t listHeight = visibleRows * rowHeight;

    // Clear list area (from header to bottom)
    tft->fillRect(0, startY, 320, 240 - startY, bgColor);

    for (uint8_t row = 0; row < visibleRows; row++) {
        uint8_t buddyIdx = scrollOffset + row;
        if (buddyIdx >= buddyCount) break;
        drawBuddyRow(row, buddyIdx);
    }

    // Simple scrollbar on the right (Deep Space theme)
    if (buddyCount > visibleRows) {
        uint16_t barWidth = 4;
        uint16_t barX = 320 - barWidth;
        uint16_t scrollbarHeight = 240 - startY;  // Full height from header to bottom
        // Dark track (darker than background) - use slightly darker midnight blue
        tft->fillRect(barX, startY, barWidth, scrollbarHeight, 0x0021);  // Very dark midnight blue track

        float ratio = (float)visibleRows / (float)buddyCount;
        uint16_t thumbHeight = scrollbarHeight * ratio;
        if (thumbHeight < 8) thumbHeight = 8;

        float scrollPercent = (float)scrollOffset / (float)(buddyCount - visibleRows);
        if (scrollPercent < 0.0f) scrollPercent = 0.0f;
        if (scrollPercent > 1.0f) scrollPercent = 1.0f;
        uint16_t thumbY = startY + (scrollbarHeight - thumbHeight) * scrollPercent;

        // Cyan thumb for scrollbar (Tron-like accent)
        tft->fillRect(barX, thumbY, barWidth, thumbHeight, 0x07FF);  // Cyan #00FFFF
    }
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


void BuddyListScreen::draw() {
    if (tft == nullptr) return;
    tft->fillScreen(bgColor);
    drawHeader();
    drawList();
}

void BuddyListScreen::handleUp() {
    // If header button is selected, can't go up further
    if (isHeaderButtonSelected()) return;
    
    // If at first buddy (index 0), move to header button
    if (selectedIndex == 0) {
        uint8_t prevIndex = selectedIndex;
        uint8_t prevScroll = scrollOffset;
        selectedIndex = MAX_BUDDIES;  // Special value for header button
        redrawSelectionChange(prevIndex, prevScroll);
        drawHeader();  // Redraw header to show selection
        return;
    }
    
    // Normal navigation: move up in list
    uint8_t prevIndex = selectedIndex;
    uint8_t prevScroll = scrollOffset;
    selectedIndex--;
    ensureSelectionVisible();
    redrawSelectionChange(prevIndex, prevScroll);
}

void BuddyListScreen::handleDown() {
    // If header button is selected, move to first buddy
    if (isHeaderButtonSelected()) {
        if (buddyCount > 0) {
            uint8_t prevIndex = selectedIndex;
            uint8_t prevScroll = scrollOffset;
            selectedIndex = 0;
            ensureSelectionVisible();
            redrawSelectionChange(prevIndex, prevScroll);
            drawHeader();  // Redraw header to remove selection
        }
        return;
    }
    
    // Normal navigation: move down in list
    if (selectedIndex + 1 >= buddyCount) return;  // Can't go down from last buddy
    
    uint8_t prevIndex = selectedIndex;
    uint8_t prevScroll = scrollOffset;
    selectedIndex++;
    ensureSelectionVisible();
    redrawSelectionChange(prevIndex, prevScroll);
}

void BuddyListScreen::handleSelect() {
    // If header button is selected, signal that Add Friend screen should be shown
    // The actual screen display is handled in main.cpp
    if (isHeaderButtonSelected()) {
        // Do nothing here - main.cpp will check shouldShowAddFriendScreen() and show AddFriendScreen
        return;
    }
    
    // Otherwise, selection is handled by consumers via getSelectedBuddy()
}

bool BuddyListScreen::shouldShowAddFriendScreen() const {
    return isHeaderButtonSelected();
}

void BuddyListScreen::handleSelectRandom() {
    if (buddyCount == 0) {
        // If no buddies, select header button
        if (!isHeaderButtonSelected()) {
            uint8_t prevIndex = selectedIndex;
            uint8_t prevScroll = scrollOffset;
            selectedIndex = MAX_BUDDIES;  // Select header button
            redrawSelectionChange(prevIndex, prevScroll);
        }
        return;
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
    // If header button is selected or no buddies, return empty
    if (buddyCount == 0 || isHeaderButtonSelected()) return empty;
    if (selectedIndex >= buddyCount) return empty;
    return buddies[selectedIndex];
}

void BuddyListScreen::redrawSelectionChange(uint8_t previousIndex, uint8_t previousScroll) {
    // Handle header button selection changes
    bool prevWasHeader = (previousIndex == MAX_BUDDIES);
    bool currIsHeader = isHeaderButtonSelected();
    
    if (prevWasHeader || currIsHeader) {
        // Header button selection changed, redraw header
        drawHeader();
    }
    
    // If scroll window changed, redraw the list area to keep alignment
    if (previousScroll != scrollOffset) {
        drawList();
        return;
    }

    uint8_t visibleRows = getVisibleRows();

    // Redraw previous selected row to remove highlight (only if it was a buddy, not header)
    if (!prevWasHeader && previousIndex < buddyCount &&
        previousIndex >= scrollOffset &&
        previousIndex < scrollOffset + visibleRows) {
        uint8_t row = previousIndex - scrollOffset;
        drawBuddyRow(row, previousIndex);
    }

    // Redraw new selected row to show highlight (only if it's a buddy, not header)
    if (!currIsHeader && selectedIndex < buddyCount &&
        selectedIndex >= scrollOffset &&
        selectedIndex < scrollOffset + visibleRows) {
        uint8_t row = selectedIndex - scrollOffset;
        drawBuddyRow(row, selectedIndex);
    }
}

