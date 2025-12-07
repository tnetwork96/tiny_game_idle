#include <Arduino.h>
#include <TFT_eSPI.h>
#include "keyboard.h"
#include "wifi_password.h"
#include "wifi_list.h"

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite sprite = TFT_eSprite(&tft);
Keyboard* keyboard;  // Con trỏ tới đối tượng Keyboard
WiFiPasswordScreen* wifiScreen;  // Con trỏ tới màn hình nhập mật khẩu WiFi
WiFiListScreen* wifiListScreen;  // Con trỏ tới màn hình danh sách WiFi

// Callback function khi nhấn phím trên bàn phím
void onKeyboardKeySelected(String key) {
    wifiScreen->handleKeyPress(key);
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
    
    // Khởi tạo màn hình nhập mật khẩu WiFi
    wifiScreen = new WiFiPasswordScreen(&sprite, keyboard);
    
    // Khởi tạo màn hình danh sách WiFi
    wifiListScreen = new WiFiListScreen(&sprite);
    
    // Thiết lập callback cho bàn phím để xử lý khi nhấn phím
    keyboard->setOnKeySelectedCallback(onKeyboardKeySelected);
    
    // Scan và hiển thị danh sách WiFi
    wifiListScreen->scanNetworks();
    wifiListScreen->draw();
    
    Serial.println("WiFi List Screen initialized!");
    Serial.print("Found ");
    Serial.print(wifiListScreen->getNetworkCount());
    Serial.println(" networks");
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
    
    // Ví dụ test: Di chuyển trong danh sách WiFi tự động (để test)
    static unsigned long lastAction = 0;
    static bool scanDone = false;
    
    if (!scanDone && millis() > 3000) {
        // Scan lại sau 3 giây
        wifiListScreen->scanNetworks();
        wifiListScreen->draw();
        scanDone = true;
    }
    
    if (millis() - lastAction > 2000) {  // Mỗi 2 giây
        wifiListScreen->selectNext();
        lastAction = millis();
    }
    
    delay(10);
}
