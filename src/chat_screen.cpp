#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include "chat_screen.h"
#include <FS.h>
#include <SPIFFS.h>

// Deep Space Arcade Theme (matching BuddyListScreen)
#define BG_DEEP_MIDNIGHT 0x0042  // Deep Midnight Blue #020817
#define HEADER_BLUE 0x08A5       // Lighter Blue #0F172A
#define HIGHLIGHT_BLUE 0x1148    // Electric Blue tint #162845
#define CYAN_ACCENT 0x07FF       // Cyan #00FFFF
#define TEXT_WHITE 0xFFFF        // White
#define ONLINE_GREEN 0x07F3      // Minty Green #00FF99
#define OFFLINE_RED 0xF986       // Bright Red #FF3333
#define SOFT_WHITE 0xFFFF        // White
#define NEON_PINK 0xF81F         // Keep for error/warning states

// Static member definition (must be defined in .cpp file)
ChatScreen* ChatScreen::instanceForCallback = nullptr;

ChatScreen::ChatScreen(Adafruit_ST7789* tft, Keyboard* keyboard) {
    this->tft = tft;
    this->keyboard = keyboard;
    
    // Khởi tạo vị trí và kích thước (màn hình rotation 3: 320x240)
    this->titleY = 5;
    this->chatAreaY = 25;  // Bắt đầu sau title bar (titleBarHeight ~25px)
    this->chatAreaHeight = 0;  // Sẽ được tính động
    this->chatAreaWidth = 320;   // Rộng toàn màn hình (rotation 3: 320px)
    this->inputBoxY = 0;  // Sẽ được tính động
    this->inputBoxHeight = 44;   // Chiều cao ô nhập (tăng nhẹ để dễ đọc)
    this->inputBoxWidth = 300;   // Rộng ô nhập (fit màn hình 320px)
    this->maxMessageLength = 80;  // Tăng độ dài tin nhắn lên 80 ký tự
    this->titleBarFocus = 0;  // No focus initially
    
    // Khởi tạo Confirmation Dialog
    this->confirmationDialog = new ConfirmationDialog(tft);
    
    // Khởi tạo tin nhắn
    this->messageCount = 0;
    this->scrollOffset = 0;
    
    // Khởi tạo lazy loading tracking
    this->loadedMessageCount = 0;
    this->totalMessagesInFile = 0;
    this->hasMoreMessages = false;
    this->fileReadPosition = 0;
    
    // Khởi tạo loading state flags
    this->isLoadingMessages = false;
    this->lastLoadTime = 0;
    this->showLoadingIndicator = false;
    
    // Khởi tạo file position cache
    this->fileLinePositions = nullptr;
    this->cachedLineCount = 0;
    this->positionCacheValid = false;
    this->currentMessage = "";
    this->inputCursorPos = 0;
    this->keyboardVisible = false;  // Bàn phím ẩn mặc định
    this->friendStatus = 2;  // 0 = offline, 1 = online, 2 = typing
    
    // Khởi tạo nickname mặc định
    this->ownerNickname = "You";      // Tên mặc định của người dùng
    this->friendNickname = "Other";    // Tên mặc định của người khác
    
    // Khởi tạo SPIFFS
    if (!SPIFFS.begin(true)) {
        Serial.println("Chat: SPIFFS initialization failed!");
    } else {
        Serial.println("Chat: SPIFFS initialized successfully");
        
        // Load nickname từ file nếu có
        const char* nicknameFile = "/nickname.txt";
        if (SPIFFS.exists(nicknameFile)) {
            File file = SPIFFS.open(nicknameFile, "r");
            if (file) {
                String loadedNickname = file.readStringUntil('\n');
                loadedNickname.trim();
                if (loadedNickname.length() > 0) {
                    this->ownerNickname = loadedNickname;
                    Serial.print("Chat: Loaded nickname from file: ");
                    Serial.println(loadedNickname);
                }
                file.close();
            }
        }
        
        // KHÔNG tự động load tin nhắn từ file khi khởi động
        // Messages sẽ được thêm sau khi vẽ màn hình chat
        // loadMessagesFromFile();  // Disabled - messages loaded after screen is drawn
    }
    
    // Khởi tạo màu sắc mặc định - Deep Space Arcade Theme
    this->bgColor = BG_DEEP_MIDNIGHT;           // Deep Midnight Blue background
    this->titleColor = TEXT_WHITE;              // White text for title
    this->chatAreaBgColor = BG_DEEP_MIDNIGHT;   // Same as background
    this->chatAreaBorderColor = CYAN_ACCENT;    // Cyan borders
    this->inputBoxBgColor = BG_DEEP_MIDNIGHT;   // Deep Midnight Blue
    this->inputBoxBorderColor = CYAN_ACCENT;    // Cyan border
    this->userMessageColor = ONLINE_GREEN;      // Minty Green for user messages
    this->otherMessageColor = CYAN_ACCENT;      // Cyan for other messages
    this->textColor = TEXT_WHITE;              // White text
    
    // Khởi tạo decor elements (mặc định tắt)
    this->showTitleBarGradient = false;
    this->showChatAreaPattern = false;
    this->showInputBoxGlow = false;
    this->showMessageBubbles = false;
    this->showScrollbarGlow = false;
    this->decorPatternColor = CYAN_ACCENT;  // Cyan for patterns
    this->decorAccentColor = CYAN_ACCENT;   // Cyan for accents
    this->decorAnimationFrame = 0;
    
    // Khởi tạo dirty flags (default true để trigger initial draw)
    this->needsRedraw = true;
    this->needsMessagesRedraw = true;
    this->needsInputRedraw = true;
    this->lastAnimationUpdate = 0;
    
    // Tính lại bố cục sau khi khởi tạo
    recalculateLayout();
}

ChatScreen::~ChatScreen() {
    // Giải phóng file position cache
    if (fileLinePositions != nullptr) {
        delete[] fileLinePositions;
        fileLinePositions = nullptr;
    }
}

