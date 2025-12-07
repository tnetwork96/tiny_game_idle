#ifndef WIFI_LIST_H
#define WIFI_LIST_H

#include <Arduino.h>
#include <WiFi.h>
#include <TFT_eSPI.h>

struct WiFiNetwork {
    String ssid;
    int32_t rssi;
    uint8_t encryptionType;
    bool isSelected;
};

class WiFiListScreen {
private:
    TFT_eSprite* bg;
    
    // WiFi networks
    WiFiNetwork networks[5];  // Top 5 strongest networks
    uint8_t networkCount;
    uint8_t selectedIndex;
    
    // Screen layout
    uint8_t titleY;
    uint8_t listStartY;
    uint8_t itemHeight;
    uint8_t maxItems;
    
    // Colors (Arcade style)
    uint16_t bgColor;
    uint16_t titleColor;
    uint16_t itemBgColor;
    uint16_t itemSelectedBgColor;
    uint16_t itemTextColor;
    uint16_t itemSelectedTextColor;  // Text color for selected item
    uint16_t itemBorderColor;
    
    // Drawing functions
    void drawTitle();
    void drawWiFiItem(uint8_t index, uint8_t yPos);
    void drawSignalStrength(uint8_t x, uint8_t y, int32_t rssi);
    
public:
    // Constructor
    WiFiListScreen(TFT_eSprite* bg);
    
    // Destructor
    ~WiFiListScreen();
    
    // Scan and update WiFi list
    void scanNetworks();
    
    // Draw entire screen
    void draw();
    
    // Navigation
    void selectNext();
    void selectPrevious();
    void selectIndex(uint8_t index);
    
    // Getters
    String getSelectedSSID() const;
    int32_t getSelectedRSSI() const;
    uint8_t getSelectedIndex() const { return selectedIndex; }
    uint8_t getNetworkCount() const { return networkCount; }
    
    // Color setters
    void setBgColor(uint16_t color) { bgColor = color; }
    void setTitleColor(uint16_t color) { titleColor = color; }
    void setItemBgColor(uint16_t color) { itemBgColor = color; }
    void setItemSelectedBgColor(uint16_t color) { itemSelectedBgColor = color; }
    void setItemTextColor(uint16_t color) { itemTextColor = color; }
    void setItemSelectedTextColor(uint16_t color) { itemSelectedTextColor = color; }
    void setItemBorderColor(uint16_t color) { itemBorderColor = color; }
};

#endif

