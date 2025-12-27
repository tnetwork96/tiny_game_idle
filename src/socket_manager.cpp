#include "socket_manager.h"

SocketManager* SocketManager::instance = nullptr;

SocketManager::SocketManager() {
    instance = this;
    isConnected = false;
    initialized = false;
    serverHost = "";
    serverPort = 0;
    serverPath = "/";
    lastReconnectAttempt = 0;
    reconnectInterval = 5000;  // 5 seconds
    
    // Task variables
    socketTaskHandle = NULL;
    taskRunning = false;
    
    // Keep-alive settings
    lastPingTime = 0;
    pingInterval = 30000;  // 30 seconds
}

SocketManager::~SocketManager() {
    disconnect();
    // Stop task if running
    if (taskRunning && socketTaskHandle != NULL) {
        taskRunning = false;
        vTaskDelay(pdMS_TO_TICKS(100));  // Give task time to exit
        if (socketTaskHandle != NULL) {
            vTaskDelete(socketTaskHandle);
            socketTaskHandle = NULL;
        }
    }
}

void SocketManager::begin(const String& host, uint16_t port, const String& path) {
    if (initialized) {
        Serial.println("Socket Manager: Already initialized, skipping...");
        return;
    }
    
    serverHost = host;
    serverPort = port;
    serverPath = path;
    
    Serial.println("========================================");
    Serial.println("Socket Manager: Initializing WebSocket");
    Serial.print("  Host: ");
    Serial.println(host);
    Serial.print("  Port: ");
    Serial.println(port);
    Serial.print("  Path: ");
    Serial.println(path);
    Serial.print("  Full URL: ws://");
    Serial.print(host);
    Serial.print(":");
    Serial.print(port);
    Serial.println(path);
    Serial.println("========================================");
    
    // Initialize WebSocket
    webSocket.begin(host.c_str(), port, path.c_str());
    webSocket.onEvent(webSocketEvent);
    
    // Set reconnect interval
    webSocket.setReconnectInterval(reconnectInterval);
    
    initialized = true;
    
    // Create FreeRTOS task for WebSocket connection
    if (!taskRunning) {
        taskRunning = true;
        xTaskCreate(
            socketTask,           // Task function
            "WebSocketTask",      // Task name
            4096,                 // Stack size (bytes)
            this,                 // Parameter to pass
            1,                    // Priority (0-25, higher = more priority)
            &socketTaskHandle     // Task handle
        );
        Serial.println("Socket Manager: Created WebSocket task");
    }
    
    Serial.println("Socket Manager: Initialized successfully");
    Serial.println("Socket Manager: WebSocket task running...");
}

// FreeRTOS task function (static wrapper)
void SocketManager::socketTask(void* parameter) {
    SocketManager* manager = (SocketManager*)parameter;
    if (manager) {
        manager->runSocketTask();
    }
    vTaskDelete(NULL);  // Delete task when done
}

// Actual task implementation - runs in separate thread with while(true)
// Chạy liên tục sau khi begin(), không cần gọi update() trong loop
void SocketManager::runSocketTask() {
    Serial.println("Socket Manager: WebSocket task started (while true loop)");
    
    // while(true) loop - chạy liên tục sau khi init
    while (taskRunning) {
        if (initialized) {
            // Process WebSocket events (this is the main loop)
            webSocket.loop();
            
            // Keep-alive: Send ping periodically if connected
            if (isConnected) {
                unsigned long now = millis();
                
                // Send ping if interval has passed
                if (now - lastPingTime >= pingInterval) {
                    String pingMessage = "{\"type\":\"ping\",\"timestamp\":\"" + String(now) + "\"}";
                    String msgCopy = pingMessage;
                    webSocket.sendTXT(msgCopy);
                    lastPingTime = now;
                    Serial.println("Socket Manager: Sent keep-alive ping");
                }
            }
            
            // Small delay to prevent task from consuming too much CPU
            vTaskDelay(pdMS_TO_TICKS(10));  // 10ms delay
        } else {
            // If not initialized, wait longer
            vTaskDelay(pdMS_TO_TICKS(1000));  // 1 second delay
        }
    }
    
    Serial.println("Socket Manager: WebSocket task stopped");
}

void SocketManager::update() {
    // Method này không cần thiết nữa vì task đã chạy while(true) tự động
    // Task xử lý tất cả: webSocket.loop(), keep-alive ping, etc.
    // Có thể bỏ qua method này hoặc chỉ dùng để debug nếu cần
}

void SocketManager::sendMessage(const String& message) {
    if (isConnected && initialized) {
        // Create non-const copy because sendTXT requires non-const reference
        String msgCopy = message;
        webSocket.sendTXT(msgCopy);
        Serial.print("Socket Manager: Sent message: ");
        Serial.println(message);
    } else {
        Serial.println("Socket Manager: Cannot send - not connected or not initialized");
    }
}

void SocketManager::disconnect() {
    if (initialized) {
        taskRunning = false;  // Signal task to stop
        webSocket.disconnect();
        isConnected = false;
        Serial.println("Socket Manager: Disconnected");
    }
}

void SocketManager::webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
    if (instance) {
        instance->onWebSocketEvent(type, payload, length);
    }
}

void SocketManager::onWebSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
    switch(type) {
        case WStype_DISCONNECTED:
            isConnected = false;
            Serial.println("Socket Manager: WebSocket disconnected");
            break;
            
        case WStype_CONNECTED:
            isConnected = true;
            lastPingTime = millis();  // Reset ping timer
            Serial.print("Socket Manager: WebSocket connected to: ");
            Serial.println((char*)payload);
            
            // Send initial handshake message
            {
                String initMessage = "{\"type\":\"init\",\"device\":\"ESP32\"}";
                String msgCopy = initMessage;  // Create copy for sendTXT (requires non-const reference)
                webSocket.sendTXT(msgCopy);
                Serial.print("Socket Manager: Sent init message: ");
                Serial.println(initMessage);
            }
            break;
            
        case WStype_TEXT:
            {
                String message = String((char*)payload);
                Serial.print("Socket Manager: Received message: ");
                Serial.println(message);
                
                // Check if it's a pong response
                if (message.indexOf("\"type\":\"pong\"") >= 0) {
                    Serial.println("Socket Manager: Received pong - connection alive");
                } else {
                    // TODO: Handle other received messages here
                }
            }
            break;
            
        case WStype_BIN:
            Serial.print("Socket Manager: Received binary data, length: ");
            Serial.println(length);
            break;
            
        case WStype_ERROR:
            isConnected = false;
            Serial.print("Socket Manager: WebSocket ERROR: ");
            if (length > 0) {
                Serial.println((char*)payload);
            } else {
                Serial.println("Unknown error");
            }
            Serial.print("Socket Manager: Error details - Host: ");
            Serial.print(serverHost);
            Serial.print(", Port: ");
            Serial.print(serverPort);
            Serial.print(", Path: ");
            Serial.println(serverPath);
            break;
            
        case WStype_PING:
            Serial.println("Socket Manager: Received ping");
            break;
            
        case WStype_PONG:
            Serial.println("Socket Manager: Received pong from server - connection alive");
            break;
            
        default:
            Serial.print("Socket Manager: Unknown event type: ");
            Serial.println(type);
            break;
    }
}