void ChatScreen::drawTitle() {
    // Vẽ title bar fit toàn bộ màn hình (320px với rotation 3)
    // LƯU Ý: Nền đã được vẽ trong draw(), chỉ vẽ các element lên trên
    uint16_t titleBarHeight = 25;
    uint16_t titleBarY = 0;
    uint16_t screenWidth = 320;  // Chiều rộng màn hình với rotation 3
    
    // Vẽ viền dưới title bar (toàn bộ chiều rộng)
    tft->drawFastHLine(0, titleBarY + titleBarHeight - 2, screenWidth, chatAreaBorderColor);
    tft->drawFastHLine(0, titleBarY + titleBarHeight - 1, screenWidth, chatAreaBorderColor);
    
    // Spacing constants for "breathing room"
    const uint16_t screenPaddingX = 6;   // Left/Right edge padding
    const uint16_t buttonGap = 8;       // Gap between INVITE and KICK buttons
    const uint16_t sectionGap = 12;     // Gap between Name and INVITE button
    
    // Button dimensions
    const uint16_t buttonHeight = 20;    // Button height
    const uint16_t buttonPadding = 4;    // Internal padding for text
    
    // Button widths (fixed, with proper spacing for text labels)
    const uint16_t kickButtonWidth = 50;    // Width for "KICK" button
    const uint16_t inviteButtonWidth = 60;  // Width for "INVITE" button
    
    // Calculate total spacing overhead
    const uint16_t totalSpacingOverhead = screenPaddingX + sectionGap + buttonGap + screenPaddingX;  // 6 + 12 + 8 + 6 = 32px
    const uint16_t totalButtonsWidth = kickButtonWidth + inviteButtonWidth;  // 50 + 60 = 110px
    
    // Calculate max name width
    const uint16_t maxNameWidth = screenWidth - totalButtonsWidth - totalSpacingOverhead;  // 320 - 110 - 32 = 178px
    
    // Calculate max name width (left side)
    const uint16_t charWidth = 12;  // Ước lượng mỗi ký tự size 2 ~12px
    const uint16_t statusDotDiameter = 8;
    const uint16_t statusDotSpacing = 6;
    uint16_t reservedForDot = statusDotDiameter + statusDotSpacing;
    
    // Name width calculation (already calculated above: maxNameWidth = 178px)
    // Account for status dot in character calculation
    uint16_t maxNameWidthWithDot = maxNameWidth - reservedForDot;  // 178 - 14 = 164px
    uint16_t maxChars = maxNameWidthWithDot / charWidth;  // ~13-14 characters
    
    // Calculate button positions FIRST (needed for name cutoff calculation)
    uint16_t buttonY = titleBarY + (titleBarHeight - buttonHeight) / 2;  // Center vertically
    
    // Unfriend button (KICK) - rightmost, aligned at screenPaddingX from right
    uint16_t unfriendX = screenWidth - screenPaddingX - kickButtonWidth;  // 320 - 6 - 50 = 264px
    
    // Invite button (INVITE) - left of Unfriend with buttonGap
    uint16_t inviteX = unfriendX - inviteButtonWidth - buttonGap;  // 264 - 60 - 8 = 196px
    
    // Name cutoff point: Invite button X - sectionGap
    uint16_t nameCutoffX = inviteX - sectionGap;  // 196 - 12 = 184px
    
    // Vẽ tên bạn (friend) left-aligned, fallback "CHAT" nếu trống
    tft->setTextColor(titleColor, chatAreaBgColor);
    tft->setTextSize(2);  // Cỡ chữ tầm trung
    String title = friendNickname.length() > 0 ? friendNickname : "CHAT";
    
    // Truncate name if too long (based on maxChars calculation)
    if (title.length() > maxChars && maxChars > 3) {
        title = title.substring(0, maxChars - 3) + "...";
    }
    
    // Draw name text (left-aligned with screen padding)
    uint16_t textX = screenPaddingX;  // Start at 6px from left edge
    uint16_t textY = titleBarY + (titleBarHeight - 16) / 2;  // Căn giữa theo chiều dọc (text size 2 cao ~16px)
    
    // Draw name text (will be truncated if too long)
    tft->setCursor(textX, textY);
    tft->print(title);
    
    // Clip name if it would overlap (draw a background rect to cover overflow)
    uint16_t textWidth = title.length() * charWidth;
    uint16_t nameEndX = textX + textWidth;
    if (nameEndX > nameCutoffX) {
        // Draw background to cover overflow
        uint16_t overflowStart = nameCutoffX;
        uint16_t overflowWidth = nameEndX - nameCutoffX;
        tft->fillRect(overflowStart, titleBarY, overflowWidth, titleBarHeight, chatAreaBgColor);
    }
    
    // Vẽ dot trạng thái bên phải tên (only if name doesn't extend too far)
    uint16_t dotCenterX = textX + textWidth + statusDotSpacing + statusDotDiameter / 2;
    if (dotCenterX + statusDotDiameter / 2 < nameCutoffX) {
        uint16_t dotCenterY = titleBarY + titleBarHeight / 2;
        if (isStatusDotVisible()) {
            tft->fillCircle(dotCenterX, dotCenterY, statusDotDiameter / 2, getStatusDotColor());
        }
    }
    
    // Draw action buttons on the right
    bool unfriendFocused = (titleBarFocus == 2);
    drawTitleBarButton(unfriendX, buttonY, kickButtonWidth, buttonHeight, OFFLINE_RED, unfriendFocused, 'X', "KICK");
    
    bool inviteFocused = (titleBarFocus == 1);
    drawTitleBarButton(inviteX, buttonY, inviteButtonWidth, buttonHeight, CYAN_ACCENT, inviteFocused, '+', "INVITE");
    
    // Vẽ decor cho title bar nếu bật
    if (showTitleBarGradient) {
        drawTitleBarDecor();
    }
}

void ChatScreen::drawTitleBarButton(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color, bool focused, char icon, const String& label) {
    if (tft == nullptr) return;
    
    // Deep Space theme button styling:
    // Selected: Filled background (WIN_ACCENT/CYAN) + Dark text (WIN_BG_DARK)
    // Unselected: Transparent background + Cyan border + Cyan text
    uint16_t bgColor;
    uint16_t textColor;
    uint16_t borderColor = CYAN_ACCENT;  // Always use Cyan accent for border
    
    if (focused) {
        // Selected state: Filled with Cyan accent, dark text
        bgColor = CYAN_ACCENT;  // WIN_ACCENT
        textColor = BG_DEEP_MIDNIGHT;  // WIN_BG_DARK
        // Draw filled background
        tft->fillRoundRect(x, y, width, height, 3, bgColor);
        // Draw border (same color for consistency)
        tft->drawRoundRect(x, y, width, height, 3, borderColor);
    } else {
        // Unselected state: Transparent background, Cyan border and text
        bgColor = HEADER_BLUE;  // Use title bar background
        textColor = CYAN_ACCENT;  // WIN_ACCENT
        // Draw transparent background (title bar color)
        tft->fillRoundRect(x, y, width, height, 3, bgColor);
        // Draw border
        tft->drawRoundRect(x, y, width, height, 3, borderColor);
    }
    
    // Draw icon + text label
    const uint16_t iconSize = 12;  // Icon text size 2 ~12px wide
    const uint16_t textSize = 1;   // Compact text size 1 for labels
    const uint16_t iconTextSize = 2;  // Icon uses size 2
    const uint16_t spacing = 2;    // Space between icon and text
    
    // Calculate positions
    uint16_t iconX = x + 2;  // Small padding from left
    uint16_t iconY = y + (height - 16) / 2;  // Center icon vertically (text size 2 ~16px tall)
    
    uint16_t textX = iconX + iconSize + spacing;
    uint16_t textY = y + (height - 8) / 2;  // Center text vertically (text size 1 ~8px tall)
    
    // Draw icon
    tft->setTextColor(textColor, bgColor);
    tft->setTextSize(iconTextSize);
    tft->setCursor(iconX, iconY);
    tft->print(icon);
    
    // Draw text label
    tft->setTextColor(textColor, bgColor);
    tft->setTextSize(textSize);
    tft->setCursor(textX, textY);
    tft->print(label);
}

// drawConfirmationPopup() removed - now using generic ConfirmationDialog class

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

uint16_t ChatScreen::getStatusDotColor() const {
    // 0 = offline, 1 = online, 2 = typing
    switch (friendStatus) {
        case 1: // online
            return ONLINE_GREEN;  // Minty Green
        case 2: // typing
            return CYAN_ACCENT;   // Cyan
        default: // offline
            return OFFLINE_RED;   // Bright Red
    }
}

bool ChatScreen::isStatusDotVisible() const {
    // Blinking when typing, always visible otherwise
    if (friendStatus == 2) {
        return (decorAnimationFrame % 20) < 12;
    }
    return true;
}

