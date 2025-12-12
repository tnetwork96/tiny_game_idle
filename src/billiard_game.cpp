#include <Arduino.h>
#include "billiard_game.h"

BilliardGame::BilliardGame(Adafruit_ST7789* tft) {
    this->tft = tft;
    this->cueAngle = 0.0f;
    this->oldCueAngle = 0.0f;
    this->cuePower = 0.0f;
    this->oldCuePower = 0.0f;
    this->isAiming = true;
    this->isCharging = false;
    this->activeBallIndex = 0;
    this->tableDrawn = false;
    
    // Initialize old positions
    for (int i = 0; i < BALL_COUNT; i++) {
        oldBallX[i] = 0;
        oldBallY[i] = 0;
    }
}

void BilliardGame::init() {
    resetGame();
    tableDrawn = false;
    // Draw table once
    drawTable();
    tableDrawn = true;
    
    // Draw all balls at initial positions
    for (int i = 0; i < BALL_COUNT; i++) {
        oldBallX[i] = balls[i].x;
        oldBallY[i] = balls[i].y;
        drawBall(i);
    }
    
    // Draw cue stick
    if (isAiming) {
        drawCueStick();
        oldCueAngle = cueAngle;
    }
}

void BilliardGame::resetGame() {
    // Initialize cue ball (white) at bottom left
    balls[0].x = TABLE_X + 80;
    balls[0].y = TABLE_Y + TABLE_HEIGHT - 40;
    balls[0].vx = 0.0f;
    balls[0].vy = 0.0f;
    balls[0].color = COLOR_CUE_BALL;
    balls[0].isActive = false;
    
    // Initialize target balls in triangle formation
    float startX = TABLE_X + TABLE_WIDTH - 100;
    float startY = TABLE_Y + TABLE_HEIGHT / 2;
    
    // Ball 1 (Red)
    balls[1].x = startX;
    balls[1].y = startY;
    balls[1].vx = 0.0f;
    balls[1].vy = 0.0f;
    balls[1].color = COLOR_BALL1;
    balls[1].isActive = false;
    
    // Ball 2 (Yellow)
    balls[2].x = startX - 20;
    balls[2].y = startY - 12;
    balls[2].vx = 0.0f;
    balls[2].vy = 0.0f;
    balls[2].color = COLOR_BALL2;
    balls[2].isActive = false;
    
    // Ball 3 (Green)
    balls[3].x = startX - 20;
    balls[3].y = startY + 12;
    balls[3].vx = 0.0f;
    balls[3].vy = 0.0f;
    balls[3].color = COLOR_BALL3;
    balls[3].isActive = false;
    
    // Ball 4 (Blue)
    balls[4].x = startX - 40;
    balls[4].y = startY;
    balls[4].vx = 0.0f;
    balls[4].vy = 0.0f;
    balls[4].color = COLOR_BALL4;
    balls[4].isActive = false;
    
    cueAngle = 0.0f;
    oldCueAngle = 0.0f;
    cuePower = 0.0f;
    oldCuePower = 0.0f;
    isAiming = true;
    isCharging = false;
    
    // Initialize old positions
    for (int i = 0; i < BALL_COUNT; i++) {
        oldBallX[i] = balls[i].x;
        oldBallY[i] = balls[i].y;
    }
}

void BilliardGame::drawTable() {
    // Draw black background around table first (to cover any ball parts outside table)
    tft->fillScreen(0x0000);  // Black background
    
    // Draw table background (green)
    tft->fillRect(TABLE_X, TABLE_Y, TABLE_WIDTH, TABLE_HEIGHT, COLOR_TABLE);
    
    // Draw table rails (thành bàn) - vẽ dày và rõ ràng TRƯỚC
    int railThickness = 4;  // Độ dày thành bàn (tăng lên để rõ hơn)
    int topScreenY = SCREEN_HEIGHT - TABLE_Y;
    int bottomScreenY = SCREEN_HEIGHT - (TABLE_Y + TABLE_HEIGHT);
    
    // Top rail (thành trên) - vẽ dày
    for (int i = 0; i < railThickness; i++) {
        tft->drawLine(TABLE_X, topScreenY + i, TABLE_X + TABLE_WIDTH, topScreenY + i, COLOR_TABLE_BORDER);
    }
    
    // Bottom rail (thành dưới) - vẽ dày
    for (int i = 0; i < railThickness; i++) {
        tft->drawLine(TABLE_X, bottomScreenY - i, TABLE_X + TABLE_WIDTH, bottomScreenY - i, COLOR_TABLE_BORDER);
    }
    
    // Left rail (thành trái) - vẽ dày
    for (int i = 0; i < railThickness; i++) {
        tft->drawLine(TABLE_X - i, bottomScreenY, TABLE_X - i, topScreenY, COLOR_TABLE_BORDER);
    }
    
    // Right rail (thành phải) - vẽ dày
    for (int i = 0; i < railThickness; i++) {
        tft->drawLine(TABLE_X + TABLE_WIDTH + i, bottomScreenY, TABLE_X + TABLE_WIDTH + i, topScreenY, COLOR_TABLE_BORDER);
    }
    
    // Draw outer border for extra clarity
    tft->drawRect(TABLE_X - railThickness, TABLE_Y, TABLE_WIDTH + railThickness * 2, TABLE_HEIGHT, COLOR_TABLE_BORDER);
    
    // Draw pockets (6 pockets) - VẼ ĐÈ LÊN THÀNH BÀN (sau khi vẽ thành bàn)
    int pocketScreenY;
    
    // Top-left corner pocket - vẽ đè lên thành bàn
    pocketScreenY = SCREEN_HEIGHT - TABLE_Y;
    tft->fillCircle(TABLE_X, pocketScreenY, POCKET_RADIUS, COLOR_POCKET);
    tft->drawCircle(TABLE_X, pocketScreenY, POCKET_RADIUS, COLOR_TABLE_BORDER);  // Viền nâu
    
    // Top-right corner pocket - vẽ đè lên thành bàn
    tft->fillCircle(TABLE_X + TABLE_WIDTH, pocketScreenY, POCKET_RADIUS, COLOR_POCKET);
    tft->drawCircle(TABLE_X + TABLE_WIDTH, pocketScreenY, POCKET_RADIUS, COLOR_TABLE_BORDER);  // Viền nâu
    
    // Bottom-left corner pocket - vẽ đè lên thành bàn
    pocketScreenY = SCREEN_HEIGHT - (TABLE_Y + TABLE_HEIGHT);
    tft->fillCircle(TABLE_X, pocketScreenY, POCKET_RADIUS, COLOR_POCKET);
    tft->drawCircle(TABLE_X, pocketScreenY, POCKET_RADIUS, COLOR_TABLE_BORDER);  // Viền nâu
    
    // Bottom-right corner pocket - vẽ đè lên thành bàn
    tft->fillCircle(TABLE_X + TABLE_WIDTH, pocketScreenY, POCKET_RADIUS, COLOR_POCKET);
    tft->drawCircle(TABLE_X + TABLE_WIDTH, pocketScreenY, POCKET_RADIUS, COLOR_TABLE_BORDER);  // Viền nâu
    
    // Top middle pocket - vẽ đè lên thành bàn
    pocketScreenY = SCREEN_HEIGHT - TABLE_Y;
    tft->fillCircle(TABLE_X + TABLE_WIDTH / 2, pocketScreenY, POCKET_RADIUS, COLOR_POCKET);
    tft->drawCircle(TABLE_X + TABLE_WIDTH / 2, pocketScreenY, POCKET_RADIUS, COLOR_TABLE_BORDER);  // Viền nâu
    
    // Bottom middle pocket - vẽ đè lên thành bàn
    pocketScreenY = SCREEN_HEIGHT - (TABLE_Y + TABLE_HEIGHT);
    tft->fillCircle(TABLE_X + TABLE_WIDTH / 2, pocketScreenY, POCKET_RADIUS, COLOR_POCKET);
    tft->drawCircle(TABLE_X + TABLE_WIDTH / 2, pocketScreenY, POCKET_RADIUS, COLOR_TABLE_BORDER);  // Viền nâu
}

