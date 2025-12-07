#ifndef WIFI_PASSWORD_H
#define WIFI_PASSWORD_H

#include <TFT_eSPI.h>
#include "keyboard.h"

class WiFiPasswordScreen {
private:
    TFT_eSprite* bg;
    Keyboard* keyboard;
    
    // Vị trí và kích thước
    uint8_t titleY;
    uint8_t inputBoxY;
    uint8_t inputBoxHeight;
    uint8_t inputBoxWidth;
    uint8_t maxPasswordLength;
    
    // Mật khẩu
    String password;
    
    // Màu sắc
    uint16_t bgColor;
    uint16_t titleColor;
    uint16_t inputBoxBgColor;
    uint16_t inputBoxBorderColor;
    uint16_t textColor;
    
    // Vẽ tiêu đề
    void drawTitle();
    
    // Vẽ ô nhập mật khẩu
    void drawInputBox();
    
    // Vẽ mật khẩu trong ô
    void drawPassword();

public:
    // Constructor
    WiFiPasswordScreen(TFT_eSprite* bg, Keyboard* keyboard);
    
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
    void setTitleColor(uint16_t color) { titleColor = color; }
    void setInputBoxBgColor(uint16_t color) { inputBoxBgColor = color; }
    void setInputBoxBorderColor(uint16_t color) { inputBoxBorderColor = color; }
    void setTextColor(uint16_t color) { textColor = color; }
};

#endif

