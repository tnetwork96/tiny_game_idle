#include <Arduino.h>
#include "wifi_list.h"
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

// Synthwave/Vaporwave color palette (theo hình arcade)
#define NEON_PURPLE 0xD81F        // Neon purple (màu chủ đạo của arcade)
#define NEON_GREEN 0x07E0         // Neon green (sáng hơn, như trong hình)
#define NEON_CYAN 0x07FF          // Neon cyan (sáng hơn)
#define YELLOW_ORANGE 0xFE20       // Yellow-orange (như "GAME OVER" text)
#define SOFT_WHITE 0xFFFF         // White (cho text trên purple background)
#define DARK_PURPLE 0x8010        // Dark purple cho background

WiFiListScreen::WiFiListScreen(Adafruit_ST7789* tft) {
    this->tft = tft;
    this->networkCount = 0;
    this->selectedIndex = 0;
    
    // Initialize layout (màn hình 240x320)
    this->titleY = 10;
    this->listStartY = 35;
    this->itemHeight = 35;  // Chiều cao item tăng lên để chữ lớn hơn
    this->maxItems = 7;  // Có thể hiển thị 7 item với itemHeight = 35px
    
    // Initialize colors (Synthwave/Vaporwave style - theo hình arcade)
    this->bgColor = ST77XX_BLACK;  // Background đen như trong arcade
    this->titleColor = YELLOW_ORANGE;  // Yellow-orange như "GAME OVER" text
    this->itemBgColor = ST77XX_BLACK;  // Background đen cho items
    this->itemSelectedBgColor = NEON_PURPLE;  // Neon purple highlight (màu chủ đạo)
    this->itemTextColor = NEON_GREEN;  // Neon green text (sáng như trong hình)
    this->itemSelectedTextColor = SOFT_WHITE;  // White text trên purple background
    this->itemBorderColor = NEON_CYAN;  // Neon cyan border (sáng hơn)
    
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

void WiFiListScreen::drawTitle() {
    tft->setTextColor(titleColor, bgColor);
    
    // Căn giữa tiêu đề "WiFi"
    String title = "WiFi";
    tft->setTextSize(2);  // Tăng kích thước chữ tiêu đề
    uint16_t textWidth = title.length() * 12;  // Khoảng 12px mỗi ký tự với textSize 2
    uint16_t textX = (240 - textWidth) / 2;   // Căn giữa theo chiều ngang 240px (màn hình)
    tft->setCursor(textX, titleY);
    tft->print(title);
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
    for (uint16_t i = 0; i < 4; i++) {
        uint16_t barColor = (i < bars) ? NEON_GREEN : 0x4208;  // Soft green or dark grey (RGB565)
        uint16_t barX = x + i * 3;
        uint16_t barHeight = (i + 1) * 2;
        tft->fillRect(barX, y + 4 - barHeight, 2, barHeight, barColor);
    }
}

void WiFiListScreen::drawWiFiItem(uint16_t index, uint16_t yPos) {
    if (index >= networkCount) return;
    
    WiFiNetwork* net = &networks[index];
    uint16_t itemWidth = 230;  // Width tận dụng tối đa chiều rộng 240px (để margin 5px mỗi bên)
    uint16_t itemX = (240 - itemWidth) / 2;  // Căn giữa theo chiều ngang 240px (màn hình)
    uint16_t cornerRadius = 8;  // Rounded corner radius tăng lên để bo góc rõ hơn
    
    // Determine background color (tô màu background cho item được chọn)
    uint16_t bgItemColor = net->isSelected ? itemSelectedBgColor : itemBgColor;
    
    // Draw item background with rounded corners (bo góc)
    tft->fillRoundRect(itemX, yPos, itemWidth, itemHeight, cornerRadius, bgItemColor);
    
    // Vẽ border cho item được chọn để highlight rõ hơn
    if (net->isSelected) {
        tft->drawRoundRect(itemX, yPos, itemWidth, itemHeight, cornerRadius, itemBorderColor);
        tft->drawRoundRect(itemX + 1, yPos + 1, itemWidth - 2, itemHeight - 2, cornerRadius - 1, itemBorderColor);
    }
    
    // Draw SSID (truncate if too long and center it)
    // Use different text color for selected item to make it stand out
    uint16_t textColor = net->isSelected ? itemSelectedTextColor : itemTextColor;
    tft->setTextColor(textColor, bgItemColor);
    tft->setTextSize(2);  // Tăng kích thước chữ lên 2 để dễ đọc hơn
    String displaySSID = net->ssid;
    
    // Tính toán độ dài tối đa có thể hiển thị (với itemWidth = 230px, margin 8px mỗi bên, textSize 2)
    uint16_t maxChars = (itemWidth - 16) / 12;  // Trừ margin, chia cho 12px mỗi ký tự (textSize 2)
    if (displaySSID.length() > maxChars) {
        displaySSID = displaySSID.substring(0, maxChars);
    }
    
    // Calculate text width (approximately 12px per character with textSize 2)
    uint16_t textWidth = displaySSID.length() * 12;
    // Center text in item width
    uint16_t textX = itemX + (itemWidth - textWidth) / 2;  // Căn giữa theo chiều ngang trong item
    uint16_t textY = yPos + (itemHeight - 16) / 2;  // Căn giữa theo chiều dọc (text height ~16px với textSize 2)
    
    tft->setCursor(textX, textY);
    tft->print(displaySSID);
}

void WiFiListScreen::draw() {
    Serial.print("WiFi List: Drawing screen with ");
    Serial.print(networkCount);
    Serial.println(" networks");
    
    // Fill background
    tft->fillScreen(bgColor);
    
    // Draw title
    drawTitle();
    
    // Draw WiFi list items
    if (networkCount > 0) {
        for (uint16_t i = 0; i < networkCount; i++) {
            uint16_t yPos = listStartY + i * (itemHeight + 4);  // Spacing 4px giữa các item
            drawWiFiItem(i, yPos);
        }
    } else {
        // Hiển thị thông báo không có WiFi
        tft->setTextColor(NEON_GREEN, bgColor);
        tft->setTextSize(2);  // Tăng kích thước chữ
        uint16_t textX = (240 - 72) / 2;  // Căn giữa "No WiFi found" (12px * 6 chars = 72px)
        tft->setCursor(textX, 100);
        tft->print("No WiFi found");
    }
    
    Serial.println("WiFi List: Screen drawn directly to display");
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

void WiFiListScreen::selectIndex(uint16_t index) {
    if (index >= networkCount) return;
    
    networks[selectedIndex].isSelected = false;
    selectedIndex = index;
    networks[selectedIndex].isSelected = true;
    
    Serial.print("WiFi List: Selected index ");
    Serial.print(selectedIndex);
    Serial.print(" - SSID: ");
    Serial.print(networks[selectedIndex].ssid);
    Serial.print(" - RSSI: ");
    Serial.println(networks[selectedIndex].rssi);
    
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

