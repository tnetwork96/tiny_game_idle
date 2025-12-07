#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include "wifi_list.h"
#include "wifi_password.h"
#include "keyboard.h"

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
    Keyboard* keyboard;
    TFT_eSprite* bg;
    
    WiFiState currentState;
    String selectedSSID;
    String password;
    unsigned long connectStartTime;
    uint8_t connectAttempts;
    
    // Callback functions
    void onWiFiSelected();
    void onPasswordEntered();
    void connectToWiFi();
    
public:
    WiFiManager(TFT_eSprite* bg, Keyboard* keyboard);
    ~WiFiManager();
    
    // Initialize and start flow
    void begin();
    
    // Update state machine
    void update();
    
    // Navigation handlers
    void handleUp();
    void handleDown();
    void handleSelect();
    
    // Get current state
    WiFiState getState() const { return currentState; }
    bool isConnected() const { return currentState == WIFI_STATE_CONNECTED; }
    String getConnectedSSID() const { return selectedSSID; }
    
    // Get screens for external access
    WiFiPasswordScreen* getPasswordScreen() { return wifiPassword; }
    WiFiListScreen* getListScreen() { return wifiList; }
    
    // Handle keyboard input
    void handleKeyboardInput(String key);
    
    // Disconnect
    void disconnect();
};

#endif

