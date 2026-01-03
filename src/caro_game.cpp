#include <Arduino.h>
#include "caro_game.h"
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

CaroGame::CaroGame(Adafruit_ST7789* tft) {
    this->tft = tft;
    this->isPlayerXTurn = true;
    this->gameState = GAME_PLAYING;
    this->cursorRow = BOARD_ROWS / 2;  // Start at center
    this->cursorCol = BOARD_COLS / 2;
    this->oldCursorRow = this->cursorRow;
    this->oldCursorCol = this->cursorCol;
    this->cursorColor = COLOR_HIGHLIGHT;  // Default yellow
    
    // Initialize board to empty
    for (int i = 0; i < BOARD_ROWS; i++) {
        for (int j = 0; j < BOARD_COLS; j++) {
            board[i][j] = CELL_EMPTY;
        }
    }
}

void CaroGame::init() {
    resetGame();
    draw();
}

void CaroGame::resetGame() {
    // Clear board
    for (int i = 0; i < BOARD_ROWS; i++) {
        for (int j = 0; j < BOARD_COLS; j++) {
            board[i][j] = CELL_EMPTY;
        }
    }
    
    isPlayerXTurn = true;
    gameState = GAME_PLAYING;
    cursorRow = BOARD_ROWS / 2;  // Start at center
    cursorCol = BOARD_COLS / 2;
    oldCursorRow = cursorRow;
    oldCursorCol = cursorCol;
}

void CaroGame::drawBoard() {
    // Draw background
    tft->fillRect(BOARD_X, BOARD_Y, BOARD_COLS * CELL_SIZE, BOARD_ROWS * CELL_SIZE, COLOR_BOARD_BG);
    
    // Draw grid lines
    tft->drawRect(BOARD_X, BOARD_Y, BOARD_COLS * CELL_SIZE, BOARD_ROWS * CELL_SIZE, COLOR_BOARD_LINE);
    
    // Draw vertical lines
    for (int i = 1; i < BOARD_COLS; i++) {
        int x = BOARD_X + i * CELL_SIZE;
        tft->drawFastVLine(x, BOARD_Y, BOARD_ROWS * CELL_SIZE, COLOR_BOARD_LINE);
    }
    
    // Draw horizontal lines
    for (int i = 1; i < BOARD_ROWS; i++) {
        int y = BOARD_Y + i * CELL_SIZE;
        tft->drawFastHLine(BOARD_X, y, BOARD_COLS * CELL_SIZE, COLOR_BOARD_LINE);
    }
}

void CaroGame::drawCell(int row, int col, bool highlight) {
    int x = BOARD_X + col * CELL_SIZE;
    int y = BOARD_Y + row * CELL_SIZE;
    
    // Draw highlight if needed
    if (highlight) {
        tft->drawRect(x + 1, y + 1, CELL_SIZE - 2, CELL_SIZE - 2, COLOR_HIGHLIGHT);
    }
    
    // Draw X or O
    if (board[row][col] == CELL_X) {
        drawX(row, col);
    } else if (board[row][col] == CELL_O) {
        drawO(row, col);
    }
}

void CaroGame::drawX(int row, int col) {
    int x = BOARD_X + col * CELL_SIZE;
    int y = BOARD_Y + row * CELL_SIZE;
    int centerX = x + CELL_SIZE / 2;
    int centerY = y + CELL_SIZE / 2;
    int size = CELL_SIZE - 4;  // Adjust size to fit cell
    
    // Draw X (two diagonal lines)
    tft->drawLine(centerX - size/2, centerY - size/2, 
                  centerX + size/2, centerY + size/2, COLOR_X);
    tft->drawLine(centerX + size/2, centerY - size/2, 
                  centerX - size/2, centerY + size/2, COLOR_X);
}

void CaroGame::drawO(int row, int col) {
    int x = BOARD_X + col * CELL_SIZE;
    int y = BOARD_Y + row * CELL_SIZE;
    int centerX = x + CELL_SIZE / 2;
    int centerY = y + CELL_SIZE / 2;
    int radius = (CELL_SIZE - 4) / 2;  // Adjust radius to fit cell
    
    // Draw O (circle)
    tft->drawCircle(centerX, centerY, radius, COLOR_O);
    if (radius > 2) {
        tft->drawCircle(centerX, centerY, radius - 1, COLOR_O);
    }
}

