#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>
#include "wifi_manager.h"
#include "keyboard.h"
#include "keyboard_skins_wrapper.h"  // Include wrapper để có KeyboardSkins namespace
#include "game_menu.h"
#include "gunny_game.h"
#include "caro_game.h"
#include "billiard_game.h"

// ST7789 pins
#define TFT_CS    15
#define TFT_DC    2
#define TFT_RST   4
#define TFT_MOSI  19
#define TFT_SCLK  18

Adafruit_ST7789 tft = Adafruit_ST7789(&SPI, TFT_CS, TFT_DC, TFT_RST);
Keyboard* keyboard;  // Sử dụng keyboard thường
WiFiManager* wifiManager;
GameMenuScreen* gameMenu;
GunnyGame* gunnyGame;
CaroGame* caroGame;
BilliardGame* billiardGame;
bool testMode = true;  // Bật chế độ test
bool testStarted = false;
bool menuShown = false;  // Đánh dấu đã hiển thị menu chưa
bool inGame = false;  // Đánh dấu đang trong game
int currentGame = 0;  // 0 = none, 1 = gunny, 2 = caro, 3 = billiard

// Callback function cho keyboard input
void onKeyboardKeySelected(String key) {
    if (wifiManager != nullptr) {
        wifiManager->handleKeyboardInput(key);
    }
}

