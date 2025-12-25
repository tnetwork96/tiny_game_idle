#include "websocket_client.h"

// Static instance pointer for WebSocket callbacks
WebSocketClient* WebSocketClient::instance = nullptr;

// MessageData implementation
String MessageData::getValue(const String& key) const {
    for (int i = 0; i < fieldCount; i++) {
        if (keys[i] == key) {
            return values[i];
        }
    }
    return "";
}

void MessageData::setValue(const String& key, const String& value) {
    // Check if key already exists
    for (int i = 0; i < fieldCount; i++) {
        if (keys[i] == key) {
            values[i] = value;
            return;
        }
    }
    
    // Add new key-value pair
    if (fieldCount < MAX_FIELDS) {
        keys[fieldCount] = key;
        values[fieldCount] = value;
        fieldCount++;
    }
}

// Parse string pattern message: "key1:value1-*-key2:value2-*-key3:value3"
MessageData parseMessage(const String& message) {
    MessageData data;
    
    int startIndex = 0;
    int separatorIndex = message.indexOf("-*-");
    
    while (separatorIndex != -1 || startIndex < message.length()) {
        String pair;
        if (separatorIndex == -1) {
            pair = message.substring(startIndex);
            startIndex = message.length();
        } else {
            pair = message.substring(startIndex, separatorIndex);
            startIndex = separatorIndex + 3;  // Skip "-*-"
            separatorIndex = message.indexOf("-*-", startIndex);
        }
        
        // Parse key:value
        int colonIndex = pair.indexOf(":");
        if (colonIndex != -1) {
            String key = pair.substring(0, colonIndex);
            String value = pair.substring(colonIndex + 1);
            
            if (key == "type") {
                data.type = value;
            } else {
                data.setValue(key, value);
            }
        }
    }
    
    return data;
}

// Get value from MessageData with default
String getValue(const MessageData& data, const String& key, const String& defaultValue) {
    String value = data.getValue(key);
    if (value.length() > 0) {
        return value;
    }
    return defaultValue;
}

WebSocketClient::WebSocketClient() {
    authenticated = false;
    sessionId = "";
    lastPingTime = 0;
    pingInterval = 30000;  // 30 seconds default
    reconnectDelay = 5000;  // 5 seconds default
    lastReconnectAttempt = 0;
    reconnectAttempts = 0;
    
    // Initialize callbacks to null
    onConnectCallback = nullptr;
    onDisconnectCallback = nullptr;
    onMessageCallback = nullptr;
    onErrorCallback = nullptr;
    onAuthSuccessCallback = nullptr;
    onAuthFailedCallback = nullptr;
    onUsernameValidCallback = nullptr;
    onUsernameInvalidCallback = nullptr;
    
    // Set static instance for callback
    instance = this;
}

WebSocketClient::~WebSocketClient() {
    disconnect();
    instance = nullptr;
}

void WebSocketClient::begin(const char* host, int port, const char* path) {
    serverHost = String(host);
    serverPort = port;
    serverPath = String(path);
    
    // Initialize WebSocket client
    webSocket.begin(host, port, path);
    webSocket.onEvent(staticWebSocketEvent);
    
    Serial.print("WebSocket: Initialized for ");
    Serial.print(host);
    Serial.print(":");
    Serial.print(port);
    Serial.print(path);
    Serial.println();
}

void WebSocketClient::connect() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WebSocket: WiFi not connected, cannot connect to server");
        if (onErrorCallback != nullptr) {
            onErrorCallback("WiFi not connected");
        }
        return;
    }
    
    Serial.println("WebSocket: Attempting to connect...");
    webSocket.begin(serverHost.c_str(), serverPort, serverPath.c_str());
    webSocket.onEvent(staticWebSocketEvent);
    reconnectAttempts = 0;
}

void WebSocketClient::disconnect() {
    authenticated = false;
    sessionId = "";
    webSocket.disconnect();
    Serial.println("WebSocket: Disconnected");
}

bool WebSocketClient::isConnected() {
    return webSocket.isConnected();
}

bool WebSocketClient::isAuthenticated() const {
    return authenticated && sessionId.length() > 0;
}

String WebSocketClient::getSessionId() const {
    return sessionId;
}