void ChatScreen::drawChatArea() {
    // LƯU Ý: Nền đã được vẽ trong draw(), chỉ vẽ decor nếu bật
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

int ChatScreen::calculateTotalLines() const {
    int totalLines = 0;
    const int charsPerLine = 20;
    
    for (int i = 0; i < messageCount; i++) {
        if (i > 0 && messages[i].isUser != messages[i - 1].isUser) {
            totalLines += 1;  // Thêm 1 dòng spacing giữa hai người gửi
        }
        int messageLength = messages[i].text.length();
        int numLines = (messageLength + charsPerLine - 1) / charsPerLine;
        totalLines += numLines;
    }
    
    return totalLines;
}

void ChatScreen::clampScrollOffset() {
    int totalLines = calculateTotalLines();
    int maxLines = getVisibleLines();
    int maxScroll = (totalLines > maxLines) ? (totalLines - maxLines) : 0;
    
    if (scrollOffset > maxScroll) {
        scrollOffset = maxScroll;
    }
    if (scrollOffset < 0) {
        scrollOffset = 0;
    }
}

void ChatScreen::scrollToLatest() {
    int totalLines = calculateTotalLines();
    int maxLines = getVisibleLines();
    int maxScroll = (totalLines > maxLines) ? (totalLines - maxLines) : 0;
    scrollOffset = maxScroll;
}

uint16_t ChatScreen::computeKeyboardHeight() const {
    if (keyboard == nullptr) return 0;
    
    KeyboardSkin skin = keyboard->getSkin();
    // Bàn phím có 3 hàng phím + hàng Space; mỗi hàng cách nhau spacing
    uint16_t keyHeight = skin.keyHeight;
    uint16_t spacing = skin.spacing;
    
    uint16_t height = (4 * (keyHeight + spacing)) - spacing;  // 4 hàng, 3 khoảng cách
    // Chừa thêm biên nhỏ để tránh đè viền/glow
    height += 4;
    return height;
}

void ChatScreen::recalculateLayout() {
    const uint16_t screenWidth = 320;   // rotation 3
    const uint16_t screenHeight = 240;  // rotation 3
    const uint16_t titleBarHeight = 25;
    const uint16_t spacing = 6;         // Khoảng cách nhỏ giữa các khối
    const uint16_t minChatHeight = 40;  // Đảm bảo luôn có không gian hiển thị
    
    chatAreaWidth = screenWidth;
    
    // Chat Area starts immediately after Title Bar (maximizing vertical space)
    chatAreaY = titleBarHeight;
    
    uint16_t keyboardHeight = keyboardVisible ? computeKeyboardHeight() : 0;
    if (keyboardHeight > screenHeight) {
        keyboardHeight = screenHeight;
    }
    
    // Tính vị trí Y cho input box
    uint16_t proposedInputY;
    if (keyboardVisible) {
        // Đặt ngay trên bàn phím, chừa một khoảng nhỏ
        if (screenHeight > (keyboardHeight + inputBoxHeight + spacing)) {
            proposedInputY = screenHeight - keyboardHeight - inputBoxHeight - spacing;
        } else {
            proposedInputY = chatAreaY + spacing;  // fallback nếu bàn phím quá cao
        }
    } else {
        // Khi ẩn bàn phím, đặt sát đáy với khoảng cách nhỏ
        if (screenHeight > (inputBoxHeight + spacing)) {
            proposedInputY = screenHeight - inputBoxHeight - spacing;
        } else {
            proposedInputY = chatAreaY + spacing;
        }
    }
    
    // Giữ input box trong màn hình
    if (proposedInputY + inputBoxHeight > screenHeight) {
        proposedInputY = (screenHeight > inputBoxHeight) ? (screenHeight - inputBoxHeight) : 0;
    }
    if (proposedInputY < chatAreaY + spacing) {
        proposedInputY = chatAreaY + spacing;
    }
    
    inputBoxY = proposedInputY;
    
    // Tính lại chiều cao vùng chat dựa trên vị trí input box
    int16_t computedChatHeight = static_cast<int16_t>(inputBoxY) - static_cast<int16_t>(chatAreaY) - static_cast<int16_t>(spacing);
    if (computedChatHeight < static_cast<int16_t>(minChatHeight)) {
        computedChatHeight = minChatHeight;
    }
    if (computedChatHeight > static_cast<int16_t>(screenHeight - chatAreaY)) {
        computedChatHeight = screenHeight - chatAreaY;
    }
    chatAreaHeight = static_cast<uint16_t>(computedChatHeight);
    
    // Constraint check: Ensure chatAreaHeight doesn't drop below min when keyboard is visible
    if (keyboardVisible && chatAreaHeight < minChatHeight) {
        // Ensure minimum chat height is maintained
    }
    
    // Đảm bảo scrollOffset hợp lệ khi chiều cao thay đổi
    clampScrollOffset();
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
        uint16_t thumbColor = CYAN_ACCENT;  // Cyan cho thumb
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
    // Chỉ vẽ lại khi cần thiết (optimization để tránh vẽ liên tục)
    if (!needsMessagesRedraw && !needsRedraw) return;
    
    // Optional: Draw lightweight loading indicator at top when loading older messages
    if (showLoadingIndicator && isLoadingMessages) {
        int indicatorY = chatAreaY + 2;
        tft->setTextSize(1);
        tft->setTextColor(ST77XX_WHITE);
        tft->setCursor(4, indicatorY);
        tft->print("Loading...");
    }
    
    int lineHeight = 24;  // Khoảng cách cho text size 2 (16px text + 8px spacing)
    int padding = 4;
    int startY = chatAreaY + padding;
    int maxLines = getVisibleLines();
    // Nén khoảng cách giữa các nhóm tin nhắn (khác người gửi) để tăng mật độ
    const int groupGapPx = 2;            // Gap nhỏ giữa các nhóm (sát hơn)
    const int gapReduction = lineHeight - groupGapPx;  // Lượng nén so với 1 dòng đầy đủ
    int groupTransitions = 0;            // Số lần đổi người gửi đã render
    
    // LƯU Ý: Nền chat area đã được vẽ trong draw(), không cần xóa lại
    // Chỉ cần vẽ tin nhắn lên trên nền đã có
    uint16_t scrollbarWidth = 4;
    
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
                // Khác người gửi -> thêm khoảng cách (nén nhỏ hơn 1 dòng)
                groupTransitions++;
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
                int yPos = startY + lineIndex * lineHeight - groupTransitions * gapReduction;
                
                // Tính vị trí X: user căn phải, other căn trái
            uint16_t textX;
            uint16_t approxWidth = lineText.length() * 12;  // ước lượng
            if (messages[i].isUser) {
                // Tin nhắn của user: căn phải
                textX = chatAreaWidth - approxWidth - margin;
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
                tft->setTextColor(msgColor, bgColor);
                tft->setTextSize(2);
                uint16_t cursorX = textX;
                for (uint16_t cIndex = 0; cIndex < lineText.length(); cIndex++) {
                    char c = lineText.charAt(cIndex);
                    if (c >= Keyboard::ICON_SMILE && c <= Keyboard::ICON_WINK) {
                        drawIconInline(cursorX, yPos, 12, msgColor, bgColor, c);
                        cursorX += 12;
                    } else {
                        tft->setCursor(cursorX, yPos);
                        tft->print(String(c));
                        cursorX += 12;
                    }
                }
                
                lineIndex++;
            }
        }
    }
    
    // Vẽ thanh scrollbar sau khi vẽ tin nhắn
    drawScrollbar();
    
    // Reset dirty flag sau khi vẽ xong
    needsMessagesRedraw = false;
}

