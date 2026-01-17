#include "api_client.h"

// Helper: Parse login response string manually
void ApiClient::parseLoginResponse(const String& response, LoginResult& result) {
    result.success = false;
    result.accountExists = false;
    result.user_id = -1;
    result.username = "";
    result.message = "";
    
    // Parse response string manually using indexOf and substring
    
    // Check success
    int successIdx = response.indexOf("\"success\":");
    if (successIdx >= 0) {
        int valueStart = response.indexOf(":", successIdx) + 1;
        int valueEnd = response.indexOf(",", valueStart);
        if (valueEnd < 0) valueEnd = response.indexOf("}", valueStart);
        String successStr = response.substring(valueStart, valueEnd);
        successStr.trim();
        result.success = (successStr == "true");
    }
    
    // Check account_exists
    int accountExistsIdx = response.indexOf("\"account_exists\":");
    if (accountExistsIdx >= 0) {
        int valueStart = response.indexOf(":", accountExistsIdx) + 1;
        int valueEnd = response.indexOf(",", valueStart);
        if (valueEnd < 0) valueEnd = response.indexOf("}", valueStart);
        String existsStr = response.substring(valueStart, valueEnd);
        existsStr.trim();
        result.accountExists = (existsStr == "true");
    }
    
    // Get message
    int messageIdx = response.indexOf("\"message\":\"");
    if (messageIdx >= 0) {
        int valueStart = messageIdx + 10; // length of "message":"
        int valueEnd = response.indexOf("\"", valueStart);
        if (valueEnd > valueStart) {
            result.message = response.substring(valueStart, valueEnd);
        }
    }
    
    // Get user_id
    int userIdIdx = response.indexOf("\"user_id\":");
    if (userIdIdx >= 0) {
        int valueStart = response.indexOf(":", userIdIdx) + 1;
        int valueEnd = response.indexOf(",", valueStart);
        if (valueEnd < 0) valueEnd = response.indexOf("}", valueStart);
        String userIdStr = response.substring(valueStart, valueEnd);
        userIdStr.trim();
        if (userIdStr.length() > 0 && userIdStr != "null") {
            result.user_id = userIdStr.toInt();
        }
    }
    
    // Get username
    int usernameIdx = response.indexOf("\"username\":\"");
    if (usernameIdx >= 0) {
        int valueStart = usernameIdx + 12; // length of "username":"
        int valueEnd = response.indexOf("\"", valueStart);
        if (valueEnd > valueStart) {
            result.username = response.substring(valueStart, valueEnd);
        }
    }
    
    // Get nickname
    int nicknameIdx = response.indexOf("\"nickname\":\"");
    if (nicknameIdx >= 0) {
        int valueStart = nicknameIdx + 12; // length of "nickname":"
        int valueEnd = response.indexOf("\"", valueStart);
        if (valueEnd > valueStart) {
            result.nickname = response.substring(valueStart, valueEnd);
        }
    } else {
        // Fallback to username if nickname not found
        result.nickname = result.username;
    }
}

// Helper: Parse register response string
bool ApiClient::parseRegisterResponse(const String& response) {
    int successIdx = response.indexOf("\"success\":");
    if (successIdx >= 0) {
        int valueStart = response.indexOf(":", successIdx) + 1;
        int valueEnd = response.indexOf(",", valueStart);
        if (valueEnd < 0) valueEnd = response.indexOf("}", valueStart);
        String successStr = response.substring(valueStart, valueEnd);
        successStr.trim();
        return (successStr == "true");
    }
    return false;
}

// Helper: Parse friend request response string
void ApiClient::parseFriendRequestResponse(const String& response, FriendRequestResult& result) {
    result.success = false;
    result.message = "";
    result.requestId = -1;
    result.friendshipId = -1;
    result.status = "";
    
    // Parse success
    int successIdx = response.indexOf("\"success\":");
    if (successIdx >= 0) {
        int valueStart = response.indexOf(":", successIdx) + 1;
        int valueEnd = response.indexOf(",", valueStart);
        if (valueEnd < 0) valueEnd = response.indexOf("}", valueStart);
        String successStr = response.substring(valueStart, valueEnd);
        successStr.trim();
        result.success = (successStr == "true");
    }
    
    // Parse message
    int messageIdx = response.indexOf("\"message\":\"");
    if (messageIdx >= 0) {
        int valueStart = messageIdx + 10; // length of "message":"
        int valueEnd = response.indexOf("\"", valueStart);
        if (valueEnd > valueStart) {
            result.message = response.substring(valueStart, valueEnd);
        }
    }
    
    // Parse request_id
    int requestIdIdx = response.indexOf("\"request_id\":");
    if (requestIdIdx >= 0) {
        int valueStart = response.indexOf(":", requestIdIdx) + 1;
        int valueEnd = response.indexOf(",", valueStart);
        if (valueEnd < 0) valueEnd = response.indexOf("}", valueStart);
        String requestIdStr = response.substring(valueStart, valueEnd);
        requestIdStr.trim();
        if (requestIdStr.length() > 0 && requestIdStr != "null") {
            result.requestId = requestIdStr.toInt();
        }
    }
    
    // Parse friendship_id
    int friendshipIdIdx = response.indexOf("\"friendship_id\":");
    if (friendshipIdIdx >= 0) {
        int valueStart = response.indexOf(":", friendshipIdIdx) + 1;
        int valueEnd = response.indexOf(",", valueStart);
        if (valueEnd < 0) valueEnd = response.indexOf("}", valueStart);
        String friendshipIdStr = response.substring(valueStart, valueEnd);
        friendshipIdStr.trim();
        if (friendshipIdStr.length() > 0 && friendshipIdStr != "null") {
            result.friendshipId = friendshipIdStr.toInt();
        }
    }
    
    // Parse status
    int statusIdx = response.indexOf("\"status\":\"");
    if (statusIdx >= 0) {
        int valueStart = statusIdx + 9; // length of "status":"
        int valueEnd = response.indexOf("\"", valueStart);
        if (valueEnd > valueStart) {
            result.status = response.substring(valueStart, valueEnd);
        }
    }
}

