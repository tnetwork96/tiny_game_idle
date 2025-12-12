#include <Arduino.h>
#include "gunny_game.h"
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <math.h>

GunnyGame::GunnyGame(Adafruit_ST7789* tft) {
    this->tft = tft;
    this->currentAngle = 60.0f;  // Start with upward angle
    this->power = 0;
    this->isCharging = false;
    this->isFiring = false;
    this->isPlayer1Turn = true;  // Start with player 1
    this->currentPlayerX = PLAYER1_X;
    this->currentPlayerY = PLAYER1_Y;
    this->oldCursorX = 0;
    this->oldCursorY = 0;
    this->windSpeed = 3;
    this->windDirection = 1;
    this->timeStep = TIME_STEP;
    this->gravity = GRAVITY;
    this->projX = 0;
    this->projY = 0;
    this->projVx = 0;
    this->projVy = 0;
    this->projTime = 0;
    this->oldProjX = 0;
    this->oldProjY = 0;
    this->animationFrame = 0;
    this->isPlayer1Turn = true;  // Start with player 1
    this->currentPlayerX = PLAYER1_X;
    this->currentPlayerY = PLAYER1_Y;
}

void GunnyGame::init() {
    generateTerrain();
    draw();
}

void GunnyGame::generateTerrain() {
    // Generate flat terrain for 2 players to battle
    // Note: terrain[] stores Y height in game coordinates (0 at bottom, increases upward)
    int flatHeight = 100;  // Flat terrain height (game coordinates)
    
    for (int x = 0; x < SCREEN_WIDTH; x++) {
        // Mostly flat terrain with very slight variation for visual interest
        float variation = 2.0f * sin(x * 0.02f);  // Very small variation (only 2 pixels)
        terrain[x] = (int)(flatHeight + variation);
        
        // Ensure terrain is within reasonable bounds (game coordinates)
        if (terrain[x] < 95) terrain[x] = 95;
        if (terrain[x] > 105) terrain[x] = 105;
    }
}

void GunnyGame::drawTerrain() {
    // Draw sky with gradient (light blue at top, darker at bottom)
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        // Gradient from light blue (top) to darker blue (bottom)
        uint16_t ratio = (SCREEN_HEIGHT - y) * 255 / SCREEN_HEIGHT;
        uint8_t r = ((COLOR_SKY_LIGHT >> 11) & 0x1F) * ratio / 255;
        uint8_t g = ((COLOR_SKY_LIGHT >> 5) & 0x3F) * ratio / 255;
        uint8_t b = (COLOR_SKY_LIGHT & 0x1F) * ratio / 255;
        
        uint8_t r2 = ((COLOR_SKY >> 11) & 0x1F) * (255 - ratio) / 255;
        uint8_t g2 = ((COLOR_SKY >> 5) & 0x3F) * (255 - ratio) / 255;
        uint8_t b2 = (COLOR_SKY & 0x1F) * (255 - ratio) / 255;
        
        uint16_t color = ((r + r2) << 11) | ((g + g2) << 5) | (b + b2);
        tft->drawFastHLine(0, y, SCREEN_WIDTH, color);
    }
    
    // Draw clouds (light, fluffy)
    drawClouds();
    
    // Draw terrain (earth color)
    // terrain[x] is in game coordinates (0 at bottom), need to convert to screen coordinates
    for (int x = 0; x < SCREEN_WIDTH; x++) {
        int terrainY = terrain[x];  // Game coordinate (0 at bottom)
        int screenY = SCREEN_HEIGHT - terrainY;  // Convert to screen coordinate (0 at top)
        int height = SCREEN_HEIGHT - screenY;  // Height from terrain to bottom
        if (height > 0) {
            // Draw terrain with slight variation for texture
            uint16_t terrainColor = COLOR_TERRAIN;
            if ((x + screenY) % 3 == 0) {
                terrainColor = COLOR_TERRAIN_DARK;  // Add some texture
            }
            tft->drawFastVLine(x, screenY, height, terrainColor);
        }
    }
}