void ChatScreen::drawInputBox() {
    uint16_t screenWidth = 320;  // Chiều rộng màn hình với rotation 3
    uint16_t inputBoxX = (screenWidth - inputBoxWidth) / 2;
    
    // LƯU Ý: Nền đã được vẽ trong draw(), chỉ vẽ viền và decor
    
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

// Vẽ icon inline cho text/chat (kích thước khoảng 12px)
void ChatScreen::drawIconInline(uint16_t x, uint16_t y, uint16_t size, uint16_t color, uint16_t bgColor, char iconCode) {
    // clear nền nhỏ cho icon để tránh cấn text
    tft->fillRect(x, y, size, size, bgColor);
    uint16_t cx = x + size / 2;
    uint16_t cy = y + size / 2;
    uint16_t r = size / 2 - 1;
    if (r < 3) r = 3;
    switch (iconCode) {
        case Keyboard::ICON_SMILE:
        case Keyboard::ICON_WINK:
            // mặt cười
            tft->drawCircle(cx, cy, r, color);
            tft->fillCircle(cx - r/2, cy - r/3, 1, color);
            if (iconCode == Keyboard::ICON_WINK) {
                tft->drawFastHLine(cx + r/2 - 1, cy - r/3, 3, color);
            } else {
                tft->fillCircle(cx + r/2, cy - r/3, 1, color);
            }
            // miệng cong
            for (int16_t dx = -r + 1; dx <= r - 1; dx++) {
                int16_t dy = (r * r - dx * dx) / (r * 2);
                int16_t py = cy + dy / 2;
                if (py > cy) tft->drawPixel(cx + dx, py, color);
            }
            break;
        case Keyboard::ICON_HEART:
            tft->fillCircle(cx - r/2, cy - r/3, r/2, color);
            tft->fillCircle(cx + r/2, cy - r/3, r/2, color);
            tft->fillTriangle(cx - r, cy - r/3, cx + r, cy - r/3, cx, cy + r, color);
            break;
        case Keyboard::ICON_STAR:
            for (int i = 0; i < 5; i++) {
                float a1 = (72 * i - 90) * 3.14159 / 180.0;
                float a2 = (72 * (i + 2) - 90) * 3.14159 / 180.0;
                uint16_t x1 = cx + r * cos(a1);
                uint16_t y1 = cy + r * sin(a1);
                uint16_t x2 = cx + r * cos(a2);
                uint16_t y2 = cy + r * sin(a2);
                tft->drawLine(x1, y1, x2, y2, color);
            }
            break;
        case Keyboard::ICON_CHECK:
            tft->drawLine(cx - r, cy, cx - r/2, cy + r, color);
            tft->drawLine(cx - r/2, cy + r, cx + r, cy - r/2, color);
            break;
        case Keyboard::ICON_MUSIC:
            tft->drawLine(cx - r/2, cy - r, cx - r/2, cy + r/2, color);
            tft->drawLine(cx - r/2, cy - r, cx + r, cy - r/2, color);
            tft->fillCircle(cx - r/2, cy + r/2, r/3, color);
            tft->fillCircle(cx + r, cy, r/3, color);
            break;
        case Keyboard::ICON_SUN:
            tft->drawCircle(cx, cy, r-1, color);
            for (int i = 0; i < 8; i++) {
                float ang = (45 * i) * 3.14159 / 180.0;
                uint16_t x1 = cx + (r+1) * cos(ang);
                uint16_t y1 = cy + (r+1) * sin(ang);
                uint16_t x2 = cx + (r+3) * cos(ang);
                uint16_t y2 = cy + (r+3) * sin(ang);
                tft->drawLine(x1, y1, x2, y2, color);
            }
            break;
        case Keyboard::ICON_FIRE:
            tft->fillTriangle(cx, cy - r, cx - r, cy + r, cx + r, cy + r, color);
            break;
        case Keyboard::ICON_THUMBS:
            tft->fillRect(cx - r/2, cy - r/2, r/2, r + 2, color);
            tft->fillRect(cx - r/2, cy - r/2, r, r/3, color);
            tft->fillRect(cx + r/2, cy - r/2, r/3, r/2, color);
            break;
        case Keyboard::ICON_GIFT:
            tft->drawRect(cx - r, cy - r/2, r*2, r+2, color);
            tft->drawFastVLine(cx, cy - r/2, r+2, color);
            tft->drawFastHLine(cx - r, cy, r*2, color);
            tft->drawCircle(cx - r/2, cy - r/2, r/3, color);
            tft->drawCircle(cx + r/2, cy - r/2, r/3, color);
            break;
        default:
            tft->drawCircle(cx, cy, r, color);
            break;
    }
}

bool ChatScreen::containsIcon(const String& text) const {
    for (uint16_t i = 0; i < text.length(); i++) {
        char c = text.charAt(i);
        if (c >= Keyboard::ICON_SMILE && c <= Keyboard::ICON_WINK) {
            return true;
        }
    }
    return false;
}

void ChatScreen::drawCurrentMessage() {
    uint16_t screenWidth = 320;  // Chiều rộng màn hình với rotation 3
    uint16_t inputBoxX = (screenWidth - inputBoxWidth) / 2;
    uint16_t textX = inputBoxX + 5;
    
    // Căn giữa theo chiều dọc dựa trên chiều cao text size 2 (~16px)
    const uint16_t textHeight = 16;
    int16_t verticalPadding = static_cast<int16_t>((inputBoxHeight - textHeight) / 2);
    if (verticalPadding < 2) verticalPadding = 2;  // Đảm bảo có tối thiểu một chút padding
    uint16_t textY = inputBoxY + verticalPadding;
    
    // Xóa vùng text cũ
    uint16_t textAreaHeight = inputBoxHeight > (verticalPadding * 2)
                                ? (inputBoxHeight - verticalPadding * 2)
                                : inputBoxHeight;
    tft->fillRect(textX, textY, inputBoxWidth - 10, textAreaHeight, inputBoxBgColor);
    
    // Vẽ tin nhắn đang nhập (có icon)
    tft->setTextSize(2);  // Cỡ chữ tầm trung
    // Đảm bảo con trỏ luôn nằm trong phạm vi
    if (inputCursorPos < 0) inputCursorPos = 0;
    if (inputCursorPos > currentMessage.length()) inputCursorPos = currentMessage.length();
    if (currentMessage.length() == 0) {
        // Hiển thị placeholder
        uint16_t placeholderColor = 0x5008;
        tft->setTextColor(placeholderColor, inputBoxBgColor);
        tft->setCursor(textX, textY);
        tft->print("Type message...");
    } else {
        // Hiển thị tin nhắn (cắt nếu quá dài) với icon
        String displayText = currentMessage;
        int maxChars = (inputBoxWidth - 10) / 12;  // Với text size 2, khoảng 12px mỗi ký tự
        if (displayText.length() > maxChars) {
            displayText = displayText.substring(displayText.length() - maxChars);
        }
        uint16_t cursorX = textX;
        uint16_t cursorY = textY;
        for (uint16_t i = 0; i < displayText.length(); i++) {
            char c = displayText.charAt(i);
            if (c >= Keyboard::ICON_SMILE && c <= Keyboard::ICON_WINK) {
                drawIconInline(cursorX, cursorY, 12, textColor, inputBoxBgColor, c);
                cursorX += 12;
            } else {
                tft->setTextColor(textColor, inputBoxBgColor);
                tft->setCursor(cursorX, cursorY);
                tft->print(String(c));
                cursorX += 12;
            }
            if (cursorX > inputBoxX + inputBoxWidth - 12) break;  // tránh tràn
        }
    }
    
    // Vẽ lại viền dưới
    tft->drawFastHLine(inputBoxX, inputBoxY + inputBoxHeight - 2, inputBoxWidth, inputBoxBorderColor);
    tft->drawFastHLine(inputBoxX, inputBoxY + inputBoxHeight - 1, inputBoxWidth, inputBoxBorderColor);
    
    // Reset dirty flag sau khi vẽ xong
    needsInputRedraw = false;
}

void ChatScreen::forceRedraw() {
    // Force a full screen redraw by setting all dirty flags
    needsRedraw = true;
    needsMessagesRedraw = true;
    needsInputRedraw = true;
    // Call draw() which will clear the screen and redraw everything
    draw();
}

void ChatScreen::draw() {
    // Chỉ vẽ lại khi cần thiết (optimization để tránh vẽ liên tục)
    if (!needsRedraw) return;
    
    // Bố cục có thể thay đổi khi toggle keyboard -> tính lại mỗi lần vẽ
    recalculateLayout();
    
    // BƯỚC 1: Vẽ nền full màn hình trước - ALWAYS clear entire screen
    // This ensures no artifacts from previous screen remain
    tft->fillScreen(bgColor);
    
    // BƯỚC 2: Vẽ nền cho từng vùng trước
    const uint16_t screenWidth = 320;
    const uint16_t screenHeight = 240;
    const uint16_t titleBarHeight = 25;
    
    // Vẽ nền title bar với màu header (matching BuddyListScreen)
    tft->fillRect(0, 0, screenWidth, titleBarHeight, HEADER_BLUE);
    
    // Vẽ nền vùng chat (từ title bar xuống đến input box)
    tft->fillRect(0, chatAreaY, chatAreaWidth, chatAreaHeight, chatAreaBgColor);
    
    // Vẽ nền ô nhập
    uint16_t inputBoxX = (screenWidth - inputBoxWidth) / 2;
    tft->fillRect(inputBoxX, inputBoxY, inputBoxWidth, inputBoxHeight, inputBoxBgColor);
    
    // Vẽ nền cho bàn phím (nếu hiển thị)
    if (keyboardVisible && keyboard != nullptr) {
        uint16_t keyboardHeight = computeKeyboardHeight();
        uint16_t keyboardY = screenHeight - keyboardHeight;
        tft->fillRect(0, keyboardY, screenWidth, keyboardHeight, bgColor);
    }
    
    // BƯỚC 3: Vẽ các element lên trên nền đã vẽ
    // Vẽ tiêu đề (text, dot trạng thái, và action buttons)
    drawTitle();
    
    // Vẽ tin nhắn
    drawMessages();
    
    // Vẽ viền và decor cho ô nhập
    drawInputBox();
    
    // Vẽ tin nhắn đang nhập
    drawCurrentMessage();
    
    // Vẽ bàn phím chỉ khi keyboardVisible = true
    if (keyboardVisible) {
        keyboard->draw();
        
        // Vẽ lại viền sau khi bàn phím vẽ
        tft->drawFastHLine(inputBoxX, inputBoxY + inputBoxHeight - 2, inputBoxWidth, inputBoxBorderColor);
        tft->drawFastHLine(inputBoxX, inputBoxY + inputBoxHeight - 1, inputBoxWidth, inputBoxBorderColor);
    }
    
    // Vẽ Confirmation Dialog nếu đang hiển thị (vẽ sau cùng để ở trên cùng)
    if (confirmationDialog != nullptr && confirmationDialog->isVisible()) {
        confirmationDialog->draw();
    }
    
    // Reset tất cả dirty flags sau khi vẽ xong
    needsRedraw = false;
    needsMessagesRedraw = false;
    needsInputRedraw = false;
}

void ChatScreen::handleKeyPress(String key) {
    // Xử lý các phím đặc biệt
    if (key == "|e") {
        // Enter - gửi tin nhắn
        sendMessage();
    } else if (key == "<") {
        // Delete - xóa ký tự cuối
        if (currentMessage.length() > 0 && inputCursorPos > 0) {
            currentMessage.remove(inputCursorPos - 1, 1);
            inputCursorPos--;
            needsInputRedraw = true;
            drawCurrentMessage();
        }
    } else if (key == " ") {
        // Space - thêm dấu cách
        if (currentMessage.length() < maxMessageLength) {
            currentMessage = currentMessage.substring(0, inputCursorPos) + " " + currentMessage.substring(inputCursorPos);
            inputCursorPos++;
            needsInputRedraw = true;
            drawCurrentMessage();
        }
    } else if (key != "123" && key != "ABC" && key != Keyboard::KEY_ICON && key != "shift") {
        // Thêm ký tự thông thường
        if (currentMessage.length() < maxMessageLength) {
            currentMessage = currentMessage.substring(0, inputCursorPos) + key + currentMessage.substring(inputCursorPos);
            inputCursorPos += key.length();
            needsInputRedraw = true;
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
    scrollToLatest();
    
    // Lưu tin nhắn vào file
    saveMessagesToFile();
    
    // Đánh dấu cần vẽ lại messages
    needsMessagesRedraw = true;
    // drawMessages() will be called via flag system
}

void ChatScreen::sendMessage() {
    if (currentMessage.length() > 0) {
        // Yêu cầu phải có ít nhất 1 icon trước khi gửi
        if (!containsIcon(currentMessage)) {
            // Hiển thị hint nhỏ ngay trên ô input
            uint16_t screenWidth = 320;
            uint16_t inputBoxX = (screenWidth - inputBoxWidth) / 2;
            int16_t hintY = static_cast<int16_t>(inputBoxY) - 12;
            if (hintY < 0) hintY = 0;
            // Xóa vùng hint cũ và vẽ thông báo
            tft->fillRect(inputBoxX, hintY, inputBoxWidth, 10, bgColor);
            tft->setTextSize(1);
            tft->setTextColor(OFFLINE_RED, bgColor);
            tft->setCursor(inputBoxX, hintY);
            tft->print("Add an icon to send");
            // Nhấn mạnh viền dưới ô input
            tft->drawFastHLine(inputBoxX, inputBoxY + inputBoxHeight - 2, inputBoxWidth, OFFLINE_RED);
            tft->drawFastHLine(inputBoxX, inputBoxY + inputBoxHeight - 1, inputBoxWidth, OFFLINE_RED);
            return;
        }
        addMessage(currentMessage, true);
        currentMessage = "";
        inputCursorPos = 0;
        needsInputRedraw = true;
        needsMessagesRedraw = true;  // Message was added, so messages need redraw too
        drawCurrentMessage();
    }
}

void ChatScreen::handleUp() {
    // If confirmation dialog is showing, do nothing (dialog blocks navigation)
    if (confirmationDialog != nullptr && confirmationDialog->isVisible()) {
        return;
    }
    
    // If Title Bar is focused, do nothing (can't go up further)
    if (titleBarFocus > 0) {
        return;
    }
    
    // If at top of chat (scrollOffset at max), move focus to Title Bar
    int totalLines = calculateTotalLines();
    int maxLines = getVisibleLines();
    int maxScroll = (totalLines > maxLines) ? (totalLines - maxLines) : 0;
    
    if (scrollOffset >= maxScroll) {
        // Move focus to Title Bar (Invite button - rightmost default)
        titleBarFocus = 1;
        needsRedraw = true;
        draw();
        return;
    }
    
    // Cuộn lên (xem tin nhắn cũ hơn) - scroll theo từng dòng
    // SMOOTH LOADING: Load incrementally (1-2 messages) for progressive, smooth experience
    // Reduced threshold and debounce for more responsive loading
    const int PREFETCH_THRESHOLD = 5;  // Load when 5 lines from top (reduced from 7 for earlier trigger)
    const unsigned long LOAD_DEBOUNCE_MS = 200;  // Reduced from 500ms for more responsive loading
    
    if (scrollOffset >= (maxScroll - PREFETCH_THRESHOLD) && 
        hasMoreMessages && 
        !isLoadingMessages &&
        (millis() - lastLoadTime) > LOAD_DEBOUNCE_MS) {
        
        Serial.println("Chat: Smoothly loading older messages...");
        isLoadingMessages = true;
        showLoadingIndicator = true;
        lastLoadTime = millis();
        
        // Load 1-2 messages per batch for incremental, smooth loading
        // This prevents sudden jumps and provides progressive experience
        bool loaded = loadMoreMessages(2);
        
        isLoadingMessages = false;
        showLoadingIndicator = false;
        
        if (loaded) {
            // Tính lại sau khi load (loadMoreMessages đã adjust scrollOffset)
            totalLines = calculateTotalLines();
            maxLines = getVisibleLines();
            maxScroll = (totalLines > maxLines) ? (totalLines - maxLines) : 0;
            // scrollOffset đã được adjust trong loadMoreMessages() để maintain position
        }
    }
    
    if (scrollOffset < maxScroll) {
        scrollOffset++;  // Tăng 1 dòng
        // Chỉ set flag, không gọi drawMessages() trực tiếp
        needsMessagesRedraw = true;
        Serial.print("Chat: Scrolled up 1 line, offset: ");
        Serial.print(scrollOffset);
        Serial.print("/");
        Serial.println(maxScroll);
    }
}

void ChatScreen::handleDown() {
    // If confirmation dialog is showing, do nothing (dialog blocks navigation)
    if (confirmationDialog != nullptr && confirmationDialog->isVisible()) {
        return;
    }
    
    // If Title Bar is focused, move focus to Chat Area (latest message)
    if (titleBarFocus > 0) {
        titleBarFocus = 0;
        scrollToLatest();  // Scroll to latest message
        needsRedraw = true;
        draw();
        return;
    }
    
    // Cuộn xuống (xem tin nhắn mới hơn) - scroll theo từng dòng
    if (scrollOffset > 0) {
        scrollOffset--;  // Giảm 1 dòng
        // Chỉ set flag, không gọi drawMessages() trực tiếp
        needsMessagesRedraw = true;
        Serial.print("Chat: Scrolled down 1 line, offset: ");
        Serial.println(scrollOffset);
    }
}

void ChatScreen::handleLeft() {
    // If confirmation dialog is showing, navigate dialog buttons
    if (confirmationDialog != nullptr && confirmationDialog->isVisible()) {
        confirmationDialog->handleLeft();
        // No need to redraw entire screen - handleLeft() calls drawButtonSelection() internally
        return;
    }
    
    // Only works when Title Bar is focused
    if (titleBarFocus == 2) {
        // Move from Unfriend to Invite
        titleBarFocus = 1;
        needsRedraw = true;
        draw();
    }
}

void ChatScreen::handleRight() {
    // If confirmation dialog is showing, navigate dialog buttons
    if (confirmationDialog != nullptr && confirmationDialog->isVisible()) {
        confirmationDialog->handleRight();
        // No need to redraw entire screen - handleRight() calls drawButtonSelection() internally
        return;
    }
    
    // Only works when Title Bar is focused
    if (titleBarFocus == 1) {
        // Move from Invite to Unfriend
        titleBarFocus = 2;
        needsRedraw = true;
        draw();
    }
}

void ChatScreen::handleSelect() {
    // If confirmation dialog is showing, handle dialog selection
    if (confirmationDialog != nullptr && confirmationDialog->isVisible()) {
        confirmationDialog->handleSelect();
        // Handle pending action after dialog closes
        if (pendingDialogAction == 1 && !confirmationDialog->isVisible()) {
            // Check which button was selected (we'll handle this in the callback)
            pendingDialogAction = 0;
        }
        titleBarFocus = 0;  // Return focus to chat area
        needsRedraw = true;
        draw();
        return;
    }
    
    // If Title Bar is focused, trigger action
    if (titleBarFocus > 0) {
        if (titleBarFocus == 1) {
            // Invite action - send game invite message
            Serial.println("Chat: Invite to Game triggered");
            String inviteMsg = "🎮 " + ownerNickname + " invited you to play a game!";
            addMessage(inviteMsg, true);
            // Also add a response message (simulated)
            delay(500);
            String responseMsg = friendNickname + " received your game invite";
            addMessage(responseMsg, false);
        } else if (titleBarFocus == 2) {
            // Unfriend action - show confirmation dialog
            Serial.println("Chat: Unfriend triggered - showing dialog");
            if (confirmationDialog != nullptr) {
                pendingDialogAction = 1;  // Mark as unfriend action
                String message = "Unfriend " + friendNickname + "?";
                confirmationDialog->show(message, "YES", "NO", 
                    staticOnUnfriendConfirm, staticOnUnfriendCancel, OFFLINE_RED);  // Bright Red border
                needsRedraw = true;
                draw();
            }
            return;
        }
        // Return focus to chat area after action (if popup not shown)
        titleBarFocus = 0;
        needsRedraw = true;
        draw();
        return;
    }
    
    // Toggle keyboard visibility (when not in Title Bar and not in popup)
    keyboardVisible = !keyboardVisible;
    // Bố cục thay đổi -> đánh dấu cần vẽ lại toàn bộ
    needsRedraw = true;
    draw();
}

void ChatScreen::clearMessages() {
    messageCount = 0;
    scrollOffset = 0;
    
    // Reset lazy loading state
    loadedMessageCount = 0;
    totalMessagesInFile = 0;
    hasMoreMessages = false;
    fileReadPosition = 0;
    
    // Invalidate position cache
    invalidatePositionCache();
    
    // Xóa file lịch sử
    String fileName = getChatHistoryFileName();
    if (SPIFFS.exists(fileName)) {
        SPIFFS.remove(fileName);
        Serial.print("Chat: Cleared chat history file: ");
        Serial.println(fileName);
    }
    
    needsMessagesRedraw = true;
    // drawMessages() will be called via flag system
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

// Static callback wrappers for ConfirmationDialog
void ChatScreen::staticOnUnfriendConfirm() {
    if (instanceForCallback != nullptr) {
        instanceForCallback->onUnfriendConfirm();
    }
}

void ChatScreen::staticOnUnfriendCancel() {
    if (instanceForCallback != nullptr) {
        instanceForCallback->onUnfriendCancel();
    }
}

// Instance callback methods for ConfirmationDialog
void ChatScreen::onUnfriendConfirm() {
    String unfriendMsg = "You unfriended " + friendNickname;
    addMessage(unfriendMsg, true);
    Serial.print("Chat: Unfriended ");
    Serial.println(friendNickname);
    // TODO: Actually remove friend from buddy list (requires callback to main)
}

void ChatScreen::onUnfriendCancel() {
    Serial.println("Chat: Unfriend cancelled");
}

// Helper: Calculate total lines for a set of messages
int ChatScreen::calculateLinesForMessages(const ChatMessage* msgs, int msgCount) const {
    int totalLines = 0;
    const int charsPerLine = 20;
    
    for (int i = 0; i < msgCount; i++) {
        if (i > 0 && msgs[i].isUser != msgs[i-1].isUser) {
            totalLines += 1;  // Spacing between different senders
        }
        int messageLength = msgs[i].text.length();
        int numLines = (messageLength + charsPerLine - 1) / charsPerLine;
        totalLines += numLines;
    }
    
    return totalLines;
}

// Helper: Invalidate position cache
void ChatScreen::invalidatePositionCache() {
    positionCacheValid = false;
    if (fileLinePositions != nullptr) {
        delete[] fileLinePositions;
        fileLinePositions = nullptr;
        cachedLineCount = 0;
    }
}

// Helper: Build file position cache for efficient seeking
void ChatScreen::buildFilePositionCache() {
    String fileName = getChatHistoryFileName();
    if (!SPIFFS.exists(fileName)) {
        invalidatePositionCache();
        return;
    }
    
    File file = SPIFFS.open(fileName, "r");
    if (!file) {
        invalidatePositionCache();
        return;
    }
    
    // Free old cache
    if (fileLinePositions != nullptr) {
        delete[] fileLinePositions;
    }
    
    // Allocate cache for max messages
    fileLinePositions = new size_t[totalMessagesInFile + 1];  // +1 for count line
    cachedLineCount = 0;
    
    // Read first line (count) - cache its position
    fileLinePositions[0] = 0;  // Start of file
    String countLine = file.readStringUntil('\n');
    cachedLineCount = 1;
    
    // Read each message line and cache its position
    size_t currentPos = file.position();
    int messageIndex = 0;
    while (file.available() && messageIndex < totalMessagesInFile) {
        fileLinePositions[cachedLineCount] = currentPos;
        String line = file.readStringUntil('\n');
        currentPos = file.position();
        if (line.length() > 0) {
            cachedLineCount++;
            messageIndex++;
        }
    }
    
    file.close();
    positionCacheValid = true;
    
    Serial.print("Chat: Built position cache for ");
    Serial.print(cachedLineCount - 1);  // -1 for count line
    Serial.println(" messages");
}

// Helper: Read message at specific line using cache
ChatMessage ChatScreen::readMessageAtLine(File& file, int lineIndex) {
    ChatMessage msg;
    msg.text = "";
    msg.isUser = false;
    msg.timestamp = 0;
    
    if (lineIndex < 0 || lineIndex >= cachedLineCount - 1) {
        return msg;  // Invalid index
    }
    
    // Seek to line position (lineIndex + 1 because line 0 is count)
    if (positionCacheValid && fileLinePositions != nullptr) {
        file.seek(fileLinePositions[lineIndex + 1]);
    }
    
    String line = file.readStringUntil('\n');
    line.trim();
    
    if (line.length() == 0) {
        return msg;
    }
    
    // Parse: isUser|text|timestamp
    int pipe1 = line.indexOf('|');
    int pipe2 = line.indexOf('|', pipe1 + 1);
    
    if (pipe1 > 0 && pipe2 > pipe1) {
        String isUserStr = line.substring(0, pipe1);
        String text = line.substring(pipe1 + 1, pipe2);
        String timestampStr = line.substring(pipe2 + 1);
        
        msg.isUser = (isUserStr == "1");
        msg.text = text;
        msg.timestamp = timestampStr.toInt();
    }
    
    return msg;
}

// Helper: Read last N messages efficiently (Facebook-style)
void ChatScreen::readLastNMessages(File& file, int n, ChatMessage* output, int& outputCount) {
    outputCount = 0;
    
    if (n <= 0 || totalMessagesInFile == 0) {
        return;
    }
    
    int messagesToRead = (n > totalMessagesInFile) ? totalMessagesInFile : n;
    int startLineIndex = totalMessagesInFile - messagesToRead;
    
    // Cache should be built before calling this function
    if (!positionCacheValid || fileLinePositions == nullptr) {
        Serial.println("Chat: Warning - position cache not valid, falling back to sequential read");
        // Fallback: sequential read from end
        // Skip count line
        file.seek(0);
        file.readStringUntil('\n');
        
        // Read all lines to find last N
        String* tempLines = new String[totalMessagesInFile];
        int tempCount = 0;
        while (file.available() && tempCount < totalMessagesInFile) {
            String line = file.readStringUntil('\n');
            line.trim();
            if (line.length() > 0) {
                tempLines[tempCount] = line;
                tempCount++;
            }
        }
        
        // Parse last N messages
        int startIdx = (tempCount > messagesToRead) ? (tempCount - messagesToRead) : 0;
        for (int i = startIdx; i < tempCount && outputCount < messagesToRead; i++) {
            String line = tempLines[i];
            int pipe1 = line.indexOf('|');
            int pipe2 = line.indexOf('|', pipe1 + 1);
            if (pipe1 > 0 && pipe2 > pipe1) {
                output[outputCount].isUser = (line.substring(0, pipe1) == "1");
                output[outputCount].text = line.substring(pipe1 + 1, pipe2);
                output[outputCount].timestamp = line.substring(pipe2 + 1).toInt();
                outputCount++;
            }
        }
        
        delete[] tempLines;
        return;
    }
    
    // Read messages using cache
    for (int i = startLineIndex; i < totalMessagesInFile && outputCount < messagesToRead; i++) {
        ChatMessage msg = readMessageAtLine(file, i);
        if (msg.text.length() > 0) {
            output[outputCount] = msg;
            outputCount++;
        }
    }
}

// Helper: Read messages from specific position
void ChatScreen::readMessagesFromPosition(File& file, int startIndex, int count, ChatMessage* output, int& outputCount) {
    outputCount = 0;
    
    if (startIndex < 0 || count <= 0 || startIndex >= totalMessagesInFile) {
        return;
    }
    
    int messagesToRead = count;
    if (startIndex + messagesToRead > totalMessagesInFile) {
        messagesToRead = totalMessagesInFile - startIndex;
    }
    
    // Cache should be built before calling this function
    if (!positionCacheValid || fileLinePositions == nullptr) {
        Serial.println("Chat: Warning - position cache not valid, falling back to sequential read");
        // Fallback: sequential read
        file.seek(0);
        file.readStringUntil('\n');  // Skip count line
        
        // Skip to startIndex
        for (int i = 0; i < startIndex; i++) {
            file.readStringUntil('\n');
        }
        
        // Read messages
        for (int i = 0; i < messagesToRead && file.available(); i++) {
            String line = file.readStringUntil('\n');
            line.trim();
            if (line.length() > 0) {
                int pipe1 = line.indexOf('|');
                int pipe2 = line.indexOf('|', pipe1 + 1);
                if (pipe1 > 0 && pipe2 > pipe1) {
                    output[outputCount].isUser = (line.substring(0, pipe1) == "1");
                    output[outputCount].text = line.substring(pipe1 + 1, pipe2);
                    output[outputCount].timestamp = line.substring(pipe2 + 1).toInt();
                    outputCount++;
                }
            }
        }
        return;
    }
    
    // Read messages using cache
    for (int i = startIndex; i < startIndex + messagesToRead && outputCount < messagesToRead; i++) {
        ChatMessage msg = readMessageAtLine(file, i);
        if (msg.text.length() > 0) {
            output[outputCount] = msg;
            outputCount++;
        }
    }
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
    
    // Invalidate position cache when file is modified
    invalidatePositionCache();
}

void ChatScreen::loadMessagesFromFile() {
    String fileName = getChatHistoryFileName();
    
    // Kiểm tra file có tồn tại không
    if (!SPIFFS.exists(fileName)) {
        Serial.print("Chat: No history file found: ");
        Serial.println(fileName);
        totalMessagesInFile = 0;
        loadedMessageCount = 0;
        hasMoreMessages = false;
        invalidatePositionCache();
        return;
    }
    
    // Mở file để đọc
    File file = SPIFFS.open(fileName, "r");
    if (!file) {
        Serial.print("Chat: Failed to open file for reading: ");
        Serial.println(fileName);
        totalMessagesInFile = 0;
        loadedMessageCount = 0;
        hasMoreMessages = false;
        invalidatePositionCache();
        return;
    }
    
    // Đọc số lượng tin nhắn
    String countStr = file.readStringUntil('\n');
    totalMessagesInFile = countStr.toInt();
    
    if (totalMessagesInFile > MAX_MESSAGES) {
        totalMessagesInFile = MAX_MESSAGES;
    }
    
    if (totalMessagesInFile == 0) {
        file.close();
        messageCount = 0;
        loadedMessageCount = 0;
        hasMoreMessages = false;
        fileReadPosition = 0;
        invalidatePositionCache();
        return;
    }
    
    // OPTIMIZED LOADING: Chỉ load 1 tin nhắn cuối cùng khi khởi động
    // Load nhanh để hiển thị ngay, sau đó load ngầm khi user scroll up
    const int INITIAL_LOAD_COUNT = 1;
    int messagesToLoad = (totalMessagesInFile < INITIAL_LOAD_COUNT) ? totalMessagesInFile : INITIAL_LOAD_COUNT;
    
    // Build position cache for efficient seeking
    buildFilePositionCache();
    
    // Read last N messages using efficient method
    ChatMessage loadedMsgs[50];
    int loadedCount = 0;
    readLastNMessages(file, messagesToLoad, loadedMsgs, loadedCount);
    
    file.close();
    
    // Copy loaded messages to messages array
    messageCount = 0;
    for (int i = 0; i < loadedCount && messageCount < MAX_MESSAGES; i++) {
        messages[messageCount] = loadedMsgs[i];
        messageCount++;
    }
    
    // Update lazy loading state
    loadedMessageCount = messageCount;
    fileReadPosition = totalMessagesInFile - loadedCount;  // Vị trí bắt đầu đã đọc
    hasMoreMessages = (fileReadPosition > 0);  // Còn tin nhắn cũ hơn để load
    
    Serial.print("Chat: Initial load - ");
    Serial.print(messageCount);
    Serial.print(" message(s) of ");
    Serial.print(totalMessagesInFile);
    Serial.print(" total. More will load on scroll. File: ");
    Serial.println(fileName);
    
    // Vẽ lại tin nhắn sau khi load (via flag system, not direct call)
    if (messageCount > 0) {
        recalculateLayout();
        scrollToLatest();
        needsMessagesRedraw = true;
        // drawMessages() will be called via flag system
    }
}

bool ChatScreen::loadMoreMessages(int count) {
    if (!hasMoreMessages || fileReadPosition <= 0 || isLoadingMessages) {
        return false;  // Không còn tin nhắn để load hoặc đang load
    }
    
    // FACEBOOK-STYLE: Maintain scroll position - tính số dòng trước khi load
    int linesBeforeLoad = calculateTotalLines();
    
    String fileName = getChatHistoryFileName();
    if (!SPIFFS.exists(fileName)) {
        return false;
    }
    
    // Build cache if needed (before opening file for reading)
    if (!positionCacheValid) {
        buildFilePositionCache();
    }
    
    File file = SPIFFS.open(fileName, "r");
    if (!file) {
        return false;
    }
    
    // Bỏ qua dòng đầu (total count)
    file.readStringUntil('\n');
    
    // Tính số tin nhắn cần load
    int messagesToLoad = count;
    if (fileReadPosition < messagesToLoad) {
        messagesToLoad = fileReadPosition;  // Chỉ load những gì còn lại
    }
    
    // Tính vị trí bắt đầu load (từ fileReadPosition - messagesToLoad)
    int startIndex = fileReadPosition - messagesToLoad;
    
    // FACEBOOK-STYLE: Efficient reading - sử dụng cache và helper method
    ChatMessage newMessages[50];
    int newMessageCount = 0;
    readMessagesFromPosition(file, startIndex, messagesToLoad, newMessages, newMessageCount);
    
    file.close();
    
    if (newMessageCount == 0) {
        hasMoreMessages = false;
        return false;
    }
    
    // Kiểm tra nếu thêm vào sẽ vượt quá MAX_MESSAGES
    if (messageCount + newMessageCount > MAX_MESSAGES) {
        // Xóa tin nhắn cũ nhất (ở cuối mảng) để nhường chỗ
        int excess = (messageCount + newMessageCount) - MAX_MESSAGES;
        for (int i = 0; i < messageCount - excess; i++) {
            messages[i] = messages[i + excess];
        }
        messageCount = messageCount - excess;
    }
    
    // Dịch chuyển tin nhắn hiện tại để nhường chỗ cho tin nhắn cũ hơn
    for (int i = messageCount - 1; i >= 0; i--) {
        messages[i + newMessageCount] = messages[i];
    }
    
    // Chèn tin nhắn mới vào đầu (tin nhắn cũ nhất ở index 0)
    for (int i = 0; i < newMessageCount; i++) {
        messages[i] = newMessages[i];
    }
    
    messageCount += newMessageCount;
    loadedMessageCount += newMessageCount;
    fileReadPosition = startIndex;
    hasMoreMessages = (fileReadPosition > 0);
    
    // FACEBOOK-STYLE: Maintain scroll position - tính số dòng sau khi load
    int linesAfterLoad = calculateTotalLines();
    int addedLines = linesAfterLoad - linesBeforeLoad;
    
    // Adjust scrollOffset để giữ nguyên vị trí hiển thị
    scrollOffset += addedLines;
    clampScrollOffset();
    
    Serial.print("Chat: Efficiently loaded ");
    Serial.print(newMessageCount);
    Serial.print(" more messages. Total loaded: ");
    Serial.print(loadedMessageCount);
    Serial.print("/");
    Serial.print(totalMessagesInFile);
    Serial.print(" (adjusted scroll by ");
    Serial.print(addedLines);
    Serial.println(" lines)");
    
    // Đánh dấu cần vẽ lại (KHÔNG gọi drawMessages() trực tiếp)
    // drawMessages() sẽ được gọi tự động trong draw() hoặc loop()
    needsMessagesRedraw = true;
    
    return true;
}

void ChatScreen::setFriendStatus(uint8_t status) {
    friendStatus = status;
    // Chỉ cần vẽ lại title bar để cập nhật dot
    drawTitle();
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
    // Chỉ update animation nếu có decor nào đó được bật
    bool hasActiveDecor = showTitleBarGradient || showChatAreaPattern || 
                          showInputBoxGlow || showMessageBubbles || 
                          showScrollbarGlow || (friendStatus == 2); // typing status dot
    
    if (!hasActiveDecor) return; // Không cần update nếu không có decor
    
    // Throttle: chỉ update mỗi 50ms để giảm tần suất vẽ lại
    unsigned long now = millis();
    if (now - lastAnimationUpdate < 50) return;
    lastAnimationUpdate = now;
    
    // Cập nhật frame animation cho decor
    decorAnimationFrame++;
    if (decorAnimationFrame > 1000) decorAnimationFrame = 0;
    
    // Đánh dấu cần vẽ lại nếu decor đang hiển thị
    if (hasActiveDecor) {
        needsRedraw = true;
    }
}

