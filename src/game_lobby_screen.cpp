#include "game_lobby_screen.h"

GameLobbyScreen::GameLobbyScreen(Adafruit_ST7789* tft, const SocialTheme& theme) {
    this->tft = tft;
    this->theme = theme;
    this->currentFocus = FOCUS_RIGHT_PANEL;
    this->friendsCount = 0;
    this->lobbyFriends = nullptr;
    this->selectedFriendIndex = 0;
    this->startButtonFocused = true;
    this->autoStartTriggered = false;
    this->active = false;  // Initially inactive
    this->onStart = nullptr;
    this->onExit = nullptr;
}

void GameLobbyScreen::setup(const String& gameName, const String& hostName) {
    this->gameName = gameName;
    this->players[0] = {hostName, true, false}; // Host is always ready
    this->players[1] = {"", false, true};       // Guest slot empty
    this->currentFocus = FOCUS_RIGHT_PANEL;
    this->startButtonFocused = true;
    this->autoStartTriggered = false;  // Reset flag when setting up new lobby
    this->startTimeSimulation = millis();
}

void GameLobbyScreen::setGuest(const String& name) {
    this->players[1] = {name, false, false};
}

void GameLobbyScreen::setGuestReady(bool ready) {
    this->players[1].isReady = ready;
}

void GameLobbyScreen::clearGuest() {
    this->players[1] = {"", false, true};
}

void GameLobbyScreen::setFriends(MiniFriend* friends, int count) {
    this->lobbyFriends = friends;
    this->friendsCount = count;
}

void GameLobbyScreen::draw() {
    // 1. Left Sidebar
    tft->fillRect(0, 0, 80, 240, theme.colorCardBg);
    tft->drawFastVLine(80, 0, 240, theme.colorAccent); // High-tech divider
    drawFriendBox();

    // 2. Right Main Area
    tft->fillRect(81, 0, 239, 240, theme.colorBg);
    
    // Header
    tft->setTextSize(1);
    tft->setTextColor(theme.colorTextMain);
    // Center game name in the right panel (81-320)
    int16_t x1, y1;
    uint16_t w, h;
    tft->getTextBounds(gameName, 0, 0, &x1, &y1, &w, &h);
    tft->setCursor(81 + (239 - w) / 2, 15);
    tft->print(gameName);
    tft->drawFastHLine(81, 35, 239, theme.colorAccent); // Meets the divider

    drawPlayerSlots();
    drawStartButton();

}

void GameLobbyScreen::drawFriendBox() {
    const uint16_t boxX = 0;
    const uint16_t boxY = 0;
    const uint16_t boxW = 80;
    
    // List items (Text-only)
    const uint16_t itemH = 24;
    const uint16_t startY = 10;
    
    for (int i = 0; i < friendsCount && i < 9; i++) {
        uint16_t y = startY + i * itemH;
        bool isSelected = (currentFocus == FOCUS_LEFT_PANEL && i == selectedFriendIndex);
        
        if (isSelected) {
            tft->fillRect(boxX, y, boxW, itemH, theme.colorHighlight);
            tft->drawFastVLine(boxX, y, itemH, theme.colorAccent);
            tft->drawFastVLine(boxX + 1, y, itemH, theme.colorAccent);
            tft->setTextColor(theme.colorTextMain);
        } else {
            tft->setTextColor(theme.colorTextMuted); // Better hierarchy for unselected
        }

        tft->setTextSize(1);
        tft->setCursor(boxX + 16, y + 8);
        String name = lobbyFriends[i].name;
        if (name.length() > 10) name = name.substring(0, 8);
        tft->print(name);

        // Status dot
        uint16_t dotX = boxX + 8;
        uint16_t dotY = y + itemH / 2;
        uint16_t dotColor = lobbyFriends[i].online ? theme.colorSuccess : theme.colorTextMuted;
        tft->fillCircle(dotX, dotY, 3, dotColor);
    }
}

void GameLobbyScreen::drawPlayerSlots() {
    const uint16_t startX = 80;
    const uint16_t cardW = 200;
    const uint16_t cardH = 40;
    const uint16_t centerX = startX + (240 - cardW) / 2;
    
    drawPlayerCard(centerX, 55, cardW, cardH, players[0], true);
    drawPlayerCard(centerX, 105, cardW, cardH, players[1], false);
}

void GameLobbyScreen::drawPlayerCard(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const LobbyPlayer& player, bool isHost) {
    if (player.isEmpty) {
        // Leave Blank as per spec
        return;
    }

    tft->fillRoundRect(x, y, w, h, 4, theme.colorCardBg);
    
    // Info - Left-aligned with padding
    tft->setTextSize(1);
    tft->setTextColor(theme.colorTextMain);
    tft->setCursor(x + 10, y + 8);
    tft->print(player.name);
    
    tft->setCursor(x + 10, y + 24);
    if (isHost) {
        tft->setTextColor(theme.colorAccent);
        tft->print("HOST / READY");
    } else {
        if (player.isReady) {
            tft->setTextColor(theme.colorSuccess);
            tft->print("READY");
        } else {
            tft->setTextColor(theme.colorTextMuted);
            tft->print("INVITED");
        }
    }
}

