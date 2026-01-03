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
    this->inputBoxY = 45;  // Position at Y=45 (matching LoginScreen/PinScreen, ensures no keyboard overlap)
    this->inputBoxHeight = 40;  // Chunky, touch-friendly height
    this->inputBoxWidth = 280;  // Width matching LoginScreen/PinScreen
    this->maxPasswordLength = 20;
    
    // Initialize password
    this->password = "";
    
    // Initialize colors (Deep Space Arcade Theme - matching LoginScreen/PinScreen)
    this->bgColor = 0x0042;           // Deep Midnight Blue background
    this->headerColor = 0x08A5;       // Header background
    this->headerTextColor = 0xFFFF;   // White text for header
    this->inputBoxBgColor = 0x0021;   // Darker Blue for input box (matching LoginScreen)
    this->inputBoxBorderColor = 0x07FF;  // Cyan border
    this->textColor = 0xFFFF;         // White text
    this->placeholderColor = 0x8410;  // Muted gray for placeholder (matching theme)
}

WiFiPasswordScreen::~WiFiPasswordScreen() {
    // Không cần giải phóng gì
}

void WiFiPasswordScreen::drawHeader() {
    if (tft == nullptr) return;
    
    // Draw header bar
    tft->fillRect(0, 0, 320, headerHeight, headerColor);
    
    // Draw title "Enter Password" centered (matching LoginScreen/PinScreen style)
    String title = "Enter Password";
    tft->setTextColor(headerTextColor, headerColor);
    tft->setTextSize(2);
    uint16_t textWidth = title.length() * 12;  // Approx 12px per character with textSize 2
    uint16_t textX = (320 - textWidth) / 2;   // Center on 320px width
    tft->setCursor(textX, 8);  // Vertically center in 30px header (text height ~16px)
    tft->print(title);
    
    // Draw separator line below header (Cyan accent)
    tft->drawFastHLine(0, headerHeight - 1, 320, inputBoxBorderColor);
}

void WiFiPasswordScreen::drawInputBox() {
    // Hàm này không còn được dùng, việc vẽ được thực hiện trực tiếp trong draw()
    // Giữ lại để tương thích nếu có code khác gọi
}

void WiFiPasswordScreen::drawPassword() {
    uint16_t inputBoxX = (320 - inputBoxWidth) / 2;  // Center on 320px width
    
    // Calculate text to display
    String displayText = password;
    bool showPlaceholder = (password.length() == 0);
    
    if (showPlaceholder) {
        displayText = "Enter password...";
    }
    
    // Calculate text width for centering (approximate: text size 2 = ~12px per char)
    int textWidth = displayText.length() * 12;
    uint16_t textX = inputBoxX + (inputBoxWidth - textWidth) / 2;
    uint16_t textY = inputBoxY + (inputBoxHeight / 2) - 6;  // Center vertically (text size 2 is ~12px tall)
    
    // Clear old text area
    tft->fillRect(inputBoxX + 4, inputBoxY + 4, inputBoxWidth - 8, inputBoxHeight - 8, inputBoxBgColor);
    
    // Draw password text (centered)
    tft->setTextSize(2);  // Text size 2
    if (showPlaceholder) {
        tft->setTextColor(placeholderColor, inputBoxBgColor);
    } else {
        tft->setTextColor(textColor, inputBoxBgColor);
    }
    tft->setCursor(textX, textY);
    tft->print(displayText);
}

void WiFiPasswordScreen::draw() {
    // Draw background
    tft->fillScreen(bgColor);
    
    // Draw header
    drawHeader();
    
    // Draw input box as rounded rectangle (matching LoginScreen/PinScreen style)
    uint16_t inputBoxX = (320 - inputBoxWidth) / 2;  // Center on 320px width
    
    // Label above input box (muted color, matching LoginScreen/PinScreen style)
    tft->setTextSize(1);
    tft->setTextColor(placeholderColor, bgColor);  // Use muted gray for label
    String label = "WiFi Password";
    uint16_t labelWidth = label.length() * 6;  // Approximate width for text size 1
    uint16_t labelX = (320 - labelWidth) / 2;   // Center horizontally
    tft->setCursor(labelX, inputBoxY - 12);
    tft->print(label);
    
    // Draw filled rounded rectangle input card
    tft->fillRoundRect(inputBoxX, inputBoxY, inputBoxWidth, inputBoxHeight, 6, inputBoxBgColor);
    // Cyan border
    tft->drawRoundRect(inputBoxX, inputBoxY, inputBoxWidth, inputBoxHeight, 6, inputBoxBorderColor);
    
    // Draw password text (centered)
    drawPassword();
    
    // Draw keyboard (keyboard will draw directly to tft)
    keyboard->draw();
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