// Helper: Parse game session response string
void ApiClient::parseGameSessionResponse(const String& response, GameSessionResult& result) {
    result.success = false;
    result.message = "";
    result.sessionId = -1;
    result.status = "";
    result.participantCount = 0;

    int successIdx = response.indexOf("\"success\":");
    if (successIdx >= 0) {
        int valueStart = response.indexOf(":", successIdx) + 1;
        int valueEnd = response.indexOf(",", valueStart);
        if (valueEnd < 0) valueEnd = response.indexOf("}", valueStart);
        String successStr = response.substring(valueStart, valueEnd);
        successStr.trim();
        result.success = (successStr == "true");
    }

    int messageIdx = response.indexOf("\"message\":\"");
    if (messageIdx >= 0) {
        int valueStart = messageIdx + 10;
        int valueEnd = response.indexOf("\"", valueStart);
        if (valueEnd > valueStart) {
            result.message = response.substring(valueStart, valueEnd);
        }
    }

    int sessionIdx = response.indexOf("\"session_id\":");
    if (sessionIdx >= 0) {
        int valueStart = response.indexOf(":", sessionIdx) + 1;
        int valueEnd = response.indexOf(",", valueStart);
        if (valueEnd < 0) valueEnd = response.indexOf("}", valueStart);
        String sessionStr = response.substring(valueStart, valueEnd);
        sessionStr.trim();
        if (sessionStr.length() > 0 && sessionStr != "null") {
            result.sessionId = sessionStr.toInt();
        }
    }

    int statusIdx = response.indexOf("\"status\":\"");
    if (statusIdx >= 0) {
        int valueStart = statusIdx + 9;
        int valueEnd = response.indexOf("\"", valueStart);
        if (valueEnd > valueStart) {
            result.status = response.substring(valueStart, valueEnd);
        }
    }

    // Approximate participant count by counting occurrences of "{"
    int participantsIdx = response.indexOf("\"participants\":[");
    if (participantsIdx >= 0) {
        int arrayStart = response.indexOf("[", participantsIdx);
        int arrayEnd = response.indexOf("]", participantsIdx);
        if (arrayStart >= 0 && arrayEnd > arrayStart) {
            String arrayBody = response.substring(arrayStart, arrayEnd);
            int count = 0;
            int pos = 0;
            while (true) {
                int bracePos = arrayBody.indexOf("{", pos);
                if (bracePos < 0) break;
                count++;
                pos = bracePos + 1;
            }
            result.participantCount = count;
        }
    }
}

ApiClient::LoginResult ApiClient::checkLogin(const String& username, const String& pin, const String& serverHost, uint16_t port) {
    LoginResult result;
    result.success = false;
    result.accountExists = false;
    result.user_id = -1;
    result.username = "";
    result.nickname = "";
    result.message = "";
    
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("API Client: WiFi not connected!");
        result.message = "WiFi not connected";
        return result;
    }
    
    HTTPClient http;
    String url = "http://" + serverHost + ":" + String(port) + "/api/login";
    
    Serial.println("========================================");
    Serial.println("API Client: Attempting login");
    Serial.print("  URL: ");
    Serial.println(url);
    Serial.print("  Username: ");
    Serial.println(username);
    Serial.print("  PIN: ");
    Serial.println("****");  // Security: Don't log PIN plaintext
    Serial.println("========================================");
    
    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    
    // Create payload as string manually
    String payload = "{\"username\":\"";
    payload += username;
    payload += "\",\"pin\":\"";
    payload += pin;
    payload += "\"}";
    
    Serial.print("API Client: Sending payload: ");
    Serial.println(payload);
    
    int httpResponseCode = http.POST(payload);
    
    if (httpResponseCode > 0) {
        String response = http.getString();
        Serial.print("API Client: Response code: ");
        Serial.println(httpResponseCode);
        Serial.print("API Client: Response body: ");
        Serial.println(response);
        
        // Parse response string manually
        parseLoginResponse(response, result);
        
        Serial.println("========================================");
        Serial.print("API Client: Login result: ");
        Serial.println(result.success ? "SUCCESS" : "FAILED");
        Serial.print("  Message: ");
        Serial.println(result.message);
        Serial.print("  Account exists: ");
        Serial.println(result.accountExists ? "YES" : "NO");
        if (result.success) {
            Serial.print("  User ID: ");
            Serial.println(result.user_id);
            Serial.print("  Username: ");
            Serial.println(result.username);
            Serial.print("  Nickname: ");
            Serial.println(result.nickname);
        }
        Serial.println("========================================");
        
        http.end();
        return result;
    } else {
        Serial.print("API Client: HTTP Error: ");
        Serial.println(httpResponseCode);
        Serial.print("API Client: Error message: ");
        Serial.println(http.errorToString(httpResponseCode));
        result.message = "HTTP Error: " + String(httpResponseCode);
        http.end();
        return result;
    }
}

bool ApiClient::createAccount(const String& username, const String& pin, const String& nickname, const String& serverHost, uint16_t port) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("API Client: WiFi not connected!");
        return false;
    }
    
    HTTPClient http;
    String url = "http://" + serverHost + ":" + String(port) + "/api/register";
    
    Serial.println("========================================");
    Serial.println("API Client: Creating account");
    Serial.print("  URL: ");
    Serial.println(url);
    Serial.print("  Username: ");
    Serial.println(username);
    Serial.print("  PIN: ");
    Serial.println("****");  // Security: Don't log PIN plaintext
    Serial.print("  Nickname: ");
    Serial.println(nickname.length() > 0 ? nickname : "(empty)");
    Serial.println("========================================");
    
    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    
    // Create payload as string manually
    String payload = "{\"username\":\"";
    payload += username;
    payload += "\",\"pin\":\"";
    payload += pin;
    payload += "\",\"nickname\":\"";
    payload += nickname;  // Always include nickname, even if empty
    payload += "\"}";
    
    Serial.print("API Client: Sending payload: ");
    Serial.println(payload);
    
    int httpResponseCode = http.POST(payload);
    
    if (httpResponseCode > 0) {
        String response = http.getString();
        Serial.print("API Client: Response code: ");
        Serial.println(httpResponseCode);
        Serial.print("API Client: Response body: ");
        Serial.println(response);
        
        // Parse response string manually
        bool success = parseRegisterResponse(response);
        
        Serial.println("========================================");
        Serial.print("API Client: Create account result: ");
        Serial.println(success ? "SUCCESS" : "FAILED");
        Serial.println("========================================");
        
        http.end();
        return success;
    } else {
        Serial.print("API Client: HTTP Error: ");
        Serial.println(httpResponseCode);
        Serial.print("API Client: Error message: ");
        Serial.println(http.errorToString(httpResponseCode));
        http.end();
        return false;
    }
}

void ApiClient::printResponse(const String& response) {
    Serial.println("API Response:");
    Serial.println(response);
}