void BilliardGame::eraseBall(int index) {
    // Erase old ball position by redrawing table background and border
    // Use pixel-by-pixel drawing to properly handle ball crossing table border
    float oldX = oldBallX[index];
    float oldY = oldBallY[index];
    float newX = balls[index].x;
    float newY = balls[index].y;
    
    // Calculate distance moved
    float dx = newX - oldX;
    float dy = newY - oldY;
    float distance = sqrt(dx * dx + dy * dy);
    
    // Erase radius (slightly larger than ball to ensure clean erase)
    int eraseRadius = BALL_RADIUS + 2;
    
    // Background color (black for outside table, green for inside)
    uint16_t bgColor = 0x0000;  // Black background
    
    // If ball moved significantly, erase the path
    if (distance > BALL_RADIUS * 2) {
        // Erase multiple points along the path to prevent trails
        int steps = (int)(distance / (BALL_RADIUS * 0.5f)) + 1;
        for (int i = 0; i <= steps; i++) {
            float t = (float)i / steps;
            float x = oldX + dx * t;
            float y = oldY + dy * t;
            
            eraseBallAtPosition(x, y, eraseRadius);
        }
    } else {
        // Just erase the old position
        eraseBallAtPosition(oldX, oldY, eraseRadius);
    }
}

void BilliardGame::eraseBallAtPosition(float x, float y, int radius) {
    int screenX = (int)x;
    int screenY = SCREEN_HEIGHT - (int)y;
    
    // Only erase if within reasonable bounds to avoid drawing off-screen
    if (screenX < -radius || screenX > SCREEN_WIDTH + radius ||
        screenY < -radius || screenY > SCREEN_HEIGHT + radius) {
        return;
    }
    
    // Calculate bounding box
    int minX = screenX - radius;
    int maxX = screenX + radius;
    int minY = screenY - radius;
    int maxY = screenY + radius;
    
    // Clip to screen bounds
    minX = (minX < 0) ? 0 : minX;
    maxX = (maxX >= SCREEN_WIDTH) ? SCREEN_WIDTH - 1 : maxX;
    minY = (minY < 0) ? 0 : minY;
    maxY = (maxY >= SCREEN_HEIGHT) ? SCREEN_HEIGHT - 1 : maxY;
    
    // Only erase the part inside the table (green area)
    // The part outside will remain black (background)
    int tableScreenY = SCREEN_HEIGHT - TABLE_Y;
    int tableBottomScreenY = SCREEN_HEIGHT - (TABLE_Y + TABLE_HEIGHT);
    
    // Clip to table bounds
    int tableMinX = TABLE_X;
    int tableMaxX = TABLE_X + TABLE_WIDTH;
    int tableMinY = tableBottomScreenY;
    int tableMaxY = tableScreenY;
    
    // Find intersection of ball area and table area
    int eraseMinX = (minX > tableMinX) ? minX : tableMinX;
    int eraseMaxX = (maxX < tableMaxX) ? maxX : tableMaxX;
    int eraseMinY = (minY > tableMinY) ? minY : tableMinY;
    int eraseMaxY = (maxY < tableMaxY) ? maxY : tableMaxY;
    
    // Erase the entire ball circle - draw black outside table, green inside table
    for (int py = minY; py <= maxY; py++) {
        for (int px = minX; px <= maxX; px++) {
            // Check if pixel is within circle
            int dx = px - screenX;
            int dy = py - screenY;
            int distSq = dx * dx + dy * dy;
            
            if (distSq <= radius * radius) {
                // This pixel is inside the ball circle
                // Check if it's inside or outside table
                bool insideTable = (px >= tableMinX && px <= tableMaxX && 
                                   py >= tableMinY && py <= tableMaxY);
                
                if (insideTable) {
                    // Inside table - erase with table color (green)
                    tft->drawPixel(px, py, COLOR_TABLE);
                } else {
                    // Outside table - erase with black
                    tft->drawPixel(px, py, 0x0000);
                }
            }
        }
    }
    
    // Also erase parts that intersect with table (for border redraw)
    if (eraseMinX <= eraseMaxX && eraseMinY <= eraseMaxY) {
        // This section is already handled above, but keep for border redraw logic
    }
    
    // Redraw border and pockets if near table edges
    if (x >= TABLE_X - radius && x <= TABLE_X + TABLE_WIDTH + radius &&
        y >= TABLE_Y - radius && y <= TABLE_Y + TABLE_HEIGHT + radius) {
        redrawBorderNear(screenX, screenY, radius);
        redrawPocketsNear(x, y, radius);
    }
}

