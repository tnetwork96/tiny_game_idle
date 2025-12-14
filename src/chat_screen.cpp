#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include "chat_screen.h"
#include <FS.h>
#include <SPIFFS.h>

// Synthwave/Vaporwave color palette
#define NEON_PURPLE 0xD81F
#define NEON_GREEN 0x07E0
#define NEON_CYAN 0x07FF
#define YELLOW_ORANGE 0xFE20
#define SOFT_WHITE 0xFFFF
#define NEON_PINK 0xF81F

ChatScreen::ChatScreen(Adafruit_ST7789* tft, Keyboard* keyboard) {
    this->tft = tft;
    this->keyboard = keyboard;
    
    // Khởi tạo vị trí và kích thước (màn hình rotation 3: 320x240)
    this->titleY = 5;
    this->chatAreaY = 25;  // Bắt đầu sau title bar
    this->chatAreaHeight = 215;  // Chiều cao vùng chat (từ 25 đến 240)
    this->chatAreaWidth = 320;   // Rộng toàn màn hình (rotation 3: 320px)
    this->inputBoxY = 240;  // Vị trí input box (ngay dưới chat area)
    this->inputBoxHeight = 35;   // Chiều cao ô nhập
    this->inputBoxWidth = 300;   // Rộng ô nhập (fit màn hình 320px)
    this->maxMessageLength = 80;  // Tăng độ dài tin nhắn lên 80 ký tự
    
    // Khởi tạo tin nhắn
    this->messageCount = 0;
    this->scrollOffset = 0;
    this->currentMessage = "";
    this->keyboardVisible = false;  // Bàn phím ẩn mặc định
    
    // Khởi tạo nickname mặc định
    this->ownerNickname = "You";      // Tên mặc định của người dùng
    this->friendNickname = "Other";    // Tên mặc định của người khác
    
    // Khởi tạo SPIFFS
    if (!SPIFFS.begin(true)) {
        Serial.println("Chat: SPIFFS initialization failed!");
    } else {
        Serial.println("Chat: SPIFFS initialized successfully");
        // Tự động load tin nhắn từ file khi khởi động
        loadMessagesFromFile();
    }
    
    // Khởi tạo màu sắc mặc định - Phong cách Synthwave/Vaporwave
    this->bgColor = ST77XX_BLACK;
    this->titleColor = YELLOW_ORANGE;
    this->chatAreaBgColor = 0x0008;  // Đen nhạt
    this->chatAreaBorderColor = NEON_PURPLE;
    this->inputBoxBgColor = ST77XX_BLACK;
    this->inputBoxBorderColor = NEON_CYAN;
    this->userMessageColor = NEON_GREEN;  // Tin nhắn của user màu xanh lá
    this->otherMessageColor = NEON_PINK;  // Tin nhắn của người khác màu hồng
    this->textColor = SOFT_WHITE;
    
    // Khởi tạo decor elements (mặc định tắt)
    this->showTitleBarGradient = false;
    this->showChatAreaPattern = false;
    this->showInputBoxGlow = false;
    this->showMessageBubbles = false;
    this->showScrollbarGlow = false;
    this->decorPatternColor = NEON_PURPLE;
    this->decorAccentColor = NEON_CYAN;
    this->decorAnimationFrame = 0;
}

ChatScreen::~ChatScreen() {
    // Không cần giải phóng gì
}

void ChatScreen::drawTitle() {
    // Vẽ title bar fit toàn bộ màn hình (320px với rotation 3)
    uint16_t titleBarHeight = 25;
    uint16_t titleBarY = 0;
    uint16_t screenWidth = 320;  // Chiều rộng màn hình với rotation 3
    tft->fillRect(0, titleBarY, screenWidth, titleBarHeight, chatAreaBgColor);
    
    // Vẽ viền dưới title bar (toàn bộ chiều rộng)
    tft->drawFastHLine(0, titleBarY + titleBarHeight - 2, screenWidth, chatAreaBorderColor);
    tft->drawFastHLine(0, titleBarY + titleBarHeight - 1, screenWidth, chatAreaBorderColor);
    
    // Vẽ text "CHAT" căn giữa
    tft->setTextColor(titleColor, chatAreaBgColor);
    tft->setTextSize(2);  // Cỡ chữ tầm trung
    String title = "CHAT";
    uint16_t textWidth = title.length() * 12;  // Khoảng 12px mỗi ký tự với text size 2
    uint16_t textX = (screenWidth - textWidth) / 2;     // Căn giữa theo chiều ngang
    uint16_t textY = titleBarY + (titleBarHeight - 16) / 2;  // Căn giữa theo chiều dọc (text size 2 cao ~16px)
    tft->setCursor(textX, textY);
    tft->print(title);
    
    // Vẽ decor cho title bar nếu bật
    if (showTitleBarGradient) {
        drawTitleBarDecor();
    }
}

