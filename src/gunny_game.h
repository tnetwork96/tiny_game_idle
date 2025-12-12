#ifndef GUNNY_GAME_H
#define GUNNY_GAME_H

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

// Screen dimensions
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
#define GAME_AREA_HEIGHT 280  // 240 - 40 for control area
#define CONTROL_AREA_HEIGHT 40
#define CONTROL_AREA_Y 200  // Y position of control area

// Game constants
#define PLAYER1_X 60
#define PLAYER1_Y 100  // Adjusted to match flat terrain height
#define PLAYER2_X 260  // Second player on the right side
#define PLAYER2_Y 100  // Same height as player 1
#define CURSOR_LEN 50
#define MAX_ANGLE 90
#define MIN_ANGLE 30  // Minimum angle to ensure shooting upward
#define MAX_POWER 1000
#define CHARGE_RATE 5
#define V_MAX 20.0f
#define GRAVITY 9.8f
#define TIME_STEP 0.05f
#define WIND_SCALE 0.1f

// Colors
#define COLOR_SKY 0x051F      // Sky blue
#define COLOR_SKY_LIGHT 0x05DF // Light sky blue
#define COLOR_CLOUD 0xCE79    // Light gray/white for clouds
#define COLOR_TERRAIN 0x8200  // Brown/earth color
#define COLOR_TERRAIN_DARK 0x6200 // Darker brown
#define COLOR_PLAYER 0xF800   // Red
#define COLOR_CURSOR 0xFFE0   // Yellow
#define COLOR_POWER_BAR 0xF81F // Magenta
#define COLOR_PROJECTILE 0xFFFF // White
#define COLOR_TEXT 0xFFFF     // White

class GunnyGame {
private:
    Adafruit_ST7789* tft;
    
    // Game state
    float currentAngle;      // Firing angle in degrees
    int power;               // Accumulated firing power (0-1000)
    bool isCharging;         // True if fire button is held
    bool isFiring;           // True if projectile is in flight
    bool isPlayer1Turn;      // True if player 1's turn, false for player 2
    int currentPlayerX;       // Current active player X position
    int currentPlayerY;      // Current active player Y position
    
    // Cursor tracking for optimization
    int oldCursorX, oldCursorY;
    
    // Wind
    int windSpeed;           // 0-10
    int windDirection;       // 1: Right, -1: Left
    
    // Terrain
    int terrain[320];        // Y height at each X column
    
    // Projectile state
    float projX, projY;      // Current projectile position
    float projVx, projVy;    // Current projectile velocity
    float projTime;          // Time since launch
    int oldProjX, oldProjY;  // Previous position for erasing
    
    // Animation
    uint16_t animationFrame; // For cloud animation
    
    // Physics
    float timeStep;
    float gravity;
    
    // Drawing methods
    void drawTerrain();
    void drawClouds();
    void drawPlayer();
    void drawSinglePlayer(int playerX, int playerY, uint16_t color, bool isActive);
    void drawCursor();
    void eraseCursor();
    void drawTrajectory();  // Draw predicted trajectory path
    void eraseTrajectory(); // Erase old trajectory
    void drawPowerBar();
    void drawStats();
    void drawProjectile(int x, int y);
    void eraseProjectile(int x, int y);
    
    // Game logic
    void generateTerrain();
    void calculateCursorEndpoint(int& x, int& y);
    void startFiring();
    void updateProjectile();
    bool checkImpact();
    
public:
    GunnyGame(Adafruit_ST7789* tft);
    
    void init();
    void draw();
    void update();
    
    // Control handlers
    void handleAngleUp();
    void handleAngleDown();
    void handleFirePress();
    void handleFireRelease();
    
    // Getters
    bool getIsFiring() const { return isFiring; }
    bool getIsCharging() const { return isCharging; }
};

#endif

