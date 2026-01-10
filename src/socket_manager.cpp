#include "socket_manager.h"
#include "chat_screen.h"
#include "social_screen.h"
#include "caro_game_screen.h"
#include "game_lobby_screen.h"
#include <FS.h>
#include <SPIFFS.h>

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
    
    // Notification callback
    onNotificationCallback = nullptr;
    
    // Chat callbacks
    onChatMessageCallback = nullptr;
    onChatMessageDisplayCallback = nullptr;
    onChatBadgeCallback = nullptr;
    onTypingIndicatorCallback = nullptr;
    onDeliveryStatusCallback = nullptr;
    onReadReceiptCallback = nullptr;
    onUserStatusUpdateCallback = nullptr;
    onGameEventCallback = nullptr;
    onGameMoveCallback = nullptr;
    
    // Typing indicator state
    lastTypingTime = 0;
    currentTypingToUserId = -1;
    
    // User ID
    userId = -1;
    
    // ChatScreen state
    chatScreen = nullptr;
    isChatScreenActive = false;
    currentChatFriendUserId = -1;
    
    // SocialScreen state
    socialScreen = nullptr;
    isSocialScreenActive = false;
    
    // Create semaphore for thread-safe file access
    fileMutex = xSemaphoreCreateMutex();
    if (fileMutex == NULL) {
        Serial.println("Socket Manager: Failed to create file mutex!");
    }
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
    // Delete semaphore
    if (fileMutex != NULL) {
        vSemaphoreDelete(fileMutex);
        fileMutex = NULL;
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
                
                // Auto-stop typing indicator after 3 seconds of inactivity
                if (currentTypingToUserId != -1 && now - lastTypingTime >= TYPING_AUTO_STOP_INTERVAL) {
                    sendTypingStop(currentTypingToUserId);
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
                String initMessage = "{\"type\":\"init\",\"device\":\"ESP32\"";
                if (userId > 0) {
                    initMessage += ",\"user_id\":";
                    initMessage += String(userId);
                }
                initMessage += "}";
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
                
                // Check if it's a pong response (handle both with and without spaces)
                if (message.indexOf("\"type\":\"pong\"") >= 0 || message.indexOf("\"type\": \"pong\"") >= 0) {
                    Serial.println("Socket Manager: Received pong - connection alive");
                } else if (message.indexOf("\"type\":\"notification\"") >= 0 || message.indexOf("\"type\": \"notification\"") >= 0) {
                    // Handle notification message (check both with and without spaces in JSON)
                    Serial.println("Socket Manager: ✅ Received notification message - parsing...");
                    parseNotificationMessage(message);
                } else if (message.indexOf("\"type\":\"chat_message\"") >= 0 || message.indexOf("\"type\": \"chat_message\"") >= 0) {
                    // Handle chat message
                    Serial.println("Socket Manager: ✅ Received chat message - parsing...");
                    parseChatMessage(message);
                } else if (message.indexOf("\"type\":\"typing_start\"") >= 0 || message.indexOf("\"type\": \"typing_start\"") >= 0) {
                    // Handle typing start
                    Serial.println("Socket Manager: ✅ Received typing_start - parsing...");
                    parseTypingIndicator(message, true);
                } else if (message.indexOf("\"type\":\"typing_stop\"") >= 0 || message.indexOf("\"type\": \"typing_stop\"") >= 0) {
                    // Handle typing stop
                    Serial.println("Socket Manager: ✅ Received typing_stop - parsing...");
                    parseTypingIndicator(message, false);
                } else if (message.indexOf("\"type\":\"message_delivered\"") >= 0 || message.indexOf("\"type\": \"message_delivered\"") >= 0) {
                    // Handle delivery status
                    Serial.println("Socket Manager: ✅ Received message_delivered - parsing...");
                    parseDeliveryStatus(message, "delivered");
                } else if (message.indexOf("\"type\":\"message_read\"") >= 0 || message.indexOf("\"type\": \"message_read\"") >= 0) {
                    // Handle read receipt
                    Serial.println("Socket Manager: ✅ Received message_read - parsing...");
                    parseReadReceipt(message);
                } else if (message.indexOf("\"type\":\"user_status_update\"") >= 0 || message.indexOf("\"type\": \"user_status_update\"") >= 0) {
                    // Handle user status update
                    Serial.println("Socket Manager: ✅ Received user_status_update - parsing...");
                    parseUserStatusUpdate(message);
                } else if (message.indexOf("\"type\":\"game_event\"") >= 0 || message.indexOf("\"type\": \"game_event\"") >= 0) {
                    Serial.println("Socket Manager: ✅ Received game_event - parsing...");
                    parseGameEvent(message);
                } else {
                    // Handle other received messages here
                    Serial.print("Socket Manager: ⚠️  Unknown message type. Full message: ");
                    Serial.println(message);
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

void SocketManager::parseNotificationMessage(const String& message) {
    // Parse JSON manually: {"type":"notification","notification":{"id":123,"type":"friend_request_accepted","message":"...","timestamp":"...","read":false}}}
    // Simple JSON parsing for notification structure
    
    Serial.println("Socket Manager: parseNotificationMessage() called");
    
    // Bỏ check onNotificationCallback vì đã xử lý trực tiếp trong socket_manager
    Serial.println("Socket Manager: Parsing notification...");
    
    // Find notification object (handle both with and without spaces)
    int notificationStart = message.indexOf("\"notification\":{");
    if (notificationStart < 0) {
        notificationStart = message.indexOf("\"notification\": {");
    }
    if (notificationStart < 0) {
        Serial.println("Socket Manager: ❌ Invalid notification message format - cannot find \"notification\":{");
        Serial.print("Socket Manager: Message: ");
        Serial.println(message);
        return;
    }
    
    Serial.print("Socket Manager: Found notification object at position: ");
    Serial.println(notificationStart);
    
    // Skip "notification":{ or "notification": {
    if (message.charAt(notificationStart + 14) == '{') {
        notificationStart += 15;  // Skip "notification":{
    } else {
        notificationStart += 16;  // Skip "notification": {
    }
    
    Serial.print("Socket Manager: Starting parse from position: ");
    Serial.println(notificationStart);
    
    // Parse id (handle both "id": and "id": )
    int idStart = message.indexOf("\"id\":", notificationStart);
    if (idStart < 0) {
        idStart = message.indexOf("\"id\": ", notificationStart);
    }
    if (idStart < 0) {
        Serial.println("Socket Manager: ❌ Cannot find \"id\":");
        return;
    }
    idStart += 5;
    // Skip space if present
    if (message.charAt(idStart) == ' ') {
        idStart++;
    }
    int idEnd = message.indexOf(',', idStart);
    if (idEnd < 0) idEnd = message.indexOf('}', idStart);
    if (idEnd < 0) {
        Serial.println("Socket Manager: ❌ Cannot find end of id");
        return;
    }
    int notificationId = message.substring(idStart, idEnd).toInt();
    Serial.print("Socket Manager: Parsed id: ");
    Serial.println(notificationId);
    
    // Parse type (handle both "type":" and "type": ")
    int typeStart = message.indexOf("\"type\":\"", notificationStart);
    if (typeStart < 0) {
        typeStart = message.indexOf("\"type\": \"", notificationStart);
        if (typeStart >= 0) {
            typeStart += 9;  // Skip "type": "
        }
    } else {
        typeStart += 8;  // Skip "type":"
    }
    if (typeStart < 0 || typeStart < notificationStart) {
        Serial.println("Socket Manager: ❌ Cannot find \"type\":\"");
        return;
    }
    int typeEnd = message.indexOf('"', typeStart);
    if (typeEnd < 0) {
        Serial.println("Socket Manager: ❌ Cannot find end of type");
        return;
    }
    String notificationType = message.substring(typeStart, typeEnd);
    Serial.print("Socket Manager: Parsed type: ");
    Serial.println(notificationType);
    
    // Parse message (handle both "message":" and "message": ")
    int msgStart = message.indexOf("\"message\":\"", notificationStart);
    if (msgStart < 0) {
        msgStart = message.indexOf("\"message\": \"", notificationStart);
        if (msgStart >= 0) {
            msgStart += 12;  // Skip "message": "
        }
    } else {
        msgStart += 11;  // Skip "message":"
    }
    if (msgStart < 0 || msgStart < notificationStart) {
        Serial.println("Socket Manager: ❌ Cannot find \"message\":\"");
        return;
    }
    int msgEnd = message.indexOf('"', msgStart);
    if (msgEnd < 0) {
        Serial.println("Socket Manager: ❌ Cannot find end of message");
        return;
    }
    // Handle escaped quotes in message
    while (msgEnd > msgStart && message.charAt(msgEnd - 1) == '\\') {
        msgEnd = message.indexOf('"', msgEnd + 1);
        if (msgEnd < 0) {
            Serial.println("Socket Manager: ❌ Cannot find end of message (escaped quotes)");
            return;
        }
    }
    String notificationMessage = message.substring(msgStart, msgEnd);
    // Unescape JSON string
    notificationMessage.replace("\\\"", "\"");
    notificationMessage.replace("\\n", "\n");
    notificationMessage.replace("\\\\", "\\");
    Serial.print("Socket Manager: Parsed message: ");
    Serial.println(notificationMessage);
    
    // Parse timestamp (optional)
    int tsStart = message.indexOf("\"timestamp\":\"", notificationStart);
    if (tsStart < 0) {
        tsStart = message.indexOf("\"timestamp\": \"", notificationStart);
        if (tsStart >= 0) {
            tsStart += 15;  // Skip "timestamp": "
        }
    } else {
        tsStart += 14;  // Skip "timestamp":"
    }
    String notificationTimestamp = "";
    if (tsStart >= 0 && tsStart > notificationStart) {
        int tsEnd = message.indexOf('"', tsStart);
        if (tsEnd >= 0) {
            notificationTimestamp = message.substring(tsStart, tsEnd);
        }
    }
    
    // Parse read (handle both "read": and "read": )
    bool notificationRead = false;
    int readStart = message.indexOf("\"read\":", notificationStart);
    if (readStart < 0) {
        readStart = message.indexOf("\"read\": ", notificationStart);
    }
    if (readStart >= 0 && readStart > notificationStart) {
        readStart += 7;
        // Skip space if present
        if (message.charAt(readStart) == ' ') {
            readStart++;
        }
        int readEnd = message.indexOf(',', readStart);
        if (readEnd < 0) readEnd = message.indexOf('}', readStart);
        if (readEnd >= 0) {
            String readStr = message.substring(readStart, readEnd);
            readStr.trim();
            notificationRead = (readStr == "true" || readStr == "1");
        }
    }
    
    Serial.print("Socket Manager: ✅ Parsed notification - id: ");
    Serial.print(notificationId);
    Serial.print(", type: ");
    Serial.print(notificationType);
    Serial.print(", message: ");
    Serial.print(notificationMessage);
    Serial.print(", read: ");
    Serial.println(notificationRead);
    
    // Xử lý notification trực tiếp trong socket_manager
    if (socialScreen != nullptr) {
        // Check parent (SocialScreen) active first
        bool isParentActive = isSocialScreenActive && socialScreen->getActive();
        
        if (!isParentActive) {
            // Parent not active - just update data, don't draw badge
            socialScreen->addNotificationFromSocket(notificationId, notificationType, notificationMessage, notificationTimestamp, notificationRead);
            Serial.println("Socket Manager: Social screen not active - data updated only");
            return;
        }
        
        // Check child screen state
        SocialScreen::ScreenState screenState = socialScreen->getScreenState();
        
        // Check if child game screens are active
        if (screenState == SocialScreen::STATE_PLAYING_GAME) {
            if (socialScreen->getCaroGameScreen() != nullptr && 
                socialScreen->getCaroGameScreen()->isActive()) {
                // Game screen active - just update data, don't draw badge
                socialScreen->addNotificationFromSocket(notificationId, notificationType, notificationMessage, notificationTimestamp, notificationRead);
                Serial.println("Socket Manager: Caro game active - data updated only");
                return;
            }
        } else if (screenState == SocialScreen::STATE_WAITING_GAME) {
            if (socialScreen->getGameLobby() != nullptr && 
                socialScreen->getGameLobby()->isActive()) {
                // Lobby active - just update data, don't draw badge
                socialScreen->addNotificationFromSocket(notificationId, notificationType, notificationMessage, notificationTimestamp, notificationRead);
                Serial.println("Socket Manager: Game lobby active - data updated only");
                return;
            }
        }
        
        // Parent active + no child screen focused = can draw badge
        Serial.println("Socket Manager: Forwarding notification to SocialScreen...");
        socialScreen->addNotificationFromSocket(notificationId, notificationType, notificationMessage, notificationTimestamp, notificationRead);
        Serial.println("Socket Manager: Notification forwarded");
        
        // Redraw sidebar to show badge
        if (isParentActive) {
            socialScreen->redrawSidebar();
        }
    } else {
        Serial.println("Socket Manager: ⚠️  socialScreen is null!");
    }
}

// Helper function to save message to file
void SocketManager::saveChatMessageToFile(int fromUserId, int toUserId, const String& message, bool isFromUser) {
    // Lock mutex để đảm bảo thread-safe
    if (fileMutex == NULL) {
        Serial.println("Socket Manager: ⚠️  File mutex not initialized!");
        return;
    }
    
    if (xSemaphoreTake(fileMutex, portMAX_DELAY) != pdTRUE) {
        Serial.println("Socket Manager: ⚠️  Failed to take file mutex!");
        return;
    }
    
    // Khởi tạo SPIFFS nếu chưa được khởi tạo
    if (!SPIFFS.begin(true)) {
        Serial.println("Socket Manager: ⚠️  SPIFFS initialization failed in saveChatMessageToFile!");
        xSemaphoreGive(fileMutex);  // Release mutex trước khi return
        return;
    }
    
    // Tạo tên file theo format <min_id>-<max_id>.txt
    int minId = (fromUserId < toUserId) ? fromUserId : toUserId;
    int maxId = (fromUserId > toUserId) ? fromUserId : toUserId;
    
    String fileName = "/";
    fileName += String(minId);
    fileName += "-";
    fileName += String(maxId);
    fileName += ".txt";
    
    Serial.print("Socket Manager: Saving message to file: ");
    Serial.println(fileName);
    
    // Đọc file hiện tại (nếu có)
    String existingContent = "";
    int existingCount = 0;
    
    if (SPIFFS.exists(fileName)) {
        File readFile = SPIFFS.open(fileName, "r");
        if (readFile) {
            // Đọc count
            String countStr = readFile.readStringUntil('\n');
            existingCount = countStr.toInt();
            
            // Đọc tất cả messages
            while (readFile.available()) {
                String line = readFile.readStringUntil('\n');
                if (line.length() > 0) {
                    existingContent += line;
                    existingContent += "\n";
                }
            }
            readFile.close();
        }
    }
    
    // Mở file để ghi (append mode không có, nên ghi lại)
    File file = SPIFFS.open(fileName, "w");
    if (!file) {
        Serial.print("Socket Manager: Failed to open file for writing: ");
        Serial.println(fileName);
        xSemaphoreGive(fileMutex);  // Release mutex trước khi return
        return;
    }
    
    // Ghi số lượng tin nhắn mới
    int newCount = existingCount + 1;
    file.println(newCount);
    
    // Ghi lại messages cũ
    if (existingContent.length() > 0) {
        file.print(existingContent);
    }
    
    // Ghi message mới
    file.print(isFromUser ? "1" : "0");
    file.print("|");
    file.print(message);
    file.print("|");
    file.println(millis());
    
    file.close();
    Serial.print("Socket Manager: Saved message to file. Total messages: ");
    Serial.println(newCount);
    
    // Release mutex
    xSemaphoreGive(fileMutex);
}

void SocketManager::parseChatMessage(const String& message) {
    // Bỏ check onChatMessageCallback vì đã xử lý trực tiếp trong socket_manager
    Serial.println("Socket Manager: Parsing chat message...");
    
    // Parse from_user_id
    int fromUserIdStart = message.indexOf("\"from_user_id\":");
    if (fromUserIdStart < 0) {
        fromUserIdStart = message.indexOf("\"from_user_id\": ");
    }
    if (fromUserIdStart < 0) {
        Serial.println("Socket Manager: Cannot find from_user_id in chat message");
        return;
    }
    fromUserIdStart += 15;
    if (message.charAt(fromUserIdStart) == ' ') fromUserIdStart++;
    int fromUserIdEnd = message.indexOf(',', fromUserIdStart);
    if (fromUserIdEnd < 0) fromUserIdEnd = message.indexOf('}', fromUserIdStart);
    if (fromUserIdEnd < 0) return;
    int fromUserId = message.substring(fromUserIdStart, fromUserIdEnd).toInt();
    
    // Parse from_nickname
    int nicknameStart = message.indexOf("\"from_nickname\":\"");
    if (nicknameStart < 0) {
        nicknameStart = message.indexOf("\"from_nickname\": \"");
        if (nicknameStart >= 0) {
            nicknameStart += 18;
        }
    } else {
        nicknameStart += 17;
    }
    String fromNickname = "";
    if (nicknameStart >= 0) {
        int nicknameEnd = message.indexOf('"', nicknameStart);
        if (nicknameEnd >= 0) {
            fromNickname = message.substring(nicknameStart, nicknameEnd);
        }
    }
    
    // Parse message text
    int msgStart = message.indexOf("\"message\":\"");
    if (msgStart < 0) {
        msgStart = message.indexOf("\"message\": \"");
        if (msgStart >= 0) {
            msgStart += 12;
        }
    } else {
        msgStart += 11;
    }
    if (msgStart < 0) {
        Serial.println("Socket Manager: Cannot find message in chat message");
        return;
    }
    int msgEnd = message.indexOf('"', msgStart);
    if (msgEnd < 0) return;
    // Handle escaped quotes
    while (msgEnd > msgStart && message.charAt(msgEnd - 1) == '\\') {
        msgEnd = message.indexOf('"', msgEnd + 1);
        if (msgEnd < 0) return;
    }
    String chatMessage = message.substring(msgStart, msgEnd);
    chatMessage.replace("\\\"", "\"");
    chatMessage.replace("\\n", "\n");
    chatMessage.replace("\\\\", "\\");
    
    // Parse message_id
    int msgIdStart = message.indexOf("\"message_id\":\"");
    if (msgIdStart < 0) {
        msgIdStart = message.indexOf("\"message_id\": \"");
        if (msgIdStart >= 0) {
            msgIdStart += 15;
        }
    } else {
        msgIdStart += 14;
    }
    String messageId = "";
    if (msgIdStart >= 0) {
        int msgIdEnd = message.indexOf('"', msgIdStart);
        if (msgIdEnd >= 0) {
            messageId = message.substring(msgIdStart, msgIdEnd);
        }
    }
    
    // Parse timestamp
    int tsStart = message.indexOf("\"timestamp\":\"");
    if (tsStart < 0) {
        tsStart = message.indexOf("\"timestamp\": \"");
        if (tsStart >= 0) {
            tsStart += 14;
        }
    } else {
        tsStart += 13;
    }
    String timestamp = "";
    if (tsStart >= 0) {
        int tsEnd = message.indexOf('"', tsStart);
        if (tsEnd >= 0) {
            timestamp = message.substring(tsStart, tsEnd);
        }
    }
    
    Serial.print("Socket Manager: ✅ Parsed chat message - from: ");
    Serial.print(fromUserId);
    Serial.print(" (");
    Serial.print(fromNickname);
    Serial.print("), message: ");
    Serial.println(chatMessage);
    
    // Lưu message vào file (nếu có userId)
    if (userId > 0) {
        Serial.println("Socket Manager: Saving message to file...");
        saveChatMessageToFile(fromUserId, userId, chatMessage, false);  // false = từ friend (không phải current user)
    } else {
        Serial.println("Socket Manager: ⚠️  Cannot save message - userId not set");
    }
    
    // Kiểm tra và hiển thị message nếu chat screen đang mở với friend này
    // Check parent (SocialScreen) active first
    bool isSocialParentActive = isSocialScreenActive && socialScreen != nullptr && socialScreen->getActive();
    
    bool messageDisplayed = false;
    if (isSocialParentActive && isChatScreenActive && currentChatFriendUserId == fromUserId && 
        chatScreen != nullptr && chatScreen->isActive()) {
        Serial.println("Socket Manager: ✅ Parent active + ChatScreen active - adding message");
        // persist=false because SocketManager already saves incoming messages to file
        chatScreen->addMessage(chatMessage, false, false);  // false = message từ friend
        chatScreen->redrawMessages();  // Chỉ vẽ lại phần messages
        Serial.println("Socket Manager: Message added to ChatScreen and redrawn");
        messageDisplayed = true;
    }
    
    if (messageDisplayed) {
        return;  // Message đã được hiển thị, không cần xử lý badge
    }
    
    // Message chưa được hiển thị, xử lý badge trực tiếp trong socket_manager
    if (socialScreen != nullptr) {
        Serial.println("Socket Manager: Handling badge for unread message");
        Serial.print("  From User ID: ");
        Serial.print(fromUserId);
        Serial.print(" (");
        Serial.print(fromNickname);
        Serial.println(")");
        
        // Check parent screen active first
        bool isParentActive = isSocialScreenActive && socialScreen->getActive();
        
        if (!isParentActive) {
            // Parent not active - just update data, don't draw badge
            socialScreen->addUnreadChatForFriend(fromUserId);
            Serial.println("Socket Manager: Social screen not active - data updated only");
            return;
        }
        
        // Check child screen state
        SocialScreen::ScreenState screenState = socialScreen->getScreenState();
        
        // Check if child game screens are active (if so, don't draw badge)
        if (screenState == SocialScreen::STATE_PLAYING_GAME) {
            // In game - check if caro game screen is active
            if (socialScreen->getCaroGameScreen() != nullptr && 
                socialScreen->getCaroGameScreen()->isActive()) {
                // Child game screen active - just update data, don't draw badge
                socialScreen->addUnreadChatForFriend(fromUserId);
                Serial.println("Socket Manager: Caro game active - data updated only");
                return;
            }
        } else if (screenState == SocialScreen::STATE_WAITING_GAME) {
            // In lobby - check if lobby is active
            if (socialScreen->getGameLobby() != nullptr && 
                socialScreen->getGameLobby()->isActive()) {
                // Child lobby active - just update data, don't draw badge
                socialScreen->addUnreadChatForFriend(fromUserId);
                Serial.println("Socket Manager: Game lobby active - data updated only");
                return;
            }
        }
        
        // Parent active + no child screen focused = can draw badge
        bool isOnFriendsTab = screenState == SocialScreen::STATE_NORMAL &&
                              socialScreen->getCurrentTab() == SocialScreen::TAB_FRIENDS;
        
        // Luôn add unread count cho friend (để hiển thị badge trên friend card)
        socialScreen->addUnreadChatForFriend(fromUserId);
        
        if (!isOnFriendsTab) {
            // User is not viewing Friends tab, set badge trên Friends icon
            socialScreen->setHasUnreadChat(true);
            
            // Redraw sidebar if Social screen is active AND visible to show badge immediately
            Serial.println("Socket Manager: Redrawing sidebar to show badge immediately");
            socialScreen->redrawSidebar();
        } else {
            // User is on Friends tab, badge sẽ hiển thị trên friend card
            Serial.println("Socket Manager: User is on Friends tab, badge will show on friend card");
        }
    } else {
        Serial.println("Socket Manager: ⚠️  socialScreen is null!");
    }
}

void SocketManager::parseTypingIndicator(const String& message, bool isTyping) {
    if (onTypingIndicatorCallback == nullptr) {
        return;
    }
    
    // Parse from_user_id
    int fromUserIdStart = message.indexOf("\"from_user_id\":");
    if (fromUserIdStart < 0) {
        fromUserIdStart = message.indexOf("\"from_user_id\": ");
    }
    if (fromUserIdStart < 0) return;
    fromUserIdStart += 15;
    if (message.charAt(fromUserIdStart) == ' ') fromUserIdStart++;
    int fromUserIdEnd = message.indexOf(',', fromUserIdStart);
    if (fromUserIdEnd < 0) fromUserIdEnd = message.indexOf('}', fromUserIdStart);
    if (fromUserIdEnd < 0) return;
    int fromUserId = message.substring(fromUserIdStart, fromUserIdEnd).toInt();
    
    // Parse from_nickname
    int nicknameStart = message.indexOf("\"from_nickname\":\"");
    if (nicknameStart < 0) {
        nicknameStart = message.indexOf("\"from_nickname\": \"");
        if (nicknameStart >= 0) {
            nicknameStart += 18;
        }
    } else {
        nicknameStart += 17;
    }
    String fromNickname = "";
    if (nicknameStart >= 0) {
        int nicknameEnd = message.indexOf('"', nicknameStart);
        if (nicknameEnd >= 0) {
            fromNickname = message.substring(nicknameStart, nicknameEnd);
        }
    }
    
    if (onTypingIndicatorCallback != nullptr) {
        onTypingIndicatorCallback(fromUserId, fromNickname, isTyping);
    }
}

void SocketManager::parseDeliveryStatus(const String& message, const String& status) {
    if (onDeliveryStatusCallback == nullptr) {
        return;
    }
    
    // Parse message_id
    int msgIdStart = message.indexOf("\"message_id\":\"");
    if (msgIdStart < 0) {
        msgIdStart = message.indexOf("\"message_id\": \"");
        if (msgIdStart >= 0) {
            msgIdStart += 15;
        }
    } else {
        msgIdStart += 14;
    }
    String messageId = "";
    if (msgIdStart >= 0) {
        int msgIdEnd = message.indexOf('"', msgIdStart);
        if (msgIdEnd >= 0) {
            messageId = message.substring(msgIdStart, msgIdEnd);
        }
    }
    
    // Parse timestamp
    int tsStart = message.indexOf("\"timestamp\":\"");
    if (tsStart < 0) {
        tsStart = message.indexOf("\"timestamp\": \"");
        if (tsStart >= 0) {
            tsStart += 14;
        }
    } else {
        tsStart += 13;
    }
    String timestamp = "";
    if (tsStart >= 0) {
        int tsEnd = message.indexOf('"', tsStart);
        if (tsEnd >= 0) {
            timestamp = message.substring(tsStart, tsEnd);
        }
    }
    
    if (onDeliveryStatusCallback != nullptr) {
        onDeliveryStatusCallback(messageId, status, timestamp);
    }
}

void SocketManager::parseReadReceipt(const String& message) {
    if (onReadReceiptCallback == nullptr) {
        return;
    }
    
    // Parse message_id
    int msgIdStart = message.indexOf("\"message_id\":\"");
    if (msgIdStart < 0) {
        msgIdStart = message.indexOf("\"message_id\": \"");
        if (msgIdStart >= 0) {
            msgIdStart += 15;
        }
    } else {
        msgIdStart += 14;
    }
    String messageId = "";
    if (msgIdStart >= 0) {
        int msgIdEnd = message.indexOf('"', msgIdStart);
        if (msgIdEnd >= 0) {
            messageId = message.substring(msgIdStart, msgIdEnd);
        }
    }
    
    // Parse timestamp
    int tsStart = message.indexOf("\"timestamp\":\"");
    if (tsStart < 0) {
        tsStart = message.indexOf("\"timestamp\": \"");
        if (tsStart >= 0) {
            tsStart += 14;
        }
    } else {
        tsStart += 13;
    }
    String timestamp = "";
    if (tsStart >= 0) {
        int tsEnd = message.indexOf('"', tsStart);
        if (tsEnd >= 0) {
            timestamp = message.substring(tsStart, tsEnd);
        }
    }
    
    if (onReadReceiptCallback != nullptr) {
        onReadReceiptCallback(messageId, timestamp);
    }
}

void SocketManager::parseUserStatusUpdate(const String& message) {
    Serial.println("Socket Manager: parseUserStatusUpdate() called");
    
    // Parse user_id (handle both with and without spaces)
    int userIdStart = message.indexOf("\"user_id\":");
    if (userIdStart < 0) {
        userIdStart = message.indexOf("\"user_id\": ");
    }
    if (userIdStart < 0) {
        Serial.println("Socket Manager: ❌ Cannot find \"user_id\":");
        return;
    }
    userIdStart += 10;  // Skip "user_id":
    // Skip space if present
    if (message.charAt(userIdStart) == ' ') {
        userIdStart++;
    }
    int userIdEnd = message.indexOf(',', userIdStart);
    if (userIdEnd < 0) userIdEnd = message.indexOf('}', userIdStart);
    if (userIdEnd < 0) {
        Serial.println("Socket Manager: ❌ Cannot find end of user_id");
        return;
    }
    int userId = message.substring(userIdStart, userIdEnd).toInt();
    Serial.print("Socket Manager: Parsed user_id: ");
    Serial.println(userId);
    
    // Parse status (handle both with and without spaces)
    int statusStart = message.indexOf("\"status\":\"");
    if (statusStart < 0) {
        statusStart = message.indexOf("\"status\": \"");
        if (statusStart >= 0) {
            statusStart += 11;  // Skip "status": "
        }
    } else {
        statusStart += 10;  // Skip "status":"
    }
    if (statusStart < 0) {
        Serial.println("Socket Manager: ❌ Cannot find \"status\":\"");
        return;
    }
    int statusEnd = message.indexOf('"', statusStart);
    if (statusEnd < 0) {
        Serial.println("Socket Manager: ❌ Cannot find end of status");
        return;
    }
    String status = message.substring(statusStart, statusEnd);
    Serial.print("Socket Manager: Parsed status: ");
    Serial.println(status);

    // Always forward presence updates to the callback.
    // SocialScreen::updateFriendStatus() already knows when to redraw vs update data only.
    if (onUserStatusUpdateCallback != nullptr) {
        onUserStatusUpdateCallback(userId, status);
    } else {
        Serial.println("Socket Manager: ⚠️ No callback set for user status update");
    }
}

void SocketManager::parseGameEvent(const String& message) {
    Serial.println("Socket Manager: parseGameEvent() called");

    String eventType = "";
    String gameType = "";
    String status = "";
    String hostNickname = "";
    String userNickname = "";
    int sessionId = -1;
    int userId = -1;
    bool accepted = false;
    bool ready = false;

    // Server sends nested: {"type": "game_event", "event": {"event_type": ...}}
    // Find the "event": { ... } block
    int eventKeyPos = message.indexOf("\"event\":");
    int searchStart = 0;
    
    if (eventKeyPos >= 0) {
        // Find the opening brace after "event": (skip spaces)
        int bracePos = eventKeyPos + 8; // Start after "event":
        while (bracePos < message.length() && message.charAt(bracePos) == ' ') {
            bracePos++;
        }
        if (bracePos < message.length() && message.charAt(bracePos) == '{') {
            searchStart = bracePos + 1; // Start searching from inside the event object
            Serial.print("Socket Manager: Found nested event object at position ");
            Serial.println(bracePos);
        }
    }

    // Try multiple formats for event_type
    int eventTypeKeyPos = message.indexOf("\"event_type\"", searchStart);
    if (eventTypeKeyPos >= 0) {
        // Find the colon after "event_type"
        int colonPos = message.indexOf(":", eventTypeKeyPos);
        if (colonPos >= 0) {
            // Skip colon and any spaces
            int valueStart = colonPos + 1;
            while (valueStart < message.length() && (message.charAt(valueStart) == ' ' || message.charAt(valueStart) == '\t')) {
                valueStart++;
            }
            // Should be a quote now
            if (valueStart < message.length() && message.charAt(valueStart) == '"') {
                valueStart++; // Skip opening quote
                int valueEnd = message.indexOf('"', valueStart);
                if (valueEnd > valueStart) {
                    eventType = message.substring(valueStart, valueEnd);
                    Serial.print("Socket Manager: eventType found: ");
                    Serial.println(eventType);
                }
            }
        }
    }
    
    if (eventType.length() == 0) {
        Serial.print("Socket Manager: ⚠️  Could not parse event_type. eventTypeKeyPos=");
        Serial.print(eventTypeKeyPos);
        Serial.print(", searchStart=");
        Serial.println(searchStart);
    }

    int sessionStart = message.indexOf("\"session_id\":", searchStart);
    if (sessionStart >= 0) {
        int valueStart = message.indexOf(":", sessionStart) + 1;
        int valueEnd = message.indexOf(",", valueStart);
        if (valueEnd < 0) valueEnd = message.indexOf("}", valueStart);
        if (valueEnd > valueStart) {
            sessionId = message.substring(valueStart, valueEnd).toInt();
        }
    }

    int gameStart = message.indexOf("\"game_type\":\"", searchStart);
    if (gameStart < 0) {
        gameStart = message.indexOf("\"game_type\": \"", searchStart);
        if (gameStart >= 0) gameStart += 13;
    } else {
        gameStart += 12;
    }
    if (gameStart >= 0) {
        int gameEnd = message.indexOf('"', gameStart);
        if (gameEnd > gameStart) {
            gameType = message.substring(gameStart, gameEnd);
        }
    }

    int statusStart = message.indexOf("\"status\":\"", searchStart);
    if (statusStart < 0) {
        statusStart = message.indexOf("\"status\": \"", searchStart);
        if (statusStart >= 0) statusStart += 11;
    } else {
        statusStart += 10;
    }
    if (statusStart >= 0) {
        int statusEnd = message.indexOf('"', statusStart);
        if (statusEnd > statusStart) {
            status = message.substring(statusStart, statusEnd);
        }
    }

    int userStart = message.indexOf("\"user_id\":", searchStart);
    if (userStart >= 0) {
        int valueStart = message.indexOf(":", userStart) + 1;
        int valueEnd = message.indexOf(",", valueStart);
        if (valueEnd < 0) valueEnd = message.indexOf("}", valueStart);
        if (valueEnd > valueStart) {
            userId = message.substring(valueStart, valueEnd).toInt();
        }
    }

    int accStart = message.indexOf("\"accepted\":", searchStart);
    if (accStart >= 0) {
        int valueStart = message.indexOf(":", accStart) + 1;
        String v = message.substring(valueStart, valueStart + 6);
        v.trim();
        accepted = v.startsWith("true");
    }

    int readyStart = message.indexOf("\"ready\":", searchStart);
    if (readyStart >= 0) {
        int valueStart = message.indexOf(":", readyStart) + 1;
        String v = message.substring(valueStart, valueStart + 6);
        v.trim();
        ready = v.startsWith("true");
    }

    int hostStart = message.indexOf("\"host_nickname\":\"", searchStart);
    if (hostStart < 0) {
        hostStart = message.indexOf("\"host_nickname\": \"", searchStart);
        if (hostStart >= 0) hostStart += 18;
    } else {
        hostStart += 17;
    }
    if (hostStart >= 0) {
        int hostEnd = message.indexOf('"', hostStart);
        if (hostEnd > hostStart) {
            hostNickname = message.substring(hostStart, hostEnd);
        }
    }

    int userNickStart = message.indexOf("\"user_nickname\":\"", searchStart);
    if (userNickStart < 0) {
        userNickStart = message.indexOf("\"user_nickname\": \"", searchStart);
        if (userNickStart >= 0) userNickStart += 18;
    } else {
        userNickStart += 17;
    }
    if (userNickStart >= 0) {
        int userNickEnd = message.indexOf('"', userNickStart);
        if (userNickEnd > userNickStart) {
            userNickname = message.substring(userNickStart, userNickEnd);
        }
    }

    if (eventType.length() == 0) {
        Serial.print("Socket Manager: ⚠️  Game event missing event_type. Raw: ");
        Serial.println(message);
        eventType = "unknown";
    }

    Serial.print("Socket Manager: Game event -> type: ");
    Serial.print(eventType);
    Serial.print(", session: ");
    Serial.print(sessionId);
    Serial.print(", game: ");
    Serial.print(gameType);
    Serial.print(", status: ");
    Serial.println(status);

    // Handle move events separately
    if (eventType == "move") {
        int moveRow = -1;
        int moveCol = -1;
        String moveGameStatus = "";
        int moveWinnerId = -1;
        
        // Parse move-specific fields
        int rowStart = message.indexOf("\"row\":", searchStart);
        if (rowStart >= 0) {
            int valueStart = message.indexOf(":", rowStart) + 1;
            int valueEnd = message.indexOf(",", valueStart);
            if (valueEnd < 0) valueEnd = message.indexOf("}", valueStart);
            if (valueEnd > valueStart) {
                moveRow = message.substring(valueStart, valueEnd).toInt();
            }
        }
        
        int colStart = message.indexOf("\"col\":", searchStart);
        if (colStart >= 0) {
            int valueStart = message.indexOf(":", colStart) + 1;
            int valueEnd = message.indexOf(",", valueStart);
            if (valueEnd < 0) valueEnd = message.indexOf("}", valueStart);
            if (valueEnd > valueStart) {
                moveCol = message.substring(valueStart, valueEnd).toInt();
            }
        }
        
        int gameStatusStart = message.indexOf("\"game_status\":\"", searchStart);
        if (gameStatusStart >= 0) {
            int valueStart = gameStatusStart + 15;
            int valueEnd = message.indexOf("\"", valueStart);
            if (valueEnd > valueStart) {
                moveGameStatus = message.substring(valueStart, valueEnd);
            }
        }
        
        int winnerIdStart = message.indexOf("\"winner_id\":", searchStart);
        if (winnerIdStart >= 0) {
            int valueStart = message.indexOf(":", winnerIdStart) + 1;
            int valueEnd = message.indexOf(",", valueStart);
            if (valueEnd < 0) valueEnd = message.indexOf("}", valueStart);
            String winnerStr = message.substring(valueStart, valueEnd);
            winnerStr.trim();
            if (winnerStr != "null" && winnerStr.length() > 0) {
                moveWinnerId = winnerStr.toInt();
            }
        }
        
        // Parse current_turn from event
        int currentTurn = -1;
        int currentTurnStart = message.indexOf("\"current_turn\":", searchStart);
        if (currentTurnStart >= 0) {
            int valueStart = message.indexOf(":", currentTurnStart) + 1;
            int valueEnd = message.indexOf(",", valueStart);
            if (valueEnd < 0) valueEnd = message.indexOf("}", valueStart);
            if (valueEnd > valueStart) {
                currentTurn = message.substring(valueStart, valueEnd).toInt();
            }
        }
        
        if (onGameMoveCallback != nullptr && moveRow >= 0 && moveCol >= 0) {
            onGameMoveCallback(sessionId, userId, moveRow, moveCol, moveGameStatus, moveWinnerId, currentTurn);
        }
        return; // Don't process move events as regular game events
    }
    
    // Check parent (SocialScreen) active first
    if (socialScreen != nullptr) {
        bool isParentActive = isSocialScreenActive && socialScreen->getActive();
        
        if (!isParentActive) {
            Serial.println("Socket Manager: Social screen not active - skipping game event");
            return;
        }
        
        // Check child screen state
        SocialScreen::ScreenState screenState = socialScreen->getScreenState();
        
        if (screenState == SocialScreen::STATE_PLAYING_GAME) {
            // Check if caro game screen is active
            if (socialScreen->getCaroGameScreen() != nullptr && 
                socialScreen->getCaroGameScreen()->isActive()) {
                // Game screen active - handle game move events (already handled above)
                // Other game events in game screen - just update data
                if (onGameEventCallback != nullptr) {
                    onGameEventCallback(eventType, sessionId, gameType, status, userId, accepted, ready, userNickname);
                }
                return;
            }
        } else if (screenState == SocialScreen::STATE_WAITING_GAME) {
            // Check if lobby is active
            if (socialScreen->getGameLobby() != nullptr && 
                socialScreen->getGameLobby()->isActive()) {
                // Lobby active - handle game events
                if (onGameEventCallback != nullptr) {
                    onGameEventCallback(eventType, sessionId, gameType, status, userId, accepted, ready, userNickname);
                }
                return;
            }
        } else if (screenState == SocialScreen::STATE_NORMAL) {
            // Normal state - handle game invites
            if (onGameEventCallback != nullptr) {
                onGameEventCallback(eventType, sessionId, gameType, status, userId, accepted, ready, userNickname);
            }
            // Also surface as notification for UI reuse
            socialScreen->addGameInviteFromSocket(sessionId, gameType, status, eventType, hostNickname);
        }
    } else {
        // No social screen - just call callback
        if (onGameEventCallback != nullptr) {
            onGameEventCallback(eventType, sessionId, gameType, status, userId, accepted, ready, userNickname);
        }
    }
}

void SocketManager::sendChatMessage(int toUserId, const String& message, const String& messageId) {
    if (!isConnected) {
        Serial.println("Socket Manager: Cannot send chat message - not connected");
        return;
    }
    
    // Generate message_id if not provided
    String msgId = messageId;
    if (msgId.length() == 0) {
        msgId = String(millis()) + "_" + String(random(1000, 9999));
    }
    
    // Create JSON message
    String jsonMessage = "{\"type\":\"chat_message\",\"to_user_id\":";
    jsonMessage += String(toUserId);
    jsonMessage += ",\"message\":\"";
    // Escape message
    String escapedMessage = message;
    escapedMessage.replace("\\", "\\\\");
    escapedMessage.replace("\"", "\\\"");
    escapedMessage.replace("\n", "\\n");
    jsonMessage += escapedMessage;
    jsonMessage += "\",\"message_id\":\"";
    jsonMessage += msgId;
    jsonMessage += "\",\"timestamp\":\"";
    // Simple timestamp (ISO format would be better but keeping it simple)
    jsonMessage += String(millis());
    jsonMessage += "\"}";
    
    String msgCopy = jsonMessage;
    webSocket.sendTXT(msgCopy);
    Serial.print("Socket Manager: Sent chat message to user ");
    Serial.print(toUserId);
    Serial.print(", message_id: ");
    Serial.println(msgId);
}

void SocketManager::sendTypingStart(int toUserId) {
    if (!isConnected) {
        return;
    }
    
    // Stop previous typing if different user
    if (currentTypingToUserId != -1 && currentTypingToUserId != toUserId) {
        sendTypingStop(currentTypingToUserId);
    }
    
    currentTypingToUserId = toUserId;
    lastTypingTime = millis();
    
    String jsonMessage = "{\"type\":\"typing_start\",\"to_user_id\":";
    jsonMessage += String(toUserId);
    jsonMessage += "}";
    
    String msgCopy = jsonMessage;
    webSocket.sendTXT(msgCopy);
    Serial.print("Socket Manager: Sent typing_start to user ");
    Serial.println(toUserId);
}

void SocketManager::sendTypingStop(int toUserId) {
    if (!isConnected) {
        return;
    }
    
    if (currentTypingToUserId == toUserId) {
        currentTypingToUserId = -1;
        lastTypingTime = 0;
    }
    
    String jsonMessage = "{\"type\":\"typing_stop\",\"to_user_id\":";
    jsonMessage += String(toUserId);
    jsonMessage += "}";
    
    String msgCopy = jsonMessage;
    webSocket.sendTXT(msgCopy);
    Serial.print("Socket Manager: Sent typing_stop to user ");
    Serial.println(toUserId);
}

void SocketManager::sendReadReceipt(int toUserId, const String& messageId) {
    if (!isConnected) {
        return;
    }
    
    String jsonMessage = "{\"type\":\"read_receipt\",\"to_user_id\":";
    jsonMessage += String(toUserId);
    jsonMessage += ",\"message_id\":\"";
    jsonMessage += messageId;
    jsonMessage += "\"}";
    
    String msgCopy = jsonMessage;
    webSocket.sendTXT(msgCopy);
    Serial.print("Socket Manager: Sent read_receipt to user ");
    Serial.print(toUserId);
    Serial.print(", message_id: ");
    Serial.println(messageId);
}

void SocketManager::setChatScreen(ChatScreen* chatScreen) {
    this->chatScreen = chatScreen;
    
    // Tự động set SocketManager cho ChatScreen để ChatScreen có thể gửi messages
    if (chatScreen != nullptr) {
        chatScreen->setSocketManager(this);
        Serial.println("Socket Manager: Auto-set SocketManager for ChatScreen");
        Serial.print("Socket Manager: ChatScreen friendUserId: ");
        Serial.println(chatScreen->getFriendUserId());
    } else {
        Serial.println("Socket Manager: ⚠️  setChatScreen called with NULL");
    }
}

