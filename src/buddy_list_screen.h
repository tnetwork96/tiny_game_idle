#ifndef BUDDY_LIST_SCREEN_H
#define BUDDY_LIST_SCREEN_H

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include "api_client.h"

struct BuddyEntry {
    String name;
    bool online;
};

// Sidebar tab enumeration
enum SidebarTab {
    TAB_CHATS,
    TAB_NOTIFICATIONS,
    TAB_ADD_FRIEND
};

class BuddyListScreen {
public:
    explicit BuddyListScreen(Adafruit_ST7789* tft);

    // Data
    void setBuddies(const BuddyEntry* entries, uint8_t count);
    void parseFriendsString(const String& friendsString);  // Parse string format: "user1,0|user2,1|..."
    void updateStatus(uint8_t index, bool online);
    bool addFriend(String name, bool online = false);  // Add a new friend to the list
    bool removeFriend(uint8_t index);  // Remove a friend by index
    bool removeFriendByName(const String& name);  // Remove a friend by name

    // Render
    void draw();

    // Navigation
    void handleUp();
    void handleDown();
    void handleLeft();             // Move focus to sidebar or do nothing if already in sidebar
    void handleRight();            // Move focus to list or do nothing if already in list
    void handleSelect();           // Select current highlighted friend or trigger tab action
    void handleSelectRandom();     // Select a random friend
    bool shouldShowAddFriendScreen() const;  // Check if Add Friend screen should be shown (TAB_ADD_FRIEND selected)
    
    // Auto-navigation (for demo/testing)
    void triggerNavigateUp();      // Trigger up navigation (for auto-demo)
    void triggerNavigateDown();    // Trigger down navigation (for auto-demo)
    void triggerNavigateLeft();    // Trigger left navigation (for auto-demo)
    void triggerNavigateRight();   // Trigger right navigation (for auto-demo)
    void triggerSelect();          // Trigger select (for auto-demo)
    unsigned long getDrawTime() const;  // Get time when screen was drawn (for auto-navigation timing)

    // Access
    BuddyEntry getSelectedBuddy() const;

    // Utility for external consumers (e.g., chat)
    uint8_t getSelectedIndex() const { return selectedIndex; }
    uint8_t getBuddyCount() const { return buddyCount; }
    
    // Notifications
    void setNotificationCount(uint8_t count);
    void setUnreadCount(uint8_t count);
    uint8_t getUnreadCount() const { return unreadCount; }
    void setNotifications(const ApiClient::NotificationEntry* entries, uint8_t count);
    bool shouldShowNotificationsScreen() const { return shouldShowNotifications; }
    
    // Callbacks for loading data
    typedef void (*LoadFriendsCallback)(int userId);
    typedef void (*LoadNotificationsCallback)(int userId);
    void setLoadFriendsCallback(LoadFriendsCallback callback);
    void setLoadNotificationsCallback(LoadNotificationsCallback callback);
    void setUserId(int userId);

private:
    Adafruit_ST7789* tft;

    // Layout constants
    static const uint16_t SIDEBAR_WIDTH = 40;
    static const uint16_t SCREEN_HEIGHT = 240;
    static const uint16_t CONTENT_WIDTH = 280;  // 320 - 40

    static const uint8_t MAX_BUDDIES = 30;
    BuddyEntry buddies[MAX_BUDDIES];
    uint8_t buddyCount;
    uint8_t selectedIndex;
    uint8_t scrollOffset;
    uint8_t notificationCount;
    uint8_t unreadCount;
    
    // Sidebar state
    SidebarTab currentTab;
    bool isSidebarActive;
    bool shouldShowNotifications;  // Flag to show notifications view
    
    // Auto-navigation timing
    unsigned long drawTime;  // Time when screen was last drawn
    
    // Callbacks for loading data
    LoadFriendsCallback loadFriendsCallback;
    LoadNotificationsCallback loadNotificationsCallback;
    int userId;  // User ID for API calls
    
    // Notifications data
    ApiClient::NotificationEntry* notifications;
    uint8_t selectedNotificationIndex;

    // Colors
    uint16_t bgColor;
    uint16_t headerColor;
    uint16_t headerTextColor;
    uint16_t listTextColor;
    uint16_t highlightColor;
    uint16_t onlineColor;
    uint16_t offlineColor;
    uint16_t avatarColor;  // Light gray/blue for avatar
    uint16_t separatorColor;  // Cyan separator lines
    uint16_t actionButtonColor;  // Cyan for Add Friend button

    // Layout helpers
    uint8_t getVisibleRows() const;
    void ensureSelectionVisible();
    void drawSidebar();            // Draw vertical sidebar with tabs
    void drawHeader();              // Legacy method (kept for compatibility)
    void drawList(bool clearBackground = true);  // Allow partial redraw without clearing
    void drawScrollbar();  // Draw scrollbar independently
    void drawBuddyRow(uint8_t visibleRow, uint8_t buddyIdx);
    void drawNotificationsList(bool clearBackground = true);  // Draw notifications list
    void drawNotificationRow(uint8_t visibleRow, uint8_t notificationIdx);
    void drawAddFriendIcon(uint16_t x, uint16_t y, uint16_t size, uint16_t color);
    uint16_t statusColor(bool online) const;
    void redrawSelectionChange(uint8_t previousIndex, uint8_t previousScroll);
    bool isHeaderButtonSelected() const;
};

#endif