void BilliardGame::redrawAllPockets() {
    // Vẽ lại TẤT CẢ các lỗ để đảm bảo chúng luôn đè lên thành bàn
    int pocketScreenY;
    
    // Top-left corner pocket
    pocketScreenY = SCREEN_HEIGHT - TABLE_Y;
    tft->fillCircle(TABLE_X, pocketScreenY, POCKET_RADIUS, COLOR_POCKET);
    tft->drawCircle(TABLE_X, pocketScreenY, POCKET_RADIUS, COLOR_TABLE_BORDER);
    
    // Top-right corner pocket
    tft->fillCircle(TABLE_X + TABLE_WIDTH, pocketScreenY, POCKET_RADIUS, COLOR_POCKET);
    tft->drawCircle(TABLE_X + TABLE_WIDTH, pocketScreenY, POCKET_RADIUS, COLOR_TABLE_BORDER);
    
    // Bottom-left corner pocket
    pocketScreenY = SCREEN_HEIGHT - (TABLE_Y + TABLE_HEIGHT);
    tft->fillCircle(TABLE_X, pocketScreenY, POCKET_RADIUS, COLOR_POCKET);
    tft->drawCircle(TABLE_X, pocketScreenY, POCKET_RADIUS, COLOR_TABLE_BORDER);
    
    // Bottom-right corner pocket
    tft->fillCircle(TABLE_X + TABLE_WIDTH, pocketScreenY, POCKET_RADIUS, COLOR_POCKET);
    tft->drawCircle(TABLE_X + TABLE_WIDTH, pocketScreenY, POCKET_RADIUS, COLOR_TABLE_BORDER);
    
    // Top middle pocket
    pocketScreenY = SCREEN_HEIGHT - TABLE_Y;
    tft->fillCircle(TABLE_X + TABLE_WIDTH / 2, pocketScreenY, POCKET_RADIUS, COLOR_POCKET);
    tft->drawCircle(TABLE_X + TABLE_WIDTH / 2, pocketScreenY, POCKET_RADIUS, COLOR_TABLE_BORDER);
    
    // Bottom middle pocket
    pocketScreenY = SCREEN_HEIGHT - (TABLE_Y + TABLE_HEIGHT);
    tft->fillCircle(TABLE_X + TABLE_WIDTH / 2, pocketScreenY, POCKET_RADIUS, COLOR_POCKET);
    tft->drawCircle(TABLE_X + TABLE_WIDTH / 2, pocketScreenY, POCKET_RADIUS, COLOR_TABLE_BORDER);
}

void BilliardGame::redrawPocketsNear(float x, float y, int radius) {
    // Redraw all pockets that are near the ball position
    // This ensures pockets are always visible and not covered by ball erase
    
    int pockets[6][2] = {
        {TABLE_X, TABLE_Y},  // Top-left
        {TABLE_X + TABLE_WIDTH, TABLE_Y},  // Top-right
        {TABLE_X, TABLE_Y + TABLE_HEIGHT},  // Bottom-left
        {TABLE_X + TABLE_WIDTH, TABLE_Y + TABLE_HEIGHT},  // Bottom-right
        {TABLE_X + TABLE_WIDTH / 2, TABLE_Y},  // Top middle
        {TABLE_X + TABLE_WIDTH / 2, TABLE_Y + TABLE_HEIGHT}  // Bottom middle
    };
    
    for (int i = 0; i < 6; i++) {
        float dx = x - pockets[i][0];
        float dy = y - pockets[i][1];
        float distance = sqrt(dx * dx + dy * dy);
        
        // If ball is near this pocket (within radius + pocket radius), redraw it
        if (distance < radius + POCKET_RADIUS + 5) {
            int pocketScreenX = pockets[i][0];
            int pocketScreenY = SCREEN_HEIGHT - pockets[i][1];
            
            // Redraw pocket (black circle) - VẼ SAU CÙNG để đè lên thành bàn và góc bàn
            // Không vẽ lại góc bàn, để lỗ đè lên góc bàn
            tft->fillCircle(pocketScreenX, pocketScreenY, POCKET_RADIUS, COLOR_POCKET);
            tft->drawCircle(pocketScreenX, pocketScreenY, POCKET_RADIUS, COLOR_TABLE_BORDER);  // Viền nâu
        }
    }
}

