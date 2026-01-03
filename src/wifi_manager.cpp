#include <Arduino.h>
#include "wifi_manager.h"
#include "socket_manager.h"
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
    Serial.print("WiFi Manager: Password length: ");
    Serial.println(password.length());
    
    // Debug: Print each character in password (hex values)
    Serial.print("WiFi Manager: Password bytes (hex): ");
    for (int i = 0; i < password.length(); i++) {
        Serial.print("0x");
        if (password.charAt(i) < 0x10) Serial.print("0");
        Serial.print((int)password.charAt(i), HEX);
        Serial.print(" ");
    }
    Serial.println();
    
    // Debug: Print each character as readable
    Serial.print("WiFi Manager: Password chars: ");
    for (int i = 0; i < password.length(); i++) {
        char c = password.charAt(i);
        if (c >= 32 && c <= 126) {
            Serial.print("'");
            Serial.print(c);
            Serial.print("' ");
        } else {
            Serial.print("[");
            Serial.print((int)c);
            Serial.print("] ");
        }
    }
    Serial.println();
    
    // Start connecting
    currentState = WIFI_STATE_CONNECTING;
    connectToWiFi();
}

void WiFiManager::connectToWiFi() {
    Serial.print("WiFi Manager: Connecting to ");
    Serial.print(selectedSSID);
    Serial.print(" with password: ");
    Serial.println(password);
    Serial.print("WiFi Manager: SSID length: ");
    Serial.println(selectedSSID.length());
    Serial.print("WiFi Manager: Password length: ");
    Serial.println(password.length());
    
    // Disconnect if already connected
    WiFi.disconnect();
    delay(100);
    
    // Start connection
    WiFi.mode(WIFI_STA);
    
    // Debug: Verify password string before passing to WiFi.begin
    const char* ssidPtr = selectedSSID.c_str();
    const char* pwdPtr = password.c_str();
    Serial.print("WiFi Manager: SSID c_str: ");
    Serial.println(ssidPtr);
    Serial.print("WiFi Manager: Password c_str: ");
    Serial.println(pwdPtr);
    
    WiFi.begin(ssidPtr, pwdPtr);
    
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
        wl_status_t status = WiFi.status();
        unsigned long elapsed = millis() - connectStartTime;
        
        // Debug: Log status mỗi 2 giây để theo dõi
        static unsigned long lastStatusLog = 0;
        if (millis() - lastStatusLog > 2000) {
            Serial.print("WiFi Manager: Status check - elapsed=");
            Serial.print(elapsed);
            Serial.print("ms, WiFi.status()=");
            Serial.print(status);
            Serial.print(" (");
            switch(status) {
                case WL_IDLE_STATUS: Serial.print("IDLE"); break;
                case WL_NO_SSID_AVAIL: Serial.print("NO_SSID_AVAIL"); break;
                case WL_SCAN_COMPLETED: Serial.print("SCAN_COMPLETED"); break;
                case WL_CONNECTED: Serial.print("CONNECTED"); break;
                case WL_CONNECT_FAILED: Serial.print("CONNECT_FAILED"); break;
                case WL_CONNECTION_LOST: Serial.print("CONNECTION_LOST"); break;
                case WL_DISCONNECTED: Serial.print("DISCONNECTED"); break;
                default: Serial.print("UNKNOWN"); break;
            }
            Serial.println(")");
            lastStatusLog = millis();
        }
        
        // Check connection status
        if (status == WL_CONNECTED) {
            currentState = WIFI_STATE_CONNECTED;
            Serial.println("WiFi Manager: Connected successfully!");
            Serial.print("WiFi Manager: IP Address: ");
            Serial.println(WiFi.localIP());
            Serial.println("WiFi Manager: Transitioning to login screen immediately...");
            
            // Initialize socket session after WiFi connection
            extern SocketManager* socketManager;  // Forward declaration
            if (socketManager != nullptr && !socketManager->isInitialized()) {
                Serial.println("WiFi Manager: Initializing socket session...");
                // Update with your server host IP (use 192.168.1.7 for local network)
                socketManager->begin("192.168.1.7", 8080, "/ws");
            }
            
            // Skip "Connected!" screen - main.cpp will check isConnected() and transition immediately
            // Note: Login screen will be drawn by main.cpp after checking isConnected()
        } else if (status == WL_CONNECT_FAILED || status == WL_NO_SSID_AVAIL) {
            // Connection failed immediately - không cần đợi timeout
            currentState = WIFI_STATE_ERROR;
            Serial.print("WiFi Manager: Connection failed immediately! Status: ");
            Serial.print(status);
            Serial.print(" (");
            if (status == WL_CONNECT_FAILED) Serial.print("CONNECT_FAILED");
            else if (status == WL_NO_SSID_AVAIL) Serial.print("NO_SSID_AVAIL");
            Serial.println(")");
            Serial.println("WiFi Manager: Possible causes: Wrong password, SSID not found, or signal too weak");
            
            // Show error message
            tft->fillScreen(ST77XX_BLACK);
            tft->setTextColor(ST77XX_RED, ST77XX_BLACK);
            tft->setTextSize(1);
            tft->setCursor(10, 50);
            tft->print("Connection failed!");
            tft->setCursor(10, 70);
            tft->print("Wrong password?");
        } else if (elapsed > 8000) {
            // Timeout after 8 seconds (giảm từ 10s)
            currentState = WIFI_STATE_ERROR;
            Serial.print("WiFi Manager: Connection timeout after ");
            Serial.print(elapsed);
            Serial.println("ms");
            Serial.print("WiFi Manager: Final status: ");
            Serial.print(status);
            Serial.print(" (");
            switch(status) {
                case WL_IDLE_STATUS: Serial.print("IDLE"); break;
                case WL_NO_SSID_AVAIL: Serial.print("NO_SSID_AVAIL"); break;
                case WL_SCAN_COMPLETED: Serial.print("SCAN_COMPLETED"); break;
                case WL_CONNECTED: Serial.print("CONNECTED"); break;
                case WL_CONNECT_FAILED: Serial.print("CONNECT_FAILED"); break;
                case WL_CONNECTION_LOST: Serial.print("CONNECTION_LOST"); break;
                case WL_DISCONNECTED: Serial.print("DISCONNECTED"); break;
                default: Serial.print("UNKNOWN"); break;
            }
            Serial.println(")");
            
            // Show error message
            tft->fillScreen(ST77XX_BLACK);
            tft->setTextColor(ST77XX_RED, ST77XX_BLACK);
            tft->setTextSize(1);
            tft->setCursor(10, 50);
            tft->print("Connection timeout!");
            tft->setCursor(10, 70);
            tft->print("Check password");
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

