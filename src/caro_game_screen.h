#ifndef CARO_GAME_SCREEN_H
#define CARO_GAME_SCREEN_H

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include "caro_game.h"
#include "api_client.h"
#include "social_theme.h"

class CaroGameScreen {
public:
    CaroGameScreen(Adafruit_ST7789* tft, const SocialTheme& theme);
    ~CaroGameScreen();
    
    void setup(int sessionId, const String& hostName, const String& guestName, int myUserId, bool isHost, const String& serverHost, uint16_t serverPort);
    void draw();
    void handleKeyPress(const String& key);
    void update();
    
    // Navigation handlers (for consistency with WiFi password/login/pin screens)
    void handleUp();
    void handleDown();
    void handleLeft();
    void handleRight();
    void handleSelect();
    void handleExit();
    
    // Handle move received from server
    void onMoveReceived(int row, int col, int userId, const String& gameStatus, int winnerId, int currentTurn);
    
    // Callbacks
    typedef void (*OnExitGameCallback)();
    void setOnExitGame(OnExitGameCallback cb) { onExit = cb; }
    
    bool isActive() const { return active; }
    void setActive(bool active) { this->active = active; }
    int getSessionId() const { return sessionId; }

private:
    Adafruit_ST7789* tft;
    SocialTheme theme;
    CaroGame* caroGame;
    
    bool active;
    int sessionId;
    String hostName;
    String guestName;
    int myUserId;
    bool isHost;
    String serverHost;
    uint16_t serverPort;
    
    int hostUserId;
    int guestUserId;
    int currentTurn;
    String gameStatus;
    int winnerId;
    bool needsRedraw;  // Flag to prevent unnecessary redraws
    
    // Auto-play mode
    bool autoPlay;
    unsigned long lastAutoPlayTime;
    static const unsigned long AUTO_PLAY_DELAY = 1000;  // 1 second delay before auto move
    
    OnExitGameCallback onExit;
    
    void drawUIOverlay();
    void drawPlayerNames();
    void drawTurnIndicator();
    void submitMove(int row, int col);
    void syncGameState();
    bool isMyTurn() const;
    String getCurrentPlayerName() const;
};

#endif