void CaroGame::drawCursor() {
    int x = BOARD_X + cursorCol * CELL_SIZE;
    int y = BOARD_Y + cursorRow * CELL_SIZE;
    
    // Draw cursor với màu đã set (green khi đến turn mình, red khi hết lượt)
    tft->drawRect(x + 2, y + 2, CELL_SIZE - 4, CELL_SIZE - 4, cursorColor);
    tft->drawRect(x + 3, y + 3, CELL_SIZE - 6, CELL_SIZE - 6, cursorColor);
    
    oldCursorRow = cursorRow;
    oldCursorCol = cursorCol;
}

void CaroGame::eraseCursor() {
    int x = BOARD_X + oldCursorCol * CELL_SIZE;
    int y = BOARD_Y + oldCursorRow * CELL_SIZE;
    
    // Erase cursor by redrawing cell
    tft->fillRect(x + 2, y + 2, CELL_SIZE - 4, CELL_SIZE - 4, COLOR_BOARD_BG);
    
    // Redraw cell content if any
    drawCell(oldCursorRow, oldCursorCol, false);
}

void CaroGame::drawStatus() {
    // Clear status area (below board)
    int statusY = BOARD_Y + BOARD_ROWS * CELL_SIZE + 2;
    if (statusY < 240) {
        tft->fillRect(0, statusY, 320, 30, COLOR_BOARD_BG);
        
        tft->setTextSize(2);
        tft->setTextColor(COLOR_TEXT, COLOR_BOARD_BG);
        
        if (gameState == GAME_PLAYING) {
            tft->setCursor(10, statusY + 5);
            if (isPlayerXTurn) {
                tft->print("Player X Turn");
            } else {
                tft->print("Player O Turn");
            }
        } else if (gameState == GAME_X_WIN) {
            tft->setCursor(10, statusY + 5);
            tft->setTextColor(COLOR_X, COLOR_BOARD_BG);
            tft->print("X WINS!");
        } else if (gameState == GAME_O_WIN) {
            tft->setCursor(10, statusY + 5);
            tft->setTextColor(COLOR_O, COLOR_BOARD_BG);
            tft->print("O WINS!");
        } else if (gameState == GAME_DRAW) {
            tft->setCursor(10, statusY + 5);
            tft->print("DRAW!");
        }
    }
}

void CaroGame::draw() {
    drawBoard();
    
    // Draw all cells
    for (int i = 0; i < BOARD_ROWS; i++) {
        for (int j = 0; j < BOARD_COLS; j++) {
            drawCell(i, j, false);
        }
    }
    
    drawCursor();
    // Xóa drawStatus() - không vẽ text "Player X Turn" nữa
    // drawStatus();
}

void CaroGame::update() {
    // Nothing to update continuously
}

void CaroGame::handleUp() {
    if (gameState == GAME_PLAYING) {
        eraseCursor();
        cursorRow = (cursorRow - 1 + BOARD_ROWS) % BOARD_ROWS;
        drawCursor();
    }
}

void CaroGame::handleDown() {
    if (gameState == GAME_PLAYING) {
        eraseCursor();
        cursorRow = (cursorRow + 1) % BOARD_ROWS;
        drawCursor();
    }
}

void CaroGame::handleLeft() {
    if (gameState == GAME_PLAYING) {
        eraseCursor();
        cursorCol = (cursorCol - 1 + BOARD_COLS) % BOARD_COLS;
        drawCursor();
    }
}

void CaroGame::handleRight() {
    if (gameState == GAME_PLAYING) {
        eraseCursor();
        cursorCol = (cursorCol + 1) % BOARD_COLS;
        drawCursor();
    }
}

void CaroGame::handleSelect() {
    if (gameState != GAME_PLAYING) {
        // Reset game if finished
        resetGame();
        draw();
        return;
    }
    
    // Check if cell is empty
    if (board[cursorRow][cursorCol] != CELL_EMPTY) {
        return;  // Cell already occupied
    }
    
    // Place X or O
    if (isPlayerXTurn) {
        board[cursorRow][cursorCol] = CELL_X;
    } else {
        board[cursorRow][cursorCol] = CELL_O;
    }
    
    // Redraw cell
    drawCell(cursorRow, cursorCol, false);
    drawCursor();
    
    // Check for win
    if (checkWin(cursorRow, cursorCol)) {
        if (isPlayerXTurn) {
            gameState = GAME_X_WIN;
        } else {
            gameState = GAME_O_WIN;
        }
        drawStatus();
        Serial.println("WIN DETECTED!");
        return;
    }
    
    // Check for draw
    if (isBoardFull()) {
        gameState = GAME_DRAW;
        drawStatus();
        Serial.println("DRAW!");
        return;
    }
    
    // Switch turns
    isPlayerXTurn = !isPlayerXTurn;
    drawStatus();
}

