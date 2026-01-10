#include <Arduino.h>
#include "wifi_manager.h"
#include "socket_manager.h"
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPIFFS.h>

// Synthwave/Vaporwave color palette (softened, less harsh)
#define NEON_PURPLE 0xB81A        // Soft purple (reduced brightness)
#define NEON_GREEN 0x05A0         // Soft green (reduced brightness)
#define NEON_CYAN 0x05DF          // Soft cyan (reduced brightness)
#define YELLOW_ORANGE 0xFC00      // Soft yellow-orange (reduced brightness)
#define SOFT_WHITE 0xCE79         // Soft white (not too bright)

// Static instance pointer for callback wrapper
static WiFiManager* g_wifiManagerInstance = nullptr;

// Static wrapper function for Enter key callback
static void onEnterPressedWrapper() {
    if (g_wifiManagerInstance != nullptr) {
        g_wifiManagerInstance->triggerPasswordEntered();
    }
}

// Helper function to sanitize SSID for filename (remove invalid characters)
String sanitizeSSIDForFilename(const String& ssid) {
    String sanitized = ssid;
    // Replace invalid characters with underscore
    sanitized.replace(" ", "_");
    sanitized.replace("/", "_");
    sanitized.replace("\\", "_");
    sanitized.replace(":", "_");
    sanitized.replace("*", "_");
    sanitized.replace("?", "_");
    sanitized.replace("\"", "_");
    sanitized.replace("<", "_");
    sanitized.replace(">", "_");
    sanitized.replace("|", "_");
    return sanitized;
}

WiFiManager::WiFiManager(Adafruit_ST7789* tft, Keyboard* keyboard) {  // Sử dụng keyboard thường
    this->tft = tft;
    this->keyboard = keyboard;
    this->currentState = WIFI_STATE_SCAN;
    this->selectedSSID = "";
    this->password = "";
    this->connectStartTime = 0;
    this->connectAttempts = 0;
    
    // Set static instance pointer
    g_wifiManagerInstance = this;
    
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
    
    // Ensure first item is selected after scan
    if (wifiList->getNetworkCount() > 0) {
        wifiList->selectIndex(0);
        Serial.println("WiFi Manager: First WiFi item selected");
    }
    
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
    
    // Try to load saved password from file
    String savedPassword = loadWiFiPassword();
    if (savedPassword.length() > 0) {
        Serial.print("WiFi Manager: Found saved password for ");
        Serial.println(selectedSSID);
        wifiPassword->setPassword(savedPassword);
        password = savedPassword;  // Also set in WiFiManager
    } else {
        Serial.print("WiFi Manager: No saved password found for ");
        Serial.println(selectedSSID);
        wifiPassword->clearPassword();
    }
    
    // Set callback for Enter key press in password screen
    wifiPassword->setOnEnterPressedCallback(onEnterPressedWrapper);
    
    wifiPassword->draw();
    
    Serial.print("WiFi Manager: Ready to type password for: ");
    Serial.println(selectedSSID);
    Serial.println("WiFi Manager: Use keyboard to enter password, press Enter to connect");
}