// Hàm di chuyển keyboard và chọn từng ký tự
// Bảng phím chữ cái (qwertyKeysArray):
// row 0: q w e r t y u i o p
// row 1: 123 a s d f g h j k l
// row 2: ic z x c v b n m |e <
// Bảng phím số (numericKeysArray):
// row 0: 1 2 3 4 5 6 7 8 9 0
// row 1: ABC / : ; ( ) $ & @ "
// row 2: ic # . , ? ! ' - |e <
void typePasswordByMovingKeyboard(String password) {
    // Bảng phím chữ cái (qwertyKeysArray)
    String qwertyKeys[3][10] = {
        { "q", "w", "e", "r", "t", "y", "u", "i", "o", "p" },
        { "123", "a", "s", "d", "f", "g", "h", "j", "k", "l"},
        { "ic", "z", "x", "c", "v", "b", "n", "m", "|e", "<"}
    };
    
    // Bảng phím số và ký tự đặc biệt (numericKeysArray)
    String numericKeys[3][10] = {
        { "1", "2", "3", "4", "5", "6", "7", "8", "9", "0" },
        { "ABC", "/", ":", ";", "(", ")", "$", "&", "@", "\"" },
        { "ic", "#", ".", ",", "?", "!", "'", "-", "|e", "<"}
    };
    
    Serial.println("TEST: Starting to type password by moving keyboard cursor...");
    
    for (uint16_t i = 0; i < password.length(); i++) {
        char targetChar = password.charAt(i);
        Serial.print("TEST: Looking for character '");
        Serial.print(targetChar);
        Serial.print("' (ASCII: ");
        Serial.print((int)targetChar);
        Serial.println(")");
        
        // Xác định ký tự là số, chữ cái hay ký tự đặc biệt
        bool isDigit = (targetChar >= '0' && targetChar <= '9');
        bool isLetter = (targetChar >= 'a' && targetChar <= 'z');
        bool isSpecial = !isDigit && !isLetter;
        
        // Chuyển đổi chế độ nếu cần
        bool needAlphabetMode = isLetter;
        bool needNumericMode = isDigit || isSpecial;
        
        if (needAlphabetMode && !keyboard->getIsAlphabetMode()) {
            // Chuyển sang chế độ chữ cái
            Serial.println("TEST: Switching to alphabet mode...");
            keyboard->moveCursorTo(1, 0);  // Di chuyển đến "ABC"
            delay(300);
            keyboard->moveCursorByCommand("select", 0, 0);  // Chuyển sang chế độ chữ cái
            delay(300);
        } else if (needNumericMode && keyboard->getIsAlphabetMode()) {
            // Chuyển sang chế độ số
            Serial.println("TEST: Switching to numeric mode...");
            keyboard->moveCursorTo(1, 0);  // Di chuyển đến "123"
            delay(300);
            keyboard->moveCursorByCommand("select", 0, 0);  // Chuyển sang chế độ số
            delay(300);
        }
        
        // Chọn bảng phím phù hợp
        String (*currentKeys)[10];
        if (keyboard->getIsAlphabetMode()) {
            currentKeys = qwertyKeys;
        } else {
            currentKeys = numericKeys;
        }
        
        bool found = false;
        
        // Tìm ký tự trong bảng phím hiện tại
        for (uint16_t row = 0; row < 3; row++) {
            for (uint16_t col = 0; col < 10; col++) {
                String key = currentKeys[row][col];
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

// Hàm test tự động chọn WiFi "Van Ninh" và nhập password
void testAutoConnect() {
    if (wifiManager == nullptr || !testMode) return;
    
    // Chờ scan xong
    if (wifiManager->getState() != WIFI_STATE_SELECT) return;
    
    WiFiListScreen* wifiList = wifiManager->getListScreen();
    if (wifiList == nullptr) return;
    
    // Tìm WiFi "Van Ninh" bằng cách di chuyển từng bước từ trên xuống
    bool foundVanNinh = false;
    
    // Đảm bảo bắt đầu từ đầu danh sách (index 0)
    while (wifiList->getSelectedIndex() != 0) {
        wifiList->selectPrevious();
        delay(100);  // Delay nhỏ để thấy di chuyển
    }
    
    // Di chuyển từng bước từ trên xuống để tìm "Van Ninh"
    for (uint16_t i = 0; i < wifiList->getNetworkCount(); i++) {
        String ssid = wifiList->getSelectedSSID();
        Serial.print("  Checking [");
        Serial.print(wifiList->getSelectedIndex());
        Serial.print("]: ");
        Serial.println(ssid);
        
        if (ssid == "Van Ninh") {
            foundVanNinh = true;
            Serial.println("========================================");
            Serial.println("TEST: Found 'Van Ninh'! Auto-selecting...");
            Serial.println("========================================");
            break;
        }
        
        // Di chuyển xuống item tiếp theo (nếu chưa phải item cuối)
        if (i < wifiList->getNetworkCount() - 1) {
            wifiList->selectNext();
            delay(200);  // Delay để thấy di chuyển và kiểm tra nháy
        }
    }
    
    if (foundVanNinh) {
        // Chọn WiFi "Van Ninh"
        delay(1000);
        wifiManager->handleSelect();  // Chuyển sang màn hình password
        
        // Đợi chuyển sang màn hình password và vẽ xong
        delay(1000);
        
        // Tự động nhập password "123456a@" bằng cách di chuyển keyboard
        Serial.println("TEST: Auto-typing password '123456a@' by moving keyboard...");
        typePasswordByMovingKeyboard("123456a@");
        
        Serial.println("TEST: Password typed, waiting before auto-connecting...");
        delay(2000);  // Tăng delay để đảm bảo ký tự cuối cùng được hiển thị
        
        // Tự động nhấn Enter để kết nối
        Serial.println("TEST: Pressing Enter to connect...");
        wifiManager->handleKeyboardInput("|e");
        
        testStarted = true;
    } else {
        Serial.println("TEST: 'Van Ninh' not found in WiFi list");
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
    
    // Khởi tạo Game Menu
    gameMenu = new GameMenuScreen(&tft);
    
    // Khởi tạo Gunny Game
    gunnyGame = new GunnyGame(&tft);
    
    // Khởi tạo Caro Game
    caroGame = new CaroGame(&tft);
    
    // Khởi tạo Billiard Game
    billiardGame = new BilliardGame(&tft);
    
    // Bắt đầu quy trình scan WiFi
    wifiManager->begin();
    
    if (testMode) {
        Serial.println("TEST MODE: Will auto-select 'Van Ninh' and enter password '123456a@'");
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
        
        // Chuyển sang màn hình menu game sau khi kết nối WiFi thành công
        if (wifiManager->isConnected() && !menuShown && !inGame) {
            Serial.println("Main: WiFi connected, showing game menu...");
            delay(1000);  // Đợi một chút để thấy thông báo "Connected"
            if (gameMenu != nullptr) {
                gameMenu->draw();
                menuShown = true;
                Serial.println("Main: Game menu displayed!");
            }
        }
    }
    
    // Xử lý game menu navigation và game start
    if (menuShown && !inGame && gameMenu != nullptr) {
        gameMenu->update();
        
        // TODO: Handle menu selection (Up/Down buttons, Select button)
        // For now, auto-start games for testing
        static bool gameStarted = false;
        static int testGameIndex = 2;  // Start with Billiard game for testing
        if (!gameStarted) {
            delay(2000);  // Wait 2 seconds on menu
            
            // Test Billiard game first (for testing)
            if (testGameIndex == 2 && billiardGame != nullptr) {
                Serial.println("Main: Starting Billiard game...");
                // Tô đen màn hình trước khi chuyển game
                tft.fillScreen(0x0000);  // Black screen
                delay(100);  // Short delay for screen clear
                billiardGame->init();
                inGame = true;
                menuShown = false;
                currentGame = 3;  // Billiard game
                gameStarted = true;
                testGameIndex = 0;  // Next: Caro
            }
            // Then test Caro game
            else if (testGameIndex == 0 && caroGame != nullptr) {
                Serial.println("Main: Starting Caro game...");
                caroGame->init();
                inGame = true;
                menuShown = false;
                currentGame = 2;  // Caro game
                gameStarted = true;
                testGameIndex = 1;
            }
            // Then test Gunny game
            else if (testGameIndex == 1 && gunnyGame != nullptr) {
                Serial.println("Main: Starting Gunny game...");
                gunnyGame->init();
                inGame = true;
                menuShown = false;
                currentGame = 1;  // Gunny game
                gameStarted = true;
                testGameIndex = 2;  // Reset for next cycle
            }
        }
    }
    
    // Update Caro game if active
    if (inGame && currentGame == 2 && caroGame != nullptr) {
        caroGame->update();
        
        // Test controls: Auto-play Caro game to test win conditions
        static unsigned long lastMoveTime = 0;
        static int moveCount = 0;
        
        if (millis() - lastMoveTime > 500 && caroGame->getGameState() == GAME_PLAYING) {
            // Test different moves on 15x15 board
            // Try to create winning patterns: horizontal, vertical, diagonal
            int pattern = moveCount % 20;
            
            // Pattern 1: Horizontal line (row 7, cols 5-9)
            if (pattern < 5) {
                caroGame->handleLeft();
                caroGame->handleLeft();
                caroGame->handleLeft();
                caroGame->handleLeft();
                caroGame->handleLeft();
                for (int i = 0; i < pattern; i++) caroGame->handleRight();
                caroGame->handleSelect();
            }
            // Pattern 2: Vertical line (col 7, rows 5-9)
            else if (pattern < 10) {
                caroGame->handleUp();
                caroGame->handleUp();
                caroGame->handleUp();
                caroGame->handleUp();
                caroGame->handleUp();
                for (int i = 0; i < (pattern - 5); i++) caroGame->handleDown();
                caroGame->handleSelect();
            }
            // Pattern 3: Diagonal (from 5,5 to 9,9)
            else if (pattern < 15) {
                caroGame->handleUp();
                caroGame->handleUp();
                caroGame->handleUp();
                caroGame->handleUp();
                caroGame->handleUp();
                caroGame->handleLeft();
                caroGame->handleLeft();
                caroGame->handleLeft();
                caroGame->handleLeft();
                caroGame->handleLeft();
                for (int i = 0; i < (pattern - 10); i++) {
                    caroGame->handleDown();
                    caroGame->handleRight();
                }
                caroGame->handleSelect();
            }
            // Pattern 4: Anti-diagonal (from 5,9 to 9,5)
            else {
                caroGame->handleUp();
                caroGame->handleUp();
                caroGame->handleUp();
                caroGame->handleUp();
                caroGame->handleUp();
                caroGame->handleRight();
                caroGame->handleRight();
                caroGame->handleRight();
                caroGame->handleRight();
                caroGame->handleRight();
                for (int i = 0; i < (pattern - 15); i++) {
                    caroGame->handleDown();
                    caroGame->handleLeft();
                }
                caroGame->handleSelect();
            }
            
            moveCount++;
            lastMoveTime = millis();
            
            // If game finished, reset after delay
            if (caroGame->getGameState() != GAME_PLAYING) {
                delay(3000);
                caroGame->init();
                moveCount = 0;
            }
        }
    }
    
    // Update Gunny game if active
    if (inGame && currentGame == 1 && gunnyGame != nullptr) {
        gunnyGame->update();
        
        // Test controls: Simulate fire button press/release for testing
        static unsigned long lastTestTime = 0;
        static bool testCharging = false;
        
        if (millis() - lastTestTime > 3000 && !gunnyGame->getIsFiring()) {
            // Every 3 seconds, test fire
            if (!testCharging) {
                // Start charging
                gunnyGame->handleFirePress();
                testCharging = true;
                Serial.println("Test: Starting to charge...");
                lastTestTime = millis();
            } else {
                // Release after 1 second of charging
                if (millis() - lastTestTime > 1000) {
                    gunnyGame->handleFireRelease();
                    testCharging = false;
                    Serial.println("Test: Firing!");
                    lastTestTime = millis();
                }
            }
        }
        
        // Test angle adjustment (slowly change angle)
        static unsigned long lastAngleTime = 0;
        if (millis() - lastAngleTime > 200 && !gunnyGame->getIsFiring()) {
            gunnyGame->handleAngleUp();
            lastAngleTime = millis();
        }
    }
    
    // Update Billiard game if active
    if (inGame && currentGame == 3 && billiardGame != nullptr) {
        billiardGame->update();
        
        // Test controls: Tự động đánh cho đến khi tất cả quả bi rơi vào lỗ
        static unsigned long lastBilliardTime = 0;
        static unsigned long chargeStartTime = 0;
        static int testStep = 0;
        static bool stepExecuted = false;
        
        unsigned long currentTime = millis();
        
        // Kiểm tra số quả bi còn trên bàn
        int activeBalls = billiardGame->countActiveBalls();
        
        // Nếu chỉ còn 1 quả bi (cue ball) hoặc không còn quả bi nào, reset game
        if (activeBalls <= 1 && billiardGame->getIsAiming() && currentTime - lastBilliardTime > 2000) {
            Serial.print("Billiard Test: All balls pocketed! (");
            Serial.print(activeBalls);
            Serial.println(" balls remaining)");
            Serial.println("Billiard Test: Resetting game...");
            billiardGame->init();
            testStep = 0;
            stepExecuted = false;
            lastBilliardTime = currentTime;
        }
        
        switch (testStep) {
            case 0:
                // Tự động ngắm về quả bi gần nhất
                if (!stepExecuted || currentTime - lastBilliardTime > 500) {
                    billiardGame->aimAtNearestBall();
                    lastBilliardTime = currentTime;
                    stepExecuted = true;
                    if (billiardGame->getIsAiming()) {
                        // Sau khi ngắm, chuyển sang bước tiếp theo
                        static int aimCount = 0;
                        aimCount++;
                        if (aimCount > 2) {  // Ngắm 2 lần để đảm bảo góc đúng
                            testStep = 1;
                            stepExecuted = false;
                            aimCount = 0;
                            lastBilliardTime = currentTime;
                            Serial.print("Billiard Test: Aimed at target. Active balls: ");
                            Serial.println(activeBalls);
                        }
                    }
                }
                break;
            case 1:
                // Bắt đầu charge
                if (!stepExecuted) {
                    billiardGame->handleChargeStart();
                    chargeStartTime = currentTime;
                    stepExecuted = true;
                    Serial.println("Billiard Test: Started charging...");
                }
                // Charge trong 1.5 giây, sau đó bắn
                if (currentTime - chargeStartTime > 1500) {
                    billiardGame->handleChargeRelease();
                    testStep = 2;
                    stepExecuted = false;
                    Serial.println("Billiard Test: Shot fired!");
                    lastBilliardTime = currentTime;
                }
                break;
            case 2:
                // Đợi các quả bi ngừng di chuyển
                if (!stepExecuted) {
                    stepExecuted = true;
                    lastBilliardTime = currentTime;
                }
                // Kiểm tra xem có thể ngắm lại chưa (tất cả quả bi đã dừng)
                if (billiardGame->getIsAiming() && currentTime - lastBilliardTime > 2000) {
                    // Kiểm tra xem còn quả bi nào không
                    activeBalls = billiardGame->countActiveBalls();
                    if (activeBalls > 1) {  // Còn quả bi để đánh
                        Serial.print("Billiard Test: Ready for next shot. Active balls: ");
                        Serial.println(activeBalls);
                        testStep = 0;  // Quay lại bước ngắm
                        stepExecuted = false;
                        lastBilliardTime = currentTime;
                    }
                }
                break;
        }
        
        // Draw only changed parts (optimized to reduce flicker)
        billiardGame->draw();
    }
    
    delay(100);
}