void ChatScreen::drawTitleBarDecor() {
    // Vẽ gradient hoặc pattern cho title bar
    uint16_t titleBarHeight = 25;
    uint16_t screenWidth = 320;
    
    // Vẽ gradient đơn giản (từ trên xuống)
    for (int y = 0; y < titleBarHeight; y++) {
        // Tính màu gradient (từ decorAccentColor đến chatAreaBgColor)
        float ratio = (float)y / (float)titleBarHeight;
        uint16_t gradientColor = interpolateColor(decorAccentColor, chatAreaBgColor, ratio);
        tft->drawFastHLine(0, y, screenWidth, gradientColor);
    }
    
    // Vẽ pattern nhỏ (dots)
    for (int x = 5; x < screenWidth - 5; x += 10) {
        for (int y = 5; y < titleBarHeight - 5; y += 10) {
            if ((x + y + decorAnimationFrame) % 20 < 10) {
                tft->drawPixel(x, y, decorPatternColor);
            }
        }
    }
}

uint16_t ChatScreen::interpolateColor(uint16_t color1, uint16_t color2, float ratio) {
    // Interpolate giữa 2 màu RGB565
    uint8_t r1 = (color1 >> 11) & 0x1F;
    uint8_t g1 = (color1 >> 5) & 0x3F;
    uint8_t b1 = color1 & 0x1F;
    
    uint8_t r2 = (color2 >> 11) & 0x1F;
    uint8_t g2 = (color2 >> 5) & 0x3F;
    uint8_t b2 = color2 & 0x1F;
    
    uint8_t r = r1 + (r2 - r1) * ratio;
    uint8_t g = g1 + (g2 - g1) * ratio;
    uint8_t b = b1 + (b2 - b1) * ratio;
    
    return (r << 11) | (g << 5) | b;
}

void ChatScreen::drawChatArea() {
    // Không vẽ khung, chỉ cần xóa nền nếu cần
    // Vùng chat sẽ được vẽ trực tiếp khi vẽ tin nhắn
    
    // Vẽ decor cho vùng chat nếu bật
    if (showChatAreaPattern) {
        drawChatAreaDecor();
    }
}

int ChatScreen::getVisibleLines() {
    // Với text size 2, mỗi dòng cao khoảng 16px
    // Tính số dòng có thể hiển thị trong chatAreaHeight
    int lineHeight = 24;  // Tăng khoảng cách cho text size 2 (16px text + 8px spacing)
    int padding = 4;      // Padding trên và dưới
    return (chatAreaHeight - padding * 2) / lineHeight;
}