void WebSocketClient::sendLogin(const String& credentials) {
    if (!isConnected()) {
        Serial.println("WebSocket: Not connected, cannot send login");
        if (onErrorCallback != nullptr) {
            onErrorCallback("Not connected");
        }
        return;
    }
    
    // Parse credentials: "username:player1-pin:4321"
    String username = "";
    String pin = "";
    
    int pinIndex = credentials.indexOf("-pin:");
    if (pinIndex != -1) {
        // Extract username: "username:player1" -> "player1"
        String usernamePart = credentials.substring(0, pinIndex);
        int usernameColonIndex = usernamePart.indexOf(":");
        if (usernameColonIndex != -1) {
            username = usernamePart.substring(usernameColonIndex + 1);
        }
        
        // Extract pin: "-pin:4321" -> "4321"
        String pinPart = credentials.substring(pinIndex + 5); // Skip "-pin:"
        pin = pinPart;
    } else {
        // Try alternative format: just "username:player1" (no pin)
        int usernameColonIndex = credentials.indexOf(":");
        if (usernameColonIndex != -1) {
            username = credentials.substring(usernameColonIndex + 1);
        }
    }
    
    // Build message in correct string pattern format: type:login-*-username:xxx-*-pin:yyy
    String loginMsg = "type:login-*-username:";
    loginMsg += username;
    loginMsg += "-*-pin:";
    loginMsg += pin;
    
    Serial.print("WebSocket: Parsing credentials: username=");
    Serial.print(username);
    Serial.print(", pin=");
    Serial.println(pin);
    Serial.print("WebSocket: Sending login: ");
    Serial.println(loginMsg);
    
    webSocket.sendTXT(loginMsg);
}

void WebSocketClient::sendUsernameValidation(const String& username) {
    if (!isConnected()) {
        Serial.println("WebSocket: Not connected, cannot validate username");
        if (onErrorCallback != nullptr) {
            onErrorCallback("Not connected");
        }
        return;
    }
    
    String validationMsg = "type:validate_username-*-username:";
    validationMsg += username;
    
    Serial.print("WebSocket: Sending username validation: ");
    Serial.println(validationMsg);
    
    String msg = validationMsg;
    webSocket.sendTXT(msg);
}

void WebSocketClient::sendMessage(const String& message) {
    if (!isConnected()) {
        Serial.println("WebSocket: Not connected, cannot send message");
        return;
    }
    
    // Create non-const copy for sendTXT which requires non-const reference
    String msg = message;
    webSocket.sendTXT(msg);
}

void WebSocketClient::sendStringMessage(const String& type, const String& data) {
    if (!isConnected()) {
        return;
    }
    
    String message = "type:";
    message += type;
    if (data.length() > 0) {
        message += "-*-data:";
        message += data;
    }
    
    // message is already non-const, can pass directly
    webSocket.sendTXT(message);
}

void WebSocketClient::loop() {
    webSocket.loop();
    
    // Send ping periodically if connected and authenticated
    if (isConnected() && isAuthenticated()) {
        unsigned long now = millis();
        if (now - lastPingTime >= pingInterval) {
            sendPing();
            lastPingTime = now;
        }
    }
    
    // Attempt reconnection if disconnected
    if (!isConnected() && WiFi.status() == WL_CONNECTED) {
        unsigned long now = millis();
        if (reconnectAttempts < MAX_RECONNECT_ATTEMPTS) {
            if (now - lastReconnectAttempt >= reconnectDelay) {
                attemptReconnect();
                lastReconnectAttempt = now;
            }
        }
    }
}

void WebSocketClient::onConnect(void (*callback)()) {
    onConnectCallback = callback;
}

void WebSocketClient::onDisconnect(void (*callback)()) {
    onDisconnectCallback = callback;
}

void WebSocketClient::onMessage(void (*callback)(String)) {
    onMessageCallback = callback;
}

void WebSocketClient::onError(void (*callback)(String)) {
    onErrorCallback = callback;
}

void WebSocketClient::onAuthSuccess(void (*callback)(String sessionId)) {
    onAuthSuccessCallback = callback;
}

void WebSocketClient::onAuthFailed(void (*callback)()) {
    onAuthFailedCallback = callback;
}

void WebSocketClient::onUsernameValid(void (*callback)(String message)) {
    onUsernameValidCallback = callback;
}

void WebSocketClient::onUsernameInvalid(void (*callback)(String errorMessage)) {
    onUsernameInvalidCallback = callback;
}