void GameLobbyScreen::drawStartButton() {
    const uint16_t panelX = 80;
    const uint16_t btnW = 80;
    const uint16_t btnH = 24;
    const uint16_t btnX = panelX + (240 - btnW) / 2;
    const uint16_t btnY = 195;

    // Luôn sáng mặc định (không cần check guest)
    uint16_t btnColor = theme.colorSuccess;
    
    bool isFocused = (currentFocus == FOCUS_RIGHT_PANEL && startButtonFocused);
    
    if (isFocused) {
        tft->fillRoundRect(btnX, btnY, btnW, btnH, 4, btnColor);
        tft->setTextColor(theme.colorBg);
    } else {
        tft->drawRoundRect(btnX, btnY, btnW, btnH, 4, btnColor);
        tft->setTextColor(btnColor);
    }

    tft->setTextSize(1);
    tft->setCursor(btnX + 25, btnY + 8);
    tft->print("START");
}

// Navigation handlers
void GameLobbyScreen::handleUp() {
    if (currentFocus == FOCUS_LEFT_PANEL && selectedFriendIndex > 0) {
        selectedFriendIndex--;
        drawFriendBox();
    }
}

void GameLobbyScreen::handleDown() {
    if (currentFocus == FOCUS_LEFT_PANEL && selectedFriendIndex < friendsCount - 1) {
        selectedFriendIndex++;
        drawFriendBox();
    }
}

void GameLobbyScreen::handleLeft() {
    if (currentFocus == FOCUS_RIGHT_PANEL) {
        currentFocus = FOCUS_LEFT_PANEL;
        draw();
    }
}

void GameLobbyScreen::handleRight() {
    if (currentFocus == FOCUS_LEFT_PANEL) {
        currentFocus = FOCUS_RIGHT_PANEL;
        startButtonFocused = true;
        draw();
    }
}

void GameLobbyScreen::handleSelect() {
    if (currentFocus == FOCUS_LEFT_PANEL) {
        Serial.println("Lobby: Inviting friend " + lobbyFriends[selectedFriendIndex].name);
    } else if (currentFocus == FOCUS_RIGHT_PANEL && startButtonFocused) {
        // Luôn cho phép click START (bỏ điều kiện check guest)
        if (onStart) {
            onStart();
        } else {
            Serial.println("Lobby: ⚠️ onStart callback is NULL!");
        }
    }
}

void GameLobbyScreen::handleExit() {
    if (onExit) onExit();
}

void GameLobbyScreen::handleKeyPress(const String& key) {
    // Handle new navigation key format first (similar to WiFi password screen)
    if (key == "up") {
        handleUp();
        return;
    } else if (key == "down") {
        handleDown();
        return;
    } else if (key == "left") {
        handleLeft();
        return;
    } else if (key == "right") {
        handleRight();
        return;
    } else if (key == "select") {
        handleSelect();
        return;
    } else if (key == "exit") {
        handleExit();
        return;
    }
    
    // Backward compatibility: handle old key format
    if (key == "|l") {
        handleLeft();
        return;
    } else if (key == "|r") {
        handleRight();
        return;
    } else if (key == "|u") {
        handleUp();
        return;
    } else if (key == "|d") {
        handleDown();
        return;
    } else if (key == "|e") {
        handleSelect();
        return;
    } else if (key == "<" || key == "|b") {
        handleExit();
        return;
    }
}

void GameLobbyScreen::triggerStart() {
    // Trigger start callback programmatically
    if (onStart) {
        Serial.println("Game Lobby: Auto-triggering START");
        onStart();
    } else {
        Serial.println("Game Lobby: ⚠️ onStart callback is NULL!");
    }
}

void GameLobbyScreen::update() {
    // Check if 10 seconds have passed since setup
    unsigned long elapsed = millis() - startTimeSimulation;
    
    // Debug log every second
    static unsigned long lastLogTime = 0;
    if (elapsed - lastLogTime >= 1000) {
        Serial.print("Game Lobby: Elapsed time: ");
        Serial.print(elapsed / 1000);
        Serial.print("s, Guest present: ");
        Serial.println(!players[1].isEmpty ? "YES" : "NO");
        lastLogTime = elapsed - (elapsed % 1000);
    }
    
    if (elapsed >= 10000 && !autoStartTriggered) {
        // Check if guest has joined
        if (!players[1].isEmpty) {
            Serial.println("Game Lobby: Auto-starting game after 10 seconds (guest present)");
            autoStartTriggered = true;
            if (onStart != nullptr) {
                Serial.println("Game Lobby: Calling onStart callback");
                onStart();
            } else {
                Serial.println("Game Lobby: ⚠️ onStart callback is NULL!");
            }
        } else {
            Serial.println("Game Lobby: 10 seconds passed but no guest yet");
            // Mark as checked to avoid repeated checks
            autoStartTriggered = true;
        }
    }
}
