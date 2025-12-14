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
#include "chat_screen.h"

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
ChatScreen* chatScreen;
bool testMode = true;  // Bật chế độ test
bool testStarted = false;
bool menuShown = false;  // Đánh dấu đã hiển thị menu chưa
bool inGame = false;  // Đánh dấu đang trong game
int currentGame = 0;  // 0 = none, 1 = gunny, 2 = caro, 3 = billiard, 4 = chat

// Callback function cho keyboard input
void onKeyboardKeySelected(String key) {
    // Xử lý input cho chat nếu đang ở chế độ chat
    if (inGame && currentGame == 4 && chatScreen != nullptr) {
        chatScreen->handleKeyPress(key);
    }
    // Xử lý input cho WiFi nếu đang ở chế độ WiFi
    else if (wifiManager != nullptr) {
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
    
    // Khởi tạo Chat Screen
    chatScreen = new ChatScreen(&tft, keyboard);
    
    // TỰ ĐỘNG MỞ CHAT SCREEN
    Serial.println("Auto-starting Chat Screen...");
    // Thêm một số tin nhắn dài 80 ký tự để test
    chatScreen->addMessage("This is a very long message to test the 80 character limit! Let's see how it works!", false);
    chatScreen->addMessage("Another long message here to test scrolling and pushing up effect when messages are very long!", true);
    chatScreen->addMessage("Third long message to see how multiple long messages stack and push each other up on the screen!", false);
    chatScreen->draw();
    inGame = true;
    currentGame = 4;  // Chat mode
    menuShown = true;
    
    // TỰ ĐỘNG VÀO GAME BIDA NGAY - BỎ QUA MENU VÀ WIFI
    // Serial.println("Auto-starting Billiard Game...");
    // billiardGame->init();
    // inGame = true;
    // currentGame = 3;  // Billiard game
    // menuShown = true;  // Đánh dấu đã qua menu để không hiển thị lại
    
    // Bỏ qua WiFi và menu
    // wifiManager->begin();
    
    Serial.println("Billiard Game started!");
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
                // Tự động ngắm vào quả bi gần nhất (để đánh các quả bi khác vào lỗ)
                if (!stepExecuted || currentTime - lastBilliardTime > 500) {
                    billiardGame->aimAtNearestBall();
                    lastBilliardTime = currentTime;
                    stepExecuted = true;
                    if (billiardGame->getIsAiming()) {
                        // Sau khi ngắm, chuyển sang bước charge ngay
                        static int aimCount = 0;
                        aimCount++;
                        if (aimCount > 2) {  // Ngắm 2 lần để đảm bảo góc đúng
                            testStep = 1;  // Chuyển sang bước charge
                            stepExecuted = false;
                            aimCount = 0;
                            lastBilliardTime = currentTime;
                            Serial.print("Billiard Test: Aimed at nearest ball. Active balls: ");
                            Serial.println(activeBalls);
                        }
                    }
                }
                break;
            case 1:
                // Bắt đầu charge đến MAX LỰC (100%)
                if (!stepExecuted) {
                    billiardGame->handleChargeStart();
                    chargeStartTime = currentTime;
                    stepExecuted = true;
                    Serial.println("Billiard Test: Started charging to MAX POWER (100%)...");
                }
                
                // Tăng lực nhanh bằng handlePowerUp() để đạt max lực nhanh hơn
                if (currentTime - lastBilliardTime > 50) {  // Mỗi 50ms tăng lực một lần
                    float currentPower = billiardGame->getCuePower();
                    if (currentPower < 100.0f) {
                        // Tăng lực nhanh đến 100%
                        billiardGame->handlePowerUp();
                        lastBilliardTime = currentTime;
                        
                        Serial.print("Billiard Test: Charging... Power: ");
                        Serial.print(currentPower);
                        Serial.println("%");
                    } else {
                        // Đã đạt max lực (100%), đánh ngay
                        Serial.print("Billiard Test: MAX POWER (100%) reached! Firing...");
                        Serial.println(billiardGame->getCuePower());
                        billiardGame->handleChargeRelease();
                        testStep = 2;
                        stepExecuted = false;
                        lastBilliardTime = currentTime;
                    }
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
    
    // Update Chat screen if active
    if (inGame && currentGame == 4 && chatScreen != nullptr) {
        // Cập nhật animation cho decor
        chatScreen->updateDecorAnimation();
        
        // Test: Tự động gửi vài tin nhắn, sau đó scroll up/down từ từ
        static unsigned long lastTestMessageTime = 0;
        static unsigned long lastScrollTime = 0;
        static int testMessageCount = 0;
        static int scrollTestStep = 0;  // 0 = gửi tin nhắn, 1 = scroll up, 2 = scroll down
        static bool isUserTurn = true;
        static int scrollUpCount = 0;
        static int maxScrollUp = 0;
        
        unsigned long currentTime = millis();
        
        // Bước 1: Gửi vài tin nhắn trước (tối đa 10 tin nhắn)
        if (testMessageCount < 10 && currentTime - lastTestMessageTime > 1500) {
            // Tạo tin nhắn với độ dài ngẫu nhiên (5-80 ký tự)
            int messageLength = random(5, 81);  // Random từ 5 đến 80 ký tự
            String testMessage = "";
            
            // Tạo nội dung tin nhắn với độ dài ngẫu nhiên
            if (messageLength <= 10) {
                // Tin nhắn ngắn
                String shortMessages[] = {"Hi!", "OK", "Yes", "No", "Thanks", "Hello", "Bye", "Sure", "Cool", "Nice"};
                testMessage = shortMessages[random(0, 10)];
            } else if (messageLength <= 30) {
                // Tin nhắn trung bình
                testMessage = "Msg " + String(testMessageCount) + " - This is a medium length message!";
                if (testMessage.length() > messageLength) {
                    testMessage = testMessage.substring(0, messageLength);
                } else {
                    while (testMessage.length() < messageLength) {
                        testMessage += " x";
                    }
                    if (testMessage.length() > messageLength) {
                        testMessage = testMessage.substring(0, messageLength);
                    }
                }
            } else {
                // Tin nhắn dài
                testMessage = "Msg " + String(testMessageCount) + " - This is a very long message to test scrolling functionality with random length!";
                while (testMessage.length() < messageLength) {
                    testMessage += " x";
                }
                if (testMessage.length() > messageLength) {
                    testMessage = testMessage.substring(0, messageLength);
                }
            }
            
            chatScreen->addMessage(testMessage, isUserTurn);
            
            testMessageCount++;
            isUserTurn = !isUserTurn;  // Đổi lượt user/other
            lastTestMessageTime = currentTime;
            
            Serial.print("Chat Test: Sent message #");
            Serial.print(testMessageCount);
            Serial.print(" (");
            Serial.print(isUserTurn ? "Other" : "You");
            Serial.print(", length: ");
            Serial.print(testMessage.length());
            Serial.println(" chars)");
            
            // Tính max scroll up có thể
            if (testMessageCount > 5) {
                maxScrollUp = testMessageCount - 5;  // Có thể scroll lên tối đa
            }
        }
        // Bước 2: Sau khi gửi đủ tin nhắn, bắt đầu scroll test
        else if (testMessageCount >= 10 && scrollTestStep == 0) {
            // Chờ 2 giây sau tin nhắn cuối
            if (currentTime - lastTestMessageTime > 2000) {
                scrollTestStep = 1;  // Bắt đầu scroll up
                scrollUpCount = 0;
                lastScrollTime = currentTime;
                Serial.println("Chat Test: Starting scroll test - scrolling UP...");
            }
        }
        // Bước 3: Scroll up từ từ
        else if (scrollTestStep == 1 && currentTime - lastScrollTime > 800) {
            if (scrollUpCount < maxScrollUp) {
                chatScreen->handleUp();
                scrollUpCount++;
                lastScrollTime = currentTime;
                Serial.print("Chat Test: Scrolled UP (");
                Serial.print(scrollUpCount);
                Serial.print("/");
                Serial.print(maxScrollUp);
                Serial.println(")");
            } else {
                // Đã scroll up hết, chờ 2 giây rồi scroll down
                scrollTestStep = 2;
                lastScrollTime = currentTime;
                Serial.println("Chat Test: Finished scrolling UP, waiting 2s then scrolling DOWN...");
            }
        }
        // Bước 4: Scroll down từ từ
        else if (scrollTestStep == 2) {
            if (currentTime - lastScrollTime > 2000 && scrollUpCount > 0) {
                // Bắt đầu scroll down
                if (currentTime - lastScrollTime > 2000 + 800) {
                    chatScreen->handleDown();
                    scrollUpCount--;
                    lastScrollTime = currentTime;
                    Serial.print("Chat Test: Scrolled DOWN (remaining: ");
                    Serial.print(scrollUpCount);
                    Serial.println(")");
                    
                    if (scrollUpCount == 0) {
                        // Đã scroll down hết, reset để test lại
                        Serial.println("Chat Test: Finished scroll test! Resetting in 3s...");
                        delay(3000);
                        testMessageCount = 0;
                        scrollTestStep = 0;
                        scrollUpCount = 0;
                        chatScreen->clearMessages();
                        lastTestMessageTime = currentTime;
                    }
                }
            }
        }
    }
    
    delay(100);
}
