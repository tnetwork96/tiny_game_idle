#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include "wifi_list.h"
#include "wifi_password.h"
#include "keyboard.h"  // Sử dụng keyboard thường

enum WiFiState {
    WIFI_STATE_SCAN,
    WIFI_STATE_SELECT,
    WIFI_STATE_PASSWORD,
    WIFI_STATE_CONNECTING,
    WIFI_STATE_CONNECTED,
    WIFI_STATE_ERROR
};

class WiFiManager {
private:
    WiFiListScreen* wifiList;
    WiFiPasswordScreen* wifiPassword;
    Keyboard* keyboard;  // Sử dụng keyboard thường
    Adafruit_ST7789* tft;
    
    WiFiState currentState;
    String selectedSSID;
    String password;
    unsigned long connectStartTime;
    uint16_t connectAttempts;
    
    // Callback functions
    void onWiFiSelected();
    void onPasswordEntered();
    void connectToWiFi();
    void saveWiFiPassword();  // Save password to file
    String loadWiFiPassword();  // Load password from file, returns empty string if not found
    
public:
    WiFiManager(Adafruit_ST7789* tft, Keyboard* keyboard);  // Sử dụng keyboard thường
    ~WiFiManager();
    
    // Initialize and start flow
    void begin();
    
    // Update state machine
    void update();
    
    // Navigation handlers
    void handleUp();
    void handleDown();
    void handleLeft();
    void handleRight();
    void handleSelect();
    void handleExit();
    
    // Get current state
    WiFiState getState() const { return currentState; }
    bool isConnected() const { return currentState == WIFI_STATE_CONNECTED; }
    String getConnectedSSID() const { return selectedSSID; }
    
    // Get screens for external access
    WiFiPasswordScreen* getPasswordScreen() { return wifiPassword; }
    WiFiListScreen* getListScreen() { return wifiList; }
    
    // Handle keyboard input
    void handleKeyboardInput(String key);
    
    // Public method to trigger password entered (for callback)
    void triggerPasswordEntered() { onPasswordEntered(); }
    
    // Disconnect
    void disconnect();
};

#endif

