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
        int relatedId;  // Session ID for game_invite, friend_request_id for friend_request
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

    struct GameSessionResult {
        bool success;
        String message;
        int sessionId;
        String status;
        int participantCount;
    };

    struct GameMoveResult {
        bool success;
        String message;
        int moveId;
        String gameStatus;
        int winnerId;
    };

    struct GameStateResult {
        bool success;
        String message;
        int sessionId;
        String gameType;
        String status;
        int currentTurn;
        int moveCount;
        String hostName;
        String guestName;
        int hostId;
        int guestId;
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
    static GameSessionResult createGameSession(int hostUserId, const String& gameType, int maxPlayers, const int* participantIds, int participantCount, const String& serverHost, uint16_t port);
    static GameSessionResult respondGameInvite(int sessionId, int userId, bool accept, const String& serverHost, uint16_t port);
    static GameSessionResult setGameReady(int sessionId, int userId, bool ready, const String& serverHost, uint16_t port);
    static GameSessionResult leaveGameSession(int sessionId, int userId, const String& serverHost, uint16_t port);
    static GameMoveResult submitGameMove(int sessionId, int userId, int row, int col, const String& serverHost, uint16_t port);
    static GameStateResult getGameState(int sessionId, const String& serverHost, uint16_t port);
    static void printResponse(const String& response);
    
private:
    // Helper methods to parse response strings manually
    static void parseLoginResponse(const String& response, LoginResult& result);
    static bool parseRegisterResponse(const String& response);
    static void parseFriendRequestResponse(const String& response, FriendRequestResult& result);
    static void parseGameSessionResponse(const String& response, GameSessionResult& result);
};

#endif

