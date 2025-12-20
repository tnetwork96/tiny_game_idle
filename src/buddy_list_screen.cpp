#include "buddy_list_screen.h"

BuddyListScreen::BuddyListScreen(Adafruit_ST7789* tft) {
    this->tft = tft;
    buddyCount = 0;
    selectedIndex = 0;
    scrollOffset = 0;

    bgColor = ST77XX_BLACK;
    headerColor = 0x780F;       // Yahoo-like purple
    headerTextColor = ST77XX_WHITE;
    listTextColor = ST77XX_WHITE;
    highlightColor = 0x2104;    // Dark gray highlight
    onlineColor = 0x07E0;       // Green
    offlineColor = 0xF800;      // Red
}

void BuddyListScreen::setBuddies(const BuddyEntry* entries, uint8_t count) {
    buddyCount = min(count, MAX_BUDDIES);
    for (uint8_t i = 0; i < buddyCount; i++) {
        buddies[i] = entries[i];
    }
    if (selectedIndex >= buddyCount) {
        selectedIndex = buddyCount > 0 ? buddyCount - 1 : 0;
    }
    ensureSelectionVisible();
}

void BuddyListScreen::updateStatus(uint8_t index, bool online) {
    if (index >= buddyCount) return;
    buddies[index].online = online;
    draw();
}

uint8_t BuddyListScreen::getVisibleRows() const {
    const uint16_t headerHeight = 30;
    const uint16_t rowHeight = 20;
    uint16_t available = (240 > headerHeight) ? (240 - headerHeight) : 0;
    uint8_t rows = available / rowHeight;
    return rows < 1 ? 1 : rows;
}

void BuddyListScreen::ensureSelectionVisible() {
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

    tft->setTextColor(headerTextColor, headerColor);
    tft->setTextSize(2);
    tft->setCursor(10, 8);
    tft->print("Chat");
}

uint16_t BuddyListScreen::statusColor(bool online) const {
    return online ? onlineColor : offlineColor;
}

void BuddyListScreen::drawBuddyRow(uint8_t visibleRow, uint8_t buddyIdx) {
    if (tft == nullptr || buddyIdx >= buddyCount) return;

    const uint16_t headerHeight = 30;
    const uint16_t rowHeight = 20;
    const uint16_t y = headerHeight + visibleRow * rowHeight;

    bool isSelected = (buddyIdx == selectedIndex);
    uint16_t rowBg = isSelected ? highlightColor : bgColor;

    tft->fillRect(0, y, 320, rowHeight, rowBg);

    // Status dot
    uint16_t dotX = 14;
    uint16_t dotY = y + rowHeight / 2;
    tft->fillCircle(dotX, dotY, 5, statusColor(buddies[buddyIdx].online));

    // Name
    tft->setTextColor(listTextColor, rowBg);
    tft->setTextSize(2);
    uint16_t textX = 26;
    uint16_t textY = y + 2;
    tft->setCursor(textX, textY);
    String displayName = buddies[buddyIdx].name;
    const uint8_t charWidth = 12;  // Approx for text size 2
    const uint16_t rightMargin = 92;
    uint8_t maxChars = (320 - textX - rightMargin) / charWidth;
    if (displayName.length() > maxChars && maxChars > 3) {
        displayName = displayName.substring(0, maxChars - 3) + "...";
    }
    tft->print(displayName);

    // No status label text (Yahoo-style: only dot)
}

void BuddyListScreen::drawList() {
    if (tft == nullptr) return;

    const uint16_t headerHeight = 30;
    const uint16_t rowHeight = 20;
    const uint16_t startY = headerHeight;
    const uint8_t visibleRows = getVisibleRows();
    const uint16_t listHeight = visibleRows * rowHeight;

    // Clear list area
    tft->fillRect(0, startY, 320, listHeight, bgColor);

    for (uint8_t row = 0; row < visibleRows; row++) {
        uint8_t buddyIdx = scrollOffset + row;
        if (buddyIdx >= buddyCount) break;
        drawBuddyRow(row, buddyIdx);
    }

    // Simple scrollbar on the right
    if (buddyCount > visibleRows) {
        uint16_t barWidth = 4;
        uint16_t barX = 320 - barWidth;
        tft->fillRect(barX, startY, barWidth, listHeight, 0x4208);  // Dark gray track

        float ratio = (float)visibleRows / (float)buddyCount;
        uint16_t thumbHeight = listHeight * ratio;
        if (thumbHeight < 8) thumbHeight = 8;

        float scrollPercent = (float)scrollOffset / (float)(buddyCount - visibleRows);
        if (scrollPercent < 0.0f) scrollPercent = 0.0f;
        if (scrollPercent > 1.0f) scrollPercent = 1.0f;
        uint16_t thumbY = startY + (listHeight - thumbHeight) * scrollPercent;

        tft->fillRect(barX, thumbY, barWidth, thumbHeight, onlineColor);
    }
}

void BuddyListScreen::draw() {
    if (tft == nullptr) return;
    tft->fillScreen(bgColor);
    drawHeader();
    drawList();
}

void BuddyListScreen::handleUp() {
    if (buddyCount == 0 || selectedIndex == 0) return;
    uint8_t prevIndex = selectedIndex;
    uint8_t prevScroll = scrollOffset;
    selectedIndex--;
    ensureSelectionVisible();
    redrawSelectionChange(prevIndex, prevScroll);
}

void BuddyListScreen::handleDown() {
    if (buddyCount == 0 || selectedIndex + 1 >= buddyCount) return;
    uint8_t prevIndex = selectedIndex;
    uint8_t prevScroll = scrollOffset;
    selectedIndex++;
    ensureSelectionVisible();
    redrawSelectionChange(prevIndex, prevScroll);
}

void BuddyListScreen::handleSelect() {
    // No-op here; selection is handled by consumers via getSelectedBuddy()
}

void BuddyListScreen::handleSelectRandom() {
    if (buddyCount == 0) return;
    uint8_t prevIndex = selectedIndex;
    uint8_t prevScroll = scrollOffset;
    uint8_t idx = random(0, buddyCount);
    selectedIndex = idx;
    ensureSelectionVisible();
    redrawSelectionChange(prevIndex, prevScroll);
}

BuddyEntry BuddyListScreen::getSelectedBuddy() const {
    BuddyEntry empty = { "", false };
    if (buddyCount == 0 || selectedIndex >= buddyCount) return empty;
    return buddies[selectedIndex];
}

void BuddyListScreen::redrawSelectionChange(uint8_t previousIndex, uint8_t previousScroll) {
    // If scroll window changed, redraw the list area to keep alignment
    if (previousScroll != scrollOffset) {
        drawList();
        return;
    }

    uint8_t visibleRows = getVisibleRows();

    // Redraw previous selected row to remove highlight
    if (previousIndex < buddyCount &&
        previousIndex >= scrollOffset &&
        previousIndex < scrollOffset + visibleRows) {
        uint8_t row = previousIndex - scrollOffset;
        drawBuddyRow(row, previousIndex);
    }

    // Redraw new selected row to show highlight
    if (selectedIndex < buddyCount &&
        selectedIndex >= scrollOffset &&
        selectedIndex < scrollOffset + visibleRows) {
        uint8_t row = selectedIndex - scrollOffset;
        drawBuddyRow(row, selectedIndex);
    }
}

