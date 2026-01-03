#include "caro_game_screen.h"

CaroGameScreen::CaroGameScreen(Adafruit_ST7789* tft, const SocialTheme& theme) {
    this->tft = tft;
    this->theme = theme;
    this->caroGame = new CaroGame(tft);
    this->active = false;
    this->sessionId = -1;
    this->hostName = "";
    this->guestName = "";
    this->myUserId = -1;
    this->isHost = false;
    this->serverHost = "";
    this->serverPort = 0;
    this->hostUserId = -1;
    this->guestUserId = -1;
    this->currentTurn = -1;
    this->gameStatus = "playing";
    this->winnerId = -1;
    this->needsRedraw = true;  // Initial draw needed
    this->onExit = nullptr;
    this->autoPlay = true;  // Enable auto-play by default
    this->lastAutoPlayTime = 0;
}

CaroGameScreen::~CaroGameScreen() {
    if (caroGame != nullptr) {
        delete caroGame;
    }
}

void CaroGameScreen::setup(int sessionId, const String& hostName, const String& guestName, int myUserId, bool isHost, const String& serverHost, uint16_t serverPort) {
    this->sessionId = sessionId;
    this->hostName = hostName;
    this->guestName = guestName;
    this->myUserId = myUserId;
    this->isHost = isHost;
    this->serverHost = serverHost;
    this->serverPort = serverPort;
    this->active = true;
    this->gameStatus = "playing";
    this->winnerId = -1;
    this->needsRedraw = true;  // Force initial draw
    
    // Initialize game
    caroGame->init();
    
    // Sync initial game state from server
    syncGameState();
    
    draw();
}

void CaroGameScreen::draw() {
    if (!active) return;
    
    // Chỉ vẽ khi có thay đổi thực sự để tránh nháy màn hình
    if (!needsRedraw) return;
    
    // Set cursor color dựa trên turn trước khi vẽ
    caroGame->setCursorColor(isMyTurn());
    
    // Draw game board
    caroGame->draw();
    
    // Draw UI overlay (player names, turn indicator)
    drawUIOverlay();
    
    // Reset flag sau khi vẽ
    needsRedraw = false;
}

void CaroGameScreen::drawUIOverlay() {
    // Draw player names header at top
    // drawPlayerNames();
    
    // Xóa turn indicator - không vẽ text "Your turn" nữa
    // drawTurnIndicator();
}

void CaroGameScreen::drawPlayerNames() {
    // Header area: top 20 pixels
    tft->fillRect(0, 0, 320, 20, theme.colorCardBg);
    tft->drawFastHLine(0, 20, 320, theme.colorAccent);
    
    tft->setTextSize(1);
    tft->setTextColor(COLOR_X, theme.colorCardBg);
    tft->setCursor(10, 6);
    tft->print(hostName);
    tft->print(" (X)");
    
    tft->setTextColor(COLOR_O, theme.colorCardBg);
    int16_t x1, y1;
    uint16_t w, h;
    String guestText = guestName + " (O)";
    tft->getTextBounds(guestText, 0, 0, &x1, &y1, &w, &h);
    tft->setCursor(320 - w - 10, 6);
    tft->print(guestText);
}

void CaroGameScreen::drawTurnIndicator() {
    // Status area: bottom 20 pixels (below board at y=240)
    // Board is 15 rows * 16px = 240px, so status goes below
    int statusY = 240;
    if (statusY < 240) {  // If board doesn't fill screen
        tft->fillRect(0, statusY, 320, 20, COLOR_BOARD_BG);
        
        tft->setTextSize(1);
        tft->setTextColor(COLOR_TEXT, COLOR_BOARD_BG);
        tft->setCursor(10, statusY + 6);
        
        if (gameStatus == "completed") {
            if (winnerId == myUserId) {
                tft->setTextColor(COLOR_X, COLOR_BOARD_BG);
                tft->print("YOU WIN!");
            } else if (winnerId > 0) {
                tft->setTextColor(COLOR_O, COLOR_BOARD_BG);
                tft->print(getCurrentPlayerName() + " WINS!");
            } else {
                tft->print("DRAW!");
            }
        } else {
            if (isMyTurn()) {
                tft->setTextColor(COLOR_HIGHLIGHT, COLOR_BOARD_BG);
                tft->print("Your turn");
            } else {
                tft->print("Waiting for " + getCurrentPlayerName() + "...");
            }
        }
    }
}

