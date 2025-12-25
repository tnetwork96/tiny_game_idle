#ifndef WEBSOCKET_CLIENT_H
#define WEBSOCKET_CLIENT_H

#include <Arduino.h>
#include <WebSocketsClient.h>
#include <WiFi.h>

// Simple structure to hold parsed message data
struct MessageData {
    String type;
    String getValue(const String& key) const;
    void setValue(const String& key, const String& value);
    
private:
    static const int MAX_FIELDS = 10;
    String keys[MAX_FIELDS];
    String values[MAX_FIELDS];
    int fieldCount;
    
public:
    MessageData() : fieldCount(0) {}
};

// Parse string pattern message into MessageData
MessageData parseMessage(const String& message);

// Get value from MessageData with default
String getValue(const MessageData& data, const String& key, const String& defaultValue = "");

class WebSocketClient {
private:
    WebSocketsClient webSocket;
    String serverHost;
    int serverPort;
    String serverPath;
    bool authenticated;
    String sessionId;
    unsigned long lastPingTime;
    unsigned long pingInterval;  // milliseconds
    unsigned long reconnectDelay;
    unsigned long lastReconnectAttempt;
    int reconnectAttempts;
    static const int MAX_RECONNECT_ATTEMPTS = 5;
    
    // Callback function pointers
    void (*onConnectCallback)();
    void (*onDisconnectCallback)();
    void (*onMessageCallback)(String);
    void (*onErrorCallback)(String);
    void (*onAuthSuccessCallback)(String sessionId);
    void (*onAuthFailedCallback)();
    void (*onUsernameValidCallback)(String message);
    void (*onUsernameInvalidCallback)(String errorMessage);
    
    // Internal WebSocket event handlers
    void webSocketEvent(WStype_t type, uint8_t * payload, size_t length);
    static void staticWebSocketEvent(WStype_t type, uint8_t * payload, size_t length);
    static WebSocketClient* instance;
    
    void handleMessage(String message);
    void sendPing();
    void attemptReconnect();

public:
    WebSocketClient();
    ~WebSocketClient();
    
    // Initialization
    void begin(const char* host, int port, const char* path = "/ws/esp32");
    
    // Connection management
    void connect();
    void disconnect();
    bool isConnected();
    bool isAuthenticated() const;
    String getSessionId() const;
    
    // Authentication
    void sendLogin(const String& credentials);
    void sendUsernameValidation(const String& username);
    
    // Message sending
    void sendMessage(const String& message);
    void sendStringMessage(const String& type, const String& data = "");
    
    // Main loop - call this in Arduino loop()
    void loop();
    
    // Callback setters
    void onConnect(void (*callback)());
    void onDisconnect(void (*callback)());
    void onMessage(void (*callback)(String));
    void onError(void (*callback)(String));
    void onAuthSuccess(void (*callback)(String sessionId));
    void onAuthFailed(void (*callback)());
    void onUsernameValid(void (*callback)(String message));
    void onUsernameInvalid(void (*callback)(String errorMessage));
    
    // Configuration
    void setPingInterval(unsigned long interval);
    void setReconnectDelay(unsigned long delay);
};

#endif

