#ifndef SOCKET_MANAGER_H
#define SOCKET_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebSocketsClient.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

// Typing indicator constants
#define TYPING_AUTO_STOP_INTERVAL 3000  // 3 seconds

// Forward declaration
class ChatScreen;
class SocialScreen;

class SocketManager {
private:
    WebSocketsClient webSocket;
    String serverHost;
    uint16_t serverPort;
    String serverPath;
    bool isConnected;
    unsigned long lastReconnectAttempt;
    unsigned long reconnectInterval;
    bool initialized;
    
    // FreeRTOS task for WebSocket connection
    TaskHandle_t socketTaskHandle;
    bool taskRunning;
    
    // Keep-alive variables
    unsigned long lastPingTime;
    unsigned long pingInterval;  // Send ping every 30 seconds
    
    // Typing indicator state
    unsigned long lastTypingTime;
    int currentTypingToUserId;
    
    // User ID for init message
    int userId;
    
    // ChatScreen pointer and state (để xử lý message display)
    ChatScreen* chatScreen;
    bool isChatScreenActive;
    int currentChatFriendUserId;
    
    // SocialScreen pointer and state (để xử lý notification và badge)
    SocialScreen* socialScreen;
    bool isSocialScreenActive;
    
    // Semaphore for thread-safe file access
    SemaphoreHandle_t fileMutex;
    
    // Notification callback
    typedef void (*OnNotificationCallback)(int id, const String& type, const String& message, const String& timestamp, bool read);
    OnNotificationCallback onNotificationCallback;
    
    // Chat message callback
    typedef void (*OnChatMessageCallback)(int fromUserId, const String& fromNickname, const String& message, const String& messageId, const String& timestamp);
    OnChatMessageCallback onChatMessageCallback;
    
    // Chat message display callback - returns true if message was displayed in active chat
    typedef bool (*OnChatMessageDisplayCallback)(int fromUserId, const String& message);
    OnChatMessageDisplayCallback onChatMessageDisplayCallback;
    
    // Chat badge callback - xử lý badge khi message chưa được hiển thị
    typedef void (*OnChatBadgeCallback)(int fromUserId, const String& fromNickname);
    OnChatBadgeCallback onChatBadgeCallback;
    
    // Typing indicator callback
    typedef void (*OnTypingIndicatorCallback)(int fromUserId, const String& fromNickname, bool isTyping);
    OnTypingIndicatorCallback onTypingIndicatorCallback;
    
    // Delivery status callback
    typedef void (*OnDeliveryStatusCallback)(const String& messageId, const String& status, const String& timestamp);
    OnDeliveryStatusCallback onDeliveryStatusCallback;
    
    // Read receipt callback
    typedef void (*OnReadReceiptCallback)(const String& messageId, const String& timestamp);
    OnReadReceiptCallback onReadReceiptCallback;
    
    // User status update callback
    typedef void (*OnUserStatusUpdateCallback)(int userId, const String& status);
    OnUserStatusUpdateCallback onUserStatusUpdateCallback;

    // Game event callback
    typedef void (*OnGameEventCallback)(const String& eventType, int sessionId, const String& gameType, const String& status, int userId, bool accepted, bool ready, const String& userNickname);
    OnGameEventCallback onGameEventCallback;
    
    // Game move callback
    typedef void (*OnGameMoveCallback)(int sessionId, int userId, int row, int col, const String& gameStatus, int winnerId, int currentTurn);
    OnGameMoveCallback onGameMoveCallback;
    
    // Helper to parse JSON chat message
    void parseChatMessage(const String& message);
    void parseTypingIndicator(const String& message, bool isTyping);
    void parseDeliveryStatus(const String& message, const String& status);
    void parseReadReceipt(const String& message);
    void parseUserStatusUpdate(const String& message);
    void parseGameEvent(const String& message);
    
    // Helper to save chat message to file
    void saveChatMessageToFile(int fromUserId, int toUserId, const String& message, bool isFromUser);
    
