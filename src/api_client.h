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
        String username;  // Keep for authentication
        String nickname;  // Display name for UI
    };
    
    struct FriendEntry {
        String nickname;  // Display name (nickname or username fallback)
        bool online;
    };
    
    struct FriendsListResult {
        bool success;
        String message;
        FriendEntry* friends;
        int count;
    };
    
    struct NotificationEntry {
        int id;
        String type;
        String message;
        String timestamp;
        bool read;
    };
    
    struct NotificationsResult {
        bool success;
        String message;
        NotificationEntry* notifications;
        int count;
    };
    
    struct FriendRequestResult {
        bool success;
        String message;
        int requestId;
        int friendshipId;  // For accept response
        String status;  // "pending", "accepted", "rejected"
    };
    
    static LoginResult checkLogin(const String& username, const String& pin, const String& serverHost, uint16_t port);
    static bool createAccount(const String& username, const String& pin, const String& nickname, const String& serverHost, uint16_t port);
    static FriendsListResult getFriends(int userId, const String& serverHost, uint16_t port);
    static String getFriendsList(int userId, const String& serverHost, uint16_t port);  // Returns simple string format: "nickname1,0|nickname2,1|..."
    static NotificationsResult getNotifications(int userId, const String& serverHost, uint16_t port);
    static FriendRequestResult sendFriendRequest(int fromUserId, const String& toNickname, const String& serverHost, uint16_t port);
    static FriendRequestResult acceptFriendRequest(int userId, int notificationId, const String& serverHost, uint16_t port);
    static FriendRequestResult rejectFriendRequest(int userId, int notificationId, const String& serverHost, uint16_t port);
    static FriendRequestResult cancelFriendRequest(int fromUserId, int toUserId, const String& serverHost, uint16_t port);
    static FriendRequestResult removeFriend(int userId, int friendId, const String& serverHost, uint16_t port);
    static void printResponse(const String& response);
    
private:
    // Helper methods to parse response strings manually
    static void parseLoginResponse(const String& response, LoginResult& result);
    static bool parseRegisterResponse(const String& response);
    static void parseFriendRequestResponse(const String& response, FriendRequestResult& result);
};

#endif

