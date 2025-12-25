#ifndef CHAT_SCREEN_H
#define CHAT_SCREEN_H

#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include "keyboard.h"
#include "confirmation_dialog.h"
#include <FS.h>
#include <SPIFFS.h>

// Callback type for exiting chat screen
typedef void (*ExitCallback)();

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
    
    // Title Bar button state (inline with title)
    uint8_t titleBarFocus;  // 0 = none, 1 = Invite, 2 = Unfriend
    
    // Confirmation Dialog (reusable)
    ConfirmationDialog* confirmationDialog;
    uint8_t pendingDialogAction;  // 0 = none, 1 = unfriend
    
    // Exit callback for navigation
    ExitCallback onExitCallback;
    
    // Danh sách tin nhắn
    static const int MAX_MESSAGES = 50;
    ChatMessage messages[MAX_MESSAGES];
    int messageCount;
    int scrollOffset;  // Số dòng đã cuộn (scroll theo dòng, không phải theo tin nhắn)
    
    // Lazy loading tracking
    int loadedMessageCount;      // Số tin nhắn đã load từ file
    int totalMessagesInFile;     // Tổng số tin nhắn trong file
    bool hasMoreMessages;        // Còn tin nhắn để load không
    int fileReadPosition;        // Vị trí đọc file (số dòng đã đọc từ đầu file)
    
    // Loading state flags to prevent race conditions
    bool isLoadingMessages;      // Flag để prevent concurrent loading
    unsigned long lastLoadTime;  // Thời gian load cuối cùng (để debounce)
    bool showLoadingIndicator;   // Flag để hiển thị loading indicator khi đang load
    
    // File position cache for efficient seeking (Facebook-style)
    size_t* fileLinePositions;   // Cache byte positions of each line in file
    int cachedLineCount;         // Number of cached positions
    bool positionCacheValid;     // Whether cache is up to date
    
    // Tin nhắn đang nhập
    String currentMessage;
    int inputCursorPos;  // Vị trí con trỏ trong currentMessage (theo ký tự)
    
    // Trạng thái bàn phím
    bool keyboardVisible;
    
    // Nickname
    String ownerNickname;   // Tên của chủ sở hữu (người dùng)
    String friendNickname;  // Tên của người bạn (người khác)
    uint8_t friendStatus;   // Trạng thái bạn (offline/online/typing)
    
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
    
    // Dirty flags for conditional rendering optimization
    bool needsRedraw;               // Indicates full screen needs redraw
    bool needsMessagesRedraw;        // Indicates messages area needs redraw
    bool needsInputRedraw;          // Indicates input box needs redraw
    unsigned long lastAnimationUpdate;  // Timestamp of last animation update
    
    // Vẽ tiêu đề
    void drawTitle();
    
    // Helper to draw icon buttons in title bar with text labels
    void drawTitleBarButton(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color, bool focused, char icon, const String& label);
    
    // Confirmation Dialog is handled by ConfirmationDialog class
    
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
    uint16_t getStatusDotColor() const;   // Màu dot theo trạng thái
    bool isStatusDotVisible() const;      // Ẩn/hiện dot (typing blink)
    
    // Vẽ tin nhắn
    void drawMessages();
    
    // Vẽ ô nhập tin nhắn
    void drawInputBox();
    
    // Vẽ tin nhắn đang nhập
    void drawCurrentMessage();
    void drawIconInline(uint16_t x, uint16_t y, uint16_t size, uint16_t color, uint16_t bgColor, char iconCode);  // Vẽ icon trong text/input
    bool containsIcon(const String& text) const;  // Kiểm tra tin nhắn có chứa icon hay không
    
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
    void loadMessagesFromFile();       // Tải tin nhắn từ file (lazy load: chỉ 5 tin nhắn đầu)
    bool loadMoreMessages(int count = 5);  // Load thêm tin nhắn cũ hơn khi scroll lên
    
    // File reading helpers (Facebook-style efficient reading)
    void buildFilePositionCache();     // Build cache of file line positions
    void invalidatePositionCache();    // Mark cache as invalid
    ChatMessage readMessageAtLine(File& file, int lineIndex);  // Read message at specific line using cache
    void readLastNMessages(File& file, int n, ChatMessage* output, int& count);  // Read last N messages efficiently
    void readMessagesFromPosition(File& file, int startIndex, int count, ChatMessage* output, int& outputCount);  // Read messages from position
    int calculateLinesForMessages(const ChatMessage* msgs, int msgCount) const;  // Calculate total lines for messages

public:
    // Constructor
    ChatScreen(Adafruit_ST7789* tft, Keyboard* keyboard);
    
    // Destructor
    ~ChatScreen();
    
    // Vẽ toàn bộ màn hình
    void draw();
    
    // Force a full screen redraw (clears screen and redraws everything)
    // Use this when switching to Chat Screen from another screen
    void forceRedraw();
    
    // Xử lý khi nhấn phím
    void handleKeyPress(String key);
    
    // Thêm tin nhắn
    void addMessage(String text, bool isUser = true);
    
    // Gửi tin nhắn hiện tại
    void sendMessage();
    
    // Getter/Setter
    String getCurrentMessage() const { return currentMessage; }
    void setCurrentMessage(String msg) { currentMessage = msg; inputCursorPos = currentMessage.length(); }
    void clearCurrentMessage() { currentMessage = ""; inputCursorPos = 0; }
    
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
    void handleLeft();
    void handleRight();
    void handleSelect();  // Toggle keyboard visibility or trigger action
    
    // Title Bar button navigation
    bool isTitleBarFocused() const { return titleBarFocus > 0; }
    uint8_t getTitleBarFocus() const { return titleBarFocus; }
    void setTitleBarFocus(uint8_t focus) { titleBarFocus = focus; }
    
    // Callback methods for ConfirmationDialog (static wrappers)
    static void staticOnUnfriendConfirm();
    static void staticOnUnfriendCancel();
    
    // Instance methods (called by static wrappers)
    void onUnfriendConfirm();
    void onUnfriendCancel();
    
    // Static instance pointer for callbacks
    static ChatScreen* instanceForCallback;
    
    // Callback for exiting screen after unfriend
    void setOnExitCallback(ExitCallback callback);
    
    // Xóa tất cả tin nhắn
    void clearMessages();
    
    // Getter cho keyboard visibility
    bool isKeyboardVisible() const { return keyboardVisible; }
    
    // Getter/Setter cho nickname
    String getOwnerNickname() const { return ownerNickname; }
    void setOwnerNickname(String nickname) { ownerNickname = nickname; }
    String getFriendNickname() const { return friendNickname; }
    void setFriendNickname(String nickname) { friendNickname = nickname; }
    // Status: 0 = offline, 1 = online, 2 = typing
    void setFriendStatus(uint8_t status);
    uint8_t getFriendStatus() const { return friendStatus; }
    
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

