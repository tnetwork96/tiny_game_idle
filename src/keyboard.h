#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <TFT_eSPI.h>

class Keyboard {
private:
    // Con trỏ tới TFT_eSprite
    TFT_eSprite* bg;
    
    // Vị trí con trỏ
    uint8_t cursorRow;
    int8_t cursorCol;
    
    // Tọa độ vẽ bàn phím
    uint8_t kb_x;
    uint8_t kb_y;
    
    // Ký tự hiện tại
    String currentChar;
    
    // Mảng lưu các ký tự đã chọn
    char selectedText[100];
    uint8_t selectedIndex;
    
    // Chế độ bàn phím
    bool isAlphabetMode; // true là chữ cái, false là số/ký tự đặc biệt
    
    // Con trỏ tới bảng phím hiện tại
    String (*qwertyKeys)[10];
    
    // Kích thước text
    uint8_t textSize;
    
    // Màu sắc bàn phím
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
    
    // Callback function pointer cho khi nhấn select
    void (*onKeySelected)(String key);

public:
    // Constructor
    Keyboard(TFT_eSprite* bg);
    
    // Destructor
    ~Keyboard();
    
    // Vẽ bàn phím
    void draw();
    
    // Di chuyển con trỏ theo lệnh
    void moveCursorByCommand(String command, int x, int y);
    
    // Tắt bàn phím (xóa màn hình)
    void turnOff();
    
    // Getter/Setter methods
    String getCurrentChar() const { return currentChar; }
    uint8_t getCursorRow() const { return cursorRow; }
    int8_t getCursorCol() const { return cursorCol; }
    bool getIsAlphabetMode() const { return isAlphabetMode; }
    uint8_t getTextSize() const { return textSize; }
    void setTextSize(uint8_t size) { textSize = size; }
    
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
};

#endif