void ChatScreen::drawScrollbar() {
    // Vẽ thanh scrollbar ở bên phải màn hình
    uint16_t scrollbarWidth = 4;  // Độ rộng thanh scrollbar
    uint16_t scrollbarX = chatAreaWidth - scrollbarWidth;  // Vị trí X (bên phải)
    uint16_t scrollbarY = chatAreaY;
    uint16_t scrollbarHeight = chatAreaHeight;
    
    // Vẽ nền thanh scrollbar (màu tối)
    tft->fillRect(scrollbarX, scrollbarY, scrollbarWidth, scrollbarHeight, 0x1082);  // Màu xám đen nhạt hơn
    
    // Tính tổng số dòng và vị trí scroll
    int totalLines = 0;
    int charsPerLine = 20;
    for (int i = 0; i < messageCount; i++) {
        if (i > 0 && messages[i].isUser != messages[i-1].isUser) {
            totalLines += 1;  // Spacing
        }
        int messageLength = messages[i].text.length();
        int numLines = (messageLength + charsPerLine - 1) / charsPerLine;
        totalLines += numLines;
    }
    
    int maxLines = getVisibleLines();
    
    // Chỉ vẽ scrollbar nếu có nhiều dòng hơn số dòng hiển thị
    if (totalLines > maxLines) {
        // Tính kích thước thumb (phần có thể kéo)
        float scrollRatio = (float)maxLines / (float)totalLines;
        uint16_t thumbHeight = scrollbarHeight * scrollRatio;
        if (thumbHeight < 10) thumbHeight = 10;  // Tối thiểu 10px
        
        // Tính vị trí thumb dựa trên scrollOffset
        float scrollPercent = (float)scrollOffset / (float)(totalLines - maxLines);
        if (scrollPercent > 1.0) scrollPercent = 1.0;
        if (scrollPercent < 0.0) scrollPercent = 0.0;
        
        uint16_t thumbY = scrollbarY + (scrollbarHeight - thumbHeight) * scrollPercent;
        
        // Vẽ thumb (phần có thể kéo) - màu khác
        uint16_t thumbColor = NEON_CYAN;  // Màu cyan cho thumb
        if (showScrollbarGlow) {
            // Thêm glow effect cho scrollbar
            thumbColor = decorAccentColor;
            // Vẽ glow xung quanh thumb
            tft->drawFastVLine(scrollbarX - 1, thumbY, thumbHeight, decorAccentColor);
            tft->drawFastVLine(scrollbarX + scrollbarWidth, thumbY, thumbHeight, decorAccentColor);
        }
        tft->fillRect(scrollbarX, thumbY, scrollbarWidth, thumbHeight, thumbColor);
    }
}

