#include <Arduino.h>
#include "wifi_list.h"

WiFiListScreen::WiFiListScreen(TFT_eSprite* bg) {
    this->bg = bg;
    this->networkCount = 0;
    this->selectedIndex = 0;
    
    // Initialize layout
    this->titleY = 5;
    this->listStartY = 20;
    this->itemHeight = 18;
    this->maxItems = 5;
    
    // Initialize colors (Arcade style)
    this->bgColor = TFT_BLACK;
    this->titleColor = TFT_YELLOW;  // Neon yellow
    this->itemBgColor = TFT_BLACK;
    this->itemSelectedBgColor = TFT_CYAN;  // Neon cyan for arcade highlight
    this->itemTextColor = TFT_GREEN;  // Neon green
    this->itemSelectedTextColor = TFT_YELLOW;  // Neon yellow for selected item text
    this->itemBorderColor = TFT_CYAN;  // Neon cyan
    
    // Initialize networks
    for (uint8_t i = 0; i < 5; i++) {
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
    // Start WiFi scan
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);
    
    int n = WiFi.scanNetworks();
    
    if (n == 0) {
        networkCount = 0;
        return;
    }
    
    // Create temporary array to store all networks
    struct TempNetwork {
        String ssid;
        int32_t rssi;
        uint8_t encryptionType;
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
    for (uint8_t i = 0; i < networkCount; i++) {
        networks[i].ssid = tempNetworks[i].ssid;
        networks[i].rssi = tempNetworks[i].rssi;
        networks[i].encryptionType = tempNetworks[i].encryptionType;
        networks[i].isSelected = (i == selectedIndex);
    }
    
    // Clear remaining slots
    for (uint8_t i = networkCount; i < 5; i++) {
        networks[i].ssid = "";
        networks[i].rssi = -100;
        networks[i].encryptionType = 0;
        networks[i].isSelected = false;
    }
}

void WiFiListScreen::drawTitle() {
    bg->setTextColor(titleColor, bgColor);
    bg->setTextSize(1);
    bg->setCursor(10, titleY);
    bg->print("WiFi Networks");
}

void WiFiListScreen::drawSignalStrength(uint8_t x, uint8_t y, int32_t rssi) {
    // Draw signal strength bars (1-4 bars based on RSSI)
    uint8_t bars = 0;
    if (rssi > -50) bars = 4;      // Excellent
    else if (rssi > -60) bars = 3;  // Good
    else if (rssi > -70) bars = 2;  // Fair
    else if (rssi > -80) bars = 1;  // Weak
    else bars = 0;                  // Very weak
    
    // Draw bars (each bar is 2px wide, 3px tall, 1px spacing)
    for (uint8_t i = 0; i < 4; i++) {
        uint16_t barColor = (i < bars) ? TFT_GREEN : 0x4208;  // Dark grey (RGB565)
        uint8_t barX = x + i * 3;
        uint8_t barHeight = (i + 1) * 2;
        bg->fillRect(barX, y + 4 - barHeight, 2, barHeight, barColor);
    }
}

void WiFiListScreen::drawWiFiItem(uint8_t index, uint8_t yPos) {
    if (index >= networkCount) return;
    
    WiFiNetwork* net = &networks[index];
    uint8_t itemWidth = 100;  // Shorter width
    uint8_t itemX = (128 - itemWidth) / 2;  // Center
    uint8_t cornerRadius = 3;  // Rounded corner radius
    
    // Determine background color (tô màu background cho item được chọn)
    uint16_t bgItemColor = net->isSelected ? itemSelectedBgColor : itemBgColor;
    
    // Draw item background with rounded corners
    bg->fillRoundRect(itemX, yPos, itemWidth, itemHeight, cornerRadius, bgItemColor);
    
    // Draw SSID (truncate if too long and center it)
    // Use different text color for selected item to make it stand out
    uint16_t textColor = net->isSelected ? itemSelectedTextColor : itemTextColor;
    bg->setTextColor(textColor, bgItemColor);
    bg->setTextSize(1);
    String displaySSID = net->ssid;
    if (displaySSID.length() > 15) {
        displaySSID = displaySSID.substring(0, 15);
    }
    
    // Calculate text width (approximately 6px per character)
    uint8_t textWidth = displaySSID.length() * 6;
    // Center text in item width
    uint8_t textX = itemX + (itemWidth - textWidth) / 2;  // Center horizontally in item
    uint8_t textY = yPos + 5;  // Center vertically
    
    bg->setCursor(textX, textY);
    bg->print(displaySSID);
}

void WiFiListScreen::draw() {
    // Fill background
    bg->fillScreen(bgColor);
    
    // Draw title
    drawTitle();
    
    // Draw WiFi list items
    for (uint8_t i = 0; i < networkCount; i++) {
        uint8_t yPos = listStartY + i * (itemHeight + 2);
        drawWiFiItem(i, yPos);
    }
    
    // Push sprite to display
    bg->pushSprite(0, 0);
}

void WiFiListScreen::selectNext() {
    if (networkCount == 0) return;
    
    networks[selectedIndex].isSelected = false;
    selectedIndex = (selectedIndex + 1) % networkCount;
    networks[selectedIndex].isSelected = true;
    draw();
}

void WiFiListScreen::selectPrevious() {
    if (networkCount == 0) return;
    
    networks[selectedIndex].isSelected = false;
    selectedIndex = (selectedIndex == 0) ? networkCount - 1 : selectedIndex - 1;
    networks[selectedIndex].isSelected = true;
    draw();
}

void WiFiListScreen::selectIndex(uint8_t index) {
    if (index >= networkCount) return;
    
    networks[selectedIndex].isSelected = false;
    selectedIndex = index;
    networks[selectedIndex].isSelected = true;
    draw();
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

