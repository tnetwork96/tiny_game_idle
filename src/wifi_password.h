#ifndef WIFI_PASSWORD_H
#define WIFI_PASSWORD_H

#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include "keyboard.h"  // Sử dụng keyboard thường

class WiFiPasswordScreen {
private:
    Adafruit_ST7789* tft;
    Keyboard* keyboard;  // Sử dụng keyboard thường
    
    // Vị trí và kích thước
    uint16_t headerHeight;
    uint16_t inputBoxY;
    uint16_t inputBoxHeight;
    uint16_t inputBoxWidth;
    uint16_t maxPasswordLength;
    
    // Mật khẩu
    String password;
    
    // Màu sắc (Deep Space Arcade Theme)
    uint16_t bgColor;
    uint16_t headerColor;
    uint16_t headerTextColor;
    uint16_t inputBoxBgColor;
    uint16_t inputBoxBorderColor;
    uint16_t textColor;
    uint16_t placeholderColor;
    
    // Vẽ header
    void drawHeader();
    
    // Vẽ ô nhập mật khẩu
    void drawInputBox();
    
    // Vẽ mật khẩu trong ô
    void drawPassword();

public:
    // Constructor
    WiFiPasswordScreen(Adafruit_ST7789* tft, Keyboard* keyboard);  // Sử dụng keyboard thường
    
    // Destructor
    ~WiFiPasswordScreen();
    
    // Vẽ toàn bộ màn hình
    void draw();
    
    // Xử lý khi nhấn phím
    void handleKeyPress(String key);
    
    // Getter/Setter
    String getPassword() const { return password; }
    void setPassword(String pwd) { password = pwd; }
    void clearPassword() { password = ""; }
    void appendPassword(String str) { password += str; }
    
    // Setter cho màu sắc
    void setBgColor(uint16_t color) { bgColor = color; }
    void setHeaderColor(uint16_t color) { headerColor = color; }
    void setHeaderTextColor(uint16_t color) { headerTextColor = color; }
    void setInputBoxBgColor(uint16_t color) { inputBoxBgColor = color; }
    void setInputBoxBorderColor(uint16_t color) { inputBoxBorderColor = color; }
    void setTextColor(uint16_t color) { textColor = color; }
    void setPlaceholderColor(uint16_t color) { placeholderColor = color; }
};

#endif

