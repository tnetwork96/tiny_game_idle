#include <Arduino.h>
#include "keyboard.h"
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
const String Keyboard::KEY_ENTER = "|e";
const String Keyboard::KEY_DELETE = "<";
const String Keyboard::KEY_SHIFT = "shift";
const String Keyboard::KEY_SPACE = " ";
const String Keyboard::KEY_ICON = String(Keyboard::KEY_ICON_CHAR);

// Bảng phím chữ cái (chữ viết thường)
String Keyboard::qwertyKeysArray[3][10] = {
  { "q", "w", "e", "r", "t", "y", "u", "i", "o", "p" },
  { "123", "a", "s", "d", "f", "g", "h", "j", "k", "l"},
  { "shift", "z", "x", "c", "v", "b", "n", "m", "|e", "<"}
};

// Bảng phím số và ký tự đặc biệt
String Keyboard::numericKeysArray[3][10] = {
  { "1", "2", "3", "4", "5", "6", "7", "8", "9", "0" },
  { "ABC", "/", ":", ";", "(", ")", "$", "&", "@", "\"" },
  { String(Keyboard::KEY_ICON_CHAR), "#", ".", ",", "?", "!", "'", "-", "|e", "<"}
};

// Bảng phím icon/emoji (dùng control char để chèn vào text)
String Keyboard::iconKeysArray[3][10] = {
  { "ABC", String(Keyboard::ICON_SMILE), String(Keyboard::ICON_HEART), String(Keyboard::ICON_STAR), String(Keyboard::ICON_CHECK), String(Keyboard::ICON_MUSIC), String(Keyboard::ICON_SUN), String(Keyboard::ICON_FIRE), String(Keyboard::ICON_THUMBS), String(Keyboard::ICON_GIFT) },
  { "123", String(Keyboard::ICON_WINK), String(Keyboard::ICON_SMILE), String(Keyboard::ICON_HEART), String(Keyboard::ICON_STAR), String(Keyboard::ICON_CHECK), String(Keyboard::ICON_MUSIC), String(Keyboard::ICON_SUN), String(Keyboard::ICON_FIRE), String(Keyboard::ICON_THUMBS) },
  { String(Keyboard::KEY_ICON_CHAR), String(Keyboard::ICON_GIFT), String(Keyboard::ICON_SMILE), String(Keyboard::ICON_HEART), String(Keyboard::ICON_STAR), String(Keyboard::ICON_CHECK), String(Keyboard::ICON_MUSIC), String(Keyboard::ICON_SUN), "|e", "<" }
};

