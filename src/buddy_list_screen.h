#ifndef BUDDY_LIST_SCREEN_H
#define BUDDY_LIST_SCREEN_H

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

struct BuddyEntry {
    String name;
    bool online;
};

class BuddyListScreen {
public:
    explicit BuddyListScreen(Adafruit_ST7789* tft);

    // Data
    void setBuddies(const BuddyEntry* entries, uint8_t count);
    void updateStatus(uint8_t index, bool online);
    bool addFriend(String name, bool online = false);  // Add a new friend to the list
    bool removeFriend(uint8_t index);  // Remove a friend by index
    bool removeFriendByName(const String& name);  // Remove a friend by name

    // Render
    void draw();

    // Navigation
    void handleUp();
    void handleDown();
    void handleSelect();           // Select current highlighted friend or trigger Add Friend screen
    void handleSelectRandom();     // Select a random friend
    bool shouldShowAddFriendScreen() const;  // Check if Add Friend screen should be shown (header button selected)

    // Access
    BuddyEntry getSelectedBuddy() const;

    // Utility for external consumers (e.g., chat)
    uint8_t getSelectedIndex() const { return selectedIndex; }
    uint8_t getBuddyCount() const { return buddyCount; }

private:
    Adafruit_ST7789* tft;

    static const uint8_t MAX_BUDDIES = 30;
    BuddyEntry buddies[MAX_BUDDIES];
    uint8_t buddyCount;
    uint8_t selectedIndex;
    uint8_t scrollOffset;

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
    void drawHeader();
    void drawList();
    void drawBuddyRow(uint8_t visibleRow, uint8_t buddyIdx);
    void drawAddFriendIcon(uint16_t x, uint16_t y, uint16_t size, uint16_t color);
    uint16_t statusColor(bool online) const;
    void redrawSelectionChange(uint8_t previousIndex, uint8_t previousScroll);
    bool isHeaderButtonSelected() const;
};

#endif