void CaroGameScreen::handleKeyPress(const String& key) {
    if (!active) return;
    
    if (gameStatus == "completed") {
        // Game finished, any key to exit
        if (onExit != nullptr) {
            onExit();
        }
        return;
    }
    
    if (key == "|u") {
        caroGame->setCursorColor(isMyTurn());  // Update color before move
        caroGame->handleUp();
        needsRedraw = true;  // Cursor moved, need redraw
        draw();
    } else if (key == "|d") {
        caroGame->setCursorColor(isMyTurn());  // Update color before move
        caroGame->handleDown();
        needsRedraw = true;  // Cursor moved, need redraw
        draw();
    } else if (key == "|l") {
        caroGame->setCursorColor(isMyTurn());  // Update color before move
        caroGame->handleLeft();
        needsRedraw = true;  // Cursor moved, need redraw
        draw();
    } else if (key == "|r") {
        caroGame->setCursorColor(isMyTurn());  // Update color before move
        caroGame->handleRight();
        needsRedraw = true;  // Cursor moved, need redraw
        draw();
    } else if (key == "|e") {
        // Submit move
        if (isMyTurn() && caroGame->getGameState() == GAME_PLAYING) {
            int row = caroGame->getCursorRow();
            int col = caroGame->getCursorCol();
            submitMove(row, col);
        }
    } else if (key == "<" || key == "|b") {
        // Exit game
        if (onExit != nullptr) {
            onExit();
        }
    }
}

void CaroGameScreen::update() {
    if (!active) return;
    
    // Periodically sync game state from server (every 3 seconds) to ensure we have correct turn
    static unsigned long lastSyncTime = 0;
    unsigned long currentSyncTime = millis();
    if (currentSyncTime - lastSyncTime >= 3000) {
        lastSyncTime = currentSyncTime;
        syncGameState();
    }
    
    caroGame->update();
    
    // Auto-play: automatically move cursor and submit when it's my turn
    // Debug: print turn status every 5 seconds
    static unsigned long lastDebugTime = 0;
    unsigned long currentDebugTime = millis();
    if (currentDebugTime - lastDebugTime >= 5000) {
        lastDebugTime = currentDebugTime;
        Serial.print("Caro Game Screen: Auto-play check - isMyTurn=");
        Serial.print(isMyTurn() ? "true" : "false");
        Serial.print(", gameStatus=");
        Serial.print(gameStatus);
        Serial.print(", currentTurn=");
        Serial.print(currentTurn);
        Serial.print(", myUserId=");
        Serial.print(myUserId);
        Serial.print(", autoPlay=");
        Serial.println(autoPlay ? "true" : "false");
    }
    
    // Chỉ auto-play khi đến turn của mình
    if (autoPlay && isMyTurn() && (gameStatus == "in_progress" || gameStatus == "playing")) {
        unsigned long currentTime = millis();
        
        // Wait a bit before auto-playing (to avoid too fast moves)
        if (currentTime - lastAutoPlayTime >= AUTO_PLAY_DELAY) {
            // Tìm cell trống từ center (đơn giản, không tìm gần cursor)
            int bestRow = -1;
            int bestCol = -1;
            int currentRow = caroGame->getCursorRow();
            int currentCol = caroGame->getCursorCol();
            
            // Tìm từ center ra ngoài
            int centerRow = 7;  // Board center
            int centerCol = 10;
            bool found = false;
            
            for (int radius = 0; radius < 8 && !found; radius++) {
                for (int dr = -radius; dr <= radius && !found; dr++) {
                    for (int dc = -radius; dc <= radius && !found; dc++) {
                        // Only check cells on the current radius
                        if (abs(dr) != radius && abs(dc) != radius) continue;
                        
                        int row = centerRow + dr;
                        int col = centerCol + dc;
                        
                        // Check bounds
                        if (row < 0 || row >= 15 || col < 0 || col >= 20) continue;
                        
                        // Check if cell is empty
                        if (caroGame->getCell(row, col) == CELL_EMPTY) {
                            bestRow = row;
                            bestCol = col;
                            found = true;
                        }
                    }
                }
            }
            
            // Nếu tìm thấy cell, điều hướng cursor rồi đánh
            if (bestRow >= 0 && bestCol >= 0) {
                // Kiểm tra cursor đã ở đúng vị trí chưa
                if (currentRow == bestRow && currentCol == bestCol) {
                    // Cursor đã ở đúng vị trí, submit move
                    Serial.print("Caro Game Screen: Auto-play - submitting move at (");
                    Serial.print(bestRow);
                    Serial.print(", ");
                    Serial.print(bestCol);
                    Serial.println(")");
                    lastAutoPlayTime = currentTime;
                    submitMove(bestRow, bestCol);
                } else {
                    // Di chuyển cursor đến đúng vị trí (một bước mỗi lần)
                    int rowDiff = bestRow - currentRow;
                    int colDiff = bestCol - currentCol;
                    
                    // Di chuyển theo hướng có khoảng cách lớn hơn
                    if (abs(rowDiff) > abs(colDiff)) {
                        if (rowDiff > 0) {
                            caroGame->handleDown();
                        } else {
                            caroGame->handleUp();
                        }
                    } else {
                        if (colDiff > 0) {
                            caroGame->handleRight();
                        } else {
                            caroGame->handleLeft();
                        }
                    }
                    
                    // Không set lastAutoPlayTime - để cursor tiếp tục di chuyển
                    // handleUp/Down/Left/Right đã tự động vẽ lại cursor
                }
            } else {
                // Không tìm thấy cell trống
                Serial.println("Caro Game Screen: Auto-play - no empty cell found");
                lastAutoPlayTime = currentTime;
            }
        }
    }
}

