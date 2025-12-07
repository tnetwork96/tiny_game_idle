#include <Arduino.h>
#include "keyboard.h"
#define FORMAT_SPIFFS_IF_FAILED true
#define BLACK 0x0000
#define BLUE 0x001F
#define RED 0xF800
#define GREEN 0x07E0
#define CYAN 0x07FF
#define MAGENTA 0xF81F
#define YELLOW 0xFFE0
#define WHITE 0xFFFF
#define LIGHT_BLUE 0x5CE1E6
#define TFT_LIGHT_BLUE 0xADD8E6  // Màu xanh dương nhạt (Light Blue)
#define TFT_BLACK 0x0000         // Màu đen
// Khởi tạo các static members
const String Keyboard::KEY_ENTER = "|e";
const String Keyboard::KEY_DELETE = "<";
const String Keyboard::KEY_SHIFT = "shift";
const String Keyboard::KEY_SPACE = " ";
const String Keyboard::KEY_ICON = "ic";

// Bảng phím chữ cái (chữ viết thường)
String Keyboard::qwertyKeysArray[3][10] = {
  { "q", "w", "e", "r", "t", "y", "u", "i", "o", "p" },
  { "123", "a", "s", "d", "f", "g", "h", "j", "k", "l"},
  { "ic", "z", "x", "c", "v", "b", "n", "m", "|e", "<"}
};

// Bảng phím số và ký tự đặc biệt
String Keyboard::numericKeysArray[3][10] = {
  { "1", "2", "3", "4", "5", "6", "7", "8", "9", "0" },
  { "ABC", "/", ":", ";", "(", ")", "$", "&", "@", "\"" },
  { "ic", "#", ".", ",", "?", "!", "'", "-", "|e", "<"}
};

// Constructor
Keyboard::Keyboard(TFT_eSprite* bg) {
    this->bg = bg;
    this->cursorRow = 0;
    this->cursorCol = 0;
    this->kb_x = 0;
    this->kb_y = 80;
    this->currentChar = "";
    this->selectedIndex = 0;
    this->isAlphabetMode = true;
    this->textSize = 1;  // Mặc định text size = 1
    this->onKeySelected = nullptr;  // Khởi tạo callback = null
    
    // Khởi tạo màu sắc mặc định
    this->keyBgColor = TFT_WHITE;      // Màu nền phím bình thường
    this->keySelectedColor = TFT_YELLOW; // Màu nền phím được chọn
    this->keyTextColor = TFT_BLACK;    // Màu chữ trên phím
    this->bgScreenColor = TFT_BLACK;   // Màu nền màn hình
    
    // Khởi tạo bảng phím theo chế độ hiện tại
    if (isAlphabetMode) {
        this->qwertyKeys = qwertyKeysArray;
    } else {
        this->qwertyKeys = numericKeysArray;
    }
}

// Destructor
Keyboard::~Keyboard() {
    // Không cần giải phóng gì vì chỉ lưu con trỏ
}

