#include <Arduino.h>
#include "wifi_list.h"
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

WiFiListScreen::WiFiListScreen(Adafruit_ST7789* tft) {
    this->tft = tft;
    this->networkCount = 0;
    this->selectedIndex = 0;
    this->scrollOffset = 0;
    
    // Initialize layout (màn hình 320x240 landscape)
    const uint16_t headerHeight = 30;
    const uint16_t rowHeight = 24;
    this->headerHeight = headerHeight;
    this->listStartY = headerHeight;
    this->itemHeight = rowHeight;
    
    // Calculate max visible items (screen height - header) / row height
    uint16_t availableHeight = 240 - headerHeight;
    this->maxItems = availableHeight / rowHeight;
    
    // Initialize colors (Deep Space Arcade Theme)
    this->bgColor = 0x0042;           // Deep Midnight Blue background
    this->headerColor = 0x08A5;        // Header background
    this->headerTextColor = 0xFFFF;   // White text for header
    this->itemBgColor = 0x0042;       // Deep Midnight Blue for items
    this->itemSelectedBgColor = 0x1148;  // Electric Blue tint for selection
    this->itemTextColor = 0xFFFF;     // White text
    this->itemSelectedTextColor = 0xFFFF;  // White text for selected
    this->accentColor = 0x07FF;       // Cyan accent/border
    
    // Initialize networks
    for (uint16_t i = 0; i < 5; i++) {
        networks[i].ssid = "";
        networks[i].rssi = -100;
        networks[i].encryptionType = 0;
        networks[i].isSelected = false;
    }
}

WiFiListScreen::~WiFiListScreen() {
    // Nothing to free
}

void WiFiListScreen::scanNetworks() {
    Serial.println("WiFi List: Starting scan...");
    // Start WiFi scan
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);
    
    Serial.println("WiFi List: Scanning networks (this may take a few seconds)...");
    int n = WiFi.scanNetworks(false, true);  // async=false, show_hidden=true
    Serial.print("WiFi List: Scan complete. Found ");
    Serial.print(n);
    Serial.println(" networks");
    
    if (n == 0) {
        networkCount = 0;
        Serial.println("WiFi List: No networks found!");
        return;
    }
    
    // Create temporary array to store all networks
    struct TempNetwork {
        String ssid;
        int32_t rssi;
        uint16_t encryptionType;
    };
    
    TempNetwork tempNetworks[n];
    
    // Copy all networks
    for (int i = 0; i < n; i++) {
        tempNetworks[i].ssid = WiFi.SSID(i);
        tempNetworks[i].rssi = WiFi.RSSI(i);
        tempNetworks[i].encryptionType = WiFi.encryptionType(i);
    }
    
    // Sort by RSSI (strongest first) - simple bubble sort
    for (int i = 0; i < n - 1; i++) {
        for (int j = 0; j < n - i - 1; j++) {
            if (tempNetworks[j].rssi < tempNetworks[j + 1].rssi) {
                // Swap
                TempNetwork temp = tempNetworks[j];
                tempNetworks[j] = tempNetworks[j + 1];
                tempNetworks[j + 1] = temp;
            }
        }
    }
    
    // Copy top 5 to networks array
    networkCount = (n < 5) ? n : 5;
    Serial.print("WiFi List: Found ");
    Serial.print(networkCount);
    Serial.println(" networks:");
    for (uint16_t i = 0; i < networkCount; i++) {
        networks[i].ssid = tempNetworks[i].ssid;
        networks[i].rssi = tempNetworks[i].rssi;
        networks[i].encryptionType = tempNetworks[i].encryptionType;
        networks[i].isSelected = (i == selectedIndex);
        
        Serial.print("  [");
        Serial.print(i);
        Serial.print("] ");
        Serial.print(networks[i].ssid);
        Serial.print(" (RSSI: ");
        Serial.print(networks[i].rssi);
        Serial.print(")");
        if (networks[i].isSelected) {
            Serial.print(" <-- SELECTED");
        }
        Serial.println();
    }
    
    // Clear remaining slots
    for (uint16_t i = networkCount; i < 5; i++) {
        networks[i].ssid = "";
        networks[i].rssi = -100;
        networks[i].encryptionType = 0;
        networks[i].isSelected = false;
    }
}

void WiFiListScreen::drawHeader() {
    if (tft == nullptr) return;
    
    // Draw header bar
    tft->fillRect(0, 0, 320, headerHeight, headerColor);
    
    // Draw title "WiFi Networks" centered
    String title = "WiFi Networks";
    tft->setTextColor(headerTextColor, headerColor);
    tft->setTextSize(2);
    uint16_t textWidth = title.length() * 12;  // Approx 12px per character with textSize 2
    uint16_t textX = (320 - textWidth) / 2;   // Center on 320px width
    tft->setCursor(textX, 8);  // Vertically center in 30px header (text height ~16px)
    tft->print(title);
    
    // Draw separator line below header
    tft->drawFastHLine(0, headerHeight - 1, 320, accentColor);
}