void GunnyGame::drawClouds() {
    // Draw simple clouds using circles
    // Cloud 1
    int cloud1X = 50 + (animationFrame % 200) / 10;  // Slowly moving
    int cloud1Y = 30;
    tft->fillCircle(cloud1X, cloud1Y, 8, COLOR_CLOUD);
    tft->fillCircle(cloud1X + 6, cloud1Y, 10, COLOR_CLOUD);
    tft->fillCircle(cloud1X + 12, cloud1Y, 8, COLOR_CLOUD);
    tft->fillCircle(cloud1X + 6, cloud1Y - 5, 6, COLOR_CLOUD);
    
    // Cloud 2
    int cloud2X = 200 - (animationFrame % 300) / 15;
    int cloud2Y = 50;
    tft->fillCircle(cloud2X, cloud2Y, 7, COLOR_CLOUD);
    tft->fillCircle(cloud2X + 5, cloud2Y, 9, COLOR_CLOUD);
    tft->fillCircle(cloud2X + 10, cloud2Y, 7, COLOR_CLOUD);
    tft->fillCircle(cloud2X + 5, cloud2Y - 4, 5, COLOR_CLOUD);
    
    // Cloud 3
    int cloud3X = 120 + (animationFrame % 250) / 12;
    int cloud3Y = 20;
    tft->fillCircle(cloud3X, cloud3Y, 6, COLOR_CLOUD);
    tft->fillCircle(cloud3X + 4, cloud3Y, 8, COLOR_CLOUD);
    tft->fillCircle(cloud3X + 8, cloud3Y, 6, COLOR_CLOUD);
}

void GunnyGame::drawPlayer() {
    // Draw player 1 (left side)
    drawSinglePlayer(PLAYER1_X, PLAYER1_Y, COLOR_PLAYER, isPlayer1Turn);
    
    // Draw player 2 (right side)
    uint16_t player2Color = 0x07E0;  // Green for player 2
    drawSinglePlayer(PLAYER2_X, PLAYER2_Y, player2Color, !isPlayer1Turn);
}

void GunnyGame::drawSinglePlayer(int playerX, int playerY, uint16_t color, bool isActive) {
    // Convert from game coordinates to screen coordinates
    int screenY = SCREEN_HEIGHT - playerY;
    
    // Draw small player (tank/character) - simple representation
    // Body (small rectangle)
    tft->fillRect(playerX - 4, screenY - 3, 8, 4, color);
    // Head (small circle)
    tft->fillCircle(playerX, screenY - 5, 3, color);
    
    // Only draw barrel for active player
    if (isActive) {
        // Barrel/cannon (small line pointing in firing direction)
        // Angle goes upward from player position
        float angleRad = currentAngle * (M_PI / 180.0f);
        int barrelEndX = playerX + (int)(6 * cos(angleRad));
        int barrelEndY = screenY - 3 - (int)(6 * sin(angleRad));  // Negative sin to go up on screen
        tft->drawLine(playerX, screenY - 3, barrelEndX, barrelEndY, color);
    }
}

void GunnyGame::calculateCursorEndpoint(int& x, int& y) {
    // Convert angle from degrees to radians
    float angleRad = currentAngle * (M_PI / 180.0f);
    
    // Use current active player position
    int playerX = isPlayer1Turn ? PLAYER1_X : PLAYER2_X;
    int playerY = isPlayer1Turn ? PLAYER1_Y : PLAYER2_Y;
    
    // Calculate endpoint - angle goes upward from player Y position
    // For upward angles: sin is positive, so we subtract to go up in game coordinates
    x = playerX + (int)(CURSOR_LEN * cos(angleRad));
    y = playerY + (int)(CURSOR_LEN * sin(angleRad));  // Add to go upward in game coords
    
    // Convert to screen coordinates (screen Y is inverted)
    y = SCREEN_HEIGHT - y;
}

void GunnyGame::drawCursor() {
    int x, y;
    calculateCursorEndpoint(x, y);
    
    // Draw line from current active player to cursor endpoint
    int playerX = isPlayer1Turn ? PLAYER1_X : PLAYER2_X;
    int playerY = isPlayer1Turn ? PLAYER1_Y : PLAYER2_Y;
    int playerScreenX = playerX;
    int playerScreenY = SCREEN_HEIGHT - playerY;
    
    tft->drawLine(playerScreenX, playerScreenY, x, y, COLOR_CURSOR);
    
    // Store for erasing later
    oldCursorX = x;
    oldCursorY = y;
}

void GunnyGame::eraseCursor() {
    int playerX = isPlayer1Turn ? PLAYER1_X : PLAYER2_X;
    int playerY = isPlayer1Turn ? PLAYER1_Y : PLAYER2_Y;
    int playerScreenX = playerX;
    int playerScreenY = SCREEN_HEIGHT - playerY;
    
    // Erase old cursor line by drawing with background color
    tft->drawLine(playerScreenX, playerScreenY, oldCursorX, oldCursorY, COLOR_SKY);
}

