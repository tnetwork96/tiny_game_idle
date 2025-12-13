#ifndef BILLIARD_GAME_H
#define BILLIARD_GAME_H

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <math.h>

// Screen dimensions
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240

// Table dimensions - smaller to show cue stick and have margins
#define TABLE_X 20
#define TABLE_Y 20
#define TABLE_WIDTH 280
#define TABLE_HEIGHT 200
#define POCKET_RADIUS 12  // Larger pocket radius for clearer visibility

// Ball properties
#define BALL_RADIUS 6
#define BALL_COUNT 20  // 1 cue ball + 19 target balls
#define FRICTION 0.975f  // Friction coefficient (tăng ma sát để quả bi dừng lại nhanh hơn)
#define MIN_VELOCITY 0.2f  // Minimum velocity to stop ball (tăng để quả bi dừng sớm hơn)
#define POCKET_ATTRACTION 0.3f  // Attraction force when near pocket

// Colors
#define COLOR_TABLE 0x03E0      // Green table
#define COLOR_TABLE_BORDER 0x8200  // Brown border (màu nâu)
#define COLOR_CUE_BALL 0xFFFF      // White
#define COLOR_BALL1 0xF800          // Red
#define COLOR_BALL2 0xFFE0          // Yellow
#define COLOR_BALL3 0x07E0          // Green
#define COLOR_BALL4 0x001F          // Blue
#define COLOR_BALL5 0xF81F          // Magenta
#define COLOR_BALL6 0x8410          // Brown/Orange
#define COLOR_BALL7 0x07FF          // Cyan
#define COLOR_BALL8 0x0000          // Black (8-ball)
#define COLOR_BALL9 0xFFE0          // Yellow (9-ball, same as 2)
#define COLOR_BALL10 0xFC00         // Orange
#define COLOR_BALL11 0x7BE0         // Light Green
#define COLOR_BALL12 0x07FF         // Light Blue
#define COLOR_BALL13 0xF81F         // Pink
#define COLOR_BALL14 0xFF00         // Orange-Red
#define COLOR_BALL15 0x07E0         // Green (15-ball)
#define COLOR_BALL16 0x001F         // Dark Blue
#define COLOR_BALL17 0xF800         // Red
#define COLOR_BALL18 0xFFE0         // Yellow
#define COLOR_BALL19 0x8410         // Brown
#define COLOR_CUE_STICK 0x8410      // Brown
#define COLOR_POCKET 0x0000         // Black

struct Ball {
    float x, y;           // Position
    float vx, vy;         // Velocity
    uint16_t color;       // Ball color
    bool isActive;        // Is ball moving
};

class BilliardGame {
private:
    Adafruit_ST7789* tft;
    
    Ball balls[BALL_COUNT];
    float oldBallX[BALL_COUNT];  // Previous ball positions for erasing
    float oldBallY[BALL_COUNT];
    float cueAngle;       // Angle of cue stick (in radians)
    float oldCueAngle;    // Previous cue angle
    float cuePower;       // Power of shot (0-100)
    float oldCuePower;    // Previous power for bar update
    bool isAiming;        // Is player aiming
    bool isCharging;      // Is player charging shot
    int activeBallIndex;  // Index of cue ball (usually 0)
    bool tableDrawn;      // Has table been drawn
    
    void drawTable();
    void eraseBall(int index);
    void eraseBallAtPosition(float x, float y, int radius);  // Erase ball at specific position with proper clipping
    void drawBall(int index);
    void eraseCueStick();
    void eraseCueStickAtPosition(float angle, float power);  // Erase cue stick at specific angle and power (không phụ thuộc isAiming)
    void drawCueStick();
    void updatePhysics();
    void checkWallCollisions(Ball& ball);
    void checkBallCollisions();
    bool isBallInPocket(Ball& ball);
    void resetGame();
    void redrawBorderNear(int screenX, int screenY, int radius);  // Redraw border near erased area
    void redrawPocketsNear(float x, float y, int radius);  // Redraw pockets near ball position
    void redrawAllPockets();  // Redraw all pockets (to ensure they're always on top)
    uint16_t getBackgroundColorAt(int screenX, int screenY);  // Get background color at screen position
    bool isBallNearPocket(float x, float y);  // Kiểm tra quả bi có gần lỗ không
    void drawPowerBar();  // Vẽ thanh lực
    
public:
    BilliardGame(Adafruit_ST7789* tft);
    void init();
    void update();
    void draw();
    
    // Input handlers
    void handleAimLeft();
    void handleAimRight();
    void handleAimUp();      // Xoay lên (90 độ)
    void handleAimDown();     // Xoay xuống (270 độ)
    void handleAimAngle(float angle);  // Xoay theo góc cụ thể (radians)
    void handleAimRotate(float deltaAngle);  // Xoay thêm một góc (radians) từ góc hiện tại
    // Xoay theo 8 hướng chính
    void handleAimNorth();    // Bắc (lên, -90 độ)
    void handleAimNorthEast(); // Đông Bắc (45 độ)
    void handleAimEast();     // Đông (phải, 0 độ)
    void handleAimSouthEast(); // Đông Nam (135 độ)
    void handleAimSouth();    // Nam (xuống, 90 độ)
    void handleAimSouthWest(); // Tây Nam (225 độ)
    void handleAimWest();     // Tây (trái, 180 độ)
    void handleAimNorthWest(); // Tây Bắc (315 độ)
    void handleChargeStart();
    void handleChargeRelease();
    void handlePowerUp();      // Tăng lực khi đang charge
    void handlePowerDown();    // Giảm lực khi đang charge
    
    // Getters
    bool getIsAiming() const { return isAiming; }
    bool getIsCharging() const { return isCharging; }
    float getCueAngle() const { return cueAngle; }  // Lấy góc hiện tại của gậy
    float getCuePower() const { return cuePower; }  // Lấy lực hiện tại (0-100)
    int countActiveBalls() const;  // Đếm số quả bi còn trên bàn
    void aimAtNearestBall();  // Tự động ngắm về quả bi gần nhất
    void aimAtNearestPocket();  // Tự động ngắm thẳng vào lỗ gần nhất (để test)
};

#endif