ApiClient::FriendsListResult ApiClient::getFriends(int userId, const String& serverHost, uint16_t port) {
    FriendsListResult result;
    result.success = false;
    result.message = "";
    result.friends = nullptr;
    result.count = 0;
    
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("API Client: WiFi not connected!");
        result.message = "WiFi not connected";
        return result;
    }
    
    HTTPClient http;
    String url = "http://" + serverHost + ":" + String(port) + "/api/friends/" + String(userId);
    Serial.print("API Client: Getting friends from: ");
    Serial.println(url);
    
    http.begin(url);
    http.setTimeout(5000);
    
    int httpCode = http.GET();
    Serial.print("API Client: HTTP response code: ");
    Serial.println(httpCode);
    
    if (httpCode == HTTP_CODE_OK) {
        String response = http.getString();
        Serial.print("API Client: Friends response: ");
        Serial.println(response);
        
        // Parse response manually
        // Expected format: {"success":true,"friends":[{"nickname":"user1","online":false},...],"message":"..."}
        
        // Check success
        int successIdx = response.indexOf("\"success\":");
        if (successIdx >= 0) {
            int valueStart = response.indexOf(":", successIdx) + 1;
            int valueEnd = response.indexOf(",", valueStart);
            if (valueEnd < 0) valueEnd = response.indexOf("}", valueStart);
            String successStr = response.substring(valueStart, valueEnd);
            successStr.trim();
            result.success = (successStr == "true");
        }
        
        if (result.success) {
            // Parse friends array
            int friendsStart = response.indexOf("\"friends\":[");
            if (friendsStart >= 0) {
                friendsStart = response.indexOf("[", friendsStart) + 1;
                int friendsEnd = response.indexOf("]", friendsStart);
                
                if (friendsEnd > friendsStart) {
                    String friendsArray = response.substring(friendsStart, friendsEnd);
                    
                    // Count friends
                    int count = 0;
                    int pos = 0;
                    while ((pos = friendsArray.indexOf("}", pos)) >= 0) {
                        count++;
                        pos++;
                    }
                    
                    result.count = count;
                    if (count > 0) {
                        // Allocate memory for friends
                        result.friends = new FriendEntry[count];
                        
                        // Parse each friend
                        int friendIdx = 0;
                        pos = 0;
                        while (friendIdx < count) {
                            int friendStart = friendsArray.indexOf("{", pos);
                            if (friendStart < 0) break;
                            
                            int friendEnd = friendsArray.indexOf("}", friendStart);
                            if (friendEnd < 0) break;
                            
                            String friendObj = friendsArray.substring(friendStart, friendEnd + 1);
                            
                            // Parse nickname (fallback to username if not found)
                            int nicknameIdx = friendObj.indexOf("\"nickname\":\"");
                            if (nicknameIdx >= 0) {
                                int nameStart = nicknameIdx + 12;
                                int nameEnd = friendObj.indexOf("\"", nameStart);
                                if (nameEnd > nameStart) {
                                    result.friends[friendIdx].nickname = friendObj.substring(nameStart, nameEnd);
                                }
                            } else {
                                // Fallback to username if nickname not found
                                int usernameIdx = friendObj.indexOf("\"username\":\"");
                                if (usernameIdx >= 0) {
                                    int nameStart = usernameIdx + 12;
                                    int nameEnd = friendObj.indexOf("\"", nameStart);
                                    if (nameEnd > nameStart) {
                                        result.friends[friendIdx].nickname = friendObj.substring(nameStart, nameEnd);
                                    }
                                }
                            }
                            
                            // Parse online (default to false)
                            result.friends[friendIdx].online = false;
                            int onlineIdx = friendObj.indexOf("\"online\":");
                            if (onlineIdx >= 0) {
                                int onlineStart = onlineIdx + 9;
                                int onlineEnd = friendObj.indexOf(",", onlineStart);
                                if (onlineEnd < 0) onlineEnd = friendObj.indexOf("}", onlineStart);
                                String onlineStr = friendObj.substring(onlineStart, onlineEnd);
                                onlineStr.trim();
                                result.friends[friendIdx].online = (onlineStr == "true");
                            }
                            
                            friendIdx++;
                            pos = friendEnd + 1;
                        }
                    }
                }
            }
        }
    } else {
        String error = http.getString();
        Serial.print("API Client: Get friends failed: ");
        Serial.println(error);
        result.message = "HTTP error: " + String(httpCode);
    }
    
    http.end();
    return result;
}

ApiClient::GameSessionResult ApiClient::createGameSession(int hostUserId, const String& gameType, int maxPlayers, const int* participantIds, int participantCount, const String& serverHost, uint16_t port) {
    GameSessionResult result;
    result.success = false;
    result.message = "";
    result.sessionId = -1;
    result.status = "";
    result.participantCount = 0;

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("API Client: WiFi not connected!");
        result.message = "WiFi not connected";
        return result;
    }

    HTTPClient http;
    String url = "http://" + serverHost + ":" + String(port) + "/api/games/create";
    Serial.print("API Client: Creating game session at: ");
    Serial.println(url);

    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    http.setTimeout(7000);

    // Build participant_ids array
    String payload = "{\"host_user_id\":";
    payload += String(hostUserId);
    payload += ",\"game_type\":\"";
    payload += gameType;
    payload += "\",\"max_players\":";
    payload += String(maxPlayers);
    payload += ",\"participant_ids\":[";
    for (int i = 0; i < participantCount; i++) {
        payload += String(participantIds[i]);
        if (i < participantCount - 1) payload += ",";
    }
    payload += "]}";

    Serial.print("API Client: Payload: ");
    Serial.println(payload);

    int httpCode = http.POST(payload);
    Serial.print("API Client: HTTP response code: ");
    Serial.println(httpCode);

    if (httpCode > 0) {
        String response = http.getString();
        Serial.print("API Client: Response: ");
        Serial.println(response);
        parseGameSessionResponse(response, result);
    } else {
        Serial.print("API Client: Create game session failed: ");
        Serial.println(http.errorToString(httpCode));
        result.message = "HTTP error: " + String(httpCode);
    }

    http.end();
    return result;
}

ApiClient::GameSessionResult ApiClient::inviteToSession(int sessionId, int hostUserId, const int* participantIds, int participantCount, const String& serverHost, uint16_t port) {
    GameSessionResult result;
    result.success = false;
    result.message = "";
    result.sessionId = sessionId;
    result.status = "";
    result.participantCount = 0;

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("API Client: WiFi not connected!");
        result.message = "WiFi not connected";
        return result;
    }

    HTTPClient http;
    String url = "http://" + serverHost + ":" + String(port) + "/api/games/" + String(sessionId) + "/invite";
    Serial.print("API Client: Inviting to session: ");
    Serial.println(url);

    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    http.setTimeout(7000);

    String payload = "{\"host_user_id\":";
    payload += String(hostUserId);
    payload += ",\"participant_ids\":[";
    for (int i = 0; i < participantCount; i++) {
        payload += String(participantIds[i]);
        if (i < participantCount - 1) payload += ",";
    }
    payload += "]}";

    Serial.print("API Client: Payload: ");
    Serial.println(payload);

    int httpCode = http.POST(payload);
    Serial.print("API Client: HTTP response code: ");
    Serial.println(httpCode);

    if (httpCode > 0) {
        String response = http.getString();
        Serial.print("API Client: Response: ");
        Serial.println(response);
        parseGameSessionResponse(response, result);
    } else {
        Serial.print("API Client: Invite to session failed: ");
        Serial.println(http.errorToString(httpCode));
        result.message = "HTTP error: " + String(httpCode);
    }

    http.end();
    return result;
}