void WiFiManager::onPasswordEntered() {
    password = wifiPassword->getPassword();
    
    // Trim password to remove leading/trailing whitespace
    password.trim();
    
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
    delay(500);  // Tăng delay để đảm bảo disconnect hoàn tất
    
    // Start connection
    WiFi.mode(WIFI_STA);
    delay(100);
    
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
            
            // Save WiFi password to file
            saveWiFiPassword();
            
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
            // Connection failed immediately - quay lại màn hình password để nhập lại
            Serial.print("WiFi Manager: Connection failed immediately! Status: ");
            Serial.print(status);
            Serial.print(" (");
            if (status == WL_CONNECT_FAILED) Serial.print("CONNECT_FAILED");
            else if (status == WL_NO_SSID_AVAIL) Serial.print("NO_SSID_AVAIL");
            Serial.println(")");
            Serial.println("WiFi Manager: Possible causes: Wrong password, SSID not found, or signal too weak");
            Serial.println("WiFi Manager: Returning to password screen to retry...");
            
            // Quay lại màn hình password để nhập lại
            currentState = WIFI_STATE_PASSWORD;
            // Keep the entered password so user can edit it (don't wipe on failure)
            wifiPassword->setOnEnterPressedCallback(onEnterPressedWrapper);  // Set lại callback
            wifiPassword->draw();  // Hiển thị lại màn hình password
        } else if (elapsed > 20000) {
            // Timeout after 20 seconds - quay lại màn hình password để nhập lại
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
            Serial.println("WiFi Manager: Returning to password screen to retry...");
            
            // Quay lại màn hình password để nhập lại
            currentState = WIFI_STATE_PASSWORD;
            // Keep the entered password so user can edit it (don't wipe on failure/timeout)
            wifiPassword->setOnEnterPressedCallback(onEnterPressedWrapper);  // Set lại callback
            wifiPassword->draw();  // Hiển thị lại màn hình password
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

void WiFiManager::handleLeft() {
    // Left navigation - currently not used in WiFi selection
    // Reserved for future use (e.g., horizontal navigation in password screen)
}

void WiFiManager::handleRight() {
    // Right navigation - currently not used in WiFi selection
    // Reserved for future use (e.g., horizontal navigation in password screen)
}

void WiFiManager::handleExit() {
    Serial.println("WiFi Manager: Exit key pressed");
    
    switch (currentState) {
        case WIFI_STATE_SELECT:
        case WIFI_STATE_SCAN:
            // In WiFi list - exit does nothing (WiFi selection is required)
            // Could optionally reset to scan state, but for now do nothing
            Serial.println("WiFi Manager: Exit in SELECT/SCAN state - no action (WiFi selection required)");
            break;
            
        case WIFI_STATE_PASSWORD:
            // Go back to WiFi list selection
            Serial.println("WiFi Manager: Exit from password screen - returning to WiFi list");
            currentState = WIFI_STATE_SELECT;
            // Ensure selection is maintained and visible
            if (wifiList->getNetworkCount() > 0) {
                uint16_t currentIndex = wifiList->getSelectedIndex();
                wifiList->selectIndex(currentIndex);  // Re-select current item to ensure it's marked
            }
            wifiList->draw();
            break;
            
        case WIFI_STATE_CONNECTING:
            // Cancel connection attempt, return to password screen
            Serial.println("WiFi Manager: Exit during connection - canceling and returning to password screen");
            WiFi.disconnect();
            currentState = WIFI_STATE_PASSWORD;
            wifiPassword->draw();
            break;
            
        case WIFI_STATE_CONNECTED:
            // Already connected - exit does nothing (or could disconnect)
            Serial.println("WiFi Manager: Exit in CONNECTED state - no action");
            break;
            
        case WIFI_STATE_ERROR:
            // Return to WiFi list selection
            Serial.println("WiFi Manager: Exit from error state - returning to WiFi list");
            currentState = WIFI_STATE_SELECT;
            // Ensure selection is maintained and visible
            if (wifiList->getNetworkCount() > 0) {
                uint16_t currentIndex = wifiList->getSelectedIndex();
                wifiList->selectIndex(currentIndex);  // Re-select current item to ensure it's marked
            }
            wifiList->draw();
            break;
    }
}

void WiFiManager::handleKeyboardInput(String key) {
    // If in password state, route all keys to WiFiPasswordScreen (including navigation)
    if (currentState == WIFI_STATE_PASSWORD) {
        // Route all keys (including "|e" Enter key) to WiFiPasswordScreen
        // WiFiPasswordScreen will handle Enter key to connect
        wifiPassword->handleKeyPress(key);
        return;
    }
    
    // Handle new navigation key format for other states
    if (key == "up") {
        handleUp();
        return;
    } else if (key == "down") {
        handleDown();
        return;
    } else if (key == "left") {
        handleLeft();
        return;
    } else if (key == "right") {
        handleRight();
        return;
    } else if (key == "select") {
        handleSelect();
        return;
    } else if (key == "exit") {
        handleExit();
        return;
    }
    
    // Backward compatibility: handle old key format
    if (key == "|u") {
        handleUp();
        return;
    } else if (key == "|d") {
        handleDown();
        return;
    } else if (key == "|l") {
        handleLeft();
        return;
    } else if (key == "|r") {
        handleRight();
        return;
    } else if (key == "|e") {
        handleSelect();
        return;
    } else if (key == "<" || key == "|b") {
        handleExit();
        return;
    }
}

void WiFiManager::disconnect() {
    WiFi.disconnect();
    currentState = WIFI_STATE_SCAN;
    selectedSSID = "";
    password = "";
}

// Function to save WiFi password to file
void WiFiManager::saveWiFiPassword() {
    // Initialize SPIFFS if not already initialized
    if (!SPIFFS.begin(true)) {
        Serial.println("WiFi Manager: SPIFFS initialization failed!");
        return;
    }
    
    // Sanitize SSID for filename
    String sanitizedSSID = sanitizeSSIDForFilename(selectedSSID);
    String fileName = "/wifi_";
    fileName += sanitizedSSID;
    fileName += ".txt";
    
    Serial.print("WiFi Manager: Saving password to file: ");
    Serial.println(fileName);
    
    // Open file for writing
    File file = SPIFFS.open(fileName, "w");
    if (!file) {
        Serial.print("WiFi Manager: Failed to open file for writing: ");
        Serial.println(fileName);
        return;
    }
    
    // Write password to file
    file.println(password);
    file.close();
    
    Serial.print("WiFi Manager: Password saved successfully to: ");
    Serial.println(fileName);
}

// Function to load WiFi password from file
String WiFiManager::loadWiFiPassword() {
    // Initialize SPIFFS if not already initialized
    if (!SPIFFS.begin(true)) {
        Serial.println("WiFi Manager: SPIFFS initialization failed for reading!");
        return "";
    }
    
    // Sanitize SSID for filename
    String sanitizedSSID = sanitizeSSIDForFilename(selectedSSID);
    String fileName = "/wifi_";
    fileName += sanitizedSSID;
    fileName += ".txt";
    
    Serial.print("WiFi Manager: Trying to load password from file: ");
    Serial.println(fileName);
    
    // Check if file exists
    if (!SPIFFS.exists(fileName)) {
        Serial.println("WiFi Manager: Password file does not exist");
        return "";
    }
    
    // Open file for reading
    File file = SPIFFS.open(fileName, "r");
    if (!file) {
        Serial.print("WiFi Manager: Failed to open file for reading: ");
        Serial.println(fileName);
        return "";
    }
    
    // Read password from file (first line)
    String loadedPassword = file.readStringUntil('\n');
    loadedPassword.trim();  // Remove trailing newline and whitespace
    file.close();
    
    Serial.print("WiFi Manager: Password loaded successfully (length: ");
    Serial.print(loadedPassword.length());
    Serial.println(")");
    
    return loadedPassword;
}