// Constructor
Keyboard::Keyboard(Adafruit_ST7789* tft) {
    this->tft = tft;
    this->cursorRow = 0;
    this->cursorCol = 0;
    this->kb_x = 0;
    this->kb_y = 0;  // Vị trí bàn phím cho màn hình 320x240 với rotation 3 (ở dưới cùng vì Y đảo ngược)
    this->currentChar = "";
    this->selectedIndex = 0;
    this->isAlphabetMode = true;
    this->isUppercaseMode = false;
    this->isIconMode = false;
    this->textSize = 1;
    this->animationFrame = 0;  // Khởi tạo animation frame
    this->onKeySelected = nullptr;  // Khởi tạo callback = null
    this->drawingEnabled = true;  // Enable drawing by default
    
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
Keyboard::~Keyboard() {
    // Không cần giải phóng gì vì chỉ lưu con trỏ
}

void Keyboard::draw() {
    // Don't draw if drawing is disabled (e.g., when social screen is active)
    if (!drawingEnabled) {
        return;
    }
    
    // Tăng animation frame cho hiệu ứng cyberpunk
    animationFrame++;
    if (animationFrame > 1000) animationFrame = 0;  // Reset sau 1000 frame
    
    // Sử dụng kích thước từ skin
    uint16_t keyWidth = currentSkin.keyWidth;
    uint16_t keyHeight = currentSkin.keyHeight;
    uint16_t spacing = currentSkin.spacing;
    
    // Tính toán để căn giữa keyboard - phù hợp với rotation 3 (-90 độ)
    // Với rotation 3: màn hình 320x240, gốc (0,0) ở góc dưới bên trái
    // X: 0 ở trái, tăng sang phải (bình thường)
    // Y: 0 ở dưới, tăng lên trên (đảo ngược)
    uint16_t keyboardWidth = 10 * (keyWidth + spacing) - spacing; // Tổng chiều rộng 10 phím
    uint16_t keyboardHeight = 3 * (keyHeight + spacing) - spacing; // Tổng chiều cao 3 hàng phím
    uint16_t screenWidth = 320;   // Chiều rộng màn hình sau rotation 3
    uint16_t screenHeight = 240;  // Chiều cao màn hình sau rotation 3
    
    uint16_t xStart = (screenWidth - keyboardWidth) / 2;  // Căn giữa theo chiều ngang 320px
    // Với rotation 3, Y đảo ngược (0 ở dưới, tăng lên trên)
    // Giảm yStart để bàn phím có khoảng cách với đáy, không quá sát
    uint16_t yStart = 120;  // Bàn phím cách đáy một chút (giảm từ 130 để có khoảng cách)

    // Chọn bảng phím dựa vào chế độ hiện tại
    String (*currentKeys)[10];
    if (isIconMode) {
        currentKeys = iconKeysArray;    // Icon mode
    } else if (isAlphabetMode) {
        currentKeys = qwertyKeysArray;  // Sử dụng bảng phím chữ cái
    } else {
        currentKeys = numericKeysArray; // Sử dụng bảng phím số/ký tự đặc biệt
    }

    // BƯỚC 1: Vẽ tất cả các phím (nền) trước
    for (uint16_t row = 0; row < 3; row++) {  // Điều chỉnh để duyệt qua 3 hàng
        for (uint16_t col = 0; col < 10; col++) {
            uint16_t xPos = xStart + col * (keyWidth + spacing);
            uint16_t yPos = yStart + row * (keyHeight + spacing);

            // Vẽ nền phím
            uint16_t bgColor;
            if (row == cursorRow && col == cursorCol) {
                bgColor = currentSkin.keySelectedColor; // Tô sáng phím hiện tại
            } else {
                bgColor = currentSkin.keyBgColor;  // Màu nền phím bình thường
            }
            
            // Vẽ phím với bo góc hoặc không
            if (currentSkin.roundedCorners && currentSkin.cornerRadius > 0) {
                tft->fillRoundRect(xPos, yPos, keyWidth, keyHeight, currentSkin.cornerRadius, bgColor);
                if (currentSkin.hasBorder) {
                    tft->drawRoundRect(xPos, yPos, keyWidth, keyHeight, currentSkin.cornerRadius, currentSkin.keyBorderColor);
                }
            } else {
            tft->fillRect(xPos, yPos, keyWidth, keyHeight, bgColor);
                if (currentSkin.hasBorder) {
                    tft->drawRect(xPos, yPos, keyWidth, keyHeight, currentSkin.keyBorderColor);
                }
            }
            
            // Vẽ hoa văn nữ tính nếu đang dùng feminine skin
            // Kiểm tra bằng cách so sánh màu nền (màu hồng đặc trưng)
            if (currentSkin.keyBgColor == 0xF81F || currentSkin.keyBgColor == 0xFE1F) {
                drawFemininePattern(xPos, yPos, keyWidth, keyHeight);
            }
            
            // Vẽ hiệu ứng cyberpunk/tron (grid pattern và glow) nếu đang dùng cyberpunk skin
            // Kiểm tra bằng cách so sánh màu nền đen và viền cyan
            if (currentSkin.keyBgColor == 0x0000 && currentSkin.keyBorderColor == 0x07FF) {
                // Vẽ grid pattern nhỏ ở giữa phím
                uint16_t gridColor = 0x07FF;  // Neon cyan
                // Vẽ đường kẻ ngang nhỏ ở giữa
                tft->drawFastHLine(xPos + 2, yPos + keyHeight / 2, keyWidth - 4, gridColor);
                // Vẽ đường kẻ dọc nhỏ ở giữa
                tft->drawFastVLine(xPos + keyWidth / 2, yPos + 2, keyHeight - 4, gridColor);
                
                // Vẽ hiệu ứng điện chạy xung quanh phím
                drawCyberpunkGlow(xPos, yPos, keyWidth, keyHeight, animationFrame);
            }
            
            // Vẽ pattern máy móc cơ học steampunk nếu đang dùng mechanical skin
            // Kiểm tra bằng cách so sánh màu nền đỏ và viền đồng
            if (currentSkin.keyBgColor == 0x8000 && currentSkin.keyBorderColor == 0xFD20) {
                drawMechanicalPattern(xPos, yPos, keyWidth, keyHeight);
            }
        }
    }
    
    // Vẽ hiệu ứng điện xung quanh toàn bộ bàn phím (cyberpunk skin)
    if (currentSkin.keyBgColor == 0x0000 && currentSkin.keyBorderColor == 0x07FF) {
        uint16_t keyboardHeight = 3 * (keyHeight + spacing) - spacing; // Tổng chiều cao 3 hàng
        drawCyberpunkKeyboardBorder(xStart, yStart, keyboardWidth, keyboardHeight, animationFrame);
    }

    // BƯỚC 2: Điền chữ vào các phím sau
    for (uint16_t row = 0; row < 3; row++) {
        for (uint16_t col = 0; col < 10; col++) {

            uint16_t xPos = xStart + col * (keyWidth + spacing);
            uint16_t yPos = yStart + row * (keyHeight + spacing) - 2;
            if (currentKeys[row][col].length() > 1) {
                xPos -= 1;
            }
            // In ký tự của từng phím nếu không phải là phím trống
            if (currentKeys[row][col] != "") {
                // Xác định màu nền và màu chữ của phím
                uint16_t bgColor;
                uint16_t textColor;
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
                
                String rawKey = currentKeys[row][col];
                String keyText = formatKeyLabel(rawKey);
                
                // Nếu là phím Enter, vẽ ký tự Enter (mũi tên) thay vì text
                if (rawKey == KEY_ENTER) {
                    drawEnterSymbol(xPos, yPos, keyWidth, keyHeight, textColor);
                }
                // Nếu là phím icon toggle, vẽ mặt cười thay vì text
                else if (rawKey == KEY_ICON) {
                    drawSmileyIcon(xPos, yPos, keyWidth, keyHeight, textColor);
                }
                // Nếu là phím icon glyph, vẽ icon tương ứng
                else if (isIconToken(rawKey)) {
                    drawIconGlyph(xPos, yPos, keyWidth, keyHeight, textColor, toIconCode(rawKey));
                } else {
                // Rút gọn text nếu quá dài (chỉ lấy 1-2 ký tự đầu)
                if (keyText.length() > 1) {
                    keyText = keyText.substring(0, 2);
                }
                
                // Tính toán vị trí để căn giữa text (ước tính text width ~6px * textSize cho 1 ký tự với font size 1)
                uint16_t textWidth = keyText.length() * 6 * currentSkin.textSize;  // Tính theo textSize từ skin
                uint16_t marginX = 2;  // Margin bên trái/phải để tránh che viền
                uint16_t textX = xPos + marginX;  // Bắt đầu từ cạnh trái với margin
                if (textWidth < keyWidth - (marginX * 2)) {
                    textX = xPos + (keyWidth - textWidth) / 2;  // Căn giữa nếu đủ chỗ
                }
                uint16_t textY = yPos + (keyHeight - 8) / 2;  // Căn giữa theo chiều dọc (text height ~8px với size 1)
                
                tft->setCursor(textX, textY);
                tft->print(keyText);  // In ký tự
                }
            }
        }
    }

    // Vẽ phím Space ở hàng dưới cùng và căn giữa - phù hợp với rotation 3
    uint16_t spaceKeyWidth = 8 * (keyWidth + spacing) - spacing; // Phím Space chiếm 8 ô
    uint16_t spaceXStart = (screenWidth - spaceKeyWidth) / 2; // Căn giữa theo chiều ngang (320px width)
    // Với rotation 3, Y đảo ngược, Space sẽ ở trên cùng của bàn phím (hàng 3)
    uint16_t spaceYStart = yStart + 3 * (keyHeight + spacing); // Đặt phím Space ở hàng 3 (trên cùng của bàn phím)

    // BƯỚC 1: Vẽ nền phím Space trước
    uint16_t spaceBgColor;
    if (cursorRow == 3) {
        spaceBgColor = currentSkin.keySelectedColor; // Tô sáng phím Space khi chọn
    } else {
        spaceBgColor = currentSkin.keyBgColor;  // Màu nền phím bình thường
    }
    
    // Vẽ phím Space với bo góc hoặc không
    if (currentSkin.roundedCorners && currentSkin.cornerRadius > 0) {
        tft->fillRoundRect(spaceXStart, spaceYStart, spaceKeyWidth, keyHeight, currentSkin.cornerRadius, spaceBgColor);
        if (currentSkin.hasBorder) {
            tft->drawRoundRect(spaceXStart, spaceYStart, spaceKeyWidth, keyHeight, currentSkin.cornerRadius, currentSkin.keyBorderColor);
        }
    } else {
    tft->fillRect(spaceXStart, spaceYStart, spaceKeyWidth, keyHeight, spaceBgColor);
        if (currentSkin.hasBorder) {
            tft->drawRect(spaceXStart, spaceYStart, spaceKeyWidth, keyHeight, currentSkin.keyBorderColor);
        }
    }
    
    // BƯỚC 2: Điền chữ vào phím Space sau
    // Luôn tự động tính màu chữ tương phản để đảm bảo dễ nhìn
    uint16_t spaceTextColor = getContrastColor(spaceBgColor);
    tft->setTextColor(spaceTextColor, spaceBgColor);
    tft->setTextSize(currentSkin.textSize);  // Sử dụng textSize từ skin
    // String spaceText = "sp";  // Rút gọn để fit vào phím
    // uint16_t spaceTextWidth = spaceText.length() * 6 * textSize;  // Tính theo textSize (6px cho font size 1)
    // uint16_t spaceTextX = spaceXStart + (spaceKeyWidth - spaceTextWidth) / 2;
    // uint16_t spaceTextY = spaceYStart + (keyHeight - 8) / 2;  // Căn giữa theo chiều dọc
    // tft->setCursor(spaceTextX, spaceTextY);
    // tft->print(spaceText);  // In text "Sp"
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
    if (isIconMode) {
        currentKeys = iconKeysArray;    // Icon mode
    } else if (isAlphabetMode) {
        currentKeys = qwertyKeysArray;  // Sử dụng bảng phím chữ cái
    } else {
        currentKeys = numericKeysArray; // Sử dụng bảng phím số/ký tự đặc biệt
    }

    String rawKey = currentKeys[cursorRow][cursorCol];
    if (isAlphabetMode && isLetterKey(rawKey) && isUppercaseMode) {
        char upperChar = rawKey.charAt(0) - 'a' + 'A';
        return String(upperChar);
    }
    return rawKey;
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
        // Nếu nhấn phím Shift, toggle chế độ chữ hoa/thường
        if (isAlphabetMode && currentKey == KEY_SHIFT) {
            toggleUppercaseMode();
        }
        // Nếu nhấn phím "123", chuyển sang bàn phím số/ký tự đặc biệt
        else if (!isIconMode && isAlphabetMode && currentKey == "123") {
            isAlphabetMode = false;  // Chuyển sang chế độ số/ký tự đặc biệt
            isIconMode = false;
            qwertyKeys = numericKeysArray;  // Cập nhật con trỏ bảng phím
            draw();  // Vẽ lại bàn phím số
        }
        // Nếu nhấn phím "ABC", quay lại bàn phím chữ cái
        else if (!isIconMode && !isAlphabetMode && currentKey == "ABC") {
            isAlphabetMode = true;   // Quay lại chế độ chữ cái
            isIconMode = false;
            qwertyKeys = qwertyKeysArray;  // Cập nhật con trỏ bảng phím
            draw();  // Vẽ lại bàn phím chữ cái
        }
        // Từ bất kỳ chế độ nào, nhấn phím icon để vào icon mode
        else if (currentKey == KEY_ICON) {
            isIconMode = true;
            isAlphabetMode = false;
            qwertyKeys = iconKeysArray;
            draw();  // Vẽ lại bàn phím icon
        }
        // Trong icon mode, nhấn "ABC" để về chữ hoặc "123" để sang số
        else if (isIconMode && currentKey == "ABC") {
            isIconMode = false;
            isAlphabetMode = true;
            qwertyKeys = qwertyKeysArray;
            draw();
        } else if (isIconMode && currentKey == "123") {
            isIconMode = false;
            isAlphabetMode = false;
            qwertyKeys = numericKeysArray;
            draw();
        } else {
            // Gọi callback nếu có để xử lý phím được chọn
            Serial.print("Keyboard::moveCursorByCommand: Selected key '");
            Serial.print(currentKey);
            Serial.print("' (length=");
            Serial.print(currentKey.length());
            if (currentKey.length() > 0) {
                Serial.print(", first char='");
                Serial.print(currentKey.charAt(0));
                Serial.print("' (0x");
                Serial.print((int)currentKey.charAt(0), HEX);
                Serial.print(")");
            }
            Serial.print("), calling callback...");
            Serial.println(onKeySelected != nullptr ? " (callback exists)" : " (NO CALLBACK!)");
            
            if (onKeySelected != nullptr) {
                onKeySelected(currentKey);
                Serial.println("Keyboard::moveCursorByCommand: Callback executed");
            }
        }
    }
}