void BilliardGame::redrawBorderNear(int screenX, int screenY, int radius) {
    // Always redraw all rails (thành bàn) when ball is near any edge
    // NHƯNG không vẽ lại các đoạn gần lỗ để tránh nháy
    
    int railThickness = 4;  // Độ dày thành bàn
    int topScreenY = SCREEN_HEIGHT - TABLE_Y;
    int bottomScreenY = SCREEN_HEIGHT - (TABLE_Y + TABLE_HEIGHT);
    int pocketSkipRadius = POCKET_RADIUS + 2;  // Bỏ qua đoạn này quanh mỗi lỗ
    
    // Top rail (thành trên) - vẽ dày, nhưng bỏ qua các đoạn gần lỗ
    for (int i = 0; i < railThickness; i++) {
        // Đoạn 1: từ TABLE_X + pocketSkipRadius đến TABLE_X + TABLE_WIDTH/2 - pocketSkipRadius (tránh góc trái và lỗ giữa)
        if (TABLE_X + pocketSkipRadius < TABLE_X + TABLE_WIDTH/2 - pocketSkipRadius) {
            tft->drawLine(TABLE_X + pocketSkipRadius, topScreenY + i, 
                         TABLE_X + TABLE_WIDTH/2 - pocketSkipRadius, topScreenY + i, COLOR_TABLE_BORDER);
        }
        // Đoạn 2: từ TABLE_X + TABLE_WIDTH/2 + pocketSkipRadius đến TABLE_X + TABLE_WIDTH - pocketSkipRadius (tránh lỗ giữa và góc phải)
        if (TABLE_X + TABLE_WIDTH/2 + pocketSkipRadius < TABLE_X + TABLE_WIDTH - pocketSkipRadius) {
            tft->drawLine(TABLE_X + TABLE_WIDTH/2 + pocketSkipRadius, topScreenY + i, 
                         TABLE_X + TABLE_WIDTH - pocketSkipRadius, topScreenY + i, COLOR_TABLE_BORDER);
        }
    }
    
    // Bottom rail (thành dưới) - vẽ dày, nhưng bỏ qua các đoạn gần lỗ
    for (int i = 0; i < railThickness; i++) {
        // Đoạn 1: từ TABLE_X + pocketSkipRadius đến TABLE_X + TABLE_WIDTH/2 - pocketSkipRadius
        if (TABLE_X + pocketSkipRadius < TABLE_X + TABLE_WIDTH/2 - pocketSkipRadius) {
            tft->drawLine(TABLE_X + pocketSkipRadius, bottomScreenY - i, 
                         TABLE_X + TABLE_WIDTH/2 - pocketSkipRadius, bottomScreenY - i, COLOR_TABLE_BORDER);
        }
        // Đoạn 2: từ TABLE_X + TABLE_WIDTH/2 + pocketSkipRadius đến TABLE_X + TABLE_WIDTH - pocketSkipRadius
        if (TABLE_X + TABLE_WIDTH/2 + pocketSkipRadius < TABLE_X + TABLE_WIDTH - pocketSkipRadius) {
            tft->drawLine(TABLE_X + TABLE_WIDTH/2 + pocketSkipRadius, bottomScreenY - i, 
                         TABLE_X + TABLE_WIDTH - pocketSkipRadius, bottomScreenY - i, COLOR_TABLE_BORDER);
        }
    }
    
    // Left rail (thành trái) - vẽ dày, nhưng bỏ qua các đoạn gần lỗ
    for (int i = 0; i < railThickness; i++) {
        // Đoạn 1: từ bottomScreenY + pocketSkipRadius đến topScreenY - pocketSkipRadius (tránh góc dưới và góc trên)
        if (bottomScreenY + pocketSkipRadius < topScreenY - pocketSkipRadius) {
            tft->drawLine(TABLE_X - i, bottomScreenY + pocketSkipRadius, 
                         TABLE_X - i, topScreenY - pocketSkipRadius, COLOR_TABLE_BORDER);
        }
    }
    
    // Right rail (thành phải) - vẽ dày, nhưng bỏ qua các đoạn gần lỗ
    for (int i = 0; i < railThickness; i++) {
        // Đoạn 1: từ bottomScreenY + pocketSkipRadius đến topScreenY - pocketSkipRadius (tránh góc dưới và góc trên)
        if (bottomScreenY + pocketSkipRadius < topScreenY - pocketSkipRadius) {
            tft->drawLine(TABLE_X + TABLE_WIDTH + i, bottomScreenY + pocketSkipRadius, 
                         TABLE_X + TABLE_WIDTH + i, topScreenY - pocketSkipRadius, COLOR_TABLE_BORDER);
        }
    }
    
    // Vẽ lại TẤT CẢ các lỗ đè lên thành bàn (sau khi vẽ thành bàn)
    // Điều này đảm bảo lỗ luôn hiển thị đúng, không bị thành bàn che
    redrawAllPockets();
}

void BilliardGame::drawBall(int index) {
    Ball& ball = balls[index];
    
    if (ball.x < TABLE_X - BALL_RADIUS || ball.x > TABLE_X + TABLE_WIDTH + BALL_RADIUS ||
        ball.y < TABLE_Y - BALL_RADIUS || ball.y > TABLE_Y + TABLE_HEIGHT + BALL_RADIUS) {
        return;  // Ball is out of bounds (in pocket)
    }
    
    // Convert to screen coordinates (Y-axis is inverted)
    int screenX = (int)ball.x;
    int screenY = SCREEN_HEIGHT - (int)ball.y;
    int radius = BALL_RADIUS;
    
    // Draw ball using fillCircle - this ensures perfect circle shape
    // The ball will be drawn completely, even if part is outside table
    // This keeps the ball's shape and color intact when hitting the border
    
    // Draw ball shadow first (darker circle, offset)
    tft->fillCircle(screenX - 1, screenY + 1, radius, 0x2104);
    
    // Draw ball main body (full circle, keeps shape even when crossing border)
    tft->fillCircle(screenX, screenY, radius, ball.color);
    
    // Draw ball highlight (white dot) - only for colored balls
    if (ball.color != COLOR_CUE_BALL) {
        tft->fillCircle(screenX - 2, screenY - 2, 2, 0xFFFF);
    }
    
    // Update old position
    oldBallX[index] = ball.x;
    oldBallY[index] = ball.y;
}

void BilliardGame::eraseCueStick() {
    if (!isAiming || balls[activeBallIndex].isActive) {
        return;
    }
    
    Ball& cueBall = balls[activeBallIndex];
    int screenX = (int)cueBall.x;
    int screenY = SCREEN_HEIGHT - (int)cueBall.y;
    
    // Calculate old cue stick position (gậy nằm phía sau, ngược với hướng bắn)
    float oldStickLength = 40.0f + oldCuePower * 0.3f;
    float oldStickEndX = cueBall.x - cos(oldCueAngle) * (BALL_RADIUS + oldStickLength);
    float oldStickEndY = cueBall.y - sin(oldCueAngle) * (BALL_RADIUS + oldStickLength);
    
    int oldStickEndScreenX = (int)oldStickEndX;
    int oldStickEndScreenY = SCREEN_HEIGHT - (int)oldStickEndY;
    
    // Erase cue stick by drawing correct background color pixel-by-pixel
    // Sử dụng Bresenham line algorithm để vẽ lại từng pixel với màu nền đúng
    int x0 = screenX;
    int y0 = screenY;
    int x1 = oldStickEndScreenX;
    int y1 = oldStickEndScreenY;
    
    // Vẽ lại 3 đường (thick line) với màu nền đúng
    for (int offset = -1; offset <= 1; offset++) {
        int x0_offset = x0 + offset;
        int y0_offset = y0;
        int x1_offset = x1 + offset;
        int y1_offset = y1;
        
        // Bresenham line algorithm
        int dx = abs(x1_offset - x0_offset);
        int dy = abs(y1_offset - y0_offset);
        int sx = (x0_offset < x1_offset) ? 1 : -1;
        int sy = (y0_offset < y1_offset) ? 1 : -1;
        int err = dx - dy;
        
        int x = x0_offset;
        int y = y0_offset;
        
        while (true) {
            // Xác định màu nền tại vị trí (x, y)
            uint16_t bgColor = getBackgroundColorAt(x, y);
            tft->drawPixel(x, y, bgColor);
            
            if (x == x1_offset && y == y1_offset) break;
            
            int e2 = 2 * err;
            if (e2 > -dy) {
                err -= dy;
                x += sx;
            }
            if (e2 < dx) {
                err += dx;
                y += sy;
            }
        }
    }
}

