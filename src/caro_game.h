#ifndef CARO_GAME_H
#define CARO_GAME_H

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

// Board size
#define BOARD_ROWS 15  // 15 rows (height)
#define BOARD_COLS 20  // 20 columns (width) - fit full screen width
#define CELL_SIZE 16   // Size of each cell in pixels (15*16 = 240px height, 20*16 = 320px width)
#define BOARD_X 0      // X position of board (start from left, full width)
#define BOARD_Y 0      // Y position of board (start from top)
#define WIN_COUNT 5    // Number of consecutive pieces to win

// Colors
#define COLOR_BOARD_BG 0x0000      // Black
#define COLOR_BOARD_LINE 0xFFFF    // White
#define COLOR_X 0xF800             // Red
#define COLOR_O 0x07E0             // Green
#define COLOR_HIGHLIGHT 0xFFE0     // Yellow
#define COLOR_TEXT 0xFFFF          // White

enum CellState {
    CELL_EMPTY,
    CELL_X,
    CELL_O
};

enum GameState {
    GAME_PLAYING,
    GAME_X_WIN,
    GAME_O_WIN,
    GAME_DRAW
};

class CaroGame {
private:
    Adafruit_ST7789* tft;
    
    // Game state
    CellState board[BOARD_ROWS][BOARD_COLS];
    bool isPlayerXTurn;  // true = X's turn, false = O's turn
    GameState gameState;
    int cursorRow, cursorCol;  // Current cursor position
    int oldCursorRow, oldCursorCol;  // Previous cursor position
    uint16_t cursorColor;  // Current cursor color (green when my turn, red when not)
    
    // Drawing methods
    void drawBoard();
    void drawCell(int row, int col, bool highlight);
    void drawX(int row, int col);
    void drawO(int row, int col);
    void drawCursor();
    void eraseCursor();
    void drawStatus();
    
    // Game logic
    bool checkWin(int row, int col);
    bool checkHorizontal(int row, int col);
    bool checkVertical(int row, int col);
    bool checkDiagonal(int row, int col);
    bool checkAntiDiagonal(int row, int col);
    bool isBoardFull();
    void resetGame();
    
public:
    CaroGame(Adafruit_ST7789* tft);
    
    void init();
    void draw();
    void update();
    
    // Control handlers
    void handleUp();
    void handleDown();
    void handleLeft();
    void handleRight();
    void handleSelect();
    
    // Getters
    GameState getGameState() const { return gameState; }
    bool getIsPlayerXTurn() const { return isPlayerXTurn; }
    int getCursorRow() const { return cursorRow; }
    int getCursorCol() const { return cursorCol; }
    
    // Methods for external control
    void placeMove(int row, int col, bool isX);
    CellState getCell(int row, int col) const;  // Get cell state at position
    void setBoardState(CellState newBoard[BOARD_ROWS][BOARD_COLS]);
    void setGameState(GameState state);
    void setTurn(bool isXTurn);
    void setCursorColor(bool isMyTurn);  // Set cursor color: green when my turn, red when not
};

#endif

