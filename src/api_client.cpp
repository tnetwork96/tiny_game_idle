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

ApiClient::LoginResult ApiClient::checkLogin(const String& username, const String& pin, const String& serverHost, uint16_t port) {
    LoginResult result;
    result.success = false;
    result.accountExists = false;
    result.user_id = -1;
    result.username = "";
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

bool ApiClient::createAccount(const String& username, const String& pin, const String& serverHost, uint16_t port) {
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
        // Expected format: {"success":true,"friends":[{"username":"user1","online":false},...],"message":"..."}
        
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
                            
                            // Parse username
                            int usernameIdx = friendObj.indexOf("\"username\":\"");
                            if (usernameIdx >= 0) {
                                int nameStart = usernameIdx + 12;
                                int nameEnd = friendObj.indexOf("\"", nameStart);
                                if (nameEnd > nameStart) {
                                    result.friends[friendIdx].username = friendObj.substring(nameStart, nameEnd);
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