uint16_t BilliardGame::getBackgroundColorAt(int screenX, int screenY) {
    // Chuyển đổi screen coordinates về game coordinates
    float gameY = SCREEN_HEIGHT - screenY;
    
    // Kiểm tra xem có nằm trong bàn không
    if (screenX >= TABLE_X && screenX < TABLE_X + TABLE_WIDTH &&
        gameY >= TABLE_Y && gameY < TABLE_Y + TABLE_HEIGHT) {
        // Nằm trong bàn - kiểm tra xem có phải thành bàn không
        int railThickness = 4;
        int topScreenY = SCREEN_HEIGHT - TABLE_Y;
        int bottomScreenY = SCREEN_HEIGHT - (TABLE_Y + TABLE_HEIGHT);
        
        // Kiểm tra thành trên
        if (screenY >= topScreenY && screenY < topScreenY + railThickness) {
            return COLOR_TABLE_BORDER;
        }
        // Kiểm tra thành dưới
        if (screenY <= bottomScreenY && screenY > bottomScreenY - railThickness) {
            return COLOR_TABLE_BORDER;
        }
        // Kiểm tra thành trái
        if (screenX < TABLE_X && screenX >= TABLE_X - railThickness) {
            return COLOR_TABLE_BORDER;
        }
        // Kiểm tra thành phải
        if (screenX >= TABLE_X + TABLE_WIDTH && screenX < TABLE_X + TABLE_WIDTH + railThickness) {
            return COLOR_TABLE_BORDER;
        }
        
        // Nằm trong bàn, không phải thành bàn
        return COLOR_TABLE;
    }
    
    // Nằm ngoài bàn
    return 0x0000;  // Màu đen
}

void BilliardGame::drawCueStick() {
    if (!isAiming || balls[activeBallIndex].isActive) {
        return;  // Don't draw cue stick if not aiming or ball is moving
    }
    
    Ball& cueBall = balls[activeBallIndex];
    int screenX = (int)cueBall.x;
    int screenY = SCREEN_HEIGHT - (int)cueBall.y;
    
    // Calculate cue stick position
    // Gậy nằm phía sau quả bi (ngược với hướng bắn), đẩy quả bi về phía trước
    float stickLength = 40.0f + cuePower * 0.3f;  // Stick extends based on power
    float stickEndX = cueBall.x - cos(cueAngle) * (BALL_RADIUS + stickLength);
    float stickEndY = cueBall.y - sin(cueAngle) * (BALL_RADIUS + stickLength);
    
    int stickStartX = screenX;
    int stickStartY = screenY;
    int stickEndScreenX = (int)stickEndX;
    int stickEndScreenY = SCREEN_HEIGHT - (int)stickEndY;
    
    // Draw cue stick (thick line)
    tft->drawLine(stickStartX, stickStartY, stickEndScreenX, stickEndScreenY, COLOR_CUE_STICK);
    tft->drawLine(stickStartX - 1, stickStartY, stickEndScreenX - 1, stickEndScreenY, COLOR_CUE_STICK);
    tft->drawLine(stickStartX + 1, stickStartY, stickEndScreenX + 1, stickEndScreenY, COLOR_CUE_STICK);
    
    // Update old values
    oldCueAngle = cueAngle;
    oldCuePower = cuePower;
}

void BilliardGame::draw() {
    // Only draw table once
    if (!tableDrawn) {
        drawTable();
        tableDrawn = true;
    }
    
    // Update balls (erase old, draw new)
    for (int i = 0; i < BALL_COUNT; i++) {
        // Skip if ball is pocketed
        if (balls[i].x < 0) continue;
        
        // Always erase and redraw if ball is active (moving)
        // This ensures no trails are left
        if (balls[i].isActive) {
            eraseBall(i);
            drawBall(i);
        } else {
            // For stationary balls, only update if position changed
            if (balls[i].x != oldBallX[i] || balls[i].y != oldBallY[i]) {
                eraseBall(i);
                drawBall(i);
            }
        }
    }
    
    // Update cue stick if angle or power changed
    if (isAiming && !balls[activeBallIndex].isActive) {
        if (cueAngle != oldCueAngle || cuePower != oldCuePower) {
            eraseCueStick();
            drawCueStick();
        }
    }
    
    // Update power bar only if power changed
    if (isCharging && cuePower != oldCuePower) {
        int barX = 10;
        int barY = SCREEN_HEIGHT - 30;
        int barWidth = 100;
        int barHeight = 10;
        
        // Clear old fill
        int oldFillWidth = (int)(oldCuePower * barWidth / 100.0f);
        tft->fillRect(barX, barY, oldFillWidth, barHeight, COLOR_TABLE);
        
        // Draw background
        tft->drawRect(barX, barY, barWidth, barHeight, 0xFFFF);
        // Power fill
        int fillWidth = (int)(cuePower * barWidth / 100.0f);
        tft->fillRect(barX, barY, fillWidth, barHeight, 0xF800);
        
        oldCuePower = cuePower;
    } else if (!isCharging && oldCuePower > 0) {
        // Clear power bar when not charging
        int barX = 10;
        int barY = SCREEN_HEIGHT - 30;
        int barWidth = 100;
        int barHeight = 10;
        tft->fillRect(barX, barY, barWidth, barHeight, COLOR_TABLE);
        oldCuePower = 0;
    }
}