ApiClient::GameSessionResult ApiClient::respondGameInvite(int sessionId, int userId, bool accept, const String& serverHost, uint16_t port) {
    GameSessionResult result;
    result.success = false;
    result.message = "";
    result.sessionId = sessionId;
    result.status = "";
    result.participantCount = 0;

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("API Client: WiFi not connected!");
        result.message = "WiFi not connected";
        return result;
    }

    HTTPClient http;
    String url = "http://" + serverHost + ":" + String(port) + "/api/games/" + String(sessionId) + "/respond";
    Serial.print("API Client: Responding to game invite: ");
    Serial.println(url);

    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    http.setTimeout(7000);

    String payload = "{\"user_id\":";
    payload += String(userId);
    payload += ",\"accept\":";
    payload += accept ? "true" : "false";
    payload += ",\"ready_on_accept\":";
    payload += accept ? "true" : "false";
    payload += "}";

    Serial.print("API Client: Payload: ");
    Serial.println(payload);

    int httpCode = http.POST(payload);
    Serial.print("API Client: HTTP response code: ");
    Serial.println(httpCode);

    if (httpCode > 0) {
        String response = http.getString();
        Serial.print("API Client: Response: ");
        Serial.println(response);
        parseGameSessionResponse(response, result);
    } else {
        Serial.print("API Client: Respond invite failed: ");
        Serial.println(http.errorToString(httpCode));
        result.message = "HTTP error: " + String(httpCode);
    }

    http.end();
    return result;
}

ApiClient::GameSessionResult ApiClient::setGameReady(int sessionId, int userId, bool ready, const String& serverHost, uint16_t port) {
    GameSessionResult result;
    result.success = false;
    result.message = "";
    result.sessionId = sessionId;
    result.status = "";
    result.participantCount = 0;

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("API Client: WiFi not connected!");
        result.message = "WiFi not connected";
        return result;
    }

    HTTPClient http;
    String url = "http://" + serverHost + ":" + String(port) + "/api/games/" + String(sessionId) + "/ready";
    Serial.print("API Client: Setting ready state at: ");
    Serial.println(url);

    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    http.setTimeout(7000);

    String payload = "{\"user_id\":";
    payload += String(userId);
    payload += ",\"ready\":";
    payload += ready ? "true" : "false";
    payload += "}";

    Serial.print("API Client: Payload: ");
    Serial.println(payload);

    int httpCode = http.POST(payload);
    Serial.print("API Client: HTTP response code: ");
    Serial.println(httpCode);

    if (httpCode > 0) {
        String response = http.getString();
        Serial.print("API Client: Response: ");
        Serial.println(response);
        parseGameSessionResponse(response, result);
    } else {
        Serial.print("API Client: Ready toggle failed: ");
        Serial.println(http.errorToString(httpCode));
        result.message = "HTTP error: " + String(httpCode);
    }

    http.end();
    return result;
}

ApiClient::GameSessionResult ApiClient::leaveGameSession(int sessionId, int userId, const String& serverHost, uint16_t port) {
    GameSessionResult result;
    result.success = false;
    result.message = "";
    result.sessionId = sessionId;
    result.status = "";
    result.participantCount = 0;

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("API Client: WiFi not connected!");
        result.message = "WiFi not connected";
        return result;
    }

    HTTPClient http;
    String url = "http://" + serverHost + ":" + String(port) + "/api/games/" + String(sessionId) + "/leave";
    Serial.print("API Client: Leaving game session: ");
    Serial.println(url);

    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    http.setTimeout(7000);

    String payload = "{\"user_id\":";
    payload += String(userId);
    payload += "}";

    Serial.print("API Client: Payload: ");
    Serial.println(payload);

    int httpCode = http.POST(payload);
    Serial.print("API Client: HTTP response code: ");
    Serial.println(httpCode);

    if (httpCode > 0) {
        String response = http.getString();
        Serial.print("API Client: Response: ");
        Serial.println(response);
        parseGameSessionResponse(response, result);
    } else {
        Serial.print("API Client: Leave session failed: ");
        Serial.println(http.errorToString(httpCode));
        result.message = "HTTP error: " + String(httpCode);
    }

    http.end();
    return result;
}

ApiClient::GameMoveResult ApiClient::submitGameMove(int sessionId, int userId, int row, int col, const String& serverHost, uint16_t port) {
    GameMoveResult result;
    result.success = false;
    result.message = "";
    result.moveId = -1;
    result.gameStatus = "";
    result.winnerId = -1;

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("API Client: WiFi not connected!");
        result.message = "WiFi not connected";
        return result;
    }

    HTTPClient http;
    String url = "http://" + serverHost + ":" + String(port) + "/api/games/" + String(sessionId) + "/move";
    Serial.print("API Client: Submitting game move: ");
    Serial.println(url);

    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    http.setTimeout(7000);

    String payload = "{\"user_id\":";
    payload += String(userId);
    payload += ",\"row\":";
    payload += String(row);
    payload += ",\"col\":";
    payload += String(col);
    payload += "}";

    Serial.print("API Client: Payload: ");
    Serial.println(payload);

    int httpCode = http.POST(payload);
    Serial.print("API Client: HTTP response code: ");
    Serial.println(httpCode);

    if (httpCode > 0) {
        String response = http.getString();
        Serial.print("API Client: Response: ");
        Serial.println(response);
        
        // Parse response
        int successIdx = response.indexOf("\"success\":");
        if (successIdx >= 0) {
            int valueStart = response.indexOf(":", successIdx) + 1;
            int valueEnd = response.indexOf(",", valueStart);
            if (valueEnd < 0) valueEnd = response.indexOf("}", valueStart);
            String successStr = response.substring(valueStart, valueEnd);
            successStr.trim();
            result.success = (successStr == "true");
        }
        
        int messageIdx = response.indexOf("\"message\":\"");
        if (messageIdx >= 0) {
            int valueStart = messageIdx + 11;
            int valueEnd = response.indexOf("\"", valueStart);
            if (valueEnd > valueStart) {
                result.message = response.substring(valueStart, valueEnd);
            }
        }
        
        if (result.success) {
            int moveIdIdx = response.indexOf("\"move_id\":");
            if (moveIdIdx >= 0) {
                int valueStart = response.indexOf(":", moveIdIdx) + 1;
                int valueEnd = response.indexOf(",", valueStart);
                if (valueEnd < 0) valueEnd = response.indexOf("}", valueStart);
                String moveIdStr = response.substring(valueStart, valueEnd);
                moveIdStr.trim();
                result.moveId = moveIdStr.toInt();
            }
            
            int statusIdx = response.indexOf("\"game_status\":\"");
            if (statusIdx >= 0) {
                int valueStart = statusIdx + 15;
                int valueEnd = response.indexOf("\"", valueStart);
                if (valueEnd > valueStart) {
                    result.gameStatus = response.substring(valueStart, valueEnd);
                }
            }
            
            int winnerIdx = response.indexOf("\"winner_id\":");
            if (winnerIdx >= 0) {
                int valueStart = response.indexOf(":", winnerIdx) + 1;
                int valueEnd = response.indexOf(",", valueStart);
                if (valueEnd < 0) valueEnd = response.indexOf("}", valueStart);
                String winnerStr = response.substring(valueStart, valueEnd);
                winnerStr.trim();
                if (winnerStr != "null" && winnerStr.length() > 0) {
                    result.winnerId = winnerStr.toInt();
                }
            }
        }
    } else {
        Serial.print("API Client: Submit move failed: ");
        Serial.println(http.errorToString(httpCode));
        result.message = "HTTP error: " + String(httpCode);
    }

    http.end();
    return result;
}