void ChatScreen::drawMessages() {
    int lineHeight = 24;  // Khoảng cách cho text size 2 (16px text + 8px spacing)
    int padding = 4;
    int startY = chatAreaY + padding;
    int maxLines = getVisibleLines();
    int messageGroupSpacing = 8;  // Khoảng cách giữa các nhóm tin nhắn (khi đổi người gửi)
    
    // Xóa vùng tin nhắn (trừ phần scrollbar)
    uint16_t scrollbarWidth = 4;
    tft->fillRect(0, chatAreaY, chatAreaWidth - scrollbarWidth, chatAreaHeight, bgColor);
    
    // Logic: Vẽ tin nhắn từ trên xuống (tin nhắn cũ ở trên, mới ở dưới)
    // Khi có nhiều tin nhắn hơn số dòng hiển thị, chỉ hiển thị các tin nhắn mới nhất
    // Tin nhắn mới nhất luôn ở dưới cùng, tin nhắn cũ tự động đẩy lên (giống Facebook/Messenger)
    
    // Tính tổng số dòng của tất cả tin nhắn
    int totalLines = 0;
    int charsPerLine = 20;
    // Lưu số dòng của mỗi tin nhắn và spacing
    int lineCounts[50];  // MAX_MESSAGES = 50
    bool hasSpacing[50]; // MAX_MESSAGES = 50
    
    for (int i = 0; i < messageCount; i++) {
        // Kiểm tra có spacing trước tin nhắn này không
        hasSpacing[i] = (i > 0 && messages[i].isUser != messages[i-1].isUser);
        if (hasSpacing[i]) {
            totalLines += 1;  // Thêm 1 dòng spacing
        }
        
        // Tính số dòng của tin nhắn này
        int messageLength = messages[i].text.length();
        int numLines = (messageLength + charsPerLine - 1) / charsPerLine;
        lineCounts[i] = numLines;
        totalLines += numLines;
    }
    
    // Tính index tin nhắn bắt đầu hiển thị dựa trên scrollOffset (số dòng)
    // scrollOffset là số dòng đã cuộn lên
    int startIndex = 0;
    int linesBeforeStart = 0;
    
    if (messageCount > 0 && scrollOffset > 0) {
        // Tìm tin nhắn bắt đầu dựa trên số dòng đã cuộn
        for (int i = 0; i < messageCount; i++) {
            int linesForThisMessage = lineCounts[i];
            if (hasSpacing[i]) {
                linesForThisMessage += 1;  // Thêm spacing
            }
            
            if (linesBeforeStart + linesForThisMessage > scrollOffset) {
                startIndex = i;
                break;
            }
            linesBeforeStart += linesForThisMessage;
        }
    }
    
    // Tính số dòng đã bỏ qua trong tin nhắn đầu tiên (nếu scrollOffset nằm giữa tin nhắn)
    int linesSkippedInFirstMessage = 0;
    if (startIndex > 0 || scrollOffset > 0) {
        int linesBeforeFirst = 0;
        for (int i = 0; i < startIndex; i++) {
            int linesForThisMessage = lineCounts[i];
            if (hasSpacing[i]) {
                linesForThisMessage += 1;
            }
            linesBeforeFirst += linesForThisMessage;
        }
        linesSkippedInFirstMessage = scrollOffset - linesBeforeFirst;
        if (linesSkippedInFirstMessage < 0) linesSkippedInFirstMessage = 0;
        if (linesSkippedInFirstMessage > lineCounts[startIndex]) {
            linesSkippedInFirstMessage = lineCounts[startIndex];
        }
    }
    
    // Vẽ từ trên xuống: tin nhắn cũ ở trên, mới ở dưới
    // Mỗi tin nhắn có thể xuống nhiều dòng nếu dài hơn 20 ký tự
    int lineIndex = 0;
    for (int i = startIndex; i < messageCount && lineIndex < maxLines; i++) {
        if (i >= 0 && i < messageCount) {
            // Kiểm tra nếu tin nhắn này khác người gửi với tin nhắn trước đó
            // -> thêm khoảng cách (message group spacing)
            if (i > startIndex && messages[i].isUser != messages[i-1].isUser) {
                // Khác người gửi -> thêm khoảng cách (1 dòng trống)
                lineIndex++;
                
                // Kiểm tra không vượt quá maxLines
                if (lineIndex >= maxLines) break;
            }
            
            uint16_t msgColor = messages[i].isUser ? userMessageColor : otherMessageColor;
            tft->setTextColor(msgColor, chatAreaBgColor);
            tft->setTextSize(2);  // Cỡ chữ tầm trung (size 2)
            
            String messageText = messages[i].text;
            uint16_t margin = 10;  // Margin từ cạnh màn hình
            
            // Chia tin nhắn thành các dòng 20 ký tự
            int messageLength = messageText.length();
            int numLines = lineCounts[i];  // Sử dụng số dòng đã tính
            
            // Vẽ từng dòng của tin nhắn
            int startLine = (i == startIndex) ? linesSkippedInFirstMessage : 0;
            for (int line = startLine; line < numLines && lineIndex < maxLines; line++) {
                int startChar = line * charsPerLine;
                int endChar = (startChar + charsPerLine < messageLength) ? (startChar + charsPerLine) : messageLength;
                String lineText = messageText.substring(startChar, endChar);
                
                // Tính vị trí Y
                int yPos = startY + lineIndex * lineHeight;
                
                // Tính vị trí X: user căn phải, other căn trái
                uint16_t textX;
                if (messages[i].isUser) {
                    // Tin nhắn của user: căn phải
                    uint16_t textWidth = lineText.length() * 12;  // Khoảng 12px mỗi ký tự với text size 2
                    textX = chatAreaWidth - textWidth - margin;
                } else {
                    // Tin nhắn của người khác: căn trái
                    textX = margin;
                }
                
                // Vẽ bubble background nếu bật decor
                if (showMessageBubbles && line == 0) {
                    // Chỉ vẽ bubble cho dòng đầu tiên của mỗi tin nhắn
                    int bubbleWidth = lineText.length() * 12 + 10;  // Width của dòng đầu
                    int bubbleHeight = lineHeight;
                    int bubbleX = messages[i].isUser ? (textX - 5) : (textX - 5);
                    int bubbleY = yPos - 2;
                    drawMessageBubble(bubbleX, bubbleY, bubbleWidth, bubbleHeight, msgColor, messages[i].isUser);
                }
                
                // Vẽ dòng này
                tft->setCursor(textX, yPos);
                tft->setTextColor(msgColor, bgColor);
                tft->print(lineText);
                
                lineIndex++;
            }
        }
    }
    
    // Vẽ thanh scrollbar sau khi vẽ tin nhắn
    drawScrollbar();
}