void Keyboard::moveCursorTo(uint16_t row, int8_t col) {
    cursorRow = row;
    cursorCol = col;
    
    // Giới hạn vị trí
    if (cursorRow > 3) cursorRow = 3;
    if (cursorCol < 0) cursorCol = 0;
    if (cursorCol > 9) cursorCol = 9;
    
    updateCurrentCursor();
    draw();
}

bool Keyboard::typeChar(char c) {
    Serial.print("Keyboard::typeChar: Attempting to type char '");
    Serial.print(c);
    Serial.print("' (0x");
    Serial.print((int)c, HEX);
    Serial.println(")");
    
    // Phân loại ký tự
    bool isDigit = (c >= '0' && c <= '9');
    bool isLowercaseLetter = (c >= 'a' && c <= 'z');
    bool isUppercaseLetter = (c >= 'A' && c <= 'Z');
    bool isLetter = isLowercaseLetter || isUppercaseLetter;
    bool isIcon = (c >= ICON_SMILE && c <= ICON_WINK);
    char normalizedChar = c;
    if (isUppercaseLetter) {
        normalizedChar = c - 'A' + 'a';  // Lưu layout chữ thường, hiển thị chữ hoa qua state
    }
    
    Serial.print("Keyboard::typeChar: isDigit=");
    Serial.print(isDigit);
    Serial.print(", isLetter=");
    Serial.print(isLetter);
    Serial.print(", normalizedChar='");
    Serial.print(normalizedChar);
    Serial.print("', current mode: isAlphabetMode=");
    Serial.print(isAlphabetMode);
    Serial.print(", isIconMode=");
    Serial.println(isIconMode);
    
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
    
    // Nếu là icon, chuyển sang icon mode
    if (isIcon && !isIconMode) {
        // Phím icon nằm hàng 2 cột 0 (KEY_ICON) trong layout chữ/số
        moveCursorTo(2, 0);
        delay(100);
        moveCursorByCommand("select", 0, 0);
        delay(100);
    }
    
    // Đặt chế độ chữ hoa/thường theo ký tự cần gõ
    if (isLetter) {
        setUppercaseMode(isUppercaseLetter);
    }
    
    // Tìm ký tự trong bảng phím hiện tại
    String (*currentKeys)[10];
    if (isAlphabetMode) {
        currentKeys = qwertyKeysArray;
    } else {
        currentKeys = isIconMode ? iconKeysArray : numericKeysArray;
    }
    
    for (uint16_t row = 0; row < 3; row++) {
        for (uint16_t col = 0; col < 10; col++) {
            String key = currentKeys[row][col];
            // So sánh ký tự (bỏ qua các phím đặc biệt như "123", "ABC", "|e", "<", icon toggle)
            if (key.length() == 1 && key.charAt(0) == normalizedChar) {
                Serial.print("Keyboard::typeChar: Found '");
                Serial.print(c);
                Serial.print("' at row=");
                Serial.print(row);
                Serial.print(", col=");
                Serial.print(col);
                Serial.println(" in current layout");
                // Di chuyển đến ký tự này
                moveCursorTo(row, col);
                delay(150);  // Delay để người dùng thấy di chuyển
                // Nhấn select
                moveCursorByCommand("select", 0, 0);
                delay(150);
                Serial.println("Keyboard::typeChar: Successfully typed character");
                return true;
            }
        }
    }
    
    Serial.println("Keyboard::typeChar: Not found in current layout, searching in other layout...");
    
    // Nếu không tìm thấy, thử tìm trong bảng kia
    if (isAlphabetMode) {
        currentKeys = isIconMode ? iconKeysArray : numericKeysArray;
    } else {
        currentKeys = qwertyKeysArray;
    }
    
    for (uint16_t row = 0; row < 3; row++) {
        for (uint16_t col = 0; col < 10; col++) {
            String key = currentKeys[row][col];
            if (key.length() == 1 && key.charAt(0) == normalizedChar) {
                Serial.print("Keyboard::typeChar: Found '");
                Serial.print(c);
                Serial.print("' at row=");
                Serial.print(row);
                Serial.print(", col=");
                Serial.print(col);
                Serial.println(" in other layout, switching mode...");
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
                Serial.println("Keyboard::typeChar: Successfully typed character after mode switch");
                return true;
            }
        }
    }
    
    Serial.print("Keyboard::typeChar: FAILED - Character '");
    Serial.print(c);
    Serial.println("' not found in any layout!");
    return false;  // Không tìm thấy ký tự
}