void WebSocketClient::setPingInterval(unsigned long interval) {
    pingInterval = interval;
}

void WebSocketClient::setReconnectDelay(unsigned long delay) {
    reconnectDelay = delay;
}

// Static callback wrapper
void WebSocketClient::staticWebSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
    if (instance != nullptr) {
        instance->webSocketEvent(type, payload, length);
    }
}

// WebSocket event handler
void WebSocketClient::webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
    switch(type) {
        case WStype_DISCONNECTED:
            Serial.println("WebSocket: Disconnected");
            authenticated = false;
            sessionId = "";
            reconnectAttempts++;
            if (onDisconnectCallback != nullptr) {
                onDisconnectCallback();
            }
            break;
            
        case WStype_CONNECTED:
            Serial.print("WebSocket: Connected to ");
            Serial.println((char*)payload);
            authenticated = false;  // Not authenticated yet
            reconnectAttempts = 0;  // Reset reconnect attempts on successful connection
            if (onConnectCallback != nullptr) {
                onConnectCallback();
            }
            break;
            
        case WStype_TEXT:
            {
                String message = String((char*)payload);
                Serial.print("WebSocket: Received: ");
                Serial.println(message);
                handleMessage(message);
            }
            break;
            
        case WStype_ERROR:
            Serial.print("WebSocket: Error: ");
            Serial.println((char*)payload);
            if (onErrorCallback != nullptr) {
                onErrorCallback(String((char*)payload));
            }
            break;
            
        default:
            break;
    }
}

// Handle incoming messages
void WebSocketClient::handleMessage(String message) {
    // Parse string pattern message
    MessageData msg = parseMessage(message);
    
    if (msg.type.length() == 0) {
        Serial.println("WebSocket: Invalid message format (no type field)");
        // Still call message callback for raw message
        if (onMessageCallback != nullptr) {
            onMessageCallback(message);
        }
        return;
    }
    
    if (msg.type == "login_success") {
        // Authentication successful
        authenticated = true;
        sessionId = getValue(msg, "session_id", "");
        Serial.print("WebSocket: Authentication successful, session: ");
        Serial.println(sessionId);
        
        if (onAuthSuccessCallback != nullptr) {
            onAuthSuccessCallback(sessionId);
        }
    }
    else if (msg.type == "error") {
        String errorMsg = getValue(msg, "message", "Unknown error");
        Serial.print("WebSocket: Server error: ");
        Serial.println(errorMsg);
        
        // Check if it's a username validation error (before login)
        if (errorMsg.indexOf("Username not found") >= 0 || errorMsg.indexOf("Account inactive") >= 0) {
            // This might be a username validation error
            if (onUsernameInvalidCallback != nullptr) {
                onUsernameInvalidCallback(errorMsg);
            }
        }
        
        // Check if it's an authentication error (during login)
        if (errorMsg.indexOf("credential") >= 0 || errorMsg.indexOf("Invalid PIN") >= 0) {
            authenticated = false;
            if (onAuthFailedCallback != nullptr) {
                onAuthFailedCallback();
            }
        }
        
        if (onErrorCallback != nullptr) {
            onErrorCallback(errorMsg);
        }
    }
    else if (msg.type == "username_valid") {
        // Username validation successful
        String message_text = getValue(msg, "message", "Username exists");
        Serial.print("WebSocket: Username valid: ");
        Serial.println(message_text);
        
        if (onUsernameValidCallback != nullptr) {
            onUsernameValidCallback(message_text);
        }
        
        // Also call generic message callback
        if (onMessageCallback != nullptr) {
            onMessageCallback(message);
        }
    }
    else if (msg.type == "pong") {
        // Pong response to ping - connection is alive
        // No action needed
    }
    else {
        // Other message types - forward to message callback
        if (onMessageCallback != nullptr) {
            onMessageCallback(message);
        }
    }
}

void WebSocketClient::sendPing() {
    if (!isConnected()) {
        return;
    }
    String pingMsg = "type:ping";
    webSocket.sendTXT(pingMsg);
}

void WebSocketClient::attemptReconnect() {
    Serial.print("WebSocket: Attempting reconnect (");
    Serial.print(reconnectAttempts + 1);
    Serial.print("/");
    Serial.print(MAX_RECONNECT_ATTEMPTS);
    Serial.println(")");
    
    connect();
}

