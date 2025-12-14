#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include "wifi_password.h"

// Synthwave/Vaporwave color palette (theo hình arcade)
#define NEON_PURPLE 0xD81F        // Neon purple (màu chủ đạo của arcade)
#define NEON_GREEN 0x07E0         // Neon green (sáng như trong hình)
#define NEON_CYAN 0x07FF          // Neon cyan (sáng hơn)
#define YELLOW_ORANGE 0xFE20       // Yellow-orange (như "GAME OVER" text)
#define SOFT_WHITE 0xFFFF         // White (cho text trên purple background)

WiFiPasswordScreen::WiFiPasswordScreen(Adafruit_ST7789* tft, Keyboard* keyboard) {  // Sử dụng keyboard thường
    this->tft = tft;
    this->keyboard = keyboard;
    
    // Khởi tạo vị trí và kích thước (màn hình 240x320) - cho text size 2
    this->titleY = 10;
    this->inputBoxY = 40;
    this->inputBoxHeight = 40;  // Chiều cao ô nhập (tăng lên cho text size 2)
    this->inputBoxWidth = 220;  // Rộng cho màn hình 240px (căn giữa)
    this->maxPasswordLength = 20;
    
    // Khởi tạo mật khẩu
    this->password = "";
    
    // Khởi tạo màu sắc mặc định - Phong cách Synthwave/Vaporwave (theo hình arcade)
    this->bgColor = ST77XX_BLACK;  // Nền đen
    this->titleColor = YELLOW_ORANGE;  // Tiêu đề màu yellow-orange (như "GAME OVER")
    this->inputBoxBgColor = ST77XX_BLACK;  // Nền ô nhập đen
    this->inputBoxBorderColor = NEON_PURPLE;  // Viền neon purple
    this->textColor = NEON_GREEN;  // Chữ màu neon green (sáng)
}

WiFiPasswordScreen::~WiFiPasswordScreen() {
    // Không cần giải phóng gì
}

void WiFiPasswordScreen::drawTitle() {
    tft->setTextColor(titleColor, bgColor);
    tft->setTextSize(2);  // Text size 2
    // Căn giữa tiêu đề
    String title = "WiFi Password";
    uint16_t textWidth = title.length() * 12;  // Khoảng 12px mỗi ký tự với textSize 2
    uint16_t textX = (240 - textWidth) / 2;   // Căn giữa
    tft->setCursor(textX, titleY);
    tft->print(title);
}

void WiFiPasswordScreen::drawInputBox() {
    // Hàm này không còn được dùng, việc vẽ được thực hiện trực tiếp trong draw()
    // Giữ lại để tương thích nếu có code khác gọi
}

void WiFiPasswordScreen::drawPassword() {
    uint16_t inputBoxX = (240 - inputBoxWidth) / 2;  // Center on 240px width
    uint16_t textX = inputBoxX + 5;  // Margin trái cho text size 2
    uint16_t textY = inputBoxY + 10;  // Căn giữa với inputBoxHeight 40px (text size 2 cao ~16px)
    
    // Xóa vùng text cũ (chỉ xóa phần text, không xóa viền)
    // Giảm height để không chạm vào viền dưới
    uint16_t textAreaHeight = inputBoxHeight - 20;  // Trừ margin trên (10px) và dưới (10px)
    tft->fillRect(textX, textY, inputBoxWidth - 10, textAreaHeight, inputBoxBgColor);
    
    // Vẽ mật khẩu
    tft->setTextColor(textColor, inputBoxBgColor);
    tft->setTextSize(2);  // Text size 2
    tft->setCursor(textX, textY);
    
    if (password.length() == 0) {
        // Hiển thị placeholder (màu tím nhạt cho phong cách synthwave)
        uint16_t placeholderColor = 0x5008;  // Soft dark purple, phù hợp synthwave
        tft->setTextColor(placeholderColor, inputBoxBgColor);
        tft->print("Enter pwd...");
    } else {
        // Hiển thị đầy đủ mật khẩu (không cắt bớt ký tự cuối)
            tft->print(password);
    }
    
    // Vẽ lại viền dưới để đảm bảo không bị mất
    tft->drawFastHLine(inputBoxX, inputBoxY + inputBoxHeight - 2, inputBoxWidth, inputBoxBorderColor);
    tft->drawFastHLine(inputBoxX, inputBoxY + inputBoxHeight - 1, inputBoxWidth, inputBoxBorderColor);
}