void Keyboard::typeString(String text) {
    Serial.print("Keyboard::typeString: Typing string '");
    Serial.print(text);
    Serial.print("' (length=");
    Serial.print(text.length());
    Serial.println(")");
    
    for (uint16_t i = 0; i < text.length(); i++) {
        char c = text.charAt(i);
        Serial.print("Keyboard::typeString: [");
        Serial.print(i);
        Serial.print("/");
        Serial.print(text.length());
        Serial.print("] Typing '");
        Serial.print(c);
        Serial.println("'");
        
        bool success = typeChar(c);
        if (!success) {
            Serial.print("Keyboard::typeString: WARNING - Failed to type character '");
            Serial.print(c);
            Serial.print("' at position ");
            Serial.println(i);
        }
        delay(150);  // Delay giữa các ký tự
    }
    
    Serial.println("Keyboard::typeString: Finished typing string");
}

void Keyboard::pressEnter() {
    Serial.println("Keyboard::pressEnter: Navigating to Enter key and pressing it...");
    
    // Đảm bảo chúng ta không ở icon mode (Enter có trong tất cả layout chữ/số)
    if (isIconMode) {
        Serial.println("Keyboard::pressEnter: Currently in icon mode, switching to numeric mode...");
        // Chuyển về chế độ số nếu đang ở icon mode
        moveCursorTo(1, 0);  // "123" hoặc "ABC"
        delay(100);
        moveCursorByCommand("select", 0, 0);
        delay(100);
    }
    
    // Enter luôn ở hàng 2, cột 8 trong tất cả layout (chữ cái, số, icon)
    Serial.println("Keyboard::pressEnter: Moving to Enter key at row=2, col=8...");
    moveCursorTo(2, 8);
    delay(150);
    
    Serial.println("Keyboard::pressEnter: Pressing Enter key...");
    moveCursorByCommand("select", 0, 0);
    delay(150);
    
    Serial.println("Keyboard::pressEnter: Enter key pressed successfully");
}