void BilliardGame::update() {
    // Increase power while charging
    if (isCharging && isAiming) {
        cuePower += 2.0f;  // Increase power
        if (cuePower > 100.0f) {
            cuePower = 100.0f;  // Cap at 100
        }
    }
    
    updatePhysics();
}

void BilliardGame::updatePhysics() {
    bool anyBallMoving = false;
    
    // Update ball positions
    for (int i = 0; i < BALL_COUNT; i++) {
        if (!balls[i].isActive) continue;
        
        // Apply friction
        balls[i].vx *= FRICTION;
        balls[i].vy *= FRICTION;
        
        // Stop ball if velocity is too low
        float speed = sqrt(balls[i].vx * balls[i].vx + balls[i].vy * balls[i].vy);
        if (speed < MIN_VELOCITY) {
            balls[i].vx = 0.0f;
            balls[i].vy = 0.0f;
            balls[i].isActive = false;
        } else {
            anyBallMoving = true;
        }
        
        // Update position (use smaller steps to prevent tunneling)
        float timeStep = 0.5f;
        balls[i].x += balls[i].vx * timeStep;
        balls[i].y += balls[i].vy * timeStep;
        
        // Check if ball fell into pocket FIRST (before wall collision check)
        // This prevents ball from going too far outside before being detected
        if (balls[i].x >= 0 && isBallInPocket(balls[i])) {
            // Erase ball from screen before moving it
            eraseBall(i);
            
            balls[i].isActive = false;
            balls[i].vx = 0.0f;
            balls[i].vy = 0.0f;
            // Move ball off screen
            balls[i].x = -100;
            balls[i].y = -100;
            oldBallX[i] = -100;
            oldBallY[i] = -100;
            
            Serial.print("Ball ");
            Serial.print(i);
            Serial.println(" pocketed!");
            continue;  // Skip wall collision check for pocketed ball
        }
        
        // Check wall collisions ONLY if ball hasn't been pocketed
        if (balls[i].x >= 0) {
            checkWallCollisions(balls[i]);
        }
    }
    
    // Check ball-to-ball collisions (multiple times to handle fast collisions)
    checkBallCollisions();
    
    // Apply remaining movement
    for (int i = 0; i < BALL_COUNT; i++) {
        if (balls[i].x < 0 || !balls[i].isActive) continue;
        float timeStep = 0.5f;
        balls[i].x += balls[i].vx * timeStep;
        balls[i].y += balls[i].vy * timeStep;
        
        // Check pocket again after second movement
        if (isBallInPocket(balls[i])) {
            eraseBall(i);
            balls[i].isActive = false;
            balls[i].vx = 0.0f;
            balls[i].vy = 0.0f;
            balls[i].x = -100;
            balls[i].y = -100;
            oldBallX[i] = -100;
            oldBallY[i] = -100;
            Serial.print("Ball ");
            Serial.print(i);
            Serial.println(" pocketed!");
            continue;
        }
        
        // Check wall collisions after second movement
        checkWallCollisions(balls[i]);
    }
    
    // Check collisions again after second movement
    checkBallCollisions();
    
    // If all balls stopped, allow aiming again
    if (!anyBallMoving && !isAiming) {
        isAiming = true;
        // Redraw to show cue stick
        draw();
    }
}

