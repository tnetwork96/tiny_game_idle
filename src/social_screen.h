#ifndef SOCIAL_SCREEN_H
#define SOCIAL_SCREEN_H

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include "keyboard.h"
#include "mini_keyboard.h"
#include "mini_add_friend_screen.h"
#include "api_client.h"
#include "confirmation_dialog.h"
#include "social_theme.h"

// Forward declaration
class SocketManager;
class GameLobbyScreen;

// Social screen with sidebar tabs: Friends, Notifications, Add Friend
class SocialScreen {
public:
    enum Tab {
        TAB_FRIENDS,
        TAB_NOTIFICATIONS,
        TAB_ADD_FRIEND,
        TAB_GAMES
    };
    
    enum ScreenState {
        STATE_NORMAL = 0,
        STATE_WAITING_GAME
    };
    
    // Game identifiers
    enum GameId {
        GAME_CARO = 0,
        GAME_SEAHORSE = 1,
        GAME_CHESS = 2
    };
    
    enum FocusMode {
        FOCUS_SIDEBAR,
        FOCUS_CONTENT
    };

    SocialScreen(Adafruit_ST7789* tft, Keyboard* keyboard);
    ~SocialScreen();

    // Draw current state
    void draw();

    // Handle keyboard key (coming from onKeySelected)
    void handleKeyPress(const String& key);

    // State helpers
    void setUserId(int userId) { this->userId = userId; }
    void setOwnerNickname(const String& nickname) { this->ownerNickname = nickname; }
    void setServerInfo(const String& serverHost, uint16_t port) {
        this->serverHost = serverHost;
        this->serverPort = port;
    }
    Tab getCurrentTab() const { return currentTab; }
    
    // Get MiniKeyboard for Add Friend screen
    MiniKeyboard* getMiniKeyboard() const { return miniKeyboard; }

    // Load data for tabs
    void loadFriends();
    void loadNotifications();

    // Reset screen state
    void reset();

    // Navigate to Add Friend tab and focus keyboard
    void navigateToAddFriend();
    
    // Navigate to Notifications tab
    void navigateToNotifications();
    
    // Navigate to Games tab
    void navigateToGames();
    
    // Navigate to Friends tab (chat)
    void navigateToFriends();
    
    // Open chat with friend at index
    void openChatWithFriend(int friendIndex);
    
    // Callback type for opening chat
    typedef void (*OnOpenChatCallback)(int friendUserId, const String& friendNickname);
    void setOnOpenChatCallback(OnOpenChatCallback callback) {
        onOpenChatCallback = callback;
    }
    // Callback for launching a game
    typedef void (*OnGameSelectedCallback)(int gameId);
    void setOnGameSelectedCallback(OnGameSelectedCallback callback) {
        onGameSelectedCallback = callback;
    }
    
    // Check if confirmation dialog is visible
    bool isConfirmationDialogVisible() const {
        return confirmationDialog != nullptr && confirmationDialog->isVisible();
    }
    
    // Get notifications count
    int getNotificationsCount() const { return notificationsCount; }
    
    // Redraw sidebar (to update badges)
    void redrawSidebar();
    
    // Set unread chat status (deprecated - use addUnreadChatForFriend instead)
    void setHasUnreadChat(bool hasUnread);
    
    // Manage unread chat messages per friend
    void addUnreadChatForFriend(int friendUserId);
    void clearUnreadChatForFriend(int friendUserId);
    int getUnreadCountForFriend(int friendUserId) const;
    
    // Get first friend request notification index (returns -1 if none)
    int getFirstFriendRequestIndex() const;
    
    // Get first friend with unread messages index (returns -1 if none)
    int getFirstFriendWithUnreadIndex() const;
    
    // Select notification by index (for auto-navigation)
    void selectNotification(int index);
    
    // Select friend by index (for auto-navigation)
    void selectFriend(int index);
    
    // Remove notification by ID (optimistic update before server reload)
    void removeNotificationById(int notificationId);
    
    // Add notification from socket (thread-safe)
    void addNotificationFromSocket(int id, const String& type, const String& message, const String& timestamp, bool read);
    
    // Notification popup management
    void updateNotificationPopup();  // Check if popup should be hidden (call in loop)

    // Callback for when friend is added successfully
    typedef void (*OnAddFriendSuccessCallback)();
    void setOnAddFriendSuccessCallback(OnAddFriendSuccessCallback callback) {
        onAddFriendSuccessCallback = callback;
    }
    