ApiClient::GameStateResult ApiClient::getGameState(int sessionId, const String& serverHost, uint16_t port) {
    GameStateResult result;
    result.success = false;
    result.message = "";
    result.sessionId = sessionId;
    result.gameType = "";
    result.status = "";
    result.currentTurn = -1;
    result.moveCount = 0;
    result.hostName = "";
    result.guestName = "";
    result.hostId = -1;
    result.guestId = -1;

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("API Client: WiFi not connected!");
        result.message = "WiFi not connected";
        return result;
    }

    HTTPClient http;
    String url = "http://" + serverHost + ":" + String(port) + "/api/games/" + String(sessionId) + "/state";
    Serial.print("API Client: Getting game state: ");
    Serial.println(url);

    http.begin(url);
    http.setTimeout(7000);

    int httpCode = http.GET();
    Serial.print("API Client: HTTP response code: ");
    Serial.println(httpCode);

    if (httpCode > 0) {
        String response = http.getString();
        Serial.print("API Client: Response: ");
        Serial.println(response);
        
        // Parse response
        int successIdx = response.indexOf("\"success\":");
        if (successIdx >= 0) {
            int valueStart = response.indexOf(":", successIdx) + 1;
            int valueEnd = response.indexOf(",", valueStart);
            if (valueEnd < 0) valueEnd = response.indexOf("}", valueStart);
            String successStr = response.substring(valueStart, valueEnd);
            successStr.trim();
            result.success = (successStr == "true");
        }
        
        if (result.success) {
            int statusIdx = response.indexOf("\"status\":\"");
            if (statusIdx >= 0) {
                int valueStart = statusIdx + 10;
                int valueEnd = response.indexOf("\"", valueStart);
                if (valueEnd > valueStart) {
                    result.status = response.substring(valueStart, valueEnd);
                }
            }
            
            int turnIdx = response.indexOf("\"current_turn\":");
            if (turnIdx >= 0) {
                int valueStart = response.indexOf(":", turnIdx) + 1;
                int valueEnd = response.indexOf(",", valueStart);
                if (valueEnd < 0) valueEnd = response.indexOf("}", valueStart);
                String turnStr = response.substring(valueStart, valueEnd);
                turnStr.trim();
                result.currentTurn = turnStr.toInt();
            }
            
            int moveCountIdx = response.indexOf("\"move_count\":");
            if (moveCountIdx >= 0) {
                int valueStart = response.indexOf(":", moveCountIdx) + 1;
                int valueEnd = response.indexOf(",", valueStart);
                if (valueEnd < 0) valueEnd = response.indexOf("}", valueStart);
                String moveCountStr = response.substring(valueStart, valueEnd);
                moveCountStr.trim();
                result.moveCount = moveCountStr.toInt();
            }
            
            // Parse players object
            int playersIdx = response.indexOf("\"players\":{");
            if (playersIdx >= 0) {
                // Parse host
                int hostIdx = response.indexOf("\"host\":{", playersIdx);
                if (hostIdx >= 0) {
                    int hostIdIdx = response.indexOf("\"user_id\":", hostIdx);
                    if (hostIdIdx >= 0) {
                        int valueStart = response.indexOf(":", hostIdIdx) + 1;
                        int valueEnd = response.indexOf(",", valueStart);
                        if (valueEnd < 0) valueEnd = response.indexOf("}", valueStart);
                        String hostIdStr = response.substring(valueStart, valueEnd);
                        hostIdStr.trim();
                        result.hostId = hostIdStr.toInt();
                    }
                    
                    int hostNameIdx = response.indexOf("\"name\":\"", hostIdx);
                    if (hostNameIdx >= 0) {
                        int valueStart = hostNameIdx + 8;
                        int valueEnd = response.indexOf("\"", valueStart);
                        if (valueEnd > valueStart) {
                            result.hostName = response.substring(valueStart, valueEnd);
                        }
                    }
                }
                
                // Parse guest
                int guestIdx = response.indexOf("\"guest\":", playersIdx);
                if (guestIdx >= 0 && response.indexOf("null", guestIdx) < 0) {
                    int guestIdIdx = response.indexOf("\"user_id\":", guestIdx);
                    if (guestIdIdx >= 0) {
                        int valueStart = response.indexOf(":", guestIdIdx) + 1;
                        int valueEnd = response.indexOf(",", valueStart);
                        if (valueEnd < 0) valueEnd = response.indexOf("}", valueStart);
                        String guestIdStr = response.substring(valueStart, valueEnd);
                        guestIdStr.trim();
                        result.guestId = guestIdStr.toInt();
                    }
                    
                    int guestNameIdx = response.indexOf("\"name\":\"", guestIdx);
                    if (guestNameIdx >= 0) {
                        int valueStart = guestNameIdx + 8;
                        int valueEnd = response.indexOf("\"", valueStart);
                        if (valueEnd > valueStart) {
                            result.guestName = response.substring(valueStart, valueEnd);
                        }
                    }
                }
            }
        } else {
            int messageIdx = response.indexOf("\"message\":\"");
            if (messageIdx >= 0) {
                int valueStart = messageIdx + 11;
                int valueEnd = response.indexOf("\"", valueStart);
                if (valueEnd > valueStart) {
                    result.message = response.substring(valueStart, valueEnd);
                }
            }
        }
    } else {
        Serial.print("API Client: Get game state failed: ");
        Serial.println(http.errorToString(httpCode));
        result.message = "HTTP error: " + String(httpCode);
    }

    http.end();
    return result;
}

String ApiClient::getFriendsList(int userId, const String& serverHost, uint16_t port) {
    String result = "";
    
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("API Client: WiFi not connected!");
        return result;
    }
    
    HTTPClient http;
    String url = "http://" + serverHost + ":" + String(port) + "/api/friends/" + String(userId) + "/list";
    Serial.print("API Client: Getting friends list (string format) from: ");
    Serial.println(url);
    
    http.begin(url);
    http.setTimeout(5000);
    
    int httpCode = http.GET();
    Serial.print("API Client: HTTP response code: ");
    Serial.println(httpCode);
    Serial.print("API Client: Request URL: ");
    Serial.println(url);
    
    if (httpCode == HTTP_CODE_OK) {
        result = http.getString();
        Serial.print("API Client: Friends list string received: ");
        Serial.println(result);
        Serial.print("API Client: String length: ");
        Serial.println(result.length());
        Serial.print("API Client: Is empty? ");
        Serial.println(result.length() == 0 ? "YES" : "NO");
    } else {
        String error = http.getString();
        Serial.print("API Client: Get friends list failed: ");
        Serial.println(error);
        Serial.print("API Client: HTTP error code: ");
        Serial.println(httpCode);
    }
    
    http.end();
    return result;
}

