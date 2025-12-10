#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include "keyboard_skins.h"  // Include struct KeyboardSkin và các enum variants

class Keyboard {
private:
    // Con trỏ tới Adafruit_ST7789
    Adafruit_ST7789* tft;
    
    // Vị trí con trỏ
    uint16_t cursorRow;
    int8_t cursorCol;
    
    // Tọa độ vẽ bàn phím
    uint16_t kb_x;
    uint16_t kb_y;
    
    // Ký tự hiện tại
    String currentChar;
    
    // Mảng lưu các ký tự đã chọn
    char selectedText[100];
    uint16_t selectedIndex;
    
    // Chế độ bàn phím
    bool isAlphabetMode; // true là chữ cái, false là số/ký tự đặc biệt
    
    // Con trỏ tới bảng phím hiện tại
    String (*qwertyKeys)[10];
    
    // Skin hiện tại
    KeyboardSkin currentSkin;
    
    // Kích thước text (deprecated - dùng currentSkin.textSize)
    uint16_t textSize;
    
    // Animation frame cho hiệu ứng cyberpunk
    uint16_t animationFrame;
    
    // Màu sắc bàn phím (deprecated - dùng currentSkin)
    uint16_t keyBgColor;      // Màu nền phím bình thường
    uint16_t keySelectedColor; // Màu nền phím được chọn
    uint16_t keyTextColor;    // Màu chữ trên phím
    uint16_t bgScreenColor;   // Màu nền màn hình
    
    // Các hằng số phím đặc biệt
    static const String KEY_ENTER;
    static const String KEY_DELETE;
    static const String KEY_SHIFT;
    static const String KEY_SPACE;
    static const String KEY_ICON;
    
    // Bảng phím chữ cái (chữ viết thường)
    static String qwertyKeysArray[3][10];
    
    // Bảng phím số và ký tự đặc biệt
    static String numericKeysArray[3][10];
    
    // Các hàm helper private
    void updateCurrentCursor();
    String getCurrentKey() const;
    void moveCursor(int8_t deltaRow, int8_t deltaCol);
    void drawFemininePattern(uint16_t x, uint16_t y, uint16_t width, uint16_t height);  // Vẽ hoa văn nữ tính
    void drawMechanicalPattern(uint16_t x, uint16_t y, uint16_t width, uint16_t height);  // Vẽ pattern máy móc cơ học
    void drawCyberpunkGlow(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t animationFrame);  // Vẽ hiệu ứng điện chạy xung quanh phím
    void drawCyberpunkKeyboardBorder(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t animationFrame);  // Vẽ hiệu ứng điện chạy xung quanh toàn bộ bàn phím
    uint16_t getContrastColor(uint16_t bgColor);  // Tính màu chữ tương phản dựa trên màu nền
    void drawEnterSymbol(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color);  // Vẽ ký tự Enter (mũi tên)
    
    // Callback function pointer cho khi nhấn select
    void (*onKeySelected)(String key);

public:
    // Constructor
    Keyboard(Adafruit_ST7789* tft);
    
    // Destructor
    ~Keyboard();
    
    // Vẽ bàn phím
    void draw();
    
    // Di chuyển con trỏ theo lệnh
    void moveCursorByCommand(String command, int x, int y);
    
    // Di chuyển con trỏ đến vị trí cụ thể (row, col)
    void moveCursorTo(uint16_t row, int8_t col);
    
    // Tự động nhập một ký tự (di chuyển đến ký tự và nhấn select)
    bool typeChar(char c);
    
    // Tự động nhập một chuỗi
    void typeString(String text);
    
    // Tắt bàn phím (xóa màn hình)
    void turnOff();
    
    // Getter/Setter methods
    String getCurrentChar() const { return currentChar; }
    uint16_t getCursorRow() const { return cursorRow; }
    int8_t getCursorCol() const { return cursorCol; }
    bool getIsAlphabetMode() const { return isAlphabetMode; }
    uint16_t getTextSize() const { return textSize; }
    void setTextSize(uint16_t size) { textSize = size; }
    
    // Set callback khi nhấn phím
    void setOnKeySelectedCallback(void (*callback)(String key)) { onKeySelected = callback; }
    
    // Setter cho màu sắc
    void setKeyBgColor(uint16_t color) { keyBgColor = color; }
    void setKeySelectedColor(uint16_t color) { keySelectedColor = color; }
    void setKeyTextColor(uint16_t color) { keyTextColor = color; }
    void setBgScreenColor(uint16_t color) { bgScreenColor = color; }
    
    // Getter cho màu sắc
    uint16_t getKeyBgColor() const { return keyBgColor; }
    uint16_t getKeySelectedColor() const { return keySelectedColor; }
    uint16_t getKeyTextColor() const { return keyTextColor; }
    uint16_t getBgScreenColor() const { return bgScreenColor; }
    
    // Skin management
    void setSkin(const KeyboardSkin& skin);
    KeyboardSkin getSkin() const { return currentSkin; }
    void applySkin();  // Áp dụng skin vào các biến màu sắc cũ (backward compatibility)
    
    // Lưu ý: Các hàm get skin đã được di chuyển vào namespace KeyboardSkins
    // Sử dụng KeyboardSkins::getFeminineSkin(), KeyboardSkins::getCyberpunkSkin(), etc.
};

#endif