    // Update friend online status
    void updateFriendStatus(int friendUserId, bool isOnline);
    
    // Static callback wrapper for user status update
    static void onUserStatusUpdate(int userId, const String& status);
    
    // Static callbacks for Game Hub dialogs
    static void onGameComingSoonConfirm();
    static void onGameComingSoonCancel();
    
    // Screen state helpers
    void setScreenState(ScreenState state) { screenState = state; }
    ScreenState getScreenState() const { return screenState; }

private:
    Adafruit_ST7789* tft;
    Keyboard* keyboard;
    MiniKeyboard* miniKeyboard;
    MiniAddFriendScreen* miniAddFriend;
    ConfirmationDialog* confirmationDialog;
    GameLobbyScreen* gameLobby;
    
    // Theme configuration (hot-swappable visual style)
    SocialTheme currentTheme;

    // Current state
    Tab currentTab;
    FocusMode focusMode;
    int userId;
    String ownerNickname;
    String serverHost;
    uint16_t serverPort;

    // Friends data
    struct FriendItem {
        String nickname;  // Display name (nickname or username fallback)
        int userId;       // User ID để track unread messages
        bool online;
        int unreadCount;  // Số lượng tin nhắn chưa đọc từ friend này
    };
    FriendItem* friends;
    int friendsCount;
    int selectedFriendIndex;
    int friendsScrollOffset;

    // Notifications data
    ApiClient::NotificationEntry* notifications;
    int notificationsCount;
    int selectedNotificationIndex;
    int notificationsScrollOffset;
    
    // Games data (Game Hub)
    int selectedGameIndex;
    String pendingGameName;
    
    ScreenState screenState;
    
    // Semaphore for thread-safe access to notifications array
    SemaphoreHandle_t notificationsMutex;

    // Callbacks
    OnAddFriendSuccessCallback onAddFriendSuccessCallback;
    OnOpenChatCallback onOpenChatCallback;
    OnGameSelectedCallback onGameSelectedCallback;

    // Drawing helpers
    void drawBackground();
    void drawSidebar();
    void drawTab(int tabIndex, bool isSelected);
    void drawContentArea();
    void drawGameMenu();
    void drawFriendsList();
    void drawNotificationsList();
    void drawAddFriendContent();
    
    // Partial redraw helpers for navigation (avoid flickering)
    void redrawFriendCard(int index, bool isSelected);
    void redrawNotificationCard(int index, bool isSelected);
    
    // Friend item drawing helper (extracted for cleaner code)
    void drawFriendItem(uint16_t x, uint16_t y, uint16_t w, uint16_t h, FriendItem* friendItem, bool isSelected);
    
    // Messenger-style status indicator (simple filled/hollow circle)
    void drawStatusIndicator(uint16_t x, uint16_t y, bool isOnline);
    
    // Icon drawing helpers (bitmap icons are 16x16)
    void drawFriendsIcon(uint16_t x, uint16_t y, uint16_t color);
    void drawNotificationsIcon(uint16_t x, uint16_t y, uint16_t color);
    void drawAddFriendIcon(uint16_t x, uint16_t y, uint16_t color);
    void drawGamepadIcon(uint16_t x, uint16_t y, uint16_t color);

    // Navigation helpers
    void handleTabNavigation(const String& key);
    void handleContentNavigation(const String& key);
    void switchTab(Tab newTab);

    // Data parsing
    void parseFriendsString(const String& friendsString);
    void clearFriends();
    void clearNotifications();
    
    // Confirmation dialog callbacks (static wrappers)
    static void onConfirmAcceptFriendRequest();
    static void onCancelAcceptFriendRequest();
    
    // Instance callback methods for confirmation dialog
    void doAcceptFriendRequest();
    void doCancelAcceptFriendRequest();
    
    // Store notification ID for accept action
    int pendingAcceptNotificationId;
    
    // Notification popup/toast
    String popupMessage;
    unsigned long popupShowTime;
    bool popupVisible;
    static const unsigned long POPUP_DURATION = 5000;  // 5 seconds
    
    // Draw notification popup
    void drawNotificationPopup();
    void showNotificationPopup(const String& message);
    void hideNotificationPopup();
    
    // Red dot badge for unread notifications
    bool hasUnreadNotification;
    
    // Red dot badge for unread chat messages
    bool hasUnreadChatFlag;
};

#endif