bool CaroGame::checkWin(int row, int col) {
    // Check all directions: horizontal, vertical, diagonal, anti-diagonal
    return checkHorizontal(row, col) || 
           checkVertical(row, col) || 
           checkDiagonal(row, col) || 
           checkAntiDiagonal(row, col);
}

bool CaroGame::checkHorizontal(int row, int col) {
    CellState player = board[row][col];
    int count = 1;  // Count current cell
    
    // Check left
    for (int c = col - 1; c >= 0 && board[row][c] == player; c--) {
        count++;
    }
    
    // Check right
    for (int c = col + 1; c < BOARD_COLS && board[row][c] == player; c++) {
        count++;
    }
    
    return count >= WIN_COUNT;
}

bool CaroGame::checkVertical(int row, int col) {
    CellState player = board[row][col];
    int count = 1;  // Count current cell
    
    // Check up
    for (int r = row - 1; r >= 0 && board[r][col] == player; r--) {
        count++;
    }
    
    // Check down
    for (int r = row + 1; r < BOARD_ROWS && board[r][col] == player; r++) {
        count++;
    }
    
    return count >= WIN_COUNT;
}

bool CaroGame::checkDiagonal(int row, int col) {
    CellState player = board[row][col];
    int count = 1;  // Count current cell
    
    // Check top-left to bottom-right diagonal
    // Check top-left
    for (int r = row - 1, c = col - 1; r >= 0 && c >= 0 && board[r][c] == player; r--, c--) {
        count++;
    }
    
    // Check bottom-right
    for (int r = row + 1, c = col + 1; r < BOARD_ROWS && c < BOARD_COLS && board[r][c] == player; r++, c++) {
        count++;
    }
    
    return count >= WIN_COUNT;
}

bool CaroGame::checkAntiDiagonal(int row, int col) {
    CellState player = board[row][col];
    int count = 1;  // Count current cell
    
    // Check top-right to bottom-left diagonal
    // Check top-right
    for (int r = row - 1, c = col + 1; r >= 0 && c < BOARD_COLS && board[r][c] == player; r--, c++) {
        count++;
    }
    
    // Check bottom-left
    for (int r = row + 1, c = col - 1; r < BOARD_ROWS && c >= 0 && board[r][c] == player; r++, c--) {
        count++;
    }
    
    return count >= WIN_COUNT;
}

bool CaroGame::isBoardFull() {
    for (int i = 0; i < BOARD_ROWS; i++) {
        for (int j = 0; j < BOARD_COLS; j++) {
            if (board[i][j] == CELL_EMPTY) {
                return false;
            }
        }
    }
    return true;
}

CellState CaroGame::getCell(int row, int col) const {
    if (row < 0 || row >= BOARD_ROWS || col < 0 || col >= BOARD_COLS) {
        return CELL_EMPTY;  // Out of bounds
    }
    return board[row][col];
}

void CaroGame::placeMove(int row, int col, bool isX) {
    if (row < 0 || row >= BOARD_ROWS || col < 0 || col >= BOARD_COLS) {
        return;
    }
    if (board[row][col] != CELL_EMPTY) {
        return;
    }
    
    board[row][col] = isX ? CELL_X : CELL_O;
    
    // Check for win
    if (checkWin(row, col)) {
        gameState = isX ? GAME_X_WIN : GAME_O_WIN;
    } else if (isBoardFull()) {
        gameState = GAME_DRAW;
    } else {
        isPlayerXTurn = !isPlayerXTurn;
    }
}

void CaroGame::setBoardState(CellState newBoard[BOARD_ROWS][BOARD_COLS]) {
    for (int i = 0; i < BOARD_ROWS; i++) {
        for (int j = 0; j < BOARD_COLS; j++) {
            board[i][j] = newBoard[i][j];
        }
    }
}

void CaroGame::setGameState(GameState state) {
    gameState = state;
}

void CaroGame::setTurn(bool isXTurn) {
    isPlayerXTurn = isXTurn;
}

void CaroGame::setCursorColor(bool isMyTurn) {
    // Green (COLOR_O) khi đến turn mình, Red (COLOR_X) khi hết lượt
    if (isMyTurn) {
        cursorColor = COLOR_O;  // Green (0x07E0)
    } else {
        cursorColor = COLOR_X;  // Red (0xF800)
    }
}