void BilliardGame::checkWallCollisions(Ball& ball) {
    // First, check if ball is actually in a pocket - if so, don't apply wall collision
    // This prevents ball from bouncing when it should fall into pocket
    if (isBallInPocket(ball)) {
        return;  // Ball is in pocket, don't apply wall collision
    }
    
    // Check if ball is near pocket areas - don't bounce if too close to pocket
    // Use larger detection radius to allow balls to fall into pockets
    float pocketDetectionRadius = POCKET_RADIUS + BALL_RADIUS + 3.0f;
    
    bool nearTopLeftPocket = (ball.x - TABLE_X) * (ball.x - TABLE_X) + (ball.y - TABLE_Y) * (ball.y - TABLE_Y) < pocketDetectionRadius * pocketDetectionRadius;
    bool nearTopRightPocket = (ball.x - (TABLE_X + TABLE_WIDTH)) * (ball.x - (TABLE_X + TABLE_WIDTH)) + (ball.y - TABLE_Y) * (ball.y - TABLE_Y) < pocketDetectionRadius * pocketDetectionRadius;
    bool nearBottomLeftPocket = (ball.x - TABLE_X) * (ball.x - TABLE_X) + (ball.y - (TABLE_Y + TABLE_HEIGHT)) * (ball.y - (TABLE_Y + TABLE_HEIGHT)) < pocketDetectionRadius * pocketDetectionRadius;
    bool nearBottomRightPocket = (ball.x - (TABLE_X + TABLE_WIDTH)) * (ball.x - (TABLE_X + TABLE_WIDTH)) + (ball.y - (TABLE_Y + TABLE_HEIGHT)) * (ball.y - (TABLE_Y + TABLE_HEIGHT)) < pocketDetectionRadius * pocketDetectionRadius;
    bool nearTopMiddlePocket = (ball.x - (TABLE_X + TABLE_WIDTH / 2)) * (ball.x - (TABLE_X + TABLE_WIDTH / 2)) + (ball.y - TABLE_Y) * (ball.y - TABLE_Y) < pocketDetectionRadius * pocketDetectionRadius;
    bool nearBottomMiddlePocket = (ball.x - (TABLE_X + TABLE_WIDTH / 2)) * (ball.x - (TABLE_X + TABLE_WIDTH / 2)) + (ball.y - (TABLE_Y + TABLE_HEIGHT)) * (ball.y - (TABLE_Y + TABLE_HEIGHT)) < pocketDetectionRadius * pocketDetectionRadius;
    
    // Left wall (but not near top-left or bottom-left pockets)
    // Đặt quả bi sát mép bàn, không đi sâu vào
    if (ball.x - BALL_RADIUS < TABLE_X && !nearTopLeftPocket && !nearBottomLeftPocket) {
        ball.x = TABLE_X + BALL_RADIUS;  // Sát mép bàn
        ball.vx = -ball.vx * 0.8f;  // Bounce with some energy loss
    }
    // Right wall (but not near top-right or bottom-right pockets)
    if (ball.x + BALL_RADIUS > TABLE_X + TABLE_WIDTH && !nearTopRightPocket && !nearBottomRightPocket) {
        ball.x = TABLE_X + TABLE_WIDTH - BALL_RADIUS;  // Sát mép bàn
        ball.vx = -ball.vx * 0.8f;
    }
    // Top wall (but not near top pockets)
    if (ball.y - BALL_RADIUS < TABLE_Y && !nearTopLeftPocket && !nearTopRightPocket && !nearTopMiddlePocket) {
        ball.y = TABLE_Y + BALL_RADIUS;  // Sát mép bàn
        ball.vy = -ball.vy * 0.8f;
    }
    // Bottom wall (but not near bottom pockets)
    if (ball.y + BALL_RADIUS > TABLE_Y + TABLE_HEIGHT && !nearBottomLeftPocket && !nearBottomRightPocket && !nearBottomMiddlePocket) {
        ball.y = TABLE_Y + TABLE_HEIGHT - BALL_RADIUS;  // Sát mép bàn
        ball.vy = -ball.vy * 0.8f;
    }
    
    // Đảm bảo quả bi không đi sâu vào cạnh bàn (thêm kiểm tra an toàn)
    // Nhưng không áp dụng nếu quả bi đang gần lỗ (để quả bi có thể rơi vào lỗ)
    bool nearAnyPocket = nearTopLeftPocket || nearTopRightPocket || nearBottomLeftPocket || 
                         nearBottomRightPocket || nearTopMiddlePocket || nearBottomMiddlePocket;
    
    if (!nearAnyPocket) {
        // Chỉ đẩy quả bi ra nếu không gần lỗ
        // Đảm bảo bi không đi ra ngoài bàn quá xa
        if (ball.x < TABLE_X + BALL_RADIUS) {
            ball.x = TABLE_X + BALL_RADIUS;
            ball.vx = -ball.vx * 0.8f;  // Bounce if not near pocket
        }
        if (ball.x > TABLE_X + TABLE_WIDTH - BALL_RADIUS) {
            ball.x = TABLE_X + TABLE_WIDTH - BALL_RADIUS;
            ball.vx = -ball.vx * 0.8f;
        }
        if (ball.y < TABLE_Y + BALL_RADIUS) {
            ball.y = TABLE_Y + BALL_RADIUS;
            ball.vy = -ball.vy * 0.8f;
        }
        if (ball.y > TABLE_Y + TABLE_HEIGHT - BALL_RADIUS) {
            ball.y = TABLE_Y + TABLE_HEIGHT - BALL_RADIUS;
            ball.vy = -ball.vy * 0.8f;
        }
    } else {
        // Nếu gần lỗ, vẫn cần giới hạn bi không đi quá xa ngoài bàn
        // Chỉ cho phép bi đi ra ngoài một chút để có thể rơi vào lỗ
        float maxOutsideMargin = POCKET_RADIUS + BALL_RADIUS + 2.0f;  // Cho phép đi ra ngoài một chút
        
        // Giới hạn bi không đi quá xa ngoài bàn, ngay cả khi gần lỗ
        if (ball.x < TABLE_X - maxOutsideMargin) {
            ball.x = TABLE_X - maxOutsideMargin;
            ball.vx = 0.0f;  // Dừng lại nếu đi quá xa
        }
        if (ball.x > TABLE_X + TABLE_WIDTH + maxOutsideMargin) {
            ball.x = TABLE_X + TABLE_WIDTH + maxOutsideMargin;
            ball.vx = 0.0f;
        }
        if (ball.y < TABLE_Y - maxOutsideMargin) {
            ball.y = TABLE_Y - maxOutsideMargin;
            ball.vy = 0.0f;
        }
        if (ball.y > TABLE_Y + TABLE_HEIGHT + maxOutsideMargin) {
            ball.y = TABLE_Y + TABLE_HEIGHT + maxOutsideMargin;
            ball.vy = 0.0f;
        }
    }
}

void BilliardGame::checkBallCollisions() {
    // Check multiple times per frame to prevent fast balls from passing through
    const int iterations = 3;
    
    for (int iter = 0; iter < iterations; iter++) {
        for (int i = 0; i < BALL_COUNT; i++) {
            // Skip if ball is already pocketed
            if (balls[i].x < 0) continue;
            if (!balls[i].isActive && iter == 0) continue;  // Only check active balls on first iteration
            
            for (int j = i + 1; j < BALL_COUNT; j++) {
                // Skip if ball is already pocketed
                if (balls[j].x < 0) continue;
                if (!balls[j].isActive && iter == 0) continue;  // Only check active balls on first iteration
                
                // Calculate distance between balls
                float dx = balls[j].x - balls[i].x;
                float dy = balls[j].y - balls[i].y;
                float distance = sqrt(dx * dx + dy * dy);
                
                // Minimum distance should be 2 * BALL_RADIUS
                float minDistance = BALL_RADIUS * 2;
                
                // Check if balls are overlapping or very close
                if (distance < minDistance && distance > 0.01f) {
                    // Normalize collision vector
                    float nx = dx / distance;
                    float ny = dy / distance;
                    
                    // Separate balls to prevent overlap (always do this, even if not moving)
                    float overlap = minDistance - distance;
                    float separation = overlap * 0.6f;  // Separate by 60% of overlap to prevent jitter
                    
                    // Move balls apart
                    balls[i].x -= nx * separation;
                    balls[i].y -= ny * separation;
                    balls[j].x += nx * separation;
                    balls[j].y += ny * separation;
                    
                    // Calculate relative velocity
                    float dvx = balls[j].vx - balls[i].vx;
                    float dvy = balls[j].vy - balls[i].vy;
                    
                    // Relative velocity along collision normal
                    float dot = dvx * nx + dvy * ny;
                    
                    // Apply collision impulse if balls are moving towards each other
                    // Or if they're already overlapping (to push them apart)
                    if (dot < 0 || distance < minDistance * 0.9f) {
                        // Elastic collision: exchange momentum along collision normal
                        // Assuming equal mass (all balls have same mass)
                        float impulse = 2.0f * dot;
                        
                        balls[i].vx += impulse * nx;
                        balls[i].vy += impulse * ny;
                        balls[j].vx -= impulse * nx;
                        balls[j].vy -= impulse * ny;
                        
                        // Activate both balls after collision
                        balls[i].isActive = true;
                        balls[j].isActive = true;
                        
                        if (iter == 0) {  // Only log on first iteration
                            Serial.print("Collision: Ball ");
                            Serial.print(i);
                            Serial.print(" hit Ball ");
                            Serial.print(j);
                            Serial.print(" (distance: ");
                            Serial.print(distance);
                            Serial.println(")");
                        }
                    }
                }
            }
        }
    }
}

