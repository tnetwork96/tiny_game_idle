#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include "wifi_password.h"

WiFiPasswordScreen::WiFiPasswordScreen(Adafruit_ST7789* tft, Keyboard* keyboard) {
    this->tft = tft;
    this->keyboard = keyboard;
    
    // Initialize layout (màn hình 320x240 landscape)
    const uint16_t headerHeight = 30;
    this->headerHeight = headerHeight;
    this->inputBoxY = headerHeight + 20;  // Below header with margin
    this->inputBoxHeight = 40;  // Input box height
    this->inputBoxWidth = 300;  // Width for 320px screen (with margins)
    this->maxPasswordLength = 20;
    
    // Initialize password
    this->password = "";
    
    // Initialize colors (Deep Space Arcade Theme)
    this->bgColor = 0x0042;           // Deep Midnight Blue background
    this->headerColor = 0x08A5;        // Header background
    this->headerTextColor = 0xFFFF;   // White text for header
    this->inputBoxBgColor = 0x1148;   // Electric Blue tint for input box
    this->inputBoxBorderColor = 0x07FF;  // Cyan border
    this->textColor = 0xFFFF;         // White text
    this->placeholderColor = 0x08A5;  // Dimmed cyan/grey for placeholder
}

WiFiPasswordScreen::~WiFiPasswordScreen() {
    // Không cần giải phóng gì
}

void WiFiPasswordScreen::drawHeader() {
    if (tft == nullptr) return;
    
    // Draw header bar
    tft->fillRect(0, 0, 320, headerHeight, headerColor);
    
    // Draw title "Enter Password" centered
    String title = "Enter Password";
    tft->setTextColor(headerTextColor, headerColor);
    tft->setTextSize(2);
    uint16_t textWidth = title.length() * 12;  // Approx 12px per character with textSize 2
    uint16_t textX = (320 - textWidth) / 2;   // Center on 320px width
    tft->setCursor(textX, 8);  // Vertically center in 30px header (text height ~16px)
    tft->print(title);
    
    // Draw separator line below header
    tft->drawFastHLine(0, headerHeight - 1, 320, 0x07FF);  // Cyan separator
}

void WiFiPasswordScreen::drawInputBox() {
    // Hàm này không còn được dùng, việc vẽ được thực hiện trực tiếp trong draw()
    // Giữ lại để tương thích nếu có code khác gọi
}

void WiFiPasswordScreen::drawPassword() {
    uint16_t inputBoxX = (320 - inputBoxWidth) / 2;  // Center on 320px width
    uint16_t textX = inputBoxX + 5;  // Left margin
    uint16_t textY = inputBoxY + 10;  // Vertically center with inputBoxHeight 40px (text height ~16px)
    
    // Clear old text area (only text area, not border)
    uint16_t textAreaHeight = inputBoxHeight - 20;  // Subtract top (10px) and bottom (10px) margins
    tft->fillRect(textX, textY, inputBoxWidth - 10, textAreaHeight, inputBoxBgColor);
    
    // Draw password
    tft->setTextSize(2);  // Text size 2
    tft->setCursor(textX, textY);
    
    if (password.length() == 0) {
        // Display placeholder (dimmed cyan/grey for Deep Space Arcade theme)
        tft->setTextColor(placeholderColor, inputBoxBgColor);
        tft->print("Enter pwd...");
    } else {
        // Display full password
        tft->setTextColor(textColor, inputBoxBgColor);
        tft->print(password);
    }
    
    // Redraw bottom border to ensure it's not lost
    tft->drawFastHLine(inputBoxX, inputBoxY + inputBoxHeight - 2, inputBoxWidth, inputBoxBorderColor);
    tft->drawFastHLine(inputBoxX, inputBoxY + inputBoxHeight - 1, inputBoxWidth, inputBoxBorderColor);
}

