#include <Arduino.h>
#include "keyboard_landscape.h"
#include "keyboard_skins_wrapper.h"  // Include wrapper để có KeyboardSkins namespace
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
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

// Synthwave/Vaporwave color palette (theo hình arcade)
#define NEON_PURPLE 0xD81F        // Neon purple (màu chủ đạo của arcade)
#define NEON_GREEN 0x07E0         // Neon green (sáng như trong hình)
#define NEON_CYAN 0x07FF          // Neon cyan (sáng hơn)
#define YELLOW_ORANGE 0xFE20       // Yellow-orange (như "GAME OVER" text)
#define DARK_PURPLE 0x8010        // Dark purple background (theo hình arcade)
#define SOFT_WHITE 0xCE79         // Soft white (not too bright)

// Khởi tạo các static members
const String KeyboardLandscape::KEY_ENTER = "|e";
const String KeyboardLandscape::KEY_DELETE = "<";
const String KeyboardLandscape::KEY_SHIFT = "shift";
const String KeyboardLandscape::KEY_SPACE = " ";
const String KeyboardLandscape::KEY_ICON = "ic";

// Bảng phím chữ cái (chữ viết thường) - layout nằm ngang
String KeyboardLandscape::qwertyKeysArray[3][10] = {
  { "q", "w", "e", "r", "t", "y", "u", "i", "o", "p" },
  { "123", "a", "s", "d", "f", "g", "h", "j", "k", "l"},
  { "ic", "z", "x", "c", "v", "b", "n", "m", "|e", "<"}
};

// Bảng phím số và ký tự đặc biệt
String KeyboardLandscape::numericKeysArray[3][10] = {
  { "1", "2", "3", "4", "5", "6", "7", "8", "9", "0" },
  { "ABC", "/", ":", ";", "(", ")", "$", "&", "@", "\"" },
  { "ic", "#", ".", ",", "?", "!", "'", "-", "|e", "<"}
};

// Constructor
KeyboardLandscape::KeyboardLandscape(Adafruit_ST7789* tft) {
    this->tft = tft;
    this->cursorRow = 0;
    this->cursorCol = 0;
    this->kb_x = 0;
    this->kb_y = 0;  // Vị trí bàn phím cho màn hình 320x240 (nằm ngang)
    this->currentChar = "";
    this->selectedIndex = 0;
    this->isAlphabetMode = true;
    this->textSize = 1;
    this->animationFrame = 0;  // Khởi tạo animation frame
    this->onKeySelected = nullptr;  // Khởi tạo callback = null
    
    // Khởi tạo skin mặc định (synthwave/vaporwave theme)
    this->currentSkin = KeyboardSkins::getSynthwaveSkin();
    this->currentSkin.keyBgColor = DARK_PURPLE;
    this->currentSkin.keySelectedColor = YELLOW_ORANGE;
    this->currentSkin.keyTextColor = NEON_GREEN;
    this->currentSkin.bgScreenColor = 0x0000;
    applySkin();  // Áp dụng skin vào các biến màu sắc cũ
    
    // Khởi tạo bảng phím theo chế độ hiện tại
    if (isAlphabetMode) {
        this->qwertyKeys = qwertyKeysArray;
    } else {
        this->qwertyKeys = numericKeysArray;
    }
}

// Destructor
KeyboardLandscape::~KeyboardLandscape() {
    // Không cần giải phóng gì vì chỉ lưu con trỏ
}