void Keyboard::turnOff() {
    tft->fillScreen(currentSkin.bgScreenColor);
}

// Skin management methods
void Keyboard::setSkin(const KeyboardSkin& skin) {
    currentSkin = skin;
    applySkin();
}

void Keyboard::applySkin() {
    // Áp dụng skin vào các biến màu sắc cũ để backward compatibility
    keyBgColor = currentSkin.keyBgColor;
    keySelectedColor = currentSkin.keySelectedColor;
    keyTextColor = currentSkin.keyTextColor;
    bgScreenColor = currentSkin.bgScreenColor;
    textSize = currentSkin.textSize;
}

void Keyboard::toggleUppercaseMode() {
    setUppercaseMode(!isUppercaseMode);
}

void Keyboard::setUppercaseMode(bool uppercase) {
    if (isUppercaseMode != uppercase) {
        isUppercaseMode = uppercase;
        updateCurrentCursor();
        draw();
    }
}

bool Keyboard::isLetterKey(const String& key) const {
    return key.length() == 1 && key.charAt(0) >= 'a' && key.charAt(0) <= 'z';
}

bool Keyboard::isIconToken(const String& key) const {
    return key.length() == 1 && key.charAt(0) >= ICON_SMILE && key.charAt(0) <= ICON_WINK;
}

char Keyboard::toIconCode(const String& key) const {
    if (isIconToken(key)) return key.charAt(0);
    return 0;
}

String Keyboard::formatKeyLabel(const String& rawKey) const {
    // Giữ nguyên các phím đặc biệt
    if (rawKey == KEY_SHIFT) {
        // Hiển thị trạng thái: Aa = đang ở chữ thường, AA = đang ở chữ hoa
        return isUppercaseMode ? "AA" : "Aa";
    }
    // Icon glyph sẽ được vẽ riêng, không cần label text
    if (isIconToken(rawKey)) {
        return "";
    }
    
    // Chỉ chuyển sang chữ hoa cho phím chữ cái khi đang ở chế độ alphabet
    if (isAlphabetMode && isLetterKey(rawKey)) {
        char letter = rawKey.charAt(0);
        if (isUppercaseMode) {
            letter = letter - 'a' + 'A';
        }
        return String(letter);
    }
    
    return rawKey;
}

// Các skin mẫu đã được di chuyển vào namespace KeyboardSkins
// Sử dụng KeyboardSkins::getFeminineSkin(), KeyboardSkins::getCyberpunkSkin(), etc.

// Vẽ hoa văn nữ tính (trái tim, hoa, pattern) trên phím
void Keyboard::drawFemininePattern(uint16_t x, uint16_t y, uint16_t width, uint16_t height) {
    // Chỉ vẽ hoa văn nhỏ ở góc phím, không che chữ
    uint16_t patternColor = 0xFFFF;  // Màu trắng cho hoa văn
    
    // Vẽ trái tim nhỏ ở góc trên bên phải
    uint16_t heartX = x + width - 6;
    uint16_t heartY = y + 2;
    
    // Vẽ trái tim đơn giản (3 pixel)
    tft->drawPixel(heartX, heartY, patternColor);
    tft->drawPixel(heartX + 1, heartY, patternColor);
    tft->drawPixel(heartX, heartY + 1, patternColor);
    
    // Vẽ hoa nhỏ ở góc dưới bên trái
    uint16_t flowerX = x + 2;
    uint16_t flowerY = y + height - 4;
    
    // Vẽ hoa 5 cánh đơn giản
    tft->drawPixel(flowerX, flowerY, patternColor);      // Cánh giữa
    tft->drawPixel(flowerX - 1, flowerY - 1, patternColor);  // Cánh trên trái
    tft->drawPixel(flowerX + 1, flowerY - 1, patternColor);  // Cánh trên phải
    tft->drawPixel(flowerX - 1, flowerY + 1, patternColor);  // Cánh dưới trái
    tft->drawPixel(flowerX + 1, flowerY + 1, patternColor);  // Cánh dưới phải
    
    // Vẽ pattern nhỏ ở giữa (chấm tròn)
    if (width > 15 && height > 15) {
        uint16_t dotX = x + width / 2;
        uint16_t dotY = y + height / 2;
        tft->drawPixel(dotX, dotY, patternColor);
    }
}