void ChatScreen::drawInputBox() {
    uint16_t screenWidth = 320;  // Chiều rộng màn hình với rotation 3
    uint16_t inputBoxX = (screenWidth - inputBoxWidth) / 2;
    
    // Vẽ nền ô nhập
    tft->fillRect(inputBoxX, inputBoxY, inputBoxWidth, inputBoxHeight, inputBoxBgColor);
    
    // Vẽ viền ô nhập
    // Viền trên
    tft->drawFastHLine(inputBoxX, inputBoxY, inputBoxWidth, inputBoxBorderColor);
    tft->drawFastHLine(inputBoxX, inputBoxY + 1, inputBoxWidth, inputBoxBorderColor);
    // Viền dưới
    tft->drawFastHLine(inputBoxX, inputBoxY + inputBoxHeight - 2, inputBoxWidth, inputBoxBorderColor);
    tft->drawFastHLine(inputBoxX, inputBoxY + inputBoxHeight - 1, inputBoxWidth, inputBoxBorderColor);
    // Viền trái
    tft->drawFastVLine(inputBoxX, inputBoxY, inputBoxHeight, inputBoxBorderColor);
    tft->drawFastVLine(inputBoxX + 1, inputBoxY, inputBoxHeight, inputBoxBorderColor);
    // Viền phải
    tft->drawFastVLine(inputBoxX + inputBoxWidth - 2, inputBoxY, inputBoxHeight, inputBoxBorderColor);
    tft->drawFastVLine(inputBoxX + inputBoxWidth - 1, inputBoxY, inputBoxHeight, inputBoxBorderColor);
    
    // Vẽ decor cho input box nếu bật
    if (showInputBoxGlow) {
        drawInputBoxDecor();
    }
}

void ChatScreen::drawChatAreaDecor() {
    // Vẽ pattern trang trí cho vùng chat
    // Vẽ grid pattern nhẹ
    for (int x = 0; x < chatAreaWidth; x += 20) {
        uint16_t patternColor = (x + decorAnimationFrame) % 40 < 20 ? decorPatternColor : chatAreaBgColor;
        tft->drawFastVLine(x, chatAreaY, chatAreaHeight, patternColor);
    }
    
    // Vẽ dots pattern
    for (int x = 10; x < chatAreaWidth - 10; x += 15) {
        for (int y = chatAreaY + 10; y < chatAreaY + chatAreaHeight - 10; y += 15) {
            if ((x + y + decorAnimationFrame) % 30 < 15) {
                tft->drawPixel(x, y, decorPatternColor);
            }
        }
    }
}

void ChatScreen::drawInputBoxDecor() {
    // Vẽ hiệu ứng glow cho input box
    uint16_t screenWidth = 320;
    uint16_t inputBoxX = (screenWidth - inputBoxWidth) / 2;
    
    // Vẽ glow effect (viền sáng bên ngoài)
    uint16_t glowColor = decorAccentColor;
    int glowIntensity = (decorAnimationFrame % 20) < 10 ? 1 : 0;  // Nhấp nháy
    
    if (glowIntensity > 0) {
        // Vẽ glow trên
        tft->drawFastHLine(inputBoxX - 2, inputBoxY - 2, inputBoxWidth + 4, glowColor);
        tft->drawFastHLine(inputBoxX - 2, inputBoxY - 1, inputBoxWidth + 4, glowColor);
        // Vẽ glow dưới
        tft->drawFastHLine(inputBoxX - 2, inputBoxY + inputBoxHeight + 1, inputBoxWidth + 4, glowColor);
        tft->drawFastHLine(inputBoxX - 2, inputBoxY + inputBoxHeight + 2, inputBoxWidth + 4, glowColor);
        // Vẽ glow trái
        tft->drawFastVLine(inputBoxX - 2, inputBoxY - 2, inputBoxHeight + 4, glowColor);
        tft->drawFastVLine(inputBoxX - 1, inputBoxY - 2, inputBoxHeight + 4, glowColor);
        // Vẽ glow phải
        tft->drawFastVLine(inputBoxX + inputBoxWidth + 1, inputBoxY - 2, inputBoxHeight + 4, glowColor);
        tft->drawFastVLine(inputBoxX + inputBoxWidth + 2, inputBoxY - 2, inputBoxHeight + 4, glowColor);
    }
}