    // Callback handlers
    void onWebSocketEvent(WStype_t type, uint8_t * payload, size_t length);
    static void webSocketEvent(WStype_t type, uint8_t * payload, size_t length);
    static SocketManager* instance;
    
    // Helper to parse JSON notification
    void parseNotificationMessage(const String& message);
    
    // FreeRTOS task function
    static void socketTask(void* parameter);
    void runSocketTask();  // Actual task implementation
    
public:
    SocketManager();
    ~SocketManager();
    
    // Initialize socket connection
    void begin(const String& host, uint16_t port, const String& path = "/");
    
    // Update socket (call in loop) - now mainly for status logging
    void update();
    
    // Send message
    void sendMessage(const String& message);
    
    // Check connection status
    bool connected() const { return isConnected; }
    
    // Check if initialized
    bool isInitialized() const { return initialized; }
    
    // Disconnect
    void disconnect();
    
    // Get instance (for static callback)
    static SocketManager* getInstance() { return instance; }
    
    // Set notification callback
    void setOnNotificationCallback(OnNotificationCallback callback) {
        onNotificationCallback = callback;
    }
    
    // Set chat message callback
    void setOnChatMessageCallback(OnChatMessageCallback callback) {
        onChatMessageCallback = callback;
    }
    
    // Set chat message display callback
    void setOnChatMessageDisplayCallback(OnChatMessageDisplayCallback callback) {
        onChatMessageDisplayCallback = callback;
    }
    
    // Set chat badge callback
    void setOnChatBadgeCallback(OnChatBadgeCallback callback) {
        onChatBadgeCallback = callback;
    }
    
    // Set typing indicator callback
    void setOnTypingIndicatorCallback(OnTypingIndicatorCallback callback) {
        onTypingIndicatorCallback = callback;
    }
    
    // Set delivery status callback
    void setOnDeliveryStatusCallback(OnDeliveryStatusCallback callback) {
        onDeliveryStatusCallback = callback;
    }
    
    // Set read receipt callback
    void setOnReadReceiptCallback(OnReadReceiptCallback callback) {
        onReadReceiptCallback = callback;
    }
    
    // Set user status update callback
    void setOnUserStatusUpdateCallback(OnUserStatusUpdateCallback callback) {
        onUserStatusUpdateCallback = callback;
    }
    // Set game event callback
    void setOnGameEventCallback(OnGameEventCallback callback) {
        onGameEventCallback = callback;
    }
    
    // Set game move callback
    void setOnGameMoveCallback(OnGameMoveCallback callback) {
        onGameMoveCallback = callback;
    }
    
    // Send chat message
    void sendChatMessage(int toUserId, const String& message, const String& messageId = "");
    
    // Send typing indicator
    void sendTypingStart(int toUserId);
    void sendTypingStop(int toUserId);
    
    // Send read receipt
    void sendReadReceipt(int toUserId, const String& messageId);
    
    // Set user ID for init message (call before or after begin)
    void setUserId(int userId) {
        this->userId = userId;
        // If already connected, send updated init message
        if (isConnected && initialized) {
            String initMessage = "{\"type\":\"init\",\"device\":\"ESP32\",\"user_id\":";
            initMessage += String(userId);
            initMessage += "}";
            String msgCopy = initMessage;
            webSocket.sendTXT(msgCopy);
        }
    }
    
    // Set ChatScreen state (để xử lý message display)
    // Tự động set SocketManager cho ChatScreen để ChatScreen có thể gửi messages
    void setChatScreen(ChatScreen* chatScreen);
    
    void setChatScreenActive(bool active) {
        this->isChatScreenActive = active;
    }
    
    void setCurrentChatFriendUserId(int friendUserId) {
        this->currentChatFriendUserId = friendUserId;
    }
    
    // Set SocialScreen state (để xử lý notification và badge)
    void setSocialScreen(SocialScreen* socialScreen) {
        this->socialScreen = socialScreen;
    }
    
    void setSocialScreenActive(bool active) {
        this->isSocialScreenActive = active;
    }
};

#endif