// Vẽ pattern máy móc cơ học steampunk (bánh răng, đường kẻ cơ khí) trên phím
// Vẽ bánh răng chi tiết màu đồng nâu, nhỏ ở góc để không che chữ
void Keyboard::drawMechanicalPattern(uint16_t x, uint16_t y, uint16_t width, uint16_t height) {
    // Màu đồng nâu (copper brown)
    uint16_t copperColor = 0xCC00;   // Copper brown (đồng nâu)
    uint16_t darkCopper = 0x9800;    // Dark copper (đồng đậm)
    uint16_t lightCopper = 0xFD20;   // Light copper (đồng nhạt)
    uint16_t accentColor = 0x05DF;   // Teal nhạt cho tâm bánh răng
    
    // Vẽ bánh răng chi tiết nhỏ ở góc trên bên trái
    uint16_t gearX = x + 4;
    uint16_t gearY = y + 4;
    uint16_t gearRadius = 4;  // Bánh răng nhỏ
    
    // Vẽ vòng tròn ngoài (đồng nâu)
    tft->drawCircle(gearX, gearY, gearRadius, copperColor);
    tft->drawCircle(gearX, gearY, gearRadius - 1, darkCopper);
    
    // Vẽ tâm bánh răng (hub) với lỗ tròn
    tft->fillCircle(gearX, gearY, 1, accentColor);
    tft->drawCircle(gearX, gearY, 1, copperColor);
    
    // Vẽ các răng cưa chi tiết (8 răng)
    // Răng ở 4 hướng chính
    tft->drawPixel(gearX, gearY - gearRadius - 1, copperColor);  // Trên
    tft->drawPixel(gearX, gearY + gearRadius + 1, copperColor);  // Dưới
    tft->drawPixel(gearX - gearRadius - 1, gearY, copperColor);  // Trái
    tft->drawPixel(gearX + gearRadius + 1, gearY, copperColor);  // Phải
    
    // Răng ở 4 góc
    tft->drawPixel(gearX - 3, gearY - 3, copperColor);  // Góc trên trái
    tft->drawPixel(gearX + 3, gearY - 3, copperColor);  // Góc trên phải
    tft->drawPixel(gearX - 3, gearY + 3, copperColor);  // Góc dưới trái
    tft->drawPixel(gearX + 3, gearY + 3, copperColor);  // Góc dưới phải
    
    // Vẽ các răng phụ (chi tiết hơn)
    tft->drawPixel(gearX - 2, gearY - gearRadius, lightCopper);
    tft->drawPixel(gearX + 2, gearY - gearRadius, lightCopper);
    tft->drawPixel(gearX - 2, gearY + gearRadius, lightCopper);
    tft->drawPixel(gearX + 2, gearY + gearRadius, lightCopper);
    tft->drawPixel(gearX - gearRadius, gearY - 2, lightCopper);
    tft->drawPixel(gearX - gearRadius, gearY + 2, lightCopper);
    tft->drawPixel(gearX + gearRadius, gearY - 2, lightCopper);
    tft->drawPixel(gearX + gearRadius, gearY + 2, lightCopper);
    
    // Vẽ bánh răng nhỏ thứ 2 ở góc dưới bên phải
    uint16_t gearX2 = x + width - 5;
    uint16_t gearY2 = y + height - 5;
    
    // Vẽ vòng tròn ngoài
    tft->drawCircle(gearX2, gearY2, gearRadius, copperColor);
    tft->drawCircle(gearX2, gearY2, gearRadius - 1, darkCopper);
    
    // Tâm bánh răng
    tft->fillCircle(gearX2, gearY2, 1, accentColor);
    tft->drawCircle(gearX2, gearY2, 1, copperColor);
    
    // Răng cưa
    tft->drawPixel(gearX2, gearY2 - gearRadius - 1, copperColor);
    tft->drawPixel(gearX2, gearY2 + gearRadius + 1, copperColor);
    tft->drawPixel(gearX2 - gearRadius - 1, gearY2, copperColor);
    tft->drawPixel(gearX2 + gearRadius + 1, gearY2, copperColor);
    tft->drawPixel(gearX2 - 3, gearY2 - 3, copperColor);
    tft->drawPixel(gearX2 + 3, gearY2 - 3, copperColor);
    tft->drawPixel(gearX2 - 3, gearY2 + 3, copperColor);
    tft->drawPixel(gearX2 + 3, gearY2 + 3, copperColor);
    
    // Vẽ các điểm rivet (đinh tán) ở góc (đồng nâu)
    tft->drawPixel(x + 2, y + 2, copperColor);
    tft->drawPixel(x + width - 3, y + 2, copperColor);
    tft->drawPixel(x + 2, y + height - 3, copperColor);
    tft->drawPixel(x + width - 3, y + height - 3, copperColor);
}

// Vẽ hiệu ứng điện chạy xung quanh phím (cyberpunk/tron style)
void Keyboard::drawCyberpunkGlow(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t animationFrame) {
    uint16_t glowColor = 0x07FF;  // Neon cyan
    uint16_t brightGlow = 0xFFFF;  // White (sáng nhất)
    
    // Tính vị trí điện chạy dựa trên animation frame
    // Điện chạy theo chu vi phím (4 cạnh)
    uint16_t perimeter = (width + height) * 2;
    uint16_t currentPos = (animationFrame * 2) % perimeter;  // Tốc độ chạy
    
    // Vẽ đường điện chạy quanh viền phím
    // Cạnh trên (trái -> phải)
    if (currentPos < width) {
        uint16_t glowX = x + currentPos;
        tft->drawPixel(glowX, y, brightGlow);
        tft->drawPixel(glowX, y + 1, glowColor);
        if (currentPos > 0) tft->drawPixel(glowX - 1, y, glowColor);
        if (currentPos < width - 1) tft->drawPixel(glowX + 1, y, glowColor);
    }
    // Cạnh phải (trên -> dưới)
    else if (currentPos < width + height) {
        uint16_t glowY = y + (currentPos - width);
        tft->drawPixel(x + width - 1, glowY, brightGlow);
        tft->drawPixel(x + width - 2, glowY, glowColor);
        if (glowY > y) tft->drawPixel(x + width - 1, glowY - 1, glowColor);
        if (glowY < y + height - 1) tft->drawPixel(x + width - 1, glowY + 1, glowColor);
    }
    // Cạnh dưới (phải -> trái)
    else if (currentPos < width * 2 + height) {
        uint16_t glowX = x + width - 1 - (currentPos - width - height);
        tft->drawPixel(glowX, y + height - 1, brightGlow);
        tft->drawPixel(glowX, y + height - 2, glowColor);
        if (glowX > x) tft->drawPixel(glowX - 1, y + height - 1, glowColor);
        if (glowX < x + width - 1) tft->drawPixel(glowX + 1, y + height - 1, glowColor);
    }
    // Cạnh trái (dưới -> trên)
    else {
        uint16_t glowY = y + height - 1 - (currentPos - width * 2 - height);
        tft->drawPixel(x, glowY, brightGlow);
        tft->drawPixel(x + 1, glowY, glowColor);
        if (glowY > y) tft->drawPixel(x, glowY - 1, glowColor);
        if (glowY < y + height - 1) tft->drawPixel(x, glowY + 1, glowColor);
    }
    
    // Vẽ đuôi điện (trail) - các điểm phía sau
    for (uint16_t i = 1; i <= 3; i++) {
        uint16_t trailPos = (currentPos - i + perimeter) % perimeter;
        uint16_t trailBrightness = 0x07FF - (i * 0x1000);  // Mờ dần
        
        // Cạnh trên
        if (trailPos < width) {
            uint16_t trailX = x + trailPos;
            if (trailX >= x && trailX < x + width) {
                tft->drawPixel(trailX, y, trailBrightness);
            }
        }
        // Cạnh phải
        else if (trailPos < width + height) {
            uint16_t trailY = y + (trailPos - width);
            if (trailY >= y && trailY < y + height) {
                tft->drawPixel(x + width - 1, trailY, trailBrightness);
            }
        }
        // Cạnh dưới
        else if (trailPos < width * 2 + height) {
            uint16_t trailX = x + width - 1 - (trailPos - width - height);
            if (trailX >= x && trailX < x + width) {
                tft->drawPixel(trailX, y + height - 1, trailBrightness);
            }
        }
        // Cạnh trái
        else {
            uint16_t trailY = y + height - 1 - (trailPos - width * 2 - height);
            if (trailY >= y && trailY < y + height) {
                tft->drawPixel(x, trailY, trailBrightness);
            }
        }
    }
}