void CaroGameScreen::onMoveReceived(int row, int col, int userId, const String& gameStatus, int winnerId, int currentTurn) {
    if (!active) return;
    
    Serial.print("Caro Game Screen: onMoveReceived - row=");
    Serial.print(row);
    Serial.print(", col=");
    Serial.print(col);
    Serial.print(", userId=");
    Serial.print(userId);
    Serial.print(", gameStatus=");
    Serial.print(gameStatus);
    Serial.print(", currentTurn=");
    Serial.println(currentTurn);
    
    this->gameStatus = gameStatus;
    this->winnerId = winnerId;
    
    // Place move on board
    bool isX = (userId == hostUserId);  // Host is X, Guest is O
    caroGame->placeMove(row, col, isX);
    
    // Update turn from server (use server's current_turn instead of calculating locally)
    int oldTurn = this->currentTurn;
    if (currentTurn > 0) {
        this->currentTurn = currentTurn;  // Use server's current_turn
    } else {
        // Fallback: calculate locally if server didn't send it
        this->currentTurn = (this->currentTurn == hostUserId) ? guestUserId : hostUserId;
    }
    
    // Reset auto-play timer when turn changes to my turn
    if (this->currentTurn == myUserId && oldTurn != myUserId) {
        lastAutoPlayTime = 0;  // Reset timer so we can play immediately
        Serial.println("Caro Game Screen: Turn changed to me - resetting auto-play timer");
    }
    
    Serial.print("Caro Game Screen: Turn updated - currentTurn=");
    Serial.print(this->currentTurn);
    Serial.print(", myUserId=");
    Serial.println(myUserId);
    
    // Update game state if completed
    if (gameStatus == "completed") {
        if (winnerId == hostUserId) {
            caroGame->setGameState(GAME_X_WIN);
        } else if (winnerId == guestUserId) {
            caroGame->setGameState(GAME_O_WIN);
        } else {
            caroGame->setGameState(GAME_DRAW);
        }
    }
    
    needsRedraw = true;  // Move received, need redraw
    draw();
}

