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