void WiFiPasswordScreen::draw() {
    // Draw background
    tft->fillScreen(bgColor);
    
    // Draw header
    drawHeader();
    
    // Draw input box with border
    uint16_t inputBoxX = (320 - inputBoxWidth) / 2;  // Center on 320px width
    tft->fillRect(inputBoxX, inputBoxY, inputBoxWidth, inputBoxHeight, inputBoxBgColor);
    
    // Draw all borders (Cyan 0x07FF)
    // Top border
    tft->drawFastHLine(inputBoxX, inputBoxY, inputBoxWidth, inputBoxBorderColor);
    tft->drawFastHLine(inputBoxX, inputBoxY + 1, inputBoxWidth, inputBoxBorderColor);
    // Bottom border
    tft->drawFastHLine(inputBoxX, inputBoxY + inputBoxHeight - 2, inputBoxWidth, inputBoxBorderColor);
    tft->drawFastHLine(inputBoxX, inputBoxY + inputBoxHeight - 1, inputBoxWidth, inputBoxBorderColor);
    // Left border
    tft->drawFastVLine(inputBoxX, inputBoxY, inputBoxHeight, inputBoxBorderColor);
    tft->drawFastVLine(inputBoxX + 1, inputBoxY, inputBoxHeight, inputBoxBorderColor);
    // Right border
    tft->drawFastVLine(inputBoxX + inputBoxWidth - 2, inputBoxY, inputBoxHeight, inputBoxBorderColor);
    tft->drawFastVLine(inputBoxX + inputBoxWidth - 1, inputBoxY, inputBoxHeight, inputBoxBorderColor);
    
    // Draw password
    drawPassword();
    
    // Draw keyboard (keyboard will draw directly to tft)
    keyboard->draw();
    
    // Redraw bottom border after keyboard draws to ensure it's not covered
    tft->drawFastHLine(inputBoxX, inputBoxY + inputBoxHeight - 2, inputBoxWidth, inputBoxBorderColor);
    tft->drawFastHLine(inputBoxX, inputBoxY + inputBoxHeight - 1, inputBoxWidth, inputBoxBorderColor);
}

void WiFiPasswordScreen::handleKeyPress(String key) {
    // Debug log để kiểm tra key được nhận
    Serial.print("WiFiPassword: Received key: '");
    Serial.print(key);
    Serial.print("' (length=");
    Serial.print(key.length());
    Serial.print(", first char=");
    if (key.length() > 0) {
        Serial.print((int)key.charAt(0));
        Serial.print(" '");
        Serial.print(key.charAt(0));
        Serial.print("'");
    }
    Serial.println(")");
    
    // Xử lý các phím đặc biệt
    if (key == "|e") {
        // Enter - có thể dùng để submit
        // TODO: Xử lý submit mật khẩu
    } else if (key == "<") {
        // Delete - xóa ký tự cuối
        if (password.length() > 0) {
            password.remove(password.length() - 1);
            Serial.print("WiFiPassword: Password after delete: '");
            Serial.print(password);
            Serial.println("'");
            drawPassword();  // Chỉ vẽ lại phần password, không vẽ lại keyboard
        }
    } else if (key == " ") {
        // Space - thêm dấu cách
        if (password.length() < maxPasswordLength) {
            password += " ";
            Serial.print("WiFiPassword: Password after space: '");
            Serial.print(password);
            Serial.println("'");
            drawPassword();  // Chỉ vẽ lại phần password, không vẽ lại keyboard
        }
    } else if (key != "123" && key != "ABC" && key != Keyboard::KEY_ICON && key != "shift") {
        // Thêm ký tự thông thường (bỏ qua các phím chuyển đổi)
        if (password.length() < maxPasswordLength) {
            password += key;
            Serial.print("WiFiPassword: Password after adding '");
            Serial.print(key);
            Serial.print("': '");
            Serial.print(password);
            Serial.print("' (length=");
            Serial.print(password.length());
            Serial.println(")");
            drawPassword();  // Chỉ vẽ lại phần password, không vẽ lại keyboard
        } else {
            Serial.println("WiFiPassword: Password max length reached!");
        }
    } else {
        Serial.print("WiFiPassword: Ignored key (mode switch): '");
        Serial.print(key);
        Serial.println("'");
    }
}