ApiClient::NotificationsResult ApiClient::getNotifications(int userId, const String& serverHost, uint16_t port) {
    NotificationsResult result;
    result.success = false;
    result.message = "";
    result.notifications = nullptr;
    result.count = 0;
    
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("API Client: WiFi not connected!");
        result.message = "WiFi not connected";
        return result;
    }
    
    HTTPClient http;
    String url = "http://" + serverHost + ":" + String(port) + "/api/notifications/" + String(userId);
    Serial.print("API Client: Getting notifications from: ");
    Serial.println(url);
    
    http.begin(url);
    http.setTimeout(5000);
    
    int httpCode = http.GET();
    Serial.print("API Client: HTTP response code: ");
    Serial.println(httpCode);
    
    if (httpCode == HTTP_CODE_OK) {
        String response = http.getString();
        Serial.print("API Client: Notifications response: ");
        Serial.println(response);
        
        // Parse response manually
        // Expected format: {"success":true,"notifications":[{"id":1,"type":"friend_request","message":"...","timestamp":"...","read":false},...],"message":"..."}
        
        // Check success
        int successIdx = response.indexOf("\"success\":");
        if (successIdx >= 0) {
            int valueStart = response.indexOf(":", successIdx) + 1;
            int valueEnd = response.indexOf(",", valueStart);
            if (valueEnd < 0) valueEnd = response.indexOf("}", valueStart);
            String successStr = response.substring(valueStart, valueEnd);
            successStr.trim();
            result.success = (successStr == "true");
        }
        
        // Get message
        int messageIdx = response.indexOf("\"message\":\"");
        if (messageIdx >= 0) {
            int valueStart = messageIdx + 11; // length of "message":"
            int valueEnd = response.indexOf("\"", valueStart);
            if (valueEnd > valueStart) {
                result.message = response.substring(valueStart, valueEnd);
            }
        }
        
        if (result.success) {
            // Parse notifications array
            int notificationsStart = response.indexOf("\"notifications\":[");
            if (notificationsStart >= 0) {
                notificationsStart = response.indexOf("[", notificationsStart) + 1;
                int notificationsEnd = response.indexOf("]", notificationsStart);
                
                if (notificationsEnd > notificationsStart) {
                    String notificationsArray = response.substring(notificationsStart, notificationsEnd);
                    
                    // Count notifications
                    int count = 0;
                    int pos = 0;
                    while ((pos = notificationsArray.indexOf("}", pos)) >= 0) {
                        count++;
                        pos++;
                    }
                    
                    result.count = count;
                    if (count > 0) {
                        // Allocate memory for notifications
                        result.notifications = new NotificationEntry[count];
                        
                        // Parse each notification
                        int notificationIdx = 0;
                        pos = 0;
                        while (notificationIdx < count) {
                            int notificationStart = notificationsArray.indexOf("{", pos);
                            if (notificationStart < 0) break;
                            
                            int notificationEnd = notificationsArray.indexOf("}", notificationStart);
                            if (notificationEnd < 0) break;
                            
                            String notificationObj = notificationsArray.substring(notificationStart, notificationEnd + 1);
                            
                            // Parse id
                            result.notifications[notificationIdx].id = -1;
                            int idIdx = notificationObj.indexOf("\"id\":");
                            if (idIdx >= 0) {
                                int idStart = idIdx + 5;
                                int idEnd = notificationObj.indexOf(",", idStart);
                                if (idEnd < 0) idEnd = notificationObj.indexOf("}", idStart);
                                String idStr = notificationObj.substring(idStart, idEnd);
                                idStr.trim();
                                if (idStr.length() > 0 && idStr != "null") {
                                    result.notifications[notificationIdx].id = idStr.toInt();
                                }
                            }
                            
                            // Parse type
                            result.notifications[notificationIdx].type = "";
                            int typeIdx = notificationObj.indexOf("\"type\":\"");
                            if (typeIdx >= 0) {
                                int typeStart = typeIdx + 8; // length of "type":"
                                int typeEnd = notificationObj.indexOf("\"", typeStart);
                                if (typeEnd > typeStart) {
                                    result.notifications[notificationIdx].type = notificationObj.substring(typeStart, typeEnd);
                                }
                            }
                            
                            // Parse message
                            result.notifications[notificationIdx].message = "";
                            int msgIdx = notificationObj.indexOf("\"message\":\"");
                            if (msgIdx >= 0) {
                                int msgStart = msgIdx + 11; // length of "message":"
                                int msgEnd = notificationObj.indexOf("\"", msgStart);
                                if (msgEnd > msgStart) {
                                    result.notifications[notificationIdx].message = notificationObj.substring(msgStart, msgEnd);
                                }
                            }
                            
                            // Parse timestamp
                            result.notifications[notificationIdx].timestamp = "";
                            int timestampIdx = notificationObj.indexOf("\"timestamp\":\"");
                            if (timestampIdx >= 0) {
                                int timestampStart = timestampIdx + 13; // length of "timestamp":"
                                int timestampEnd = notificationObj.indexOf("\"", timestampStart);
                                if (timestampEnd > timestampStart) {
                                    result.notifications[notificationIdx].timestamp = notificationObj.substring(timestampStart, timestampEnd);
                                }
                            }
                            
                            // Parse read (default to false)
                            result.notifications[notificationIdx].read = false;
                            int readIdx = notificationObj.indexOf("\"read\":");
                            if (readIdx >= 0) {
                                int readStart = readIdx + 7;
                                int readEnd = notificationObj.indexOf(",", readStart);
                                if (readEnd < 0) readEnd = notificationObj.indexOf("}", readStart);
                                String readStr = notificationObj.substring(readStart, readEnd);
                                readStr.trim();
                                result.notifications[notificationIdx].read = (readStr == "true");
                            }
                            
                            // Parse related_id (optional field)
                            result.notifications[notificationIdx].relatedId = -1;
                            int relatedIdx = notificationObj.indexOf("\"related_id\":");
                            if (relatedIdx >= 0) {
                                int relatedStart = relatedIdx + 13;
                                int relatedEnd = notificationObj.indexOf(",", relatedStart);
                                if (relatedEnd < 0) relatedEnd = notificationObj.indexOf("}", relatedStart);
                                String relatedStr = notificationObj.substring(relatedStart, relatedEnd);
                                relatedStr.trim();
                                if (relatedStr.length() > 0 && relatedStr != "null") {
                                    result.notifications[notificationIdx].relatedId = relatedStr.toInt();
                                }
                            }
                            
                            notificationIdx++;
                            pos = notificationEnd + 1;
                        }
                    }
                } else {
                    // Empty array
                    result.count = 0;
                    result.notifications = nullptr;
                }
            } else {
                // No notifications array found, assume empty
                result.count = 0;
                result.notifications = nullptr;
            }
        }
    } else {
        String error = http.getString();
        Serial.print("API Client: Get notifications failed: ");
        Serial.println(error);
        result.message = "HTTP error: " + String(httpCode);
    }
    
    http.end();
    return result;
}