void GunnyGame::drawTrajectory() {
    // Only draw trajectory when charging (has power) and not firing
    if (!isCharging || power < 10 || isFiring) {
        return;
    }
    
    // Calculate trajectory points
    float v0 = (power / (float)MAX_POWER) * V_MAX;
    float angleRad = currentAngle * (M_PI / 180.0f);
    float v0x = v0 * cos(angleRad);
    float v0y = v0 * sin(angleRad);
    float aWind = windDirection * windSpeed * WIND_SCALE;
    
    int playerX = isPlayer1Turn ? PLAYER1_X : PLAYER2_X;
    int playerY = isPlayer1Turn ? PLAYER1_Y : PLAYER2_Y;
    
    // Draw trajectory curve (dotted line)
    for (float t = 0.0f; t < 2.0f; t += 0.1f) {
        float x = playerX + v0x * t + 0.5f * aWind * t * t;
        float y = playerY + v0y * t - 0.5f * gravity * t * t;
        
        int xInt = (int)x;
        int yInt = (int)y;
        
        // Check bounds
        if (xInt < 0 || xInt >= SCREEN_WIDTH) break;
        if (yInt < 0) break;
        
        // Check if hit terrain
        if (yInt <= terrain[xInt]) {
            break;
        }
        
        // Convert to screen coordinates
        int screenX = xInt;
        int screenY = SCREEN_HEIGHT - yInt;
        
        // Draw dotted line (every other point)
        if ((int)(t * 10) % 2 == 0) {
            tft->drawPixel(screenX, screenY, COLOR_CURSOR);
        }
    }
}

void GunnyGame::eraseTrajectory() {
    // Trajectory will be redrawn, so we don't need explicit erase
    // It will be redrawn in the next draw() call
}

void GunnyGame::drawPowerBar() {
    // Draw power bar in control area
    int barX = 10;
    int barY = CONTROL_AREA_Y + 10;
    int barWidth = 200;
    int barHeight = 15;
    
    // Draw frame
    tft->drawRect(barX, barY, barWidth, barHeight, COLOR_TEXT);
    
    // Draw fill
    int fillWidth = (power * barWidth) / MAX_POWER;
    if (fillWidth > 0) {
        tft->fillRect(barX + 1, barY + 1, fillWidth - 2, barHeight - 2, COLOR_POWER_BAR);
    }
}

void GunnyGame::drawStats() {
    // Draw angle and wind info in control area
    tft->setTextSize(1);
    tft->setTextColor(COLOR_TEXT, COLOR_SKY);
    
    // Angle
    tft->setCursor(220, CONTROL_AREA_Y + 5);
    tft->print("A:");
    tft->print((int)currentAngle);
    tft->print("o");
    
    // Wind
    tft->setCursor(220, CONTROL_AREA_Y + 15);
    tft->print("W:");
    if (windDirection > 0) {
        tft->print("->");
    } else {
        tft->print("<-");
    }
    tft->print(windSpeed);
}

void GunnyGame::drawProjectile(int x, int y) {
    // Convert game Y to screen Y
    int screenY = SCREEN_HEIGHT - y;
    
    // Draw projectile as small circle
    tft->fillCircle(x, screenY, 3, COLOR_PROJECTILE);
    
    // Store position for erasing
    oldProjX = x;
    oldProjY = screenY;
}

void GunnyGame::eraseProjectile(int x, int y) {
    // Erase old projectile position
    // Need to redraw sky or terrain at that position
    int screenY = SCREEN_HEIGHT - y;  // Convert game Y to screen Y
    int terrainScreenY = SCREEN_HEIGHT - terrain[x];  // Terrain in screen coordinates
    
    // Redraw sky above terrain, terrain below
    if (screenY < terrainScreenY) {
        // In sky area - redraw sky color (with gradient)
        uint16_t ratio = (SCREEN_HEIGHT - screenY) * 255 / SCREEN_HEIGHT;
        uint8_t r = ((COLOR_SKY_LIGHT >> 11) & 0x1F) * ratio / 255;
        uint8_t g = ((COLOR_SKY_LIGHT >> 5) & 0x3F) * ratio / 255;
        uint8_t b = (COLOR_SKY_LIGHT & 0x1F) * ratio / 255;
        uint8_t r2 = ((COLOR_SKY >> 11) & 0x1F) * (255 - ratio) / 255;
        uint8_t g2 = ((COLOR_SKY >> 5) & 0x3F) * (255 - ratio) / 255;
        uint8_t b2 = (COLOR_SKY & 0x1F) * (255 - ratio) / 255;
        uint16_t skyColor = ((r + r2) << 11) | ((g + g2) << 5) | (b + b2);
        tft->drawPixel(x, screenY, skyColor);
    } else {
        // On terrain - redraw terrain color
        uint16_t terrainColor = COLOR_TERRAIN;
        if ((x + screenY) % 3 == 0) {
            terrainColor = COLOR_TERRAIN_DARK;
        }
        tft->drawPixel(x, screenY, terrainColor);
    }
}

void GunnyGame::draw() {
    drawTerrain();
    drawPlayer();
    drawTrajectory();  // Draw predicted trajectory
    drawCursor();
    drawPowerBar();
    drawStats();
    
    // Update current player position
    currentPlayerX = isPlayer1Turn ? PLAYER1_X : PLAYER2_X;
    currentPlayerY = isPlayer1Turn ? PLAYER1_Y : PLAYER2_Y;
}