void KeyboardLandscape::draw() {
    // Tăng animation frame cho hiệu ứng cyberpunk
    animationFrame++;
    if (animationFrame > 1000) animationFrame = 0;  // Reset sau 1000 frame
    
    // Sử dụng kích thước từ skin
    int16_t keyWidth = currentSkin.keyWidth;
    int16_t keyHeight = currentSkin.keyHeight;
    int16_t spacing = currentSkin.spacing;
    
    // XOAY 90 ĐỘ: Tính toán để căn giữa keyboard theo chiều dọc 240px (màn hình nằm ngang)
    // Sau khi xoay: 3 hàng x 10 cột -> 10 hàng x 3 cột
    // LƯU Ý: Với rotation 3 (-90 độ), màn hình 320x240, gốc tọa độ (0,0) ở góc dưới bên trái
    // X: 0 ở bên trái, tăng sang phải (bình thường)
    // Y: 0 ở dưới cùng, tăng lên trên (đảo ngược)
    int16_t keyboardHeight = 10 * (keyWidth + spacing) - spacing; // Tổng chiều cao 10 hàng (dùng keyWidth vì xoay)
    int16_t keyboardWidth = 3 * (keyHeight + spacing) - spacing; // Tổng chiều rộng 3 cột (dùng keyHeight vì xoay)
    
    // Với rotation 3 (-90 độ): màn hình 320x240, gốc (0,0) ở góc dưới bên trái
    int16_t screenWidth = 320;   // Chiều rộng màn hình sau rotation 3
    int16_t screenHeight = 240; // Chiều cao màn hình sau rotation 3
    // X bình thường với rotation 3, tính từ trái sang phải
    int16_t xStart = (screenWidth - keyboardWidth) / 2;  // Căn giữa theo chiều ngang
    // Y bị đảo ngược với rotation 3, nên tính từ dưới lên
    int16_t yStart = screenHeight - (screenHeight - keyboardHeight) / 2 - keyboardHeight;  // Căn giữa theo chiều dọc (từ dưới)

    // Chọn bảng phím dựa vào chế độ hiện tại
    String (*currentKeys)[10];
    if (isAlphabetMode) {
        currentKeys = qwertyKeysArray;  // Sử dụng bảng phím chữ cái
    } else {
        currentKeys = numericKeysArray; // Sử dụng bảng phím số/ký tự đặc biệt
    }

    // BƯỚC 1: Vẽ tất cả các phím (nền) trước - XOAY 90 ĐỘ và LẬT NGƯỢC, phù hợp với rotation 3 (-90 độ)
    // Duyệt qua col (0-9) và row (0-2) nhưng vẽ theo chiều xoay 90 độ, đảo ngược cả row và col để lật ngược
    // Với rotation 3: Y bị đảo ngược (0 ở dưới, tăng lên trên), nên cần đảo ngược col
    for (int16_t col = 0; col < 10; col++) {  // 10 cột trong mảng ban đầu -> 10 hàng sau khi xoay
        for (int16_t row = 0; row < 3; row++) {  // 3 hàng trong mảng ban đầu -> 3 cột sau khi xoay
            // XOAY 90 ĐỘ + LẬT NGƯỢC: đổi chỗ keyWidth và keyHeight, đảo ngược cả row và col
            // row: 0->cột 2 (phải), 1->cột 1 (giữa), 2->cột 0 (trái) - đảo ngược
            // col: 0->hàng 9 (dưới), 1->hàng 8, ..., 9->hàng 0 (trên) - đảo ngược vì Y bị đảo với rotation 3
            int16_t xPos = xStart + (2 - row) * (keyHeight + spacing);  // row đảo ngược: 0->2, 1->1, 2->0
            int16_t yPos = yStart + (9 - col) * (keyWidth + spacing);   // col đảo ngược: 0->9, 1->8, ..., 9->0 (vì Y đảo với rotation 3)

            // Vẽ nền phím
            int16_t bgColor;
            if (row == cursorRow && col == cursorCol) {
                bgColor = currentSkin.keySelectedColor; // Tô sáng phím hiện tại
            } else {
                bgColor = currentSkin.keyBgColor;  // Màu nền phím bình thường
            }
            
            // Vẽ phím với bo góc hoặc không - XOAY: đổi chỗ keyWidth và keyHeight
            if (currentSkin.roundedCorners && currentSkin.cornerRadius > 0) {
                tft->fillRoundRect(xPos, yPos, keyHeight, keyWidth, currentSkin.cornerRadius, bgColor);
                if (currentSkin.hasBorder) {
                    tft->drawRoundRect(xPos, yPos, keyHeight, keyWidth, currentSkin.cornerRadius, currentSkin.keyBorderColor);
                }
            } else {
                tft->fillRect(xPos, yPos, keyHeight, keyWidth, bgColor);
                if (currentSkin.hasBorder) {
                    tft->drawRect(xPos, yPos, keyHeight, keyWidth, currentSkin.keyBorderColor);
                }
            }
            
            // Vẽ hoa văn nữ tính nếu đang dùng feminine skin - XOAY 90: đổi chỗ keyWidth và keyHeight
            // Kiểm tra bằng cách so sánh màu nền (màu hồng đặc trưng)
            if (currentSkin.keyBgColor == 0xF81F || currentSkin.keyBgColor == 0xFE1F) {
                drawFemininePattern(xPos, yPos, keyHeight, keyWidth);
            }
            
            // Vẽ hiệu ứng cyberpunk/tron (grid pattern và glow) nếu đang dùng cyberpunk skin - XOAY 90
            // Kiểm tra bằng cách so sánh màu nền đen và viền cyan
            if (currentSkin.keyBgColor == 0x0000 && currentSkin.keyBorderColor == 0x07FF) {
                // Vẽ grid pattern nhỏ ở giữa phím - XOAY 90: đổi chỗ keyWidth và keyHeight
                int16_t gridColor = 0x07FF;  // Neon cyan
                // Vẽ đường kẻ ngang nhỏ ở giữa (sau khi xoay 90)
                tft->drawFastHLine(xPos + 2, yPos + keyWidth / 2, keyHeight - 4, gridColor);
                // Vẽ đường kẻ dọc nhỏ ở giữa (sau khi xoay 90)
                tft->drawFastVLine(xPos + keyHeight / 2, yPos + 2, keyWidth - 4, gridColor);
                
                // Vẽ hiệu ứng điện chạy xung quanh phím - XOAY 90: đổi chỗ keyWidth và keyHeight
                drawCyberpunkGlow(xPos, yPos, keyHeight, keyWidth, animationFrame);
            }
            
            // Vẽ pattern máy móc cơ học steampunk nếu đang dùng mechanical skin - XOAY 90
            // Kiểm tra bằng cách so sánh màu nền đỏ và viền đồng
            if (currentSkin.keyBgColor == 0x8000 && currentSkin.keyBorderColor == 0xFD20) {
                drawMechanicalPattern(xPos, yPos, keyHeight, keyWidth);
            }
        }
    }
    
    // Vẽ hiệu ứng điện xung quanh toàn bộ bàn phím (cyberpunk skin) - XOAY 90
    if (currentSkin.keyBgColor == 0x0000 && currentSkin.keyBorderColor == 0x07FF) {
        drawCyberpunkKeyboardBorder(xStart, yStart, keyboardWidth, keyboardHeight, animationFrame);
    }

    // BƯỚC 2: Điền chữ vào các phím sau - XOAY 90 ĐỘ và LẬT NGƯỢC, phù hợp với rotation 3 (-90 độ)
    for (int16_t col = 0; col < 10; col++) {  // 10 cột trong mảng ban đầu -> 10 hàng sau khi xoay
        for (int16_t row = 0; row < 3; row++) {  // 3 hàng trong mảng ban đầu -> 3 cột sau khi xoay
            // XOAY 90 ĐỘ + LẬT NGƯỢC: đổi chỗ keyWidth và keyHeight, đảo ngược cả row và col
            // row: 0->cột 2 (phải), 1->cột 1 (giữa), 2->cột 0 (trái) - đảo ngược
            // col: 0->hàng 9 (dưới), 1->hàng 8, ..., 9->hàng 0 (trên) - đảo ngược vì Y bị đảo với rotation 3
            int16_t xPos = xStart + (2 - row) * (keyHeight + spacing);  // row đảo ngược: 0->2, 1->1, 2->0
            int16_t yPos = yStart + (9 - col) * (keyWidth + spacing);   // col đảo ngược: 0->9, 1->8, ..., 9->0 (vì Y đảo với rotation 3)
            
            if (currentKeys[row][col].length() > 1) {
                xPos -= 1;
            }
            // In ký tự của từng phím nếu không phải là phím trống
            if (currentKeys[row][col] != "") {
                // Xác định màu nền và màu chữ của phím
                int16_t bgColor;
                int16_t textColor;
                if (row == cursorRow && col == cursorCol) {
                    bgColor = currentSkin.keySelectedColor;
                    // Luôn tự động tính màu chữ tương phản để đảm bảo dễ nhìn
                    textColor = getContrastColor(bgColor);
                } else {
                    bgColor = currentSkin.keyBgColor;
                    // Luôn tự động tính màu chữ tương phản để đảm bảo dễ nhìn
                    textColor = getContrastColor(bgColor);
                }
                
                // Thiết lập màu chữ và kích thước
                tft->setTextColor(textColor, bgColor);  // Màu chữ và nền từ skin
                tft->setTextSize(currentSkin.textSize);  // Sử dụng textSize từ skin
                
                String keyText = currentKeys[row][col];
                // Rút gọn text nếu quá dài (chỉ lấy 1-2 ký tự đầu)
                if (keyText.length() > 1) {
                    keyText = keyText.substring(0, 2);
                }
                
                // Tính toán vị trí để căn giữa text - XOAY 90: đổi chỗ keyWidth và keyHeight
                int16_t textWidth = keyText.length() * 6 * currentSkin.textSize;  // Tính theo textSize từ skin
                int16_t marginX = 2;  // Margin bên trái/phải để tránh che viền
                int16_t textX = xPos + marginX;  // Bắt đầu từ cạnh trái với margin
                if (textWidth < keyHeight - (marginX * 2)) {  // XOAY 90: dùng keyHeight thay vì keyWidth
                    textX = xPos + (keyHeight - textWidth) / 2;  // Căn giữa nếu đủ chỗ
                }
                int16_t textHeight = 8 * currentSkin.textSize;  // Text height ~8px * textSize
                int16_t textY = yPos + (keyWidth - textHeight) / 2;  // XOAY 90: dùng keyWidth thay vì keyHeight
                
                tft->setCursor(textX, textY);
                tft->print(keyText);  // In ký tự
            }
        }
    }

    // Vẽ phím Space ở cột bên phải sau khi xoay 90 độ và lật ngược - XOAY 90 + LẬT NGƯỢC, phù hợp với rotation 3
    // Sau khi xoay 90 và lật ngược, Space sẽ nằm ở cột bên phải (cột 2) thay vì hàng dưới
    int16_t spaceKeyHeight = 8 * (keyWidth + spacing) - spacing; // Phím Space chiếm 8 ô (dùng keyWidth vì xoay)
    int16_t spaceXStart = xStart + 2 * (keyHeight + spacing); // Đặt phím Space ở cột 2 (bên phải sau khi lật ngược)
    int16_t spaceYStart = yStart; // Bắt đầu từ cùng vị trí Y với bàn phím

    // BƯỚC 1: Vẽ nền phím Space trước
    int16_t spaceBgColor;
    if (cursorRow == 3) {
        spaceBgColor = currentSkin.keySelectedColor; // Tô sáng phím Space khi chọn
    } else {
        spaceBgColor = currentSkin.keyBgColor;  // Màu nền phím bình thường
    }
    
    // Vẽ phím Space với bo góc hoặc không - XOAY: đổi chỗ keyWidth và keyHeight
    if (currentSkin.roundedCorners && currentSkin.cornerRadius > 0) {
        tft->fillRoundRect(spaceXStart, spaceYStart, keyHeight, spaceKeyHeight, currentSkin.cornerRadius, spaceBgColor);
        if (currentSkin.hasBorder) {
            tft->drawRoundRect(spaceXStart, spaceYStart, keyHeight, spaceKeyHeight, currentSkin.cornerRadius, currentSkin.keyBorderColor);
        }
    } else {
        tft->fillRect(spaceXStart, spaceYStart, keyHeight, spaceKeyHeight, spaceBgColor);
        if (currentSkin.hasBorder) {
            tft->drawRect(spaceXStart, spaceYStart, keyHeight, spaceKeyHeight, currentSkin.keyBorderColor);
        }
    }
    
    // BƯỚC 2: Điền chữ vào phím Space sau
    // Luôn tự động tính màu chữ tương phản để đảm bảo dễ nhìn
    int16_t spaceTextColor = getContrastColor(spaceBgColor);
    tft->setTextColor(spaceTextColor, spaceBgColor);
    tft->setTextSize(currentSkin.textSize);  // Sử dụng textSize từ skin
}