void WiFiPasswordScreen::draw() {
    // Vẽ nền màn hình
    tft->fillScreen(bgColor);
    
    // Vẽ tiêu đề
    drawTitle();
    
    // Vẽ ô nhập mật khẩu với đầy đủ viền
    uint16_t inputBoxX = (240 - inputBoxWidth) / 2;  // Center on 240px width
    tft->fillRect(inputBoxX, inputBoxY, inputBoxWidth, inputBoxHeight, inputBoxBgColor);
    
    // Vẽ tất cả các viền (bao gồm cả viền dưới)
    // Viền trên
    tft->drawFastHLine(inputBoxX, inputBoxY, inputBoxWidth, inputBoxBorderColor);
    tft->drawFastHLine(inputBoxX, inputBoxY + 1, inputBoxWidth, inputBoxBorderColor);
    // Viền dưới (vẽ trước để đảm bảo không bị che)
    tft->drawFastHLine(inputBoxX, inputBoxY + inputBoxHeight - 2, inputBoxWidth, inputBoxBorderColor);
    tft->drawFastHLine(inputBoxX, inputBoxY + inputBoxHeight - 1, inputBoxWidth, inputBoxBorderColor);
    // Viền trái
    tft->drawFastVLine(inputBoxX, inputBoxY, inputBoxHeight, inputBoxBorderColor);
    tft->drawFastVLine(inputBoxX + 1, inputBoxY, inputBoxHeight, inputBoxBorderColor);
    // Viền phải
    tft->drawFastVLine(inputBoxX + inputBoxWidth - 2, inputBoxY, inputBoxHeight, inputBoxBorderColor);
    tft->drawFastVLine(inputBoxX + inputBoxWidth - 1, inputBoxY, inputBoxHeight, inputBoxBorderColor);
    
    // Vẽ mật khẩu
    drawPassword();
    
    // Vẽ bàn phím (keyboard sẽ tự vẽ trực tiếp lên tft)
    keyboard->draw();
    
    // Vẽ lại viền dưới sau khi bàn phím vẽ để đảm bảo không bị che
    tft->drawFastHLine(inputBoxX, inputBoxY + inputBoxHeight - 2, inputBoxWidth, inputBoxBorderColor);
    tft->drawFastHLine(inputBoxX, inputBoxY + inputBoxHeight - 1, inputBoxWidth, inputBoxBorderColor);
}

void WiFiPasswordScreen::handleKeyPress(String key) {
    // Xử lý các phím đặc biệt
    if (key == "|e") {
        // Enter - có thể dùng để submit
        // TODO: Xử lý submit mật khẩu
    } else if (key == "<") {
        // Delete - xóa ký tự cuối
        if (password.length() > 0) {
            password.remove(password.length() - 1);
            drawPassword();  // Chỉ vẽ lại phần password, không vẽ lại keyboard
        }
    } else if (key == " ") {
        // Space - thêm dấu cách
        if (password.length() < maxPasswordLength) {
            password += " ";
            drawPassword();  // Chỉ vẽ lại phần password, không vẽ lại keyboard
        }
    } else if (key != "123" && key != "ABC" && key != Keyboard::KEY_ICON && key != "shift") {
        // Thêm ký tự thông thường (bỏ qua các phím chuyển đổi)
        if (password.length() < maxPasswordLength) {
            password += key;
            drawPassword();  // Chỉ vẽ lại phần password, không vẽ lại keyboard
        }
    }
}