void ChatScreen::drawMessageBubble(int x, int y, int width, int height, uint16_t color, bool isUser) {
    // Vẽ bubble background cho tin nhắn (rounded rectangle đơn giản)
    // Vẽ nền bubble
    tft->fillRect(x, y, width, height, color);
    
    // Vẽ viền bubble (màu sáng hơn)
    uint16_t borderColor = interpolateColor(color, SOFT_WHITE, 0.3);
    tft->drawRect(x, y, width, height, borderColor);
    
    // Vẽ góc bo tròn đơn giản (4 góc)
    tft->drawPixel(x, y, bgColor);
    tft->drawPixel(x + width - 1, y, bgColor);
    tft->drawPixel(x, y + height - 1, bgColor);
    tft->drawPixel(x + width - 1, y + height - 1, bgColor);
}

void ChatScreen::drawCurrentMessage() {
    uint16_t screenWidth = 320;  // Chiều rộng màn hình với rotation 3
    uint16_t inputBoxX = (screenWidth - inputBoxWidth) / 2;
    uint16_t textX = inputBoxX + 5;
    uint16_t textY = inputBoxY + 10;
    
    // Xóa vùng text cũ
    uint16_t textAreaHeight = inputBoxHeight - 20;
    tft->fillRect(textX, textY, inputBoxWidth - 10, textAreaHeight, inputBoxBgColor);
    
    // Vẽ tin nhắn đang nhập
    tft->setTextColor(textColor, inputBoxBgColor);
    tft->setTextSize(2);  // Cỡ chữ tầm trung
    tft->setCursor(textX, textY);
    
    if (currentMessage.length() == 0) {
        // Hiển thị placeholder
        uint16_t placeholderColor = 0x5008;
        tft->setTextColor(placeholderColor, inputBoxBgColor);
        tft->print("Type message...");
    } else {
        // Hiển thị tin nhắn (cắt nếu quá dài)
        String displayText = currentMessage;
        int maxChars = (inputBoxWidth - 10) / 12;  // Với text size 2, khoảng 12px mỗi ký tự
        if (displayText.length() > maxChars) {
            displayText = displayText.substring(displayText.length() - maxChars);
        }
        tft->print(displayText);
    }
    
    // Vẽ lại viền dưới
    tft->drawFastHLine(inputBoxX, inputBoxY + inputBoxHeight - 2, inputBoxWidth, inputBoxBorderColor);
    tft->drawFastHLine(inputBoxX, inputBoxY + inputBoxHeight - 1, inputBoxWidth, inputBoxBorderColor);
}

void ChatScreen::draw() {
    // Vẽ nền màn hình
    tft->fillScreen(bgColor);
    
    // Vẽ tiêu đề
    drawTitle();
    
    // Vẽ vùng chat
    drawChatArea();
    
    // Vẽ tin nhắn
    drawMessages();
    
    // Vẽ ô nhập
    drawInputBox();
    
    // Vẽ tin nhắn đang nhập
    drawCurrentMessage();
    
    // Vẽ bàn phím chỉ khi keyboardVisible = true
    if (keyboardVisible) {
        keyboard->draw();
        
        // Vẽ lại viền sau khi bàn phím vẽ
        uint16_t screenWidth = 320;  // Chiều rộng màn hình với rotation 3
        uint16_t inputBoxX = (screenWidth - inputBoxWidth) / 2;
        tft->drawFastHLine(inputBoxX, inputBoxY + inputBoxHeight - 2, inputBoxWidth, inputBoxBorderColor);
        tft->drawFastHLine(inputBoxX, inputBoxY + inputBoxHeight - 1, inputBoxWidth, inputBoxBorderColor);
    }
}

void ChatScreen::handleKeyPress(String key) {
    // Xử lý các phím đặc biệt
    if (key == "|e") {
        // Enter - gửi tin nhắn
        sendMessage();
    } else if (key == "<") {
        // Delete - xóa ký tự cuối
        if (currentMessage.length() > 0) {
            currentMessage.remove(currentMessage.length() - 1);
            drawCurrentMessage();
        }
    } else if (key == " ") {
        // Space - thêm dấu cách
        if (currentMessage.length() < maxMessageLength) {
            currentMessage += " ";
            drawCurrentMessage();
        }
    } else if (key != "123" && key != "ABC" && key != "ic") {
        // Thêm ký tự thông thường
        if (currentMessage.length() < maxMessageLength) {
            currentMessage += key;
            drawCurrentMessage();
        }
    }
}