void KeyboardLandscape::updateCurrentCursor() {
    // Lấy ký tự tại vị trí con trỏ hiện tại
    String currentKey = getCurrentKey();
    // Cập nhật thông tin ký tự hiện tại
    currentChar = currentKey;
}

// Hàm lấy ký tự hiện tại từ bàn phím
String KeyboardLandscape::getCurrentKey() const {
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

void KeyboardLandscape::moveCursor(int8_t deltaRow, int8_t deltaCol) {
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
void KeyboardLandscape::moveCursorByCommand(String command, int x, int y) {
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

void KeyboardLandscape::moveCursorTo(int16_t row, int8_t col) {
    cursorRow = row;
    cursorCol = col;
    
    // Giới hạn vị trí
    if (cursorRow > 3) cursorRow = 3;
    if (cursorCol < 0) cursorCol = 0;
    if (cursorCol > 9) cursorCol = 9;
    
    updateCurrentCursor();
    draw();
}

bool KeyboardLandscape::typeChar(char c) {
    // Chuyển đổi ký tự thành String để so sánh
    String charStr = String(c);
    
    // Kiểm tra xem ký tự có phải là số không
    bool isDigit = (c >= '0' && c <= '9');
    
    // Kiểm tra xem ký tự có phải là chữ cái không
    bool isLetter = (c >= 'a' && c <= 'z');
    
    // Nếu là số hoặc ký tự đặc biệt, cần chuyển sang chế độ số
    if ((isDigit || (!isLetter && c != ' ')) && isAlphabetMode) {
        // Tìm phím "123" để chuyển sang chế độ số
        // "123" ở hàng 1, cột 0 trong bảng chữ cái
        moveCursorTo(1, 0);
        delay(100);
        moveCursorByCommand("select", 0, 0);
        delay(100);
    }
    // Nếu là chữ cái, cần chuyển về chế độ chữ cái
    else if (isLetter && !isAlphabetMode) {
        // Tìm phím "ABC" để chuyển về chế độ chữ cái
        // "ABC" ở hàng 1, cột 0 trong bảng số
        moveCursorTo(1, 0);
        delay(100);
        moveCursorByCommand("select", 0, 0);
        delay(100);
    }
    
    // Tìm ký tự trong bảng phím hiện tại
    String (*currentKeys)[10];
    if (isAlphabetMode) {
        currentKeys = qwertyKeysArray;
    } else {
        currentKeys = numericKeysArray;
    }
    
    for (int16_t row = 0; row < 3; row++) {
        for (int16_t col = 0; col < 10; col++) {
            String key = currentKeys[row][col];
            // So sánh ký tự (bỏ qua các phím đặc biệt như "123", "ABC", "|e", "<", "ic")
            if (key.length() == 1 && key.charAt(0) == c) {
                // Di chuyển đến ký tự này
                moveCursorTo(row, col);
                delay(150);  // Delay để người dùng thấy di chuyển
                // Nhấn select
                moveCursorByCommand("select", 0, 0);
                delay(150);
                return true;
            }
        }
    }
    
    // Nếu không tìm thấy, thử tìm trong bảng kia
    if (isAlphabetMode) {
        currentKeys = numericKeysArray;
    } else {
        currentKeys = qwertyKeysArray;
    }
    
    for (int16_t row = 0; row < 3; row++) {
        for (int16_t col = 0; col < 10; col++) {
            String key = currentKeys[row][col];
            if (key.length() == 1 && key.charAt(0) == c) {
                // Chuyển đổi chế độ trước
                if (isAlphabetMode) {
                    moveCursorTo(1, 0);  // "123"
                } else {
                    moveCursorTo(1, 0);  // "ABC"
                }
                delay(100);
                moveCursorByCommand("select", 0, 0);
                delay(100);
                
                // Di chuyển đến ký tự này
                moveCursorTo(row, col);
                delay(150);
                moveCursorByCommand("select", 0, 0);
                delay(150);
                return true;
            }
        }
    }
    
    return false;  // Không tìm thấy ký tự
}

void KeyboardLandscape::typeString(String text) {
    for (int16_t i = 0; i < text.length(); i++) {
        char c = text.charAt(i);
        typeChar(c);
        delay(150);  // Delay giữa các ký tự
    }
}

void KeyboardLandscape::turnOff() {
    tft->fillScreen(currentSkin.bgScreenColor);
}

// Skin management methods
void KeyboardLandscape::setSkin(const KeyboardSkin& skin) {
    currentSkin = skin;
    applySkin();
}

void KeyboardLandscape::applySkin() {
    // Áp dụng skin vào các biến màu sắc cũ để backward compatibility
    keyBgColor = currentSkin.keyBgColor;
    keySelectedColor = currentSkin.keySelectedColor;
    keyTextColor = currentSkin.keyTextColor;
    bgScreenColor = currentSkin.bgScreenColor;
    textSize = currentSkin.textSize;
}

// Vẽ hoa văn nữ tính (trái tim, hoa, pattern) trên phím
void KeyboardLandscape::drawFemininePattern(int16_t x, int16_t y, int16_t width, int16_t height) {
    // Chỉ vẽ hoa văn nhỏ ở góc phím, không che chữ
    int16_t patternColor = 0xFFFF;  // Màu trắng cho hoa văn
    
    // Vẽ trái tim nhỏ ở góc trên bên phải
    int16_t heartX = x + width - 6;
    int16_t heartY = y + 2;
    
    // Vẽ trái tim đơn giản (3 pixel)
    tft->drawPixel(heartX, heartY, patternColor);
    tft->drawPixel(heartX + 1, heartY, patternColor);
    tft->drawPixel(heartX, heartY + 1, patternColor);
    
    // Vẽ hoa nhỏ ở góc dưới bên trái
    int16_t flowerX = x + 2;
    int16_t flowerY = y + height - 4;
    
    // Vẽ hoa 5 cánh đơn giản
    tft->drawPixel(flowerX, flowerY, patternColor);      // Cánh giữa
    tft->drawPixel(flowerX - 1, flowerY - 1, patternColor);  // Cánh trên trái
    tft->drawPixel(flowerX + 1, flowerY - 1, patternColor);  // Cánh trên phải
    tft->drawPixel(flowerX - 1, flowerY + 1, patternColor);  // Cánh dưới trái
    tft->drawPixel(flowerX + 1, flowerY + 1, patternColor);  // Cánh dưới phải
    
    // Vẽ pattern nhỏ ở giữa (chấm tròn)
    if (width > 15 && height > 15) {
        int16_t dotX = x + width / 2;
        int16_t dotY = y + height / 2;
        tft->drawPixel(dotX, dotY, patternColor);
    }
}

// Vẽ pattern máy móc cơ học steampunk (bánh răng, đường kẻ cơ khí) trên phím
void KeyboardLandscape::drawMechanicalPattern(int16_t x, int16_t y, int16_t width, int16_t height) {
    int16_t gearColor = 0xFD20;     // Copper/Bronze (đồng nâu)
    int16_t accentColor = 0xFA00;   // Orange/rust (cam gỉ)
    int16_t rivetColor = 0xFFE0;    // Golden yellow (vàng sáng)

    // Vẽ bánh răng nhỏ ở góc trên bên trái
    int16_t gearX1 = x + 4;
    int16_t gearY1 = y + 4;
    int16_t smallGearRadius = 3;
    tft->drawCircle(gearX1, gearY1, smallGearRadius, gearColor);
    tft->drawPixel(gearX1, gearY1, accentColor); // Tâm bánh răng
    // Thêm 4 răng nhỏ
    tft->drawPixel(gearX1, gearY1 - smallGearRadius - 1, gearColor);
    tft->drawPixel(gearX1, gearY1 + smallGearRadius + 1, gearColor);
    tft->drawPixel(gearX1 - smallGearRadius - 1, gearY1, gearColor);
    tft->drawPixel(gearX1 + smallGearRadius + 1, gearY1, gearColor);

    // Vẽ bánh răng nhỏ ở góc dưới bên phải
    int16_t gearX2 = x + width - 5;
    int16_t gearY2 = y + height - 5;
    tft->drawCircle(gearX2, gearY2, smallGearRadius, gearColor);
    tft->drawPixel(gearX2, gearY2, accentColor); // Tâm bánh răng
    // Thêm 4 răng nhỏ
    tft->drawPixel(gearX2, gearY2 - smallGearRadius - 1, gearColor);
    tft->drawPixel(gearX2, gearY2 + smallGearRadius + 1, gearColor);
    tft->drawPixel(gearX2 - smallGearRadius - 1, gearY2, gearColor);
    tft->drawPixel(gearX2 + smallGearRadius + 1, gearY2, gearColor);

    // Vẽ các điểm rivet (đinh tán) ở 4 góc
    tft->fillCircle(x + 2, y + 2, 1, rivetColor);
    tft->fillCircle(x + width - 3, y + 2, 1, rivetColor);
    tft->fillCircle(x + 2, y + height - 3, 1, rivetColor);
    tft->fillCircle(x + width - 3, y + height - 3, 1, rivetColor);

    // Vẽ các đường kẻ mỏng ở cạnh (texture kim loại)
    tft->drawFastHLine(x + 1, y + 1, width - 2, accentColor); // Top edge
    tft->drawFastHLine(x + 1, y + height - 2, width - 2, accentColor); // Bottom edge
    tft->drawFastVLine(x + 1, y + 1, height - 2, accentColor); // Left edge
    tft->drawFastVLine(x + width - 2, y + 1, height - 2, accentColor); // Right edge
}

// Vẽ hiệu ứng điện chạy xung quanh phím (cyberpunk/tron style)
void KeyboardLandscape::drawCyberpunkGlow(int16_t x, int16_t y, int16_t width, int16_t height, int16_t animationFrame) {
    int16_t glowColor = 0x07FF;  // Neon cyan
    int16_t brightGlow = 0xFFFF;  // White (sáng nhất)
    
    // Tính vị trí điện chạy dựa trên animation frame
    // Điện chạy theo chu vi phím (4 cạnh)
    int16_t perimeter = (width + height) * 2;
    int16_t currentPos = (animationFrame * 2) % perimeter;  // Tốc độ chạy
    
    // Vẽ đường điện chạy quanh viền phím
    // Cạnh trên (trái -> phải)
    if (currentPos < width) {
        int16_t glowX = x + currentPos;
        tft->drawPixel(glowX, y, brightGlow);
        tft->drawPixel(glowX, y + 1, glowColor);
        if (currentPos > 0) tft->drawPixel(glowX - 1, y, glowColor);
        if (currentPos < width - 1) tft->drawPixel(glowX + 1, y, glowColor);
    }
    // Cạnh phải (trên -> dưới)
    else if (currentPos < width + height) {
        int16_t glowY = y + (currentPos - width);
        tft->drawPixel(x + width - 1, glowY, brightGlow);
        tft->drawPixel(x + width - 2, glowY, glowColor);
        if (glowY > y) tft->drawPixel(x + width - 1, glowY - 1, glowColor);
        if (glowY < y + height - 1) tft->drawPixel(x + width - 1, glowY + 1, glowColor);
    }
    // Cạnh dưới (phải -> trái)
    else if (currentPos < width * 2 + height) {
        int16_t glowX = x + width - 1 - (currentPos - width - height);
        tft->drawPixel(glowX, y + height - 1, brightGlow);
        tft->drawPixel(glowX, y + height - 2, glowColor);
        if (glowX > x) tft->drawPixel(glowX - 1, y + height - 1, glowColor);
        if (glowX < x + width - 1) tft->drawPixel(glowX + 1, y + height - 1, glowColor);
    }
    // Cạnh trái (dưới -> trên)
    else {
        int16_t glowY = y + height - 1 - (currentPos - width * 2 - height);
        tft->drawPixel(x, glowY, brightGlow);
        tft->drawPixel(x + 1, glowY, glowColor);
        if (glowY > y) tft->drawPixel(x, glowY - 1, glowColor);
        if (glowY < y + height - 1) tft->drawPixel(x, glowY + 1, glowColor);
    }
    
    // Vẽ đuôi điện (trail) - các điểm phía sau
    for (int16_t i = 1; i <= 3; i++) {
        int16_t trailPos = (currentPos - i + perimeter) % perimeter;
        int16_t trailBrightness = 0x07FF - (i * 0x1000);  // Mờ dần
        
        // Cạnh trên
        if (trailPos < width) {
            int16_t trailX = x + trailPos;
            if (trailX >= x && trailX < x + width) {
                tft->drawPixel(trailX, y, trailBrightness);
            }
        }
        // Cạnh phải
        else if (trailPos < width + height) {
            int16_t trailY = y + (trailPos - width);
            if (trailY >= y && trailY < y + height) {
                tft->drawPixel(x + width - 1, trailY, trailBrightness);
            }
        }
        // Cạnh dưới
        else if (trailPos < width * 2 + height) {
            int16_t trailX = x + width - 1 - (trailPos - width - height);
            if (trailX >= x && trailX < x + width) {
                tft->drawPixel(trailX, y + height - 1, trailBrightness);
            }
        }
        // Cạnh trái
        else {
            int16_t trailY = y + height - 1 - (trailPos - width * 2 - height);
            if (trailY >= y && trailY < y + height) {
                tft->drawPixel(x, trailY, trailBrightness);
            }
        }
    }
}

// Vẽ hiệu ứng điện chạy xung quanh toàn bộ bàn phím (cyberpunk/tron style)
void KeyboardLandscape::drawCyberpunkKeyboardBorder(int16_t x, int16_t y, int16_t width, int16_t height, int16_t animationFrame) {
    int16_t glowColor = 0x07FF;  // Neon cyan
    int16_t brightGlow = 0xFFFF;  // White (sáng nhất)
    
    // Tính vị trí điện chạy dựa trên animation frame
    // Điện chạy theo chu vi toàn bộ bàn phím (4 cạnh)
    int16_t perimeter = (width + height) * 2;
    int16_t currentPos = (animationFrame * 3) % perimeter;  // Tốc độ chạy nhanh hơn (x3)
    
    // Vẽ đường điện chạy quanh viền ngoài bàn phím
    // Cạnh trên (trái -> phải)
    if (currentPos < width) {
        int16_t glowX = x + currentPos;
        // Vẽ điểm sáng chính
        tft->drawPixel(glowX, y, brightGlow);
        tft->drawPixel(glowX, y + 1, glowColor);
        // Vẽ glow xung quanh
        if (currentPos > 0) {
            tft->drawPixel(glowX - 1, y, glowColor);
            tft->drawPixel(glowX - 1, y + 1, glowColor - 0x1000);
        }
        if (currentPos < width - 1) {
            tft->drawPixel(glowX + 1, y, glowColor);
            tft->drawPixel(glowX + 1, y + 1, glowColor - 0x1000);
        }
    }
    // Cạnh phải (trên -> dưới)
    else if (currentPos < width + height) {
        int16_t glowY = y + (currentPos - width);
        tft->drawPixel(x + width - 1, glowY, brightGlow);
        tft->drawPixel(x + width - 2, glowY, glowColor);
        if (glowY > y) {
            tft->drawPixel(x + width - 1, glowY - 1, glowColor);
            tft->drawPixel(x + width - 2, glowY - 1, glowColor - 0x1000);
        }
        if (glowY < y + height - 1) {
            tft->drawPixel(x + width - 1, glowY + 1, glowColor);
            tft->drawPixel(x + width - 2, glowY + 1, glowColor - 0x1000);
        }
    }
    // Cạnh dưới (phải -> trái)
    else if (currentPos < width * 2 + height) {
        int16_t glowX = x + width - 1 - (currentPos - width - height);
        tft->drawPixel(glowX, y + height - 1, brightGlow);
        tft->drawPixel(glowX, y + height - 2, glowColor);
        if (glowX > x) {
            tft->drawPixel(glowX - 1, y + height - 1, glowColor);
            tft->drawPixel(glowX - 1, y + height - 2, glowColor - 0x1000);
        }
        if (glowX < x + width - 1) {
            tft->drawPixel(glowX + 1, y + height - 1, glowColor);
            tft->drawPixel(glowX + 1, y + height - 2, glowColor - 0x1000);
        }
    }
    // Cạnh trái (dưới -> trên)
    else {
        int16_t glowY = y + height - 1 - (currentPos - width * 2 - height);
        tft->drawPixel(x, glowY, brightGlow);
        tft->drawPixel(x + 1, glowY, glowColor);
        if (glowY > y) {
            tft->drawPixel(x, glowY - 1, glowColor);
            tft->drawPixel(x + 1, glowY - 1, glowColor - 0x1000);
        }
        if (glowY < y + height - 1) {
            tft->drawPixel(x, glowY + 1, glowColor);
            tft->drawPixel(x + 1, glowY + 1, glowColor - 0x1000);
        }
    }
    
    // Vẽ đuôi điện (trail) - các điểm phía sau với độ dài dài hơn
    for (int16_t i = 1; i <= 8; i++) {
        int16_t trailPos = (currentPos - i + perimeter) % perimeter;
        // Tính độ sáng giảm dần
        int16_t trailBrightness;
        if (i <= 3) {
            trailBrightness = glowColor - (i * 0x2000);
        } else if (i <= 6) {
            trailBrightness = glowColor - (3 * 0x2000) - ((i - 3) * 0x3000);
        } else {
            trailBrightness = glowColor - (3 * 0x2000) - (3 * 0x3000) - ((i - 6) * 0x4000);
        }
        // Đảm bảo không âm
        if (trailBrightness > 0x07FF) trailBrightness = 0x0000;
        
        // Cạnh trên
        if (trailPos < width) {
            int16_t trailX = x + trailPos;
            if (trailX >= x && trailX < x + width) {
                tft->drawPixel(trailX, y, trailBrightness);
            }
        }
        // Cạnh phải
        else if (trailPos < width + height) {
            int16_t trailY = y + (trailPos - width);
            if (trailY >= y && trailY < y + height) {
                tft->drawPixel(x + width - 1, trailY, trailBrightness);
            }
        }
        // Cạnh dưới
        else if (trailPos < width * 2 + height) {
            int16_t trailX = x + width - 1 - (trailPos - width - height);
            if (trailX >= x && trailX < x + width) {
                tft->drawPixel(trailX, y + height - 1, trailBrightness);
            }
        }
        // Cạnh trái
        else {
            int16_t trailY = y + height - 1 - (trailPos - width * 2 - height);
            if (trailY >= y && trailY < y + height) {
                tft->drawPixel(x, trailY, trailBrightness);
            }
        }
    }
}

// Tính màu chữ tương phản dựa trên màu nền
// Sử dụng công thức tính độ sáng (luminance) để quyết định dùng màu đen hay trắng
int16_t KeyboardLandscape::getContrastColor(int16_t bgColor) {
    // Chuyển đổi RGB565 sang RGB
    uint8_t r = (bgColor >> 11) & 0x1F;  // 5 bits red
    uint8_t g = (bgColor >> 5) & 0x3F;   // 6 bits green
    uint8_t b = bgColor & 0x1F;          // 5 bits blue
    
    // Mở rộng từ 5/6 bits sang 8 bits
    r = (r << 3) | (r >> 2);
    g = (g << 2) | (g >> 4);
    b = (b << 3) | (b >> 2);
    
    // Tính độ sáng (luminance) theo công thức ITU-R BT.709
    // L = 0.299*R + 0.587*G + 0.114*B
    int16_t luminance = (299 * r + 587 * g + 114 * b) / 1000;
    
    // Nếu nền sáng (luminance > 128), dùng chữ đen, ngược lại dùng chữ trắng
    if (luminance > 128) {
        return 0x0000;  // Black text
    } else {
        return 0xFFFF;  // White text
    }
}

