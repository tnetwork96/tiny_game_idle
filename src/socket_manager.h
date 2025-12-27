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
    
    // Callback handlers
    void onWebSocketEvent(WStype_t type, uint8_t * payload, size_t length);
    static void webSocketEvent(WStype_t type, uint8_t * payload, size_t length);
    static SocketManager* instance;
    
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
};

#endif