void Keyboard::draw() {
    uint8_t keyWidth = 12;  // Kích thước mỗi phím
    uint8_t keyHeight = 12; // Chiều cao mỗi phím
    uint8_t xStart = kb_x;
    uint8_t yStart = kb_y;  // Bàn phím sẽ được vẽ từ tọa độ Y = 80px
    uint8_t spacing = 1;    // Khoảng cách giữa các phím

    // Chọn bảng phím dựa vào chế độ hiện tại
    String (*currentKeys)[10];
    if (isAlphabetMode) {
        currentKeys = qwertyKeysArray;  // Sử dụng bảng phím chữ cái
    } else {
        currentKeys = numericKeysArray; // Sử dụng bảng phím số/ký tự đặc biệt
    }

    // BƯỚC 1: Vẽ tất cả các phím (nền) trước
    for (uint8_t row = 0; row < 3; row++) {  // Điều chỉnh để duyệt qua 3 hàng
        for (uint8_t col = 0; col < 10; col++) {
            uint8_t xPos = xStart + col * (keyWidth + spacing);
            uint8_t yPos = yStart + row * (keyHeight + spacing);

            // Vẽ nền phím
            uint16_t bgColor;
            if (row == cursorRow && col == cursorCol) {
                bgColor = keySelectedColor; // Tô sáng phím hiện tại
            } else {
                bgColor = keyBgColor;  // Màu nền phím bình thường
            }
            bg->fillRect(xPos, yPos, keyWidth, keyHeight, bgColor);
        }
    }

    // BƯỚC 2: Điền chữ vào các phím sau
    for (uint8_t row = 0; row < 3; row++) {
        for (uint8_t col = 0; col < 10; col++) {
            uint8_t xPos = xStart + col * (keyWidth + spacing);
            uint8_t yPos = yStart + row * (keyHeight + spacing);

            // In ký tự của từng phím nếu không phải là phím trống
            if (currentKeys[row][col] != "") {
                // Xác định màu nền của phím
                uint16_t bgColor;
                if (row == cursorRow && col == cursorCol) {
                    bgColor = keySelectedColor;
                } else {
                    bgColor = keyBgColor;
                }
                
                // Thiết lập màu chữ và kích thước
                bg->setTextColor(keyTextColor, bgColor);  // Màu chữ và nền theo biến
                bg->setTextSize(textSize);  // Sử dụng biến textSize
                
                String keyText = currentKeys[row][col];
                // Rút gọn text nếu quá dài (chỉ lấy 1-2 ký tự đầu)
                if (keyText.length() > 1) {
                    keyText = keyText.substring(0, 2);
                }
                
                // Tính toán vị trí để căn giữa text (ước tính text width ~4px * textSize cho 1 ký tự)
                uint8_t textWidth = keyText.length() * 4 * textSize;  // Tính theo textSize
                uint8_t marginX = 2;  // Margin bên trái/phải để tránh che viền
                uint8_t textX = xPos + marginX;  // Bắt đầu từ cạnh trái với margin
                if (textWidth < keyWidth - (marginX * 2)) {
                    textX = xPos + (keyWidth - textWidth) / 2;  // Căn giữa nếu đủ chỗ, có margin tự động
                }
                uint8_t textY = yPos + 1;  // Sát cạnh trên
                
                bg->setCursor(textX - 1.5, textY);
                bg->print(keyText);  // In ký tự
            }
        }
    }

    // Vẽ phím Space ở hàng dưới cùng và căn giữa
    uint8_t spaceKeyWidth = 8 * (keyWidth + spacing) - spacing; // Phím Space chiếm 8 ô
    uint8_t spaceXStart = (128 - spaceKeyWidth) / 2; // Căn giữa theo chiều ngang
    uint8_t spaceYStart = yStart + 3 * (keyHeight + spacing); // Đặt phím Space ở hàng dưới cùng

    // BƯỚC 1: Vẽ nền phím Space trước
    uint16_t spaceBgColor;
    if (cursorRow == 3) {
        spaceBgColor = keySelectedColor; // Tô sáng phím Space khi chọn
    } else {
        spaceBgColor = keyBgColor;  // Màu nền phím bình thường
    }
    bg->fillRect(spaceXStart, spaceYStart, spaceKeyWidth, keyHeight, spaceBgColor);
    
    // BƯỚC 2: Điền chữ vào phím Space sau
    bg->setTextColor(keyTextColor, spaceBgColor);
    bg->setTextSize(textSize);  // Sử dụng biến textSize
    String spaceText = "Sp";  // Rút gọn để fit vào phím
    uint8_t spaceTextWidth = spaceText.length() * 4 * textSize;  // Tính theo textSize
    uint8_t spaceTextX = spaceXStart + (spaceKeyWidth - spaceTextWidth) / 2;
    uint8_t spaceTextY = spaceYStart + 1;  // Sát cạnh trên
    bg->setCursor(spaceTextX, spaceTextY);
    bg->print(spaceText);  // In text "Sp"

    // Đẩy sprite lên màn hình
    bg->pushSprite(0, 0);
}