ApiClient::FriendRequestResult ApiClient::sendFriendRequest(int fromUserId, const String& toNickname, const String& serverHost, uint16_t port) {
    FriendRequestResult result;
    result.success = false;
    result.message = "";
    result.requestId = -1;
    result.friendshipId = -1;
    result.status = "";
    
    // Pre-flight validation
    if (fromUserId <= 0) {
        Serial.println("API Client: Invalid user ID!");
        result.message = "Invalid user ID";
        return result;
    }
    
    String trimmedNickname = toNickname;
    trimmedNickname.trim();
    trimmedNickname.replace("\r", "");
    trimmedNickname.replace("\n", "");
    // Defensive: allow users to paste quotes like "Admin"
    while (trimmedNickname.length() > 0 && (trimmedNickname.charAt(0) == '"' || trimmedNickname.charAt(0) == '\'')) {
        trimmedNickname.remove(0, 1);
        trimmedNickname.trim();
    }
    while (trimmedNickname.length() > 0 && (trimmedNickname.charAt(trimmedNickname.length() - 1) == '"' || trimmedNickname.charAt(trimmedNickname.length() - 1) == '\'')) {
        trimmedNickname.remove(trimmedNickname.length() - 1, 1);
        trimmedNickname.trim();
    }
    if (trimmedNickname.length() == 0) {
        Serial.println("API Client: Empty nickname!");
        result.message = "Nickname cannot be empty";
        return result;
    }
    
    if (trimmedNickname.length() > 255) {
        Serial.println("API Client: Nickname too long!");
        result.message = "Nickname is too long (max 255 characters)";
        return result;
    }
    
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("API Client: WiFi not connected!");
        result.message = "WiFi not connected. Please check your connection.";
        return result;
    }
    
    if (serverHost.length() == 0) {
        Serial.println("API Client: Server host not configured!");
        result.message = "Server not configured";
        return result;
    }
    
    HTTPClient http;
    String url = "http://" + serverHost + ":" + String(port) + "/api/friend-requests/send";
    Serial.print("API Client: Sending friend request to: ");
    Serial.println(url);
    
    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    http.setTimeout(10000);  // Increased timeout for reliability (10 seconds)
    
    // Create payload with trimmed nickname and escape JSON special characters
    String escapedNickname = trimmedNickname;
    // Basic JSON escaping (replace " with \", \ with \\, newlines, etc.)
    escapedNickname.replace("\\", "\\\\");
    escapedNickname.replace("\"", "\\\"");
    escapedNickname.replace("\n", "\\n");
    escapedNickname.replace("\r", "\\r");
    escapedNickname.replace("\t", "\\t");
    
    String payload = "{\"from_user_id\":";
    payload += String(fromUserId);
    payload += ",\"to_nickname\":\"";
    payload += escapedNickname;
    payload += "\"}";
    
    Serial.print("API Client: Payload: ");
    Serial.println(payload);
    
    int httpCode = http.POST(payload);
    Serial.print("API Client: HTTP response code: ");
    Serial.println(httpCode);
    
    if (httpCode == HTTP_CODE_OK || httpCode == 200) {
        String response = http.getString();
        Serial.print("API Client: Response: ");
        Serial.println(response);
        parseFriendRequestResponse(response, result);
    } else if (httpCode == HTTP_CODE_BAD_REQUEST || httpCode == 400) {
        // Bad request - try to parse error message
        String error = http.getString();
        Serial.print("API Client: Bad request (400): ");
        Serial.println(error);
        if (error.length() > 0) {
            parseFriendRequestResponse(error, result);
        } else {
            result.message = "Invalid request. Please check the nickname and try again.";
        }
    } else if (httpCode == HTTP_CODE_NOT_FOUND || httpCode == 404) {
        String error = http.getString();
        Serial.print("API Client: Not found (404): ");
        Serial.println(error);
        if (error.length() > 0) {
            parseFriendRequestResponse(error, result);
        } else {
            result.message = "User not found";
        }
    } else if (httpCode == HTTP_CODE_CONFLICT || httpCode == 409) {
        // Conflict - duplicate request, already friends, etc.
        String error = http.getString();
        Serial.print("API Client: Conflict (409): ");
        Serial.println(error);
        if (error.length() > 0) {
            parseFriendRequestResponse(error, result);
        } else {
            result.message = "Friend request conflict. You may already be friends or have a pending request.";
        }
    } else if (httpCode < 0) {
        // Network error (timeout, connection failed, etc.)
        String errorMsg = http.errorToString(httpCode);
        Serial.print("API Client: Network error: ");
        Serial.println(errorMsg);
        result.message = "Network error: " + errorMsg + ". Please check your connection and try again.";
    } else {
        // Other HTTP error
        String error = http.getString();
        Serial.print("API Client: HTTP error (");
        Serial.print(httpCode);
        Serial.print("): ");
        Serial.println(error);
        
        // Try to parse error message from response
        if (error.length() > 0) {
            parseFriendRequestResponse(error, result);
        } else {
            result.message = "Server error (HTTP " + String(httpCode) + "). Please try again later.";
        }
    }
    
    http.end();
    return result;
}

