#ifndef GAME_LOBBY_SCREEN_H
#define GAME_LOBBY_SCREEN_H

#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include "social_theme.h"

class GameLobbyScreen {
public:
    enum FocusArea {
        FOCUS_LEFT_PANEL,
        FOCUS_RIGHT_PANEL
    };

    struct LobbyPlayer {
        String name;
        bool isReady;
        bool isEmpty;
    };

    struct MiniFriend {
        String name;
        bool online;
    };

    GameLobbyScreen(Adafruit_ST7789* tft, const SocialTheme& theme);
    
    void setup(const String& gameName, const String& hostName);
    void draw();
    void handleKeyPress(const String& key);
    void update();  // Check auto-start timer
    
    // Setters for dynamic data
    void setGuest(const String& name);
    void setGuestReady(bool ready);
    void clearGuest();
    void setFriends(MiniFriend* friends, int count);

    // Callbacks
    typedef void (*OnStartGameCallback)();
    typedef void (*OnExitLobbyCallback)();
    void setOnStartGame(OnStartGameCallback cb) { onStart = cb; }
    void setOnExit(OnExitLobbyCallback cb) { onExit = cb; }
    
    // Trigger start programmatically
    void triggerStart();
    
    // Active state
    bool isActive() const { return active; }
    void setActive(bool active) { this->active = active; }

private:
    Adafruit_ST7789* tft;
    SocialTheme theme;
    
    FocusArea currentFocus;
    String gameName;
    LobbyPlayer players[2];
    MiniFriend* lobbyFriends;
    int friendsCount;
    int selectedFriendIndex;
    bool startButtonFocused;
    unsigned long startTimeSimulation;
    bool autoStartTriggered;  // Flag to prevent multiple auto-start triggers
    bool active;  // Whether this screen is currently active

    OnStartGameCallback onStart;
    OnExitLobbyCallback onExit;

    void drawFriendBox();
    void drawPlayerSlots();
    void drawPlayerCard(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const LobbyPlayer& player, bool isHost);
    void drawStartButton();
};

#endif

