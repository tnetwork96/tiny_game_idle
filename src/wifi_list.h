#ifndef WIFI_LIST_H
#define WIFI_LIST_H

#include <Arduino.h>
#include <WiFi.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

struct WiFiNetwork {
    String ssid;
    int32_t rssi;
    uint16_t encryptionType;
    bool isSelected;
};

class WiFiListScreen {
private:
    Adafruit_ST7789* tft;
    
    // WiFi networks
    WiFiNetwork networks[5];  // Top 5 strongest networks
    uint16_t networkCount;
    uint16_t selectedIndex;
    uint16_t scrollOffset;
    
    // Screen layout
    uint16_t headerHeight;
    uint16_t listStartY;
    uint16_t itemHeight;
    uint16_t maxItems;
    
    // Colors (Deep Space Arcade Theme)
    uint16_t bgColor;
    uint16_t headerColor;
    uint16_t headerTextColor;
    uint16_t itemBgColor;
    uint16_t itemSelectedBgColor;
    uint16_t itemTextColor;
    uint16_t itemSelectedTextColor;
    uint16_t accentColor;
    
    // Drawing methods
    void drawHeader();
    void drawWiFiItem(uint16_t index, uint16_t yPos);
    void drawSignalStrength(uint16_t x, uint16_t y, int32_t rssi);
    void drawScrollbar();
    void updateItem(uint16_t index);  // Chỉ vẽ lại một item cụ thể (để tránh nháy màn hình)
    void ensureSelectionVisible();
    uint8_t getVisibleRows() const;
    
public:
    // Constructor
    WiFiListScreen(Adafruit_ST7789* tft);
    
    // Destructor
    ~WiFiListScreen();
    
    // Scan and update WiFi list
    void scanNetworks();
    
    // Draw entire screen
    void draw();
    
    // Navigation
    void selectNext();
    void selectPrevious();
    void selectIndex(uint16_t index);
    
    // Getters
    uint16_t getNetworkCount() const { return networkCount; }
    uint16_t getSelectedIndex() const { return selectedIndex; }
    String getSelectedSSID() const;
    int32_t getSelectedRSSI() const;
    
    // Setters for colors
    void setBgColor(uint16_t color) { bgColor = color; }
    void setHeaderColor(uint16_t color) { headerColor = color; }
    void setHeaderTextColor(uint16_t color) { headerTextColor = color; }
    void setItemBgColor(uint16_t color) { itemBgColor = color; }
    void setItemSelectedBgColor(uint16_t color) { itemSelectedBgColor = color; }
    void setItemTextColor(uint16_t color) { itemTextColor = color; }
    void setItemSelectedTextColor(uint16_t color) { itemSelectedTextColor = color; }
    void setAccentColor(uint16_t color) { accentColor = color; }
};

#endif