void CaroGameScreen::submitMove(int row, int col) {
    if (!isMyTurn()) return;
    
    // Ensure cursor is at target position before submitting
    int currentRow = caroGame->getCursorRow();
    int currentCol = caroGame->getCursorCol();
    
    if (currentRow != row || currentCol != col) {
        Serial.print("Caro Game Screen: Cursor not at target, moving from (");
        Serial.print(currentRow);
        Serial.print(", ");
        Serial.print(currentCol);
        Serial.print(") to (");
        Serial.print(row);
        Serial.print(", ");
        Serial.print(col);
        Serial.println(")");
        
        // Don't submit yet, wait for cursor to reach target
        // Auto-play logic will move cursor in next update()
        return;
    }
    
    // Place move locally first (optimistic update)
    bool isX = isHost;  // Host is X, Guest is O
    caroGame->placeMove(row, col, isX);
    
    // Submit to server
    ApiClient::GameMoveResult result = ApiClient::submitGameMove(sessionId, myUserId, row, col, serverHost, serverPort);
    
    if (result.success) {
        gameStatus = result.gameStatus;
        winnerId = result.winnerId;
        
        if (result.gameStatus == "completed") {
            if (result.winnerId == myUserId) {
                caroGame->setGameState(isHost ? GAME_X_WIN : GAME_O_WIN);
            } else if (result.winnerId > 0) {
                caroGame->setGameState(isHost ? GAME_O_WIN : GAME_X_WIN);
            } else {
                caroGame->setGameState(GAME_DRAW);
            }
        }
        
        // Update turn
        int oldTurn = currentTurn;
        currentTurn = (currentTurn == hostUserId) ? guestUserId : hostUserId;
        
        // Reset auto-play timer when turn changes (for next time it's our turn)
        // But don't reset if it's still our turn (shouldn't happen, but just in case)
        if (currentTurn != myUserId) {
            // Turn passed to opponent, reset timer for when it comes back to us
            lastAutoPlayTime = 0;
        }
        
        Serial.print("Caro Game Screen: Move submitted successfully - new turn=");
        Serial.print(currentTurn);
        Serial.print(", myUserId=");
        Serial.println(myUserId);
        
        needsRedraw = true;  // Move submitted, need redraw
        draw();
    } else {
        Serial.print("Caro Game Screen: Failed to submit move: ");
        Serial.println(result.message);
        // Revert move by syncing state from server
        syncGameState();
        needsRedraw = true;  // State synced, need redraw
        draw();
    }
}

void CaroGameScreen::syncGameState() {
    Serial.println("Caro Game Screen: Syncing game state from server...");
    ApiClient::GameStateResult result = ApiClient::getGameState(sessionId, serverHost, serverPort);
    
    if (result.success) {
        int oldTurn = currentTurn;
        currentTurn = result.currentTurn;
        gameStatus = result.status;
        hostUserId = result.hostId;
        guestUserId = result.guestId;
        
        // Reset auto-play timer when turn changes to my turn
        if (currentTurn == myUserId && oldTurn != myUserId) {
            lastAutoPlayTime = 0;  // Reset timer so we can play immediately
            Serial.println("Caro Game Screen: Synced - turn changed to me - resetting auto-play timer");
        }
        
        Serial.print("Caro Game Screen: Synced - currentTurn=");
        Serial.print(currentTurn);
        Serial.print(", gameStatus=");
        Serial.print(gameStatus);
        Serial.print(", myUserId=");
        Serial.println(myUserId);
        
        // Update board state from server
        // CaroGame doesn't have a method to set board state
        // We need to modify CaroGame or rebuild board from moves
        // For MVP, we'll just update turn info
    }
}

bool CaroGameScreen::isMyTurn() const {
    // Accept both "in_progress" and "playing" status
    bool statusOk = (gameStatus == "in_progress" || gameStatus == "playing");
    bool turnOk = (currentTurn == myUserId);
    return turnOk && statusOk;
}

String CaroGameScreen::getCurrentPlayerName() const {
    if (currentTurn == hostUserId) {
        return hostName;
    } else if (currentTurn == guestUserId) {
        return guestName;
    }
    return "Player";
}

