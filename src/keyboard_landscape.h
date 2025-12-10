#ifndef KEYBOARD_LANDSCAPE_H
#define KEYBOARD_LANDSCAPE_H

#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include "keyboard_skins.h"  // Include struct KeyboardSkin và các enum variants

class KeyboardLandscape {
private:
    // Con trỏ tới Adafruit_ST7789
    Adafruit_ST7789* tft;
    
    // Vị trí con trỏ
    int16_t cursorRow;
    int8_t cursorCol;
    
    // Tọa độ vẽ bàn phím
    int16_t kb_x;
    int16_t kb_y;
    
    // Ký tự hiện tại
    String currentChar;
    
    // Mảng lưu các ký tự đã chọn
    char selectedText[100];
    int16_t selectedIndex;
    
    // Chế độ bàn phím
    bool isAlphabetMode; // true là chữ cái, false là số/ký tự đặc biệt
    
    // Con trỏ tới bảng phím hiện tại
    String (*qwertyKeys)[10];
    
    // Skin hiện tại
    KeyboardSkin currentSkin;
    
    // Kích thước text (deprecated - dùng currentSkin.textSize)
    int16_t textSize;
    
    // Animation frame cho hiệu ứng cyberpunk
    int16_t animationFrame;
    
    // Màu sắc bàn phím (deprecated - dùng currentSkin)
    int16_t keyBgColor;      // Màu nền phím bình thường
    int16_t keySelectedColor; // Màu nền phím được chọn
    int16_t keyTextColor;    // Màu chữ trên phím
    int16_t bgScreenColor;   // Màu nền màn hình
    
    // Các hằng số phím đặc biệt
    static const String KEY_ENTER;
    static const String KEY_DELETE;
    static const String KEY_SHIFT;
    static const String KEY_SPACE;
    static const String KEY_ICON;
    
    // Bảng phím chữ cái (chữ viết thường) - layout nằm ngang
    static String qwertyKeysArray[3][10];
    
    // Bảng phím số và ký tự đặc biệt
    static String numericKeysArray[3][10];
    
    // Các hàm helper private
    void updateCurrentCursor();
    String getCurrentKey() const;
    void moveCursor(int8_t deltaRow, int8_t deltaCol);
    void drawFemininePattern(int16_t x, int16_t y, int16_t width, int16_t height);  // Vẽ hoa văn nữ tính
    void drawMechanicalPattern(int16_t x, int16_t y, int16_t width, int16_t height);  // Vẽ pattern máy móc cơ học
    void drawCyberpunkGlow(int16_t x, int16_t y, int16_t width, int16_t height, int16_t animationFrame);  // Vẽ hiệu ứng điện chạy xung quanh phím
    void drawCyberpunkKeyboardBorder(int16_t x, int16_t y, int16_t width, int16_t height, int16_t animationFrame);  // Vẽ hiệu ứng điện chạy xung quanh toàn bộ bàn phím
    int16_t getContrastColor(int16_t bgColor);  // Tính màu chữ tương phản dựa trên màu nền
    
    // Callback function pointer cho khi nhấn select
    void (*onKeySelected)(String key);

public:
    // Constructor
    KeyboardLandscape(Adafruit_ST7789* tft);
    
    // Destructor
    ~KeyboardLandscape();
    
    // Vẽ bàn phím
    void draw();
    
    // Di chuyển con trỏ theo lệnh
    void moveCursorByCommand(String command, int x, int y);
    
    // Di chuyển con trỏ đến vị trí cụ thể (row, col)
    void moveCursorTo(int16_t row, int8_t col);
    
    // Tự động nhập một ký tự (di chuyển đến ký tự và nhấn select)
    bool typeChar(char c);
    
    // Tự động nhập một chuỗi
    void typeString(String text);
    
    // Tắt bàn phím (xóa màn hình)
    void turnOff();
    
    // Getter/Setter methods
    String getCurrentChar() const { return currentChar; }
    int16_t getCursorRow() const { return cursorRow; }
    int8_t getCursorCol() const { return cursorCol; }
    bool getIsAlphabetMode() const { return isAlphabetMode; }
    int16_t getTextSize() const { return textSize; }
    void setTextSize(int16_t size) { textSize = size; }
    
    // Set callback khi nhấn phím
    void setOnKeySelectedCallback(void (*callback)(String key)) { onKeySelected = callback; }
    
    // Setter cho màu sắc
    void setKeyBgColor(int16_t color) { keyBgColor = color; }
    void setKeySelectedColor(int16_t color) { keySelectedColor = color; }
    void setKeyTextColor(int16_t color) { keyTextColor = color; }
    void setBgScreenColor(int16_t color) { bgScreenColor = color; }
    
    // Getter cho màu sắc
    int16_t getKeyBgColor() const { return keyBgColor; }
    int16_t getKeySelectedColor() const { return keySelectedColor; }
    int16_t getKeyTextColor() const { return keyTextColor; }
    int16_t getBgScreenColor() const { return bgScreenColor; }
    
    // Skin management
    void setSkin(const KeyboardSkin& skin);
    KeyboardSkin getSkin() const { return currentSkin; }
    void applySkin();  // Áp dụng skin vào các biến màu sắc cũ (backward compatibility)
    
    // Lưu ý: Các hàm get skin đã được di chuyển vào namespace KeyboardSkins
    // Sử dụng KeyboardSkins::getFeminineSkin(), KeyboardSkins::getCyberpunkSkin(), etc.
};

#endif