void WiFiListScreen::drawSignalStrength(uint16_t x, uint16_t y, int32_t rssi) {
    // Draw signal strength bars (1-4 bars based on RSSI)
    uint16_t bars = 0;
    if (rssi > -50) bars = 4;      // Excellent
    else if (rssi > -60) bars = 3;  // Good
    else if (rssi > -70) bars = 2;  // Fair
    else if (rssi > -80) bars = 1;  // Weak
    else bars = 0;                  // Very weak
    
    // Draw bars (each bar is 2px wide, 3px tall, 1px spacing)
    uint16_t activeColor = 0x07F3;  // Mint Green for active bars (Deep Space Arcade theme)
    uint16_t inactiveColor = 0x0021;  // Very dark for inactive bars
    for (uint16_t i = 0; i < 4; i++) {
        uint16_t barColor = (i < bars) ? activeColor : inactiveColor;
        uint16_t barX = x + i * 3;
        uint16_t barHeight = (i + 1) * 2;
        tft->fillRect(barX, y + 4 - barHeight, 2, barHeight, barColor);
    }
}

void WiFiListScreen::drawWiFiItem(uint16_t index, uint16_t yPos) {
    if (index >= networkCount) return;
    
    WiFiNetwork* net = &networks[index];
    
    // Determine background color
    uint16_t bgItemColor = net->isSelected ? itemSelectedBgColor : itemBgColor;
    
    // Draw full-width row background (320px width for landscape mode)
    tft->fillRect(0, yPos, 320, itemHeight, bgItemColor);
    
    // Draw 2px cyan vertical bar on left edge for selected row
    if (net->isSelected) {
        tft->fillRect(0, yPos, 2, itemHeight, accentColor);
    }
    
    // Draw SSID text (left-aligned with margin)
    uint16_t textColor = net->isSelected ? itemSelectedTextColor : itemTextColor;
    tft->setTextColor(textColor, bgItemColor);
    tft->setTextSize(2);
    String displaySSID = net->ssid;
    
    // Calculate max characters that fit (320px width, 2px accent bar, margins, signal icon space)
    // Leave space for signal strength indicator on right (about 20px) and margins
    uint16_t textX = net->isSelected ? 8 : 6;  // Margin from left (more if selected due to accent bar)
    uint16_t textY = yPos + (itemHeight - 16) / 2;  // Vertically center (text height ~16px)
    uint16_t maxTextWidth = 320 - textX - 25;  // Leave space for signal indicator
    uint16_t maxChars = maxTextWidth / 12;  // Approx 12px per character with textSize 2
    
    if (displaySSID.length() > maxChars && maxChars > 3) {
        displaySSID = displaySSID.substring(0, maxChars - 3) + "...";
    }
    
    tft->setCursor(textX, textY);
    tft->print(displaySSID);
    
    // Draw signal strength indicator on the right
    uint16_t signalX = 320 - 20;  // Right margin
    drawSignalStrength(signalX, textY, net->rssi);
}

uint8_t WiFiListScreen::getVisibleRows() const {
    uint16_t available = (240 > headerHeight) ? (240 - headerHeight) : 0;
    uint8_t rows = available / itemHeight;
    return rows < 1 ? 1 : rows;
}

void WiFiListScreen::ensureSelectionVisible() {
    uint8_t visible = getVisibleRows();
    if (selectedIndex < scrollOffset) {
        scrollOffset = selectedIndex;
    } else if (selectedIndex >= scrollOffset + visible) {
        scrollOffset = selectedIndex - visible + 1;
    }
}

void WiFiListScreen::drawScrollbar() {
    if (tft == nullptr) return;
    
    uint16_t startY = listStartY;
    uint8_t visibleRows = getVisibleRows();
    
    // Only draw scrollbar if there are more networks than visible rows
    if (networkCount <= visibleRows) {
        // Clear scrollbar area if it was previously visible
        uint16_t barWidth = 4;
        uint16_t barX = 320 - barWidth;
        uint16_t scrollbarHeight = 240 - startY;
        tft->fillRect(barX, startY, barWidth, scrollbarHeight, bgColor);
        return;
    }
    
    uint16_t barWidth = 4;
    uint16_t barX = 320 - barWidth;
    uint16_t scrollbarHeight = 240 - startY;
    
    // Dark track
    tft->fillRect(barX, startY, barWidth, scrollbarHeight, 0x0021);
    
    float ratio = (float)visibleRows / (float)networkCount;
    uint16_t thumbHeight = scrollbarHeight * ratio;
    if (thumbHeight < 8) thumbHeight = 8;
    
    float scrollPercent = (float)scrollOffset / (float)(networkCount - visibleRows);
    if (scrollPercent < 0.0f) scrollPercent = 0.0f;
    if (scrollPercent > 1.0f) scrollPercent = 1.0f;
    uint16_t thumbY = startY + (scrollbarHeight - thumbHeight) * scrollPercent;
    
    // Cyan thumb for scrollbar
    tft->fillRect(barX, thumbY, barWidth, thumbHeight, accentColor);
}

