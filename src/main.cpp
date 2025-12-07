#include <Arduino.h>
#include <TFT_eSPI.h>
#include "keyboard.h"
#include "wifi_password.h"
#include "wifi_list.h"
#include "wifi_manager.h"

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite sprite = TFT_eSprite(&tft);
Keyboard* keyboard;  // Con trỏ tới đối tượng Keyboard
WiFiManager* wifiManager;  // WiFi Manager để quản lý flow

// Callback function khi nhấn phím trên bàn phím
void onKeyboardKeySelected(String key) {
    // Pass to WiFi manager
    wifiManager->handleKeyboardInput(key);
}

void setup() {
    Serial.begin(115200);
    
    // Khởi tạo màn hình TFT
    tft.init();
    tft.setRotation(0);
    tft.fillScreen(TFT_BLACK);
    
    // Tạo sprite với kích thước 128x128
    sprite.createSprite(128, 128);
    sprite.fillScreen(TFT_BLACK);
    
    // Thiết lập font mặc định
    sprite.setTextSize(1);
    sprite.setTextColor(TFT_WHITE, TFT_BLACK);
    
    // Khởi tạo bàn phím (tạo đối tượng Keyboard)
    keyboard = new Keyboard(&sprite);
    
    // Thiết lập callback cho bàn phím
    keyboard->setOnKeySelectedCallback(onKeyboardKeySelected);
    
    // Khởi tạo WiFi Manager (quản lý toàn bộ flow)
    wifiManager = new WiFiManager(&sprite, keyboard);
    wifiManager->begin();
    
    Serial.println("WiFi Manager initialized!");
    Serial.println("Flow: Scan -> Select 'Van Ninh' -> Enter password '123456a@' -> Connect");
}

void loop() {
    // TODO: Thay thế logic này bằng input thực tế từ joystick/nút bấm
    
    // Ví dụ: Di chuyển con trỏ bằng các lệnh
    // keyboard->moveCursorByCommand("up", 0, 0);
    // keyboard->moveCursorByCommand("down", 0, 0);
    // keyboard->moveCursorByCommand("left", 0, 0);
    // keyboard->moveCursorByCommand("right", 0, 0);
    
    // Ví dụ: Nhấn phím select để nhập ký tự
    // keyboard->moveCursorByCommand("select", 0, 0);
    
    // Ví dụ: Điều hướng danh sách WiFi
    // wifiListScreen->selectNext();
    // wifiListScreen->selectPrevious();
    
    // Update WiFi manager state machine
    wifiManager->update();
    
    // TODO: Thay thế logic này bằng input thực tế từ joystick/nút bấm
    // Ví dụ:
    // if (buttonUp) wifiManager->handleUp();
    // if (buttonDown) wifiManager->handleDown();
    // if (buttonSelect) wifiManager->handleSelect();
    
    // Auto flow: If in password screen and password is ready, press Enter to connect
    static unsigned long lastAction = 0;
    
    if (millis() - lastAction > 3000) {  // Mỗi 3 giây
        WiFiState state = wifiManager->getState();
        
        if (state == WIFI_STATE_PASSWORD) {
            // Password is auto-filled for "Van Ninh", press Enter to connect
            // Find Enter key on keyboard and press it
            // Enter key is at position row 2, col 8 (|e)
            keyboard->moveCursorByCommand("select", 0, 0);  // This will trigger Enter if cursor is on Enter key
            lastAction = millis();
        }
    }
    
    delay(10);
}