// Vẽ hiệu ứng điện chạy xung quanh toàn bộ bàn phím (cyberpunk/tron style)
void Keyboard::drawCyberpunkKeyboardBorder(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t animationFrame) {
    uint16_t glowColor = 0x07FF;  // Neon cyan
    uint16_t brightGlow = 0xFFFF;  // White (sáng nhất)
    
    // Tính vị trí điện chạy dựa trên animation frame
    // Điện chạy theo chu vi toàn bộ bàn phím (4 cạnh)
    uint16_t perimeter = (width + height) * 2;
    uint16_t currentPos = (animationFrame * 3) % perimeter;  // Tốc độ chạy nhanh hơn (x3)
    
    // Vẽ đường điện chạy quanh viền ngoài bàn phím
    // Cạnh trên (trái -> phải)
    if (currentPos < width) {
        uint16_t glowX = x + currentPos;
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
        uint16_t glowY = y + (currentPos - width);
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
        uint16_t glowX = x + width - 1 - (currentPos - width - height);
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
        uint16_t glowY = y + height - 1 - (currentPos - width * 2 - height);
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
    for (uint16_t i = 1; i <= 8; i++) {
        uint16_t trailPos = (currentPos - i + perimeter) % perimeter;
        // Tính độ sáng giảm dần
        uint16_t trailBrightness;
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
            uint16_t trailX = x + trailPos;
            if (trailX >= x && trailX < x + width) {
                tft->drawPixel(trailX, y, trailBrightness);
            }
        }
        // Cạnh phải
        else if (trailPos < width + height) {
            uint16_t trailY = y + (trailPos - width);
            if (trailY >= y && trailY < y + height) {
                tft->drawPixel(x + width - 1, trailY, trailBrightness);
            }
        }
        // Cạnh dưới
        else if (trailPos < width * 2 + height) {
            uint16_t trailX = x + width - 1 - (trailPos - width - height);
            if (trailX >= x && trailX < x + width) {
                tft->drawPixel(trailX, y + height - 1, trailBrightness);
            }
        }
        // Cạnh trái
        else {
            uint16_t trailY = y + height - 1 - (trailPos - width * 2 - height);
            if (trailY >= y && trailY < y + height) {
                tft->drawPixel(x, trailY, trailBrightness);
            }
        }
    }
}

// Vẽ ký tự ⏎ (Return Symbol) bằng các đường thẳng
void Keyboard::drawEnterSymbol(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color) {
    // Tính toán vị trí trung tâm của phím
    uint16_t centerX = x + width / 2;
    uint16_t centerY = y + height / 2;
    
    // Kích thước biểu tượng phù hợp với kích thước phím
    uint16_t symbolSize = (width < height ? width : height) / 3;  // Kích thước = 1/3 kích thước nhỏ hơn
    
    // Vẽ ký tự ⏎: một đường ngang với một đường cong xuống và mũi tên chỉ xuống
    // Đường ngang (phần trên) - từ trái sang phải
    uint16_t lineStartX = centerX - symbolSize;
    uint16_t lineEndX = centerX + symbolSize / 3;
    uint16_t lineY = centerY - symbolSize / 2;
    tft->drawLine(lineStartX, lineY, lineEndX, lineY, color);
    
    // Đường cong xuống (phần giữa) - từ điểm cuối đường ngang xuống dưới
    uint16_t curveStartX = lineEndX;
    uint16_t curveStartY = lineY;
    uint16_t curveEndX = centerX + symbolSize / 2;
    uint16_t curveEndY = centerY + symbolSize / 2;
    
    // Vẽ đường cong bằng cách vẽ nhiều đoạn thẳng ngắn
    // Hoặc đơn giản hơn: vẽ đường thẳng từ trên xuống dưới, hơi cong
    tft->drawLine(curveStartX, curveStartY, curveEndX, curveEndY, color);
    
    // Mũi tên chỉ xuống: vẽ 2 đường chéo tạo thành đầu mũi tên
    uint16_t arrowTipX = curveEndX;
    uint16_t arrowTipY = centerY + symbolSize;
    uint16_t arrowLeftX = curveEndX - symbolSize / 3;
    uint16_t arrowLeftY = centerY + symbolSize / 2;
    uint16_t arrowRightX = curveEndX + symbolSize / 3;
    uint16_t arrowRightY = centerY + symbolSize / 2;
    
    // Đường chéo trái
    tft->drawLine(arrowTipX, arrowTipY, arrowLeftX, arrowLeftY, color);
    // Đường chéo phải
    tft->drawLine(arrowTipX, arrowTipY, arrowRightX, arrowRightY, color);
}

