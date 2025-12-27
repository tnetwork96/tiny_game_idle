#ifndef API_CLIENT_H
#define API_CLIENT_H

#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFi.h>

class ApiClient {
public:
    struct LoginResult {
        bool success;
        bool accountExists;
        String message;
        int user_id;
        String username;
    };
    
    struct FriendEntry {
        String username;
        bool online;
    };
    
    struct FriendsListResult {
        bool success;
        String message;
        FriendEntry* friends;
        int count;
    };
    
    static LoginResult checkLogin(const String& username, const String& pin, const String& serverHost, uint16_t port);
    static bool createAccount(const String& username, const String& pin, const String& serverHost, uint16_t port);
    static FriendsListResult getFriends(int userId, const String& serverHost, uint16_t port);
    static String getFriendsList(int userId, const String& serverHost, uint16_t port);  // Returns simple string format: "user1,0|user2,1|..."
    static void printResponse(const String& response);
    
private:
    // Helper methods to parse response strings manually
    static void parseLoginResponse(const String& response, LoginResult& result);
    static bool parseRegisterResponse(const String& response);
};

#endif

