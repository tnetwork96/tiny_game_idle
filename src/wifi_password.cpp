#include <Arduino.h>
#include "wifi_password.h"

WiFiPasswordScreen::WiFiPasswordScreen(TFT_eSprite* bg, Keyboard* keyboard) {
    this->bg = bg;
    this->keyboard = keyboard;
    
    // Khởi tạo vị trí và kích thước
    this->titleY = 5;
    this->inputBoxY = 20;
    this->inputBoxHeight = 18;  // Tăng từ 15 lên 18 để chữ không bị mất
    this->inputBoxWidth = 120;
    this->maxPasswordLength = 20;
    
    // Khởi tạo mật khẩu
    this->password = "";
    
    // Khởi tạo màu sắc mặc định - Phong cách Arcade
    this->bgColor = TFT_BLACK;  // Nền đen
    this->titleColor = TFT_YELLOW;  // Tiêu đề màu vàng neon
    this->inputBoxBgColor = TFT_BLACK;  // Nền ô nhập đen (arcade style)
    this->inputBoxBorderColor = TFT_CYAN;  // Viền cyan neon
    this->textColor = TFT_GREEN;  // Chữ màu xanh lá neon
}

WiFiPasswordScreen::~WiFiPasswordScreen() {
    // Không cần giải phóng gì
}

void WiFiPasswordScreen::drawTitle() {
    bg->setTextColor(titleColor, bgColor);
    bg->setTextSize(1);
    bg->setCursor(10, titleY);
    bg->print("WiFi Password");
}

void WiFiPasswordScreen::drawInputBox() {
    // Hàm này không còn được dùng, việc vẽ được thực hiện trực tiếp trong draw()
    // Giữ lại để tương thích nếu có code khác gọi
}

void WiFiPasswordScreen::drawPassword() {
    uint8_t inputBoxX = (128 - inputBoxWidth) / 2;
    uint8_t textX = inputBoxX + 3;
    uint8_t textY = inputBoxY + 7;  // Điều chỉnh để căn giữa tốt hơn với height mới (18px)
    
    // Xóa vùng text cũ
    bg->fillRect(textX, textY, inputBoxWidth - 6, inputBoxHeight - 4, inputBoxBgColor);
    
    // Vẽ mật khẩu
    bg->setTextColor(textColor, inputBoxBgColor);
    bg->setTextSize(1);
    bg->setCursor(textX, textY);
    
    if (password.length() == 0) {
        // Hiển thị placeholder (màu xám sáng cho phong cách arcade)
        uint16_t placeholderColor = 0x5AEB;  // Màu xám sáng hơn, phù hợp arcade
        bg->setTextColor(placeholderColor, inputBoxBgColor);
        bg->print("Enter pwd...");
    } else {
        // Luôn hiển thị mật khẩu thật, nhưng cắt bớt 1 ký tự cuối để tránh tràn
        if (password.length() > 1) {
            String displayPassword = password.substring(0, password.length() - 1);
            bg->print(displayPassword);
        } else {
            bg->print(password);
        }
    }
}

void WiFiPasswordScreen::draw() {
    // Vẽ nền màn hình
    bg->fillScreen(bgColor);
    
    // Vẽ tiêu đề
    drawTitle();
    
    // Vẽ ô nhập mật khẩu với đầy đủ viền
    uint8_t inputBoxX = (128 - inputBoxWidth) / 2;
    bg->fillRect(inputBoxX, inputBoxY, inputBoxWidth, inputBoxHeight, inputBoxBgColor);
    
    // Vẽ tất cả các viền (bao gồm cả viền dưới)
    // Viền trên
    bg->drawFastHLine(inputBoxX, inputBoxY, inputBoxWidth, inputBoxBorderColor);
    bg->drawFastHLine(inputBoxX, inputBoxY + 1, inputBoxWidth, inputBoxBorderColor);
    // Viền dưới (vẽ trước để đảm bảo không bị che)
    bg->drawFastHLine(inputBoxX, inputBoxY + inputBoxHeight - 2, inputBoxWidth, inputBoxBorderColor);
    bg->drawFastHLine(inputBoxX, inputBoxY + inputBoxHeight - 1, inputBoxWidth, inputBoxBorderColor);
    // Viền trái
    bg->drawFastVLine(inputBoxX, inputBoxY, inputBoxHeight, inputBoxBorderColor);
    bg->drawFastVLine(inputBoxX + 1, inputBoxY, inputBoxHeight, inputBoxBorderColor);
    // Viền phải
    bg->drawFastVLine(inputBoxX + inputBoxWidth - 2, inputBoxY, inputBoxHeight, inputBoxBorderColor);
    bg->drawFastVLine(inputBoxX + inputBoxWidth - 1, inputBoxY, inputBoxHeight, inputBoxBorderColor);
    
    // Vẽ mật khẩu
    drawPassword();
    
    // Vẽ bàn phím (keyboard sẽ tự vẽ và push sprite)
    keyboard->draw();
    
    // Vẽ lại viền dưới sau khi bàn phím vẽ để đảm bảo không bị che
    bg->drawFastHLine(inputBoxX, inputBoxY + inputBoxHeight - 2, inputBoxWidth, inputBoxBorderColor);
    bg->drawFastHLine(inputBoxX, inputBoxY + inputBoxHeight - 1, inputBoxWidth, inputBoxBorderColor);
    
    // Đẩy sprite lên màn hình (sau khi vẽ lại viền dưới)
    bg->pushSprite(0, 0);
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
            draw();  // Vẽ lại toàn bộ màn hình
        }
    } else if (key == " ") {
        // Space - thêm dấu cách
        if (password.length() < maxPasswordLength) {
            password += " ";
            draw();  // Vẽ lại toàn bộ màn hình
        }
    } else if (key != "123" && key != "ABC" && key != "ic") {
        // Thêm ký tự thông thường (bỏ qua các phím chuyển đổi)
        if (password.length() < maxPasswordLength) {
            password += key;
            draw();  // Vẽ lại toàn bộ màn hình
        }
    }
}