// Vẽ icon mặt cười (smiley) cho phím icon
void Keyboard::drawSmileyIcon(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color) {
    // Xác định tâm và bán kính mặt cười
    uint16_t cx = x + width / 2;
    uint16_t cy = y + height / 2;
    uint16_t radius = (width < height ? width : height) / 3;  // Vừa phải để không chạm viền
    if (radius < 4) radius = 4;
    
    // Vẽ mặt
    tft->drawCircle(cx, cy, radius, color);
    // Vẽ mắt
    uint16_t eyeOffsetX = radius / 2;
    uint16_t eyeOffsetY = radius / 3;
    tft->fillCircle(cx - eyeOffsetX, cy - eyeOffsetY, 1, color);
    tft->fillCircle(cx + eyeOffsetX, cy - eyeOffsetY, 1, color);
    
    // Vẽ miệng cong (arc đơn giản bằng các điểm/đoạn thẳng ngắn)
    uint16_t mouthRadius = radius - 1;
    for (int16_t dx = -mouthRadius + 1; dx <= mouthRadius - 1; dx++) {
        int16_t dy = (mouthRadius * mouthRadius - dx * dx) / (mouthRadius * 2);  // simple curve
        int16_t px = cx + dx;
        int16_t py = cy + dy + 1;  // hạ miệng xuống một chút
        if (py > cy) {
            tft->drawPixel(px, py, color);
        }
    }
}

// Vẽ icon theo mã (dùng chung cho các phím icon)
void Keyboard::drawIconGlyph(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color, char iconCode) {
    uint16_t cx = x + width / 2;
    uint16_t cy = y + height / 2;
    uint16_t size = (width < height ? width : height) / 3;
    if (size < 4) size = 4;
    
    switch (iconCode) {
        case ICON_SMILE:
        case ICON_WINK:
            // Dùng mặt cười cơ bản, thêm wink nếu cần
            drawSmileyIcon(x, y, width, height, color);
            if (iconCode == ICON_WINK) {
                uint16_t eyeOffsetX = size / 2;
                uint16_t eyeOffsetY = size / 3;
                // Vẽ mắt nháy (đường ngang) thay cho mắt phải
                tft->drawFastHLine(cx + eyeOffsetX - 1, cy - eyeOffsetY, 3, color);
            }
            break;
        case ICON_HEART: {
            // Trái tim đơn giản
            uint16_t r = size / 2 + 1;
            uint16_t hx = cx;
            uint16_t hy = cy;
            tft->fillCircle(hx - r, hy - r / 2, r, color);
            tft->fillCircle(hx + r, hy - r / 2, r, color);
            tft->fillTriangle(hx - r * 2, hy - r / 2, hx + r * 2, hy - r / 2, hx, hy + r * 2, color);
            break;
        }
        case ICON_STAR: {
            // Ngôi sao nhỏ
            uint16_t r = size;
            for (int i = 0; i < 5; i++) {
                float angle1 = (72 * i - 90) * 3.14159 / 180.0;
                float angle2 = (72 * (i + 2) - 90) * 3.14159 / 180.0;
                uint16_t x1 = cx + r * cos(angle1);
                uint16_t y1 = cy + r * sin(angle1);
                uint16_t x2 = cx + r * cos(angle2);
                uint16_t y2 = cy + r * sin(angle2);
                tft->drawLine(x1, y1, x2, y2, color);
            }
            break;
        }
        case ICON_CHECK:
            tft->drawLine(cx - size, cy, cx - size / 2, cy + size, color);
            tft->drawLine(cx - size / 2, cy + size, cx + size, cy - size / 2, color);
            break;
        case ICON_MUSIC:
            tft->drawLine(cx - size / 2, cy - size, cx - size / 2, cy + size, color);
            tft->drawLine(cx - size / 2, cy - size, cx + size, cy - size / 2, color);
            tft->fillCircle(cx - size / 2, cy + size, size / 3, color);
            tft->fillCircle(cx + size, cy + size / 2, size / 3, color);
            break;
        case ICON_SUN:
            tft->drawCircle(cx, cy, size - 1, color);
            for (int i = 0; i < 8; i++) {
                float angle = (45 * i) * 3.14159 / 180.0;
                uint16_t x1 = cx + (size + 1) * cos(angle);
                uint16_t y1 = cy + (size + 1) * sin(angle);
                uint16_t x2 = cx + (size + 4) * cos(angle);
                uint16_t y2 = cy + (size + 4) * sin(angle);
                tft->drawLine(x1, y1, x2, y2, color);
            }
            break;
        case ICON_FIRE:
            tft->fillTriangle(cx, cy - size, cx - size, cy + size, cx + size, cy + size, color);
            tft->fillTriangle(cx, cy - size / 2, cx - size / 2, cy + size, cx + size / 2, cy + size, getContrastColor(color));
            break;
        case ICON_THUMBS:
            tft->fillRect(cx - size / 2, cy - size / 2, size / 2, size + 2, color);
            tft->fillRect(cx - size / 2, cy - size / 2, size, size / 3, color);
            tft->fillRect(cx + size / 2, cy - size / 2, size / 3, size / 2, color);
            break;
        case ICON_GIFT:
            tft->drawRect(cx - size, cy - size / 2, size * 2, size + 2, color);
            tft->drawFastVLine(cx, cy - size / 2, size + 2, color);
            tft->drawFastHLine(cx - size, cy, size * 2, color);
            tft->drawCircle(cx - size / 2, cy - size / 2, size / 3, color);
            tft->drawCircle(cx + size / 2, cy - size / 2, size / 3, color);
            break;
        default:
            // Fallback: vẽ mặt cười
            drawSmileyIcon(x, y, width, height, color);
            break;
    }
}

// Tính màu chữ tương phản dựa trên màu nền
// Sử dụng công thức tính độ sáng (luminance) để quyết định dùng màu đen hay trắng
uint16_t Keyboard::getContrastColor(uint16_t bgColor) {
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
    uint16_t luminance = (299 * r + 587 * g + 114 * b) / 1000;
    
    // Nếu nền sáng (luminance > 128), dùng chữ đen, ngược lại dùng chữ trắng
    if (luminance > 128) {
        return 0x0000;  // Black text
    } else {
        return 0xFFFF;  // White text
    }
}