void Keyboard::updateCurrentCursor() {
    // Lấy ký tự tại vị trí con trỏ hiện tại
    String currentKey = getCurrentKey();
    // Cập nhật thông tin ký tự hiện tại
    currentChar = currentKey;
}

// Hàm lấy ký tự hiện tại từ bàn phím
String Keyboard::getCurrentKey() const {
    if (cursorRow == 3) {
        return KEY_SPACE;  // Trả về phím Space nếu con trỏ ở hàng cuối cùng
    }

    String (*currentKeys)[10];
    if (isAlphabetMode) {
        currentKeys = qwertyKeysArray;  // Sử dụng bảng phím chữ cái
    } else {
        currentKeys = numericKeysArray; // Sử dụng bảng phím số/ký tự đặc biệt
    }

    return currentKeys[cursorRow][cursorCol];
}

void Keyboard::moveCursor(int8_t deltaRow, int8_t deltaCol) {
    cursorCol += deltaCol;
    cursorRow += deltaRow;

    // Kiểm tra và giới hạn vị trí con trỏ trong phạm vi bàn phím
    // Nếu đang ở hàng phím Space và nhấn "down", quay lại hàng đầu tiên
    if (cursorRow > 3) {  // Hàng phím Space là hàng 3
        cursorRow = 0;    // Quay lại hàng đầu tiên
    } else if (cursorRow < 0) {
        cursorRow = 3;    // Nếu nhấn "up" từ hàng đầu tiên, quay lại hàng phím Space
    }

    // Giới hạn vị trí cột
    if (cursorCol < 0) {
        cursorCol = 9;  // Nếu ở cột đầu tiên và nhấn "left", quay sang cột cuối cùng
    } else if (cursorCol > 9) {
        cursorCol = 0;  // Nếu ở cột cuối cùng và nhấn "right", quay sang cột đầu tiên
    }

    // Nếu con trỏ đang ở hàng Space (hàng 3), cố định nó ở giữa phím Space
    if (cursorRow == 3) {
        cursorCol = 4;  // Phím Space nằm ở giữa (cột 4)
    }

    updateCurrentCursor();  // Cập nhật ký tự hiện tại
    draw();  // Vẽ lại bàn phím
}

// Hàm xử lý khi người dùng nhấn phím
void Keyboard::moveCursorByCommand(String command, int x, int y) {
    if (command == "up") {
        moveCursor(-1, 0);  // Di chuyển lên
    } else if (command == "down") {
        moveCursor(1, 0);   // Di chuyển xuống
    } else if (command == "left") {
        moveCursor(0, -1);  // Di chuyển trái
    } else if (command == "right") {
        moveCursor(0, 1);   // Di chuyển phải
    } else if (command == "select") {
        String currentKey = getCurrentKey();
        // Nếu nhấn phím "123", chuyển sang bàn phím số/ký tự đặc biệt
        if (isAlphabetMode && currentKey == "123") {
            isAlphabetMode = false;  // Chuyển sang chế độ số/ký tự đặc biệt
            qwertyKeys = numericKeysArray;  // Cập nhật con trỏ bảng phím
            draw();  // Vẽ lại bàn phím số
        }
        // Nếu nhấn phím "ABC", quay lại bàn phím chữ cái
        else if (!isAlphabetMode && currentKey == "ABC") {
            isAlphabetMode = true;   // Quay lại chế độ chữ cái
            qwertyKeys = qwertyKeysArray;  // Cập nhật con trỏ bảng phím
            draw();  // Vẽ lại bàn phím chữ cái
        } else {
            // Gọi callback nếu có để xử lý phím được chọn
            if (onKeySelected != nullptr) {
                onKeySelected(currentKey);
            }
        }
    }
}

void Keyboard::turnOff() {
    bg->fillScreen(bgScreenColor);
    bg->pushSprite(0, 0);
}
