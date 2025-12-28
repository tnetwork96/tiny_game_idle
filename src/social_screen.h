#ifndef SOCIAL_SCREEN_H
#define SOCIAL_SCREEN_H

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include "keyboard.h"
#include "mini_keyboard.h"
#include "mini_add_friend_screen.h"
#include "api_client.h"

// Social screen with sidebar tabs: Friends, Notifications, Add Friend
class SocialScreen {
public:
    enum Tab {
        TAB_FRIENDS,
        TAB_NOTIFICATIONS,
        TAB_ADD_FRIEND
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
    void setServerInfo(const String& serverHost, uint16_t port) {
        this->serverHost = serverHost;
        this->serverPort = port;
    }
    Tab getCurrentTab() const { return currentTab; }

    // Load data for tabs
    void loadFriends();
    void loadNotifications();

    // Reset screen state
    void reset();

    // Callback for when friend is added successfully
    typedef void (*OnAddFriendSuccessCallback)();
    void setOnAddFriendSuccessCallback(OnAddFriendSuccessCallback callback) {
        onAddFriendSuccessCallback = callback;
    }

private:
    Adafruit_ST7789* tft;
    Keyboard* keyboard;
    MiniKeyboard* miniKeyboard;
    MiniAddFriendScreen* miniAddFriend;

    // Current state
    Tab currentTab;
    FocusMode focusMode;
    int userId;
    String serverHost;
    uint16_t serverPort;

    // Friends data
    struct FriendItem {
        String username;
        bool online;
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

    // Callbacks
    OnAddFriendSuccessCallback onAddFriendSuccessCallback;

    // Drawing helpers
    void drawBackground();
    void drawSidebar();
    void drawTab(int tabIndex, bool isSelected);
    void drawContentArea();
    void drawFriendsList();
    void drawNotificationsList();
    void drawAddFriendContent();
    
    // Partial redraw helpers for navigation (avoid flickering)
    void redrawFriendCard(int index, bool isSelected);
    void redrawNotificationCard(int index, bool isSelected);
    
    // Icon drawing helpers (bitmap icons are 16x16)
    void drawFriendsIcon(uint16_t x, uint16_t y, uint16_t color);
    void drawNotificationsIcon(uint16_t x, uint16_t y, uint16_t color);
    void drawAddFriendIcon(uint16_t x, uint16_t y, uint16_t color);

    // Navigation helpers
    void handleTabNavigation(const String& key);
    void handleContentNavigation(const String& key);
    void switchTab(Tab newTab);

    // Data parsing
    void parseFriendsString(const String& friendsString);
    void clearFriends();
    void clearNotifications();
};

#endif