void ChatScreen::addMessage(String text, bool isUser) {
    if (messageCount >= MAX_MESSAGES) {
        // Dịch chuyển mảng, xóa tin nhắn cũ nhất
        for (int i = 0; i < MAX_MESSAGES - 1; i++) {
            messages[i] = messages[i + 1];
        }
        messageCount = MAX_MESSAGES - 1;
    }
    
    messages[messageCount].text = text;
    messages[messageCount].isUser = isUser;
    messages[messageCount].timestamp = millis();
    messageCount++;
    
    // Tự động cuộn xuống tin nhắn mới nhất
    scrollOffset = 0;
    
    // Lưu tin nhắn vào file
    saveMessagesToFile();
    
    // Vẽ lại tin nhắn
    drawMessages();
}

void ChatScreen::sendMessage() {
    if (currentMessage.length() > 0) {
        addMessage(currentMessage, true);
        currentMessage = "";
        drawCurrentMessage();
    }
}

void ChatScreen::handleUp() {
    // Cuộn lên (xem tin nhắn cũ hơn) - scroll theo từng dòng
    // Tính tổng số dòng
    int totalLines = 0;
    int charsPerLine = 20;
    for (int i = 0; i < messageCount; i++) {
        if (i > 0 && messages[i].isUser != messages[i-1].isUser) {
            totalLines += 1;  // Spacing
        }
        int messageLength = messages[i].text.length();
        int numLines = (messageLength + charsPerLine - 1) / charsPerLine;
        totalLines += numLines;
    }
    
    int maxLines = getVisibleLines();
    int maxScroll = (totalLines > maxLines) ? (totalLines - maxLines) : 0;
    
    if (scrollOffset < maxScroll) {
        scrollOffset++;  // Tăng 1 dòng
        drawMessages();
        Serial.print("Chat: Scrolled up 1 line, offset: ");
        Serial.print(scrollOffset);
        Serial.print("/");
        Serial.println(maxScroll);
    }
}

void ChatScreen::handleDown() {
    // Cuộn xuống (xem tin nhắn mới hơn) - scroll theo từng dòng
    if (scrollOffset > 0) {
        scrollOffset--;  // Giảm 1 dòng
        drawMessages();
        Serial.print("Chat: Scrolled down 1 line, offset: ");
        Serial.println(scrollOffset);
    }
}

void ChatScreen::handleSelect() {
    // Toggle keyboard visibility
    keyboardVisible = !keyboardVisible;
    
    if (keyboardVisible) {
        // Hiện bàn phím
        keyboard->draw();
        
        // Vẽ lại viền input box
        uint16_t screenWidth = 320;  // Chiều rộng màn hình với rotation 3
        uint16_t inputBoxX = (screenWidth - inputBoxWidth) / 2;
        tft->drawFastHLine(inputBoxX, inputBoxY + inputBoxHeight - 2, inputBoxWidth, inputBoxBorderColor);
        tft->drawFastHLine(inputBoxX, inputBoxY + inputBoxHeight - 1, inputBoxWidth, inputBoxBorderColor);
    } else {
        // Ẩn bàn phím - xóa vùng bàn phím
        // Bàn phím thường ở vị trí y = 80 trở xuống
        uint16_t screenWidth = 320;  // Chiều rộng màn hình với rotation 3
        tft->fillRect(0, 80, screenWidth, 240 - 80, bgColor);
        
        // Vẽ lại input box và tin nhắn
        drawInputBox();
        drawCurrentMessage();
        drawMessages();
    }
}

void ChatScreen::clearMessages() {
    messageCount = 0;
    scrollOffset = 0;
    
    // Xóa file lịch sử
    String fileName = getChatHistoryFileName();
    if (SPIFFS.exists(fileName)) {
        SPIFFS.remove(fileName);
        Serial.print("Chat: Cleared chat history file: ");
        Serial.println(fileName);
    }
    
    drawMessages();
}