bool BilliardGame::isBallInPocket(Ball& ball) {
    // Kiểm tra tất cả các lỗ trước, không cần kiểm tra margin ngay lập tức
    // Vì bi có thể đi ra ngoài một chút khi rơi vào lỗ góc
    
    // Check all 6 pockets
    int pockets[6][2] = {
        {TABLE_X, TABLE_Y},  // Top-left
        {TABLE_X + TABLE_WIDTH, TABLE_Y},  // Top-right
        {TABLE_X, TABLE_Y + TABLE_HEIGHT},  // Bottom-left
        {TABLE_X + TABLE_WIDTH, TABLE_Y + TABLE_HEIGHT},  // Bottom-right
        {TABLE_X + TABLE_WIDTH / 2, TABLE_Y},  // Top middle
        {TABLE_X + TABLE_WIDTH / 2, TABLE_Y + TABLE_HEIGHT}  // Bottom middle
    };
    
    for (int i = 0; i < 6; i++) {
        float dx = ball.x - pockets[i][0];
        float dy = ball.y - pockets[i][1];
        float distance = sqrt(dx * dx + dy * dy);
        
        // Bi rơi vào lỗ CHỈ KHI tâm bi nằm SÂU TRONG lỗ
        // Để tránh bi rơi khi chỉ đi kế bên lỗ, yêu cầu tâm bi phải nằm trong
        // vòng tròn bán kính nhỏ hơn nhiều so với POCKET_RADIUS
        // Với POCKET_RADIUS = 12, để bi thực sự rơi vào lỗ, tâm bi phải nằm trong
        // vòng tròn bán kính khoảng 3-4 pixel từ tâm lỗ
        float maxDistance = 3.5f;  // Tâm bi phải nằm trong vòng tròn bán kính 3.5 từ tâm lỗ
        
        // Chỉ coi là rơi vào lỗ nếu tâm bi nằm trong phạm vi này
        // Điều này đảm bảo bi thực sự đi vào lỗ, không chỉ đi kế bên
        if (distance < maxDistance) {
            return true;
        }
    }
    
    return false;
}

void BilliardGame::handleAimLeft() {
    if (isAiming && !balls[activeBallIndex].isActive) {
        eraseCueStick();
        cueAngle -= 0.1f;  // Rotate left
        if (cueAngle < 0) cueAngle += 2 * M_PI;
        drawCueStick();
    }
}

void BilliardGame::handleAimRight() {
    if (isAiming && !balls[activeBallIndex].isActive) {
        eraseCueStick();
        cueAngle += 0.1f;  // Rotate right
        if (cueAngle > 2 * M_PI) cueAngle -= 2 * M_PI;
        drawCueStick();
    }
}

void BilliardGame::handleChargeStart() {
    if (isAiming && !balls[activeBallIndex].isActive) {
        isCharging = true;
        cuePower = 0.0f;
    }
}

void BilliardGame::handleChargeRelease() {
    if (isCharging && isAiming) {
        // Erase cue stick and power bar
        eraseCueStick();
        int barX = 10;
        int barY = SCREEN_HEIGHT - 30;
        int barWidth = 100;
        int barHeight = 10;
        tft->fillRect(barX, barY, barWidth, barHeight, COLOR_TABLE);
        
        // Shoot the cue ball
        // Quả bi chạy về hướng cueAngle (phía trước), gậy nằm phía sau
        Ball& cueBall = balls[activeBallIndex];
        float speed = cuePower * 0.3f;  // Scale power to velocity
        cueBall.vx = cos(cueAngle) * speed;  // Chạy về hướng góc (phía trước)
        cueBall.vy = sin(cueAngle) * speed;  // Chạy về hướng góc (phía trước)
        cueBall.isActive = true;
        
        isAiming = false;
        isCharging = false;
        cuePower = 0.0f;
        oldCuePower = 0.0f;
    }
}

int BilliardGame::countActiveBalls() const {
    int count = 0;
    for (int i = 0; i < BALL_COUNT; i++) {
        // Quả bi còn trên bàn nếu x >= 0 (chưa rơi vào lỗ)
        if (balls[i].x >= 0) {
            count++;
        }
    }
    return count;
}

void BilliardGame::aimAtNearestBall() {
    if (!isAiming || balls[activeBallIndex].isActive) {
        return;  // Không thể ngắm nếu đang không ở chế độ ngắm hoặc quả bi đang di chuyển
    }
    
    Ball& cueBall = balls[activeBallIndex];
    
    // Tìm quả bi gần nhất (không phải cue ball và chưa rơi vào lỗ)
    float minDistance = 10000.0f;
    int nearestIndex = -1;
    
    for (int i = 0; i < BALL_COUNT; i++) {
        if (i == activeBallIndex) continue;  // Bỏ qua cue ball
        if (balls[i].x < 0) continue;  // Bỏ qua quả bi đã rơi vào lỗ
        
        float dx = balls[i].x - cueBall.x;
        float dy = balls[i].y - cueBall.y;
        float distance = sqrt(dx * dx + dy * dy);
        
        if (distance < minDistance) {
            minDistance = distance;
            nearestIndex = i;
        }
    }
    
    // Nếu tìm thấy quả bi gần nhất, ngắm về nó
    if (nearestIndex >= 0) {
        float dx = balls[nearestIndex].x - cueBall.x;
        float dy = balls[nearestIndex].y - cueBall.y;
        
        // Tính góc để ngắm về quả bi
        float targetAngle = atan2(dy, dx);
        
        // Xóa gậy cũ trước khi cập nhật góc
        eraseCueStick();
        
        // Cập nhật góc
        cueAngle = targetAngle;
        
        // Vẽ gậy mới
        drawCueStick();
    }
}

