#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>
#include "wifi_manager.h"
#include "keyboard.h"
#include "keyboard_skins_wrapper.h"  // Include wrapper để có KeyboardSkins namespace

// ST7789 pins
#define TFT_CS    15
#define TFT_DC    2
#define TFT_RST   4
#define TFT_MOSI  19
#define TFT_SCLK  18

Adafruit_ST7789 tft = Adafruit_ST7789(&SPI, TFT_CS, TFT_DC, TFT_RST);
Keyboard* keyboard;  // Sử dụng keyboard thường
WiFiManager* wifiManager;
bool testMode = true;  // Bật chế độ test
bool testStarted = false;

// Callback function cho keyboard input
void onKeyboardKeySelected(String key) {
    if (wifiManager != nullptr) {
        wifiManager->handleKeyboardInput(key);
    }
}

// Hàm di chuyển keyboard và chọn từng ký tự
// Bảng phím qwertyKeysArray:
// row 0: q w e r t y u i o p
// row 1: 123 a s d f g h j k l
// row 2: ic z x c v b n m |e <
void typePasswordByMovingKeyboard(String password) {
    // Đảm bảo đang ở chế độ chữ cái
    if (!keyboard->getIsAlphabetMode()) {
        keyboard->moveCursorTo(1, 0);  // Di chuyển đến "ABC"
        delay(300);
        keyboard->moveCursorByCommand("select", 0, 0);  // Chuyển sang chế độ chữ cái
        delay(300);
    }
    
    // Bảng phím chữ cái (qwertyKeysArray)
    String qwertyKeys[3][10] = {
        { "q", "w", "e", "r", "t", "y", "u", "i", "o", "p" },
        { "12", "a", "s", "d", "f", "g", "h", "j", "k", "l"},
        { "ic", "z", "x", "c", "v", "b", "n", "m", "|e", "<"}
    };
    
    Serial.println("TEST: Starting to type password by moving keyboard cursor...");
    
    for (uint16_t i = 0; i < password.length(); i++) {
        char targetChar = password.charAt(i);
        Serial.print("TEST: Looking for character '");
        Serial.print(targetChar);
        Serial.print("' (ASCII: ");
        Serial.print((int)targetChar);
        Serial.println(")");
        bool found = false;
        
        // Tìm ký tự trong bảng phím
        for (uint16_t row = 0; row < 3; row++) {
            for (uint16_t col = 0; col < 10; col++) {
                String key = qwertyKeys[row][col];
                Serial.print("  Checking key '");
                Serial.print(key);
                Serial.print("' at row ");
                Serial.print(row);
                Serial.print(", col ");
                Serial.print(col);
                Serial.print(" (length: ");
                Serial.print(key.length());
                Serial.print(")");
                
                // Chỉ tìm các phím có 1 ký tự (bỏ qua "123", "ABC", "|e", "<", "ic")
                if (key.length() == 1 && key.charAt(0) == targetChar) {
                    Serial.println(" -> MATCH!");
                    Serial.print("TEST: Moving to key '");
                    Serial.print(targetChar);
                    Serial.print("' at row ");
                    Serial.print(row);
                    Serial.print(", col ");
                    Serial.println(col);
                    
                    // Di chuyển con trỏ đến phím
                    keyboard->moveCursorTo(row, col);
                    delay(400);  // Delay để thấy con trỏ di chuyển rõ ràng
                    
                    // Nhấn select
                    Serial.print("TEST: Selecting key '");
                    Serial.println(targetChar);
                    keyboard->moveCursorByCommand("select", 0, 0);
                    delay(300);  // Delay sau khi nhấn select
                    
                    found = true;
                    break;
                } else {
                    Serial.println(" -> no match");
                }
            }
            if (found) break;
        }
        
        if (!found) {
            Serial.print("TEST: Key '");
            Serial.print(targetChar);
            Serial.println("' not found in keyboard");
        }
    }
    
    Serial.println("TEST: Finished typing password");
}

