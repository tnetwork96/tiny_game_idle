#ifndef CHAT_SCREEN_H
#define CHAT_SCREEN_H

#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include "keyboard.h"
#include <FS.h>
#include <SPIFFS.h>

// Cấu trúc tin nhắn
struct ChatMessage {
    String text;
    bool isUser;  // true = tin nhắn của user, false = tin nhắn từ người khác
    unsigned long timestamp;
};

class ChatScreen {
private:
    Adafruit_ST7789* tft;
    Keyboard* keyboard;
    
    // Vị trí và kích thước
    uint16_t titleY;
    uint16_t chatAreaY;
    uint16_t chatAreaHeight;
    uint16_t chatAreaWidth;
    uint16_t inputBoxY;
    uint16_t inputBoxHeight;
    uint16_t inputBoxWidth;
    uint16_t maxMessageLength;
    
    // Danh sách tin nhắn
    static const int MAX_MESSAGES = 50;
    ChatMessage messages[MAX_MESSAGES];
    int messageCount;
    int scrollOffset;  // Số dòng đã cuộn (scroll theo dòng, không phải theo tin nhắn)
    
    // Tin nhắn đang nhập
    String currentMessage;
    
    // Trạng thái bàn phím
    bool keyboardVisible;
    
    // Nickname
    String ownerNickname;   // Tên của chủ sở hữu (người dùng)
    String friendNickname;  // Tên của người bạn (người khác)
    
    // Màu sắc
    uint16_t bgColor;
    uint16_t titleColor;
    uint16_t chatAreaBgColor;
    uint16_t chatAreaBorderColor;
    uint16_t inputBoxBgColor;
    uint16_t inputBoxBorderColor;
    uint16_t userMessageColor;
    uint16_t otherMessageColor;
    uint16_t textColor;
    
    // Decor elements (trang trí)
    bool showTitleBarGradient;      // Hiển thị gradient cho title bar
    bool showChatAreaPattern;       // Hiển thị pattern cho vùng chat
    bool showInputBoxGlow;          // Hiển thị hiệu ứng glow cho input box
    bool showMessageBubbles;        // Hiển thị bubble background cho tin nhắn
    bool showScrollbarGlow;         // Hiển thị glow cho scrollbar
    uint16_t decorPatternColor;     // Màu pattern trang trí
    uint16_t decorAccentColor;      // Màu accent trang trí
    int decorAnimationFrame;        // Frame animation cho decor
    
    // Vẽ tiêu đề
    void drawTitle();
    
    // Vẽ vùng chat
    void drawChatArea();
    
    // Vẽ thanh scrollbar
    void drawScrollbar();
    
    // Vẽ decor elements (trang trí)
    void drawTitleBarDecor();       // Vẽ decor cho title bar
    void drawChatAreaDecor();       // Vẽ decor cho vùng chat
    void drawInputBoxDecor();       // Vẽ decor cho input box
    void drawMessageBubble(int x, int y, int width, int height, uint16_t color, bool isUser);  // Vẽ bubble cho tin nhắn
    uint16_t interpolateColor(uint16_t color1, uint16_t color2, float ratio);  // Interpolate màu
    
    // Vẽ tin nhắn
    void drawMessages();
    
    // Vẽ ô nhập tin nhắn
    void drawInputBox();
    
    // Vẽ tin nhắn đang nhập
    void drawCurrentMessage();
    
    // Cuộn tin nhắn
    void scrollUp();
    void scrollDown();
    
    // Tính toán số dòng có thể hiển thị
    int getVisibleLines();
    int calculateTotalLines() const;
    void clampScrollOffset();
    void scrollToLatest();
    
    // Bố cục động (ẩn/hiện keyboard)
    uint16_t computeKeyboardHeight() const;
    void recalculateLayout();
    
    // Lưu/tải lịch sử chat
    String getChatHistoryFileName();  // Tạo tên file từ nickname
    void saveMessagesToFile();        // Lưu tin nhắn vào file
    void loadMessagesFromFile();       // Tải tin nhắn từ file

public:
    // Constructor
    ChatScreen(Adafruit_ST7789* tft, Keyboard* keyboard);
    
    // Destructor
    ~ChatScreen();
    
    // Vẽ toàn bộ màn hình
    void draw();
    
    // Xử lý khi nhấn phím
    void handleKeyPress(String key);
    
    // Thêm tin nhắn
    void addMessage(String text, bool isUser = true);
    
    // Gửi tin nhắn hiện tại
    void sendMessage();
    
    // Getter/Setter
    String getCurrentMessage() const { return currentMessage; }
    void setCurrentMessage(String msg) { currentMessage = msg; }
    void clearCurrentMessage() { currentMessage = ""; }
    
    // Setter cho màu sắc
    void setBgColor(uint16_t color) { bgColor = color; }
    void setTitleColor(uint16_t color) { titleColor = color; }
    void setChatAreaBgColor(uint16_t color) { chatAreaBgColor = color; }
    void setChatAreaBorderColor(uint16_t color) { chatAreaBorderColor = color; }
    void setInputBoxBgColor(uint16_t color) { inputBoxBgColor = color; }
    void setInputBoxBorderColor(uint16_t color) { inputBoxBorderColor = color; }
    void setUserMessageColor(uint16_t color) { userMessageColor = color; }
    void setOtherMessageColor(uint16_t color) { otherMessageColor = color; }
    void setTextColor(uint16_t color) { textColor = color; }
    
    // Navigation
    void handleUp();
    void handleDown();
    void handleSelect();  // Toggle keyboard visibility
    
    // Xóa tất cả tin nhắn
    void clearMessages();
    
    // Getter cho keyboard visibility
    bool isKeyboardVisible() const { return keyboardVisible; }
    
    // Getter/Setter cho nickname
    String getOwnerNickname() const { return ownerNickname; }
    void setOwnerNickname(String nickname) { ownerNickname = nickname; }
    String getFriendNickname() const { return friendNickname; }
    void setFriendNickname(String nickname) { friendNickname = nickname; }
    
    // Setter cho decor elements
    void setShowTitleBarGradient(bool show) { showTitleBarGradient = show; }
    void setShowChatAreaPattern(bool show) { showChatAreaPattern = show; }
    void setShowInputBoxGlow(bool show) { showInputBoxGlow = show; }
    void setShowMessageBubbles(bool show) { showMessageBubbles = show; }
    void setShowScrollbarGlow(bool show) { showScrollbarGlow = show; }
    void setDecorPatternColor(uint16_t color) { decorPatternColor = color; }
    void setDecorAccentColor(uint16_t color) { decorAccentColor = color; }
    
    // Enable/Disable tất cả decor
    void enableAllDecor();
    void disableAllDecor();
    
    // Update decor animation (gọi trong loop)
    void updateDecorAnimation();
};

#endif

