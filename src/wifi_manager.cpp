#include <Arduino.h>
#include "wifi_manager.h"
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

// Synthwave/Vaporwave color palette (softened, less harsh)
#define NEON_PURPLE 0xB81A        // Soft purple (reduced brightness)
#define NEON_GREEN 0x05A0         // Soft green (reduced brightness)
#define NEON_CYAN 0x05DF          // Soft cyan (reduced brightness)
#define YELLOW_ORANGE 0xFC00      // Soft yellow-orange (reduced brightness)
#define SOFT_WHITE 0xCE79         // Soft white (not too bright)

WiFiManager::WiFiManager(Adafruit_ST7789* tft, Keyboard* keyboard) {  // Sử dụng keyboard thường
    this->tft = tft;
    this->keyboard = keyboard;
    this->currentState = WIFI_STATE_SCAN;
    this->selectedSSID = "";
    this->password = "";
    this->connectStartTime = 0;
    this->connectAttempts = 0;
    
    // Initialize screens
    this->wifiList = new WiFiListScreen(tft);
    this->wifiPassword = new WiFiPasswordScreen(tft, keyboard);
}

WiFiManager::~WiFiManager() {
    delete wifiList;
    delete wifiPassword;
}

void WiFiManager::begin() {
    // Start with scanning WiFi
    currentState = WIFI_STATE_SCAN;
    Serial.println("WiFi Manager: Starting WiFi scan...");
    wifiList->scanNetworks();
    
    // Đợi scan hoàn thành (scanNetworks có thể mất vài giây)
    delay(3000);  // Đợi 3 giây để scan hoàn thành
    
    currentState = WIFI_STATE_SELECT;
    Serial.print("WiFi Manager: Network count = ");
    Serial.println(wifiList->getNetworkCount());
    
    // Không chọn WiFi ngay, để mặc định chọn item đầu tiên trong lần vẽ đầu tiên
    // (selectedIndex đã được khởi tạo = 0 trong constructor)
    
    // Vẽ danh sách WiFi
    delay(200);
    wifiList->draw();
    Serial.println("WiFi Manager: WiFi list drawn and displayed");
    Serial.println("WiFi Manager: Use up/down to navigate, select to choose WiFi");
}

void WiFiManager::onWiFiSelected() {
    selectedSSID = wifiList->getSelectedSSID();
    Serial.println("========================================");
    Serial.print("WiFi Manager: Selected SSID: ");
    Serial.println(selectedSSID);
    Serial.print("WiFi Manager: Selected RSSI: ");
    Serial.println(wifiList->getSelectedRSSI());
    Serial.print("WiFi Manager: Selected Index: ");
    Serial.println(wifiList->getSelectedIndex());
    Serial.println("========================================");
    
    // Always move to password screen when WiFi is selected
    currentState = WIFI_STATE_PASSWORD;
    
    // Clear password and show password screen
    wifiPassword->clearPassword();
    wifiPassword->draw();
    
    Serial.print("WiFi Manager: Ready to type password for: ");
    Serial.println(selectedSSID);
    Serial.println("WiFi Manager: Use keyboard to enter password, press Enter to connect");
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
    tft->fillScreen(ST77XX_BLACK);
    tft->setTextColor(YELLOW_ORANGE, ST77XX_BLACK);  // Yellow-orange sunset color
    tft->setTextSize(1);
    tft->setCursor(10, 50);
    tft->print("Connecting...");
}

void WiFiManager::update() {
    if (currentState == WIFI_STATE_CONNECTING) {
        // Check connection status
        if (WiFi.status() == WL_CONNECTED) {
            currentState = WIFI_STATE_CONNECTED;
            Serial.println("WiFi Manager: Connected successfully!");
            Serial.println("WiFi Manager: Transitioning to game menu...");
            // Note: Game menu will be drawn by main.cpp after checking isConnected()
        } else if (millis() - connectStartTime > 10000) {
            // Timeout after 10 seconds
            currentState = WIFI_STATE_ERROR;
            Serial.println("WiFi Manager: Connection timeout");
            
            // Show error message
            tft->fillScreen(ST77XX_BLACK);
            tft->setTextColor(ST77XX_RED, ST77XX_BLACK);
            tft->setTextSize(1);
            tft->setCursor(10, 50);
            tft->print("Connection failed!");
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