void GunnyGame::update() {
    // Update animation frame
    animationFrame++;
    if (animationFrame > 10000) animationFrame = 0;
    
    // Update charging power
    if (isCharging && !isFiring) {
        power += CHARGE_RATE;
        if (power > MAX_POWER) {
            power = MAX_POWER;
        }
        drawPowerBar();
        // Redraw trajectory as power changes
        eraseTrajectory();
        drawTrajectory();
    }
    
    // Update projectile physics
    if (isFiring) {
        updateProjectile();
    }
}

void GunnyGame::handleAngleUp() {
    if (!isFiring) {
        currentAngle += 2.0f;
        if (currentAngle > MAX_ANGLE) currentAngle = MAX_ANGLE;
        
        eraseCursor();
        drawCursor();
        drawStats();
    }
}

void GunnyGame::handleAngleDown() {
    if (!isFiring) {
        currentAngle -= 2.0f;
        if (currentAngle < MIN_ANGLE) currentAngle = MIN_ANGLE;
        
        eraseCursor();
        drawCursor();
        drawStats();
    }
}

void GunnyGame::handleFirePress() {
    if (!isFiring) {
        isCharging = true;
        power = 0;
    }
}

void GunnyGame::handleFireRelease() {
    if (isCharging && !isFiring && power > 0) {
        isCharging = false;
        startFiring();
    }
}

void GunnyGame::startFiring() {
    // Save power before resetting
    int savedPower = power;
    if (savedPower < 50) savedPower = 50;  // Minimum power
    
    // Calculate initial velocity from power
    float v0 = (savedPower / (float)MAX_POWER) * V_MAX;
    
    // Convert angle to radians
    float angleRad = currentAngle * (M_PI / 180.0f);
    
    // Calculate initial velocities
    projVx = v0 * cos(angleRad);
    projVy = v0 * sin(angleRad);
    
    // Set initial position (at current active player position)
    projX = (float)currentPlayerX;
    projY = (float)currentPlayerY + 5;  // Start slightly above player
    projTime = 0.0f;
    
    isFiring = true;
    power = 0;
    
    Serial.print("Firing from player ");
    Serial.print(isPlayer1Turn ? "1" : "2");
    Serial.print("! Power: ");
    Serial.print(savedPower);
    Serial.print(", Angle: ");
    Serial.print(currentAngle);
    Serial.print(", v0: ");
    Serial.println(v0);
    
    // Draw initial projectile
    drawProjectile((int)projX, (int)projY);
}

void GunnyGame::updateProjectile() {
    // Erase old position
    eraseProjectile((int)projX, (int)projY);
    
    // Calculate wind acceleration
    float aWind = windDirection * windSpeed * WIND_SCALE;
    
    // Update time
    projTime += timeStep;
    
    // Calculate new position using physics equations
    // x(t) = v0x * t + 0.5 * a_wind * t^2
    // y(t) = v0y * t - 0.5 * g * t^2
    float newX = projX + projVx * projTime + 0.5f * aWind * projTime * projTime;
    float newY = projY + projVy * projTime - 0.5f * gravity * projTime * projTime;
    
    // Check for impact
    int xInt = (int)newX;
    if (xInt < 0 || xInt >= SCREEN_WIDTH) {
        // Out of bounds
        isFiring = false;
        draw();  // Redraw everything
        return;
    }
    
    // Check if hit terrain
    if (newY <= terrain[xInt]) {
        // Impact!
        isFiring = false;
        
        // Draw explosion/splash effect
        int impactScreenY = SCREEN_HEIGHT - terrain[xInt];
        tft->fillCircle(xInt, impactScreenY, 10, COLOR_PROJECTILE);
        delay(200);
        
        // Check if hit opponent
        int targetX = isPlayer1Turn ? PLAYER2_X : PLAYER1_X;
        int targetY = isPlayer1Turn ? PLAYER2_Y : PLAYER1_Y;
        if (abs(xInt - targetX) < 15 && abs((int)projY - targetY) < 15) {
            Serial.println("HIT! Target destroyed!");
            // Could add hit effect here
        }
        
        // Switch turns
        isPlayer1Turn = !isPlayer1Turn;
        currentAngle = 60.0f;  // Reset angle for next player (upward)
        
        // Redraw everything
        draw();
        return;
    }
    
    // Update position
    projX = newX;
    projY = newY;
    
    // Draw new position
    drawProjectile((int)projX, (int)projY);
    
    // Small delay to control simulation speed
    delay(10);
}

bool GunnyGame::checkImpact() {
    int xInt = (int)projX;
    if (xInt < 0 || xInt >= SCREEN_WIDTH) {
        return true;
    }
    return projY <= terrain[xInt];
}