// Hàm test tự động chọn WiFi "Hai Dung" và nhập password
void testAutoConnect() {
    if (wifiManager == nullptr || !testMode) return;
    
    // Chờ scan xong
    if (wifiManager->getState() != WIFI_STATE_SELECT) return;
    
    WiFiListScreen* wifiList = wifiManager->getListScreen();
    if (wifiList == nullptr) return;
    
    // Tìm WiFi "Hai Dung" bằng cách di chuyển từng bước từ trên xuống
    bool foundHaiDung = false;
    
    // Đảm bảo bắt đầu từ đầu danh sách (index 0)
    while (wifiList->getSelectedIndex() != 0) {
        wifiList->selectPrevious();
        delay(100);  // Delay nhỏ để thấy di chuyển
    }
    
    // Di chuyển từng bước từ trên xuống để tìm "Hai Dung"
    for (uint16_t i = 0; i < wifiList->getNetworkCount(); i++) {
        String ssid = wifiList->getSelectedSSID();
        Serial.print("  Checking [");
        Serial.print(wifiList->getSelectedIndex());
        Serial.print("]: ");
        Serial.println(ssid);
        
        if (ssid == "Hai Dung") {
            foundHaiDung = true;
            Serial.println("========================================");
            Serial.println("TEST: Found 'Hai Dung'! Auto-selecting...");
            Serial.println("========================================");
            break;
        }
        
        // Di chuyển xuống item tiếp theo (nếu chưa phải item cuối)
        if (i < wifiList->getNetworkCount() - 1) {
            wifiList->selectNext();
            delay(200);  // Delay để thấy di chuyển và kiểm tra nháy
        }
    }
    
    if (foundHaiDung) {
        // Chọn WiFi "Hai Dung"
        delay(1000);
        wifiManager->handleSelect();  // Chuyển sang màn hình password
        
        // Đợi chuyển sang màn hình password và vẽ xong
        delay(1000);
        
        // Tự động nhập password "hoilamgi" bằng cách di chuyển keyboard
        Serial.println("TEST: Auto-typing password 'hoilamgi' by moving keyboard...");
        typePasswordByMovingKeyboard("hoilamgi");
        
        Serial.println("TEST: Password typed, waiting before auto-connecting...");
        delay(2000);  // Tăng delay để đảm bảo ký tự cuối cùng được hiển thị
        
        // Tự động nhấn Enter để kết nối
        Serial.println("TEST: Pressing Enter to connect...");
        wifiManager->handleKeyboardInput("|e");
        
        testStarted = true;
    } else {
        Serial.println("TEST: 'Hai Dung' not found in WiFi list");
    }
}

void setup() {
    Serial.begin(115200);
    delay(200);

    SPI.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);
    SPI.setFrequency(27000000);

    tft.init(240, 320);     // Official Adafruit init
    tft.setRotation(3);  // Xoay -90 độ (rotation 3: 320x240, gốc ở góc dưới trái)
    tft.fillScreen(ST77XX_BLACK);

    Serial.println("Display initialized: 240x320");
    Serial.println("WiFi Manager: Starting...");
    
    // Khởi tạo Keyboard thường (vẽ trực tiếp lên tft, không dùng canvas)
    keyboard = new Keyboard(&tft);
    
    // TEST: Áp dụng skin màu hồng (feminine)
    Serial.println("TEST: Applying feminine (mint) skin to keyboard...");
    keyboard->setSkin(KeyboardSkins::getFeminineLilac());
    Serial.println("TEST: Feminine (mint) skin applied to keyboard!");
    
    // Để test skin khác, uncomment:
    // keyboard->setSkin(KeyboardSkins::getCyberpunkTron());
    // keyboard->setSkin(KeyboardSkins::getMechanicalSteampunk());
    // keyboard->setSkin(KeyboardSkins::getFeminineRose());      // Hoa hồng
    // keyboard->setSkin(KeyboardSkins::getFeminineLavender());  // Oải hương
    // keyboard->setSkin(KeyboardSkins::getFemininePeach());     // Đào
    // keyboard->setSkin(KeyboardSkins::getFeminineCoral());     // San hô
    // keyboard->setSkin(KeyboardSkins::getFeminineMint());      // Bạc hà
    // keyboard->setSkin(KeyboardSkins::getFeminineLilac());     // Tím hoa cà
    // keyboard->setSkin(KeyboardSkins::getFeminineBlush());     // Đỏ hồng nhạt
    // keyboard->setSkin(KeyboardSkins::getFeminineCream());     // Kem
    // keyboard->setSkin(KeyboardSkins::getFeminineSage());     // Xanh lá cây nhạt
    
    // Thiết lập callback cho keyboard
    keyboard->setOnKeySelectedCallback(onKeyboardKeySelected);
    
    // Khởi tạo WiFi Manager
    wifiManager = new WiFiManager(&tft, keyboard);
    
    Serial.println("WiFi Manager initialized!");
    
    // Bắt đầu quy trình scan WiFi
    wifiManager->begin();
    
    if (testMode) {
        Serial.println("TEST MODE: Will auto-select 'Hai Dung' and enter password 'hoilamgi'");
    }
}

void loop() {
    // Cập nhật trạng thái WiFi Manager (kiểm tra kết nối)
    if (wifiManager != nullptr) {
        wifiManager->update();
        
        // Test mode: tự động chọn WiFi "Hai Dung" và nhập password
        if (testMode && !testStarted && wifiManager->getState() == WIFI_STATE_SELECT) {
            testAutoConnect();
        }
    }
    
    delay(100);
}