ApiClient::FriendRequestResult ApiClient::acceptFriendRequest(int userId, int notificationId, const String& serverHost, uint16_t port) {
    FriendRequestResult result;
    result.success = false;
    result.message = "";
    result.requestId = -1;
    result.friendshipId = -1;
    result.status = "";
    
    // Input validation
    if (userId <= 0) {
        Serial.println("API Client: Invalid user_id for accept friend request");
        result.message = "Invalid user ID";
        return result;
    }
    
    if (notificationId <= 0) {
        Serial.println("API Client: Invalid notification_id for accept friend request");
        result.message = "Invalid notification ID";
        return result;
    }
    
    if (serverHost.length() == 0) {
        Serial.println("API Client: Empty server host for accept friend request");
        result.message = "Server host not configured";
        return result;
    }
    
    // Check WiFi connection
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("API Client: WiFi not connected!");
        result.message = "WiFi not connected. Please check your connection.";
        return result;
    }
    
    HTTPClient http;
    String url = "http://" + serverHost + ":" + String(port) + "/api/friend-requests/accept";
    Serial.print("API Client: Accepting friend request at: ");
    Serial.println(url);
    
    // Configure HTTP client with retry and timeout settings
    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    http.setTimeout(10000);  // Increased timeout for better reliability
    http.setReuse(true);  // Reuse connection if possible
    
    // Create payload with validation
    String payload = "{\"user_id\":";
    payload += String(userId);
    payload += ",\"notification_id\":";
    payload += String(notificationId);
    payload += "}";
    
    Serial.print("API Client: Payload: ");
    Serial.println(payload);
    
    // Retry logic for network errors
    int maxRetries = 3;
    int retryDelay = 500;  // 500ms between retries
    int httpCode = 0;
    String response = "";
    
    for (int attempt = 1; attempt <= maxRetries; attempt++) {
        if (attempt > 1) {
            Serial.print("API Client: Retry attempt ");
            Serial.print(attempt);
            Serial.print(" of ");
            Serial.println(maxRetries);
            delay(retryDelay * attempt);  // Exponential backoff
        }
        
        httpCode = http.POST(payload);
        Serial.print("API Client: HTTP response code: ");
        Serial.println(httpCode);
        
        if (httpCode > 0) {
            response = http.getString();
            break;  // Success, exit retry loop
        } else {
            Serial.print("API Client: Network error on attempt ");
            Serial.print(attempt);
            Serial.print(": ");
            Serial.println(httpCode);
            
            if (attempt < maxRetries) {
                // Continue to retry
                continue;
            } else {
                // All retries failed
                result.message = "Network error: Unable to connect to server. Please check your connection.";
                http.end();
                return result;
            }
        }
    }
    
    // Process response
    if (httpCode == HTTP_CODE_OK || httpCode == 200) {
        Serial.print("API Client: Response: ");
        Serial.println(response);
        parseFriendRequestResponse(response, result);
        
        if (!result.success && result.message.length() == 0) {
            result.message = "Failed to accept friend request. Please try again.";
        }
    } else if (httpCode == HTTP_CODE_BAD_REQUEST || httpCode == 400) {
        // Bad request - try to parse error message
        Serial.print("API Client: Bad request (400): ");
        Serial.println(response);
        if (response.length() > 0) {
            parseFriendRequestResponse(response, result);
        }
        if (result.message.length() == 0) {
            result.message = "Invalid request. Please check your input.";
        }
    } else if (httpCode == HTTP_CODE_NOT_FOUND || httpCode == 404) {
        result.message = "Friend request not found. It may have been cancelled or already processed.";
    } else if (httpCode == HTTP_CODE_CONFLICT || httpCode == 409) {
        // Conflict - request may already be accepted/rejected
        if (response.length() > 0) {
            parseFriendRequestResponse(response, result);
        }
        if (result.message.length() == 0) {
            result.message = "Friend request status has changed. Please refresh and try again.";
        }
    } else if (httpCode == HTTP_CODE_INTERNAL_SERVER_ERROR || httpCode == 500) {
        result.message = "Server error. Please try again later.";
        Serial.print("API Client: Server error (500): ");
        Serial.println(response);
    } else if (httpCode > 0) {
        // Other HTTP error codes
        Serial.print("API Client: HTTP error (");
        Serial.print(httpCode);
        Serial.print("): ");
        Serial.println(response);
        
        if (response.length() > 0) {
            parseFriendRequestResponse(response, result);
        }
        
        if (result.message.length() == 0) {
            result.message = "Server error (HTTP " + String(httpCode) + "). Please try again later.";
        }
    } else {
        // Network error (httpCode <= 0)
        result.message = "Network error: Unable to connect to server. Please check your connection.";
    }
    
    http.end();
    return result;
}

ApiClient::FriendRequestResult ApiClient::rejectFriendRequest(int userId, int notificationId, const String& serverHost, uint16_t port) {
    FriendRequestResult result;
    result.success = false;
    result.message = "";
    result.requestId = -1;
    result.friendshipId = -1;
    result.status = "";
    
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("API Client: WiFi not connected!");
        result.message = "WiFi not connected";
        return result;
    }
    
    HTTPClient http;
    String url = "http://" + serverHost + ":" + String(port) + "/api/friend-requests/reject";
    Serial.print("API Client: Rejecting friend request at: ");
    Serial.println(url);
    
    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    http.setTimeout(5000);
    
    // Create payload
    String payload = "{\"user_id\":";
    payload += String(userId);
    payload += ",\"notification_id\":";
    payload += String(notificationId);
    payload += "}";
    
    Serial.print("API Client: Payload: ");
    Serial.println(payload);
    
    int httpCode = http.POST(payload);
    Serial.print("API Client: HTTP response code: ");
    Serial.println(httpCode);
    
    if (httpCode == HTTP_CODE_OK || httpCode == 200) {
        String response = http.getString();
        Serial.print("API Client: Response: ");
        Serial.println(response);
        parseFriendRequestResponse(response, result);
    } else {
        String error = http.getString();
        Serial.print("API Client: Reject friend request failed: ");
        Serial.println(error);
        result.message = "HTTP error: " + String(httpCode);
        if (error.length() > 0) {
            parseFriendRequestResponse(error, result);
        }
    }
    
    http.end();
    return result;
}

ApiClient::FriendRequestResult ApiClient::cancelFriendRequest(int fromUserId, int toUserId, const String& serverHost, uint16_t port) {
    FriendRequestResult result;
    result.success = false;
    result.message = "";
    result.requestId = -1;
    result.friendshipId = -1;
    result.status = "";
    
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("API Client: WiFi not connected!");
        result.message = "WiFi not connected";
        return result;
    }
    
    HTTPClient http;
    String url = "http://" + serverHost + ":" + String(port) + "/api/friend-requests/cancel";
    Serial.print("API Client: Cancelling friend request at: ");
    Serial.println(url);
    
    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    http.setTimeout(5000);
    
    // Create payload
    String payload = "{\"from_user_id\":";
    payload += String(fromUserId);
    payload += ",\"to_user_id\":";
    payload += String(toUserId);
    payload += "}";
    
    Serial.print("API Client: Payload: ");
    Serial.println(payload);
    
    int httpCode = http.POST(payload);
    Serial.print("API Client: HTTP response code: ");
    Serial.println(httpCode);
    
    if (httpCode == HTTP_CODE_OK || httpCode == 200) {
        String response = http.getString();
        Serial.print("API Client: Response: ");
        Serial.println(response);
        parseFriendRequestResponse(response, result);
    } else {
        String error = http.getString();
        Serial.print("API Client: Cancel friend request failed: ");
        Serial.println(error);
        result.message = "HTTP error: " + String(httpCode);
        if (error.length() > 0) {
            parseFriendRequestResponse(error, result);
        }
    }
    
    http.end();
    return result;
}

ApiClient::FriendRequestResult ApiClient::removeFriend(int userId, int friendId, const String& serverHost, uint16_t port) {
    FriendRequestResult result;
    result.success = false;
    result.message = "";
    result.requestId = -1;
    result.friendshipId = -1;
    result.status = "";
    
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("API Client: WiFi not connected!");
        result.message = "WiFi not connected";
        return result;
    }
    
    HTTPClient http;
    String url = "http://" + serverHost + ":" + String(port) + "/api/friends/" + String(userId) + "/" + String(friendId);
    Serial.print("API Client: Removing friend at: ");
    Serial.println(url);
    
    http.begin(url);
    http.setTimeout(5000);
    
    int httpCode = http.sendRequest("DELETE");
    Serial.print("API Client: HTTP response code: ");
    Serial.println(httpCode);
    
    if (httpCode == HTTP_CODE_OK || httpCode == 200) {
        String response = http.getString();
        Serial.print("API Client: Response: ");
        Serial.println(response);
        parseFriendRequestResponse(response, result);
    } else {
        String error = http.getString();
        Serial.print("API Client: Remove friend failed: ");
        Serial.println(error);
        result.message = "HTTP error: " + String(httpCode);
        if (error.length() > 0) {
            parseFriendRequestResponse(error, result);
        }
    }
    
    http.end();
    return result;
}