void WiFiListScreen::updateItem(uint16_t index) {
    // Chỉ vẽ lại một item cụ thể để tránh nháy màn hình
    if (index >= networkCount) return;
    
    // Only update if item is visible
    uint8_t visibleRows = getVisibleRows();
    if (index < scrollOffset || index >= scrollOffset + visibleRows) {
        return;  // Item not visible
    }
    
    uint8_t visibleRow = index - scrollOffset;
    uint16_t yPos = listStartY + visibleRow * itemHeight;
    drawWiFiItem(index, yPos);
}

void WiFiListScreen::draw() {
    Serial.print("WiFi List: Drawing screen with ");
    Serial.print(networkCount);
    Serial.println(" networks");
    
    // Fill background
    tft->fillScreen(bgColor);
    
    // Draw header
    drawHeader();
    
    // Ensure selection is visible
    ensureSelectionVisible();
    
    // Draw WiFi list items (only visible ones)
    if (networkCount > 0) {
        uint8_t visibleRows = getVisibleRows();
        for (uint8_t row = 0; row < visibleRows; row++) {
            uint16_t networkIdx = scrollOffset + row;
            if (networkIdx >= networkCount) {
                // Clear remaining rows
                uint16_t yPos = listStartY + row * itemHeight;
                tft->fillRect(0, yPos, 320, (visibleRows - row) * itemHeight, bgColor);
                break;
            }
            uint16_t yPos = listStartY + row * itemHeight;
            drawWiFiItem(networkIdx, yPos);
        }
    } else {
        // Display "No WiFi found" message
        tft->setTextColor(0xFFFF, bgColor);  // White text
        tft->setTextSize(2);
        String message = "No WiFi found";
        uint16_t textWidth = message.length() * 12;
        uint16_t textX = (320 - textWidth) / 2;  // Center on 320px width
        tft->setCursor(textX, 100);
        tft->print(message);
    }
    
    // Draw scrollbar
    drawScrollbar();
    
    Serial.println("WiFi List: Screen drawn directly to display");
}

void WiFiListScreen::selectNext() {
    if (networkCount == 0) return;
    
    uint16_t oldIndex = selectedIndex;
    uint16_t oldScroll = scrollOffset;
    networks[oldIndex].isSelected = false;
    selectedIndex = (selectedIndex + 1) % networkCount;
    networks[selectedIndex].isSelected = true;
    
    ensureSelectionVisible();
    
    // Redraw affected items
    if (oldScroll != scrollOffset) {
        // Scroll changed, need full redraw
        draw();
    } else {
        // Only selection changed
        updateItem(oldIndex);
        updateItem(selectedIndex);
    }
}

void WiFiListScreen::selectPrevious() {
    if (networkCount == 0) return;
    
    uint16_t oldIndex = selectedIndex;
    uint16_t oldScroll = scrollOffset;
    networks[oldIndex].isSelected = false;
    selectedIndex = (selectedIndex == 0) ? networkCount - 1 : selectedIndex - 1;
    networks[selectedIndex].isSelected = true;
    
    ensureSelectionVisible();
    
    // Redraw affected items
    if (oldScroll != scrollOffset) {
        // Scroll changed, need full redraw
        draw();
    } else {
        // Only selection changed
        updateItem(oldIndex);
        updateItem(selectedIndex);
    }
}

void WiFiListScreen::selectIndex(uint16_t index) {
    if (index >= networkCount) return;
    
    uint16_t oldIndex = selectedIndex;
    uint16_t oldScroll = scrollOffset;
    networks[oldIndex].isSelected = false;
    selectedIndex = index;
    networks[selectedIndex].isSelected = true;
    
    Serial.print("WiFi List: Selected index ");
    Serial.print(selectedIndex);
    Serial.print(" - SSID: ");
    Serial.print(networks[selectedIndex].ssid);
    Serial.print(" - RSSI: ");
    Serial.println(networks[selectedIndex].rssi);
    
    ensureSelectionVisible();
    
    // Redraw affected items
    if (oldIndex != selectedIndex) {
        if (oldScroll != scrollOffset) {
            // Scroll changed, need full redraw
            draw();
        } else {
            // Only selection changed
            updateItem(oldIndex);
            updateItem(selectedIndex);
        }
    }
}

String WiFiListScreen::getSelectedSSID() const {
    if (selectedIndex < networkCount) {
        return networks[selectedIndex].ssid;
    }
    return "";
}

int32_t WiFiListScreen::getSelectedRSSI() const {
    if (selectedIndex < networkCount) {
        return networks[selectedIndex].rssi;
    }
    return -100;
}

