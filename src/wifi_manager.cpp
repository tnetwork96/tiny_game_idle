#include <Arduino.h>
#include "wifi_manager.h"

WiFiManager::WiFiManager(TFT_eSprite* bg, Keyboard* keyboard) {
    this->bg = bg;
    this->keyboard = keyboard;
    this->currentState = WIFI_STATE_SCAN;
    this->selectedSSID = "";
    this->password = "";
    this->connectStartTime = 0;
    this->connectAttempts = 0;
    
    // Initialize screens
    this->wifiList = new WiFiListScreen(bg);
    this->wifiPassword = new WiFiPasswordScreen(bg, keyboard);
}

WiFiManager::~WiFiManager() {
    delete wifiList;
    delete wifiPassword;
}

void WiFiManager::begin() {
    // Start with scanning WiFi
    currentState = WIFI_STATE_SCAN;
    wifiList->scanNetworks();
    
    // Try to find "Van Ninh" and select it automatically
    for (uint8_t i = 0; i < wifiList->getNetworkCount(); i++) {
        wifiList->selectIndex(i);
        if (wifiList->getSelectedSSID() == "Van Ninh") {
            Serial.println("WiFi Manager: Found 'Van Ninh', auto-selecting...");
            currentState = WIFI_STATE_SELECT;
            onWiFiSelected();  // Auto-select and move to password
            return;
        }
    }
    
    // If not found, show list for manual selection
    currentState = WIFI_STATE_SELECT;
    wifiList->draw();
    Serial.println("WiFi Manager: Starting scan...");
}

void WiFiManager::onWiFiSelected() {
    selectedSSID = wifiList->getSelectedSSID();
    Serial.print("WiFi Manager: Selected SSID: ");
    Serial.println(selectedSSID);
    
    // Always move to password screen when WiFi is selected
    currentState = WIFI_STATE_PASSWORD;
    
    // Auto-fill password if it's "Van Ninh"
    if (selectedSSID == "Van Ninh") {
        wifiPassword->clearPassword();
        wifiPassword->setPassword("123456a@");  // Auto-fill password
        Serial.println("WiFi Manager: Auto-filled password for Van Ninh");
    } else {
        wifiPassword->clearPassword();
    }
    
    wifiPassword->draw();
    Serial.println("WiFi Manager: Moving to password screen");
}

void WiFiManager::onPasswordEntered() {
    password = wifiPassword->getPassword();
    Serial.print("WiFi Manager: Password entered: ");
    Serial.println(password);
    
    // Start connecting
    currentState = WIFI_STATE_CONNECTING;
    connectToWiFi();
}

void WiFiManager::connectToWiFi() {
    Serial.print("WiFi Manager: Connecting to ");
    Serial.print(selectedSSID);
    Serial.print(" with password: ");
    Serial.println(password);
    
    // Disconnect if already connected
    WiFi.disconnect();
    delay(100);
    
    // Start connection
    WiFi.mode(WIFI_STA);
    WiFi.begin(selectedSSID.c_str(), password.c_str());
    
    connectStartTime = millis();
    connectAttempts++;
    
    // Show connecting message
    bg->fillScreen(TFT_BLACK);
    bg->setTextColor(TFT_YELLOW, TFT_BLACK);
    bg->setTextSize(1);
    bg->setCursor(10, 50);
    bg->print("Connecting...");
    bg->pushSprite(0, 0);
}

void WiFiManager::update() {
    if (currentState == WIFI_STATE_CONNECTING) {
        // Check connection status
        if (WiFi.status() == WL_CONNECTED) {
            currentState = WIFI_STATE_CONNECTED;
            Serial.println("WiFi Manager: Connected successfully!");
            
            // Show connected message
            bg->fillScreen(TFT_BLACK);
            bg->setTextColor(TFT_GREEN, TFT_BLACK);
            bg->setTextSize(1);
            bg->setCursor(10, 50);
            bg->print("Connected!");
            bg->setCursor(10, 65);
            bg->setTextColor(TFT_CYAN, TFT_BLACK);
            bg->print(selectedSSID);
            bg->pushSprite(0, 0);
        } else if (millis() - connectStartTime > 10000) {
            // Timeout after 10 seconds
            currentState = WIFI_STATE_ERROR;
            Serial.println("WiFi Manager: Connection timeout");
            
            // Show error message
            bg->fillScreen(TFT_BLACK);
            bg->setTextColor(TFT_RED, TFT_BLACK);
            bg->setTextSize(1);
            bg->setCursor(10, 50);
            bg->print("Connection failed!");
            bg->pushSprite(0, 0);
        }
    }
}

void WiFiManager::handleUp() {
    if (currentState == WIFI_STATE_SELECT || currentState == WIFI_STATE_SCAN) {
        wifiList->selectPrevious();
    }
}

void WiFiManager::handleDown() {
    if (currentState == WIFI_STATE_SELECT || currentState == WIFI_STATE_SCAN) {
        wifiList->selectNext();
    }
}

void WiFiManager::handleSelect() {
    if (currentState == WIFI_STATE_SCAN) {
        // After scan, move to select mode
        currentState = WIFI_STATE_SELECT;
    } else if (currentState == WIFI_STATE_SELECT) {
        // Select WiFi
        onWiFiSelected();
    }
}

void WiFiManager::handleKeyboardInput(String key) {
    if (currentState == WIFI_STATE_PASSWORD) {
        // Handle password input
        if (key == "|e") {  // Enter key - connect
            onPasswordEntered();
        } else {
            // Normal key input
            wifiPassword->handleKeyPress(key);
        }
    }
}

void WiFiManager::disconnect() {
    WiFi.disconnect();
    currentState = WIFI_STATE_SCAN;
    selectedSSID = "";
    password = "";
}