String ChatScreen::getChatHistoryFileName() {
    // Tạo tên file từ owner và friend nickname
    // Format: "/owner_friend.txt"
    // Ví dụ: "/Tí Đô_Bạn.txt"
    String fileName = "/";
    fileName += ownerNickname;
    fileName += "_";
    fileName += friendNickname;
    fileName += ".txt";
    
    // Loại bỏ ký tự không hợp lệ trong tên file (thay space và ký tự đặc biệt)
    fileName.replace(" ", "_");
    fileName.replace("/", "_");
    fileName.replace("\\", "_");
    fileName.replace(":", "_");
    fileName.replace("*", "_");
    fileName.replace("?", "_");
    fileName.replace("\"", "_");
    fileName.replace("<", "_");
    fileName.replace(">", "_");
    fileName.replace("|", "_");
    
    return fileName;
}

void ChatScreen::saveMessagesToFile() {
    String fileName = getChatHistoryFileName();
    
    // Mở file để ghi
    File file = SPIFFS.open(fileName, "w");
    if (!file) {
        Serial.print("Chat: Failed to open file for writing: ");
        Serial.println(fileName);
        return;
    }
    
    // Ghi số lượng tin nhắn
    file.println(messageCount);
    
    // Ghi từng tin nhắn
    for (int i = 0; i < messageCount; i++) {
        // Format: isUser|text|timestamp
        file.print(messages[i].isUser ? "1" : "0");
        file.print("|");
        file.print(messages[i].text);
        file.print("|");
        file.println(messages[i].timestamp);
    }
    
    file.close();
    Serial.print("Chat: Saved ");
    Serial.print(messageCount);
    Serial.print(" messages to file: ");
    Serial.println(fileName);
}

void ChatScreen::loadMessagesFromFile() {
    String fileName = getChatHistoryFileName();
    
    // Kiểm tra file có tồn tại không
    if (!SPIFFS.exists(fileName)) {
        Serial.print("Chat: No history file found: ");
        Serial.println(fileName);
        return;
    }
    
    // Mở file để đọc
    File file = SPIFFS.open(fileName, "r");
    if (!file) {
        Serial.print("Chat: Failed to open file for reading: ");
        Serial.println(fileName);
        return;
    }
    
    // Đọc số lượng tin nhắn
    String countStr = file.readStringUntil('\n');
    int count = countStr.toInt();
    
    if (count > MAX_MESSAGES) count = MAX_MESSAGES;
    
    // Đọc từng tin nhắn
    messageCount = 0;
    for (int i = 0; i < count && messageCount < MAX_MESSAGES; i++) {
        String line = file.readStringUntil('\n');
        line.trim();
        
        if (line.length() == 0) continue;
        
        // Parse: isUser|text|timestamp
        int pipe1 = line.indexOf('|');
        int pipe2 = line.indexOf('|', pipe1 + 1);
        
        if (pipe1 > 0 && pipe2 > pipe1) {
            String isUserStr = line.substring(0, pipe1);
            String text = line.substring(pipe1 + 1, pipe2);
            String timestampStr = line.substring(pipe2 + 1);
            
            messages[messageCount].isUser = (isUserStr == "1");
            messages[messageCount].text = text;
            messages[messageCount].timestamp = timestampStr.toInt();
            messageCount++;
        }
    }
    
    file.close();
    Serial.print("Chat: Loaded ");
    Serial.print(messageCount);
    Serial.print(" messages from file: ");
    Serial.println(fileName);
    
    // Vẽ lại tin nhắn sau khi load
    if (messageCount > 0) {
        drawMessages();
    }
}

void ChatScreen::enableAllDecor() {
    showTitleBarGradient = true;
    showChatAreaPattern = true;
    showInputBoxGlow = true;
    showMessageBubbles = true;
    showScrollbarGlow = true;
}

void ChatScreen::disableAllDecor() {
    showTitleBarGradient = false;
    showChatAreaPattern = false;
    showInputBoxGlow = false;
    showMessageBubbles = false;
    showScrollbarGlow = false;
}

void ChatScreen::updateDecorAnimation() {
    // Cập nhật frame animation cho decor
    decorAnimationFrame++;
    if (decorAnimationFrame > 1000) decorAnimationFrame = 0;
}

