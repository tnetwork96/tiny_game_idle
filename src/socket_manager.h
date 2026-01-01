#ifndef SOCKET_MANAGER_H
#define SOCKET_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebSocketsClient.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

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
    
    // User ID for init message
    int userId;
    
    // Notification callback
    typedef void (*OnNotificationCallback)(int id, const String& type, const String& message, const String& timestamp, bool read);
    OnNotificationCallback onNotificationCallback;
    
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
            Serial.print("Socket Manager: Sent updated init message with user_id: ");
            Serial.println(userId);
        }
    }
};

#endif

