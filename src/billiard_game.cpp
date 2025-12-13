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
    this->collisionThisFrame = false;
    
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
    // CHỈ vẽ các quả bi còn trên bàn (x >= 0), các quả bi đã rơi vào lỗ (x < 0) không vẽ
    for (int i = 0; i < BALL_COUNT; i++) {
        if (balls[i].x >= 0) {  // Chỉ vẽ quả bi còn trên bàn
            oldBallX[i] = balls[i].x;
            oldBallY[i] = balls[i].y;
            drawBall(i);
        }
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
    
    // Initialize target balls in triangle formation (1 cue ball + 19 target balls)
    // Xếp thành hình tam giác: 1, 2, 3, 4, 5, 4 (tổng 19 quả)
    float startX = TABLE_X + TABLE_WIDTH - 100;
    float startY = TABLE_Y + TABLE_HEIGHT / 2;
    float ballSpacing = 13.0f;  // Khoảng cách giữa các quả bi
    
    uint16_t ballColors[19] = {
        COLOR_BALL1, COLOR_BALL2, COLOR_BALL3, COLOR_BALL4, COLOR_BALL5,
        COLOR_BALL6, COLOR_BALL7, COLOR_BALL8, COLOR_BALL9, COLOR_BALL10,
        COLOR_BALL11, COLOR_BALL12, COLOR_BALL13, COLOR_BALL14, COLOR_BALL15,
        COLOR_BALL16, COLOR_BALL17, COLOR_BALL18, COLOR_BALL19
    };
    
    int ballIndex = 1;  // Bắt đầu từ quả bi 1 (quả bi 0 là cue ball)
    
    // Row 1: 1 ball (center)
    balls[ballIndex].x = startX;
    balls[ballIndex].y = startY;
    balls[ballIndex].vx = 0.0f;
    balls[ballIndex].vy = 0.0f;
    balls[ballIndex].color = ballColors[ballIndex - 1];
    balls[ballIndex].isActive = false;
    ballIndex++;
    
    // Row 2: 2 balls
    for (int i = 0; i < 2; i++) {
        balls[ballIndex].x = startX - ballSpacing;
        balls[ballIndex].y = startY - ballSpacing * 0.5f + ballSpacing * i;
        balls[ballIndex].vx = 0.0f;
        balls[ballIndex].vy = 0.0f;
        balls[ballIndex].color = ballColors[ballIndex - 1];
        balls[ballIndex].isActive = false;
        ballIndex++;
    }
    
    // Row 3: 3 balls
    for (int i = 0; i < 3; i++) {
        balls[ballIndex].x = startX - ballSpacing * 2;
        balls[ballIndex].y = startY - ballSpacing + ballSpacing * i;
        balls[ballIndex].vx = 0.0f;
        balls[ballIndex].vy = 0.0f;
        balls[ballIndex].color = ballColors[ballIndex - 1];
        balls[ballIndex].isActive = false;
        ballIndex++;
    }
    
    // Row 4: 4 balls
    for (int i = 0; i < 4; i++) {
        balls[ballIndex].x = startX - ballSpacing * 3;
        balls[ballIndex].y = startY - ballSpacing * 1.5f + ballSpacing * i;
        balls[ballIndex].vx = 0.0f;
        balls[ballIndex].vy = 0.0f;
        balls[ballIndex].color = ballColors[ballIndex - 1];
        balls[ballIndex].isActive = false;
        ballIndex++;
    }
    
    // Row 5: 5 balls
    for (int i = 0; i < 5; i++) {
        balls[ballIndex].x = startX - ballSpacing * 4;
        balls[ballIndex].y = startY - ballSpacing * 2 + ballSpacing * i;
        balls[ballIndex].vx = 0.0f;
        balls[ballIndex].vy = 0.0f;
        balls[ballIndex].color = ballColors[ballIndex - 1];
        balls[ballIndex].isActive = false;
        ballIndex++;
    }
    
    // Row 6: 4 balls (last row)
    for (int i = 0; i < 4; i++) {
        balls[ballIndex].x = startX - ballSpacing * 5;
        balls[ballIndex].y = startY - ballSpacing * 1.5f + ballSpacing * i;
        balls[ballIndex].vx = 0.0f;
        balls[ballIndex].vy = 0.0f;
        balls[ballIndex].color = ballColors[ballIndex - 1];
        balls[ballIndex].isActive = false;
        ballIndex++;
    }
    
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
    // Tối ưu: chỉ xóa vị trí cũ, không xóa toàn bộ đường đi để giảm nháy
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
    
    // Tối ưu: chỉ xóa vị trí cũ và vị trí mới, không xóa toàn bộ đường đi
    // Điều này giảm nháy đáng kể khi quả bi di chuyển nhanh
    if (distance > BALL_RADIUS * 3) {
        // Quả bi di chuyển nhanh - xóa vị trí cũ và một vài điểm giữa
        int steps = 3;  // Giảm số bước để tăng tốc độ
        for (int i = 0; i <= steps; i++) {
            float t = (float)i / steps;
            float x = oldX + dx * t;
            float y = oldY + dy * t;
            eraseBallAtPosition(x, y, eraseRadius);
        }
    } else {
        // Quả bi di chuyển chậm - chỉ xóa vị trí cũ
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
    // Tối ưu: chỉ vẽ lại border/pockets khi thực sự cần (gần edge)
    bool nearEdge = (x < TABLE_X + radius * 2 || x > TABLE_X + TABLE_WIDTH - radius * 2 ||
                     y < TABLE_Y + radius * 2 || y > TABLE_Y + TABLE_HEIGHT - radius * 2);
    
    // Erase ball area
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
    
    // Chỉ vẽ lại border và pockets khi quả bi gần edge (giảm nháy)
    if (nearEdge) {
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
    
    // Kiểm tra NGAY LẬP TỨC nếu quả bi đã rơi vào lỗ (x < 0) - KHÔNG BAO GIỜ vẽ lại
    if (ball.x < 0) {
        return;  // Quả bi đã rơi vào lỗ, không vẽ
    }
    
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
    
    // Xóa gậy - tương tự như vẽ, nhưng vẽ lại màu nền
    int stickThickness = 4;  // Độ dày phần thân gậy (phải khớp với drawCueStick)
    int halfThickness = stickThickness / 2;
    int tipSize = 2;  // Kích thước đầu gậy (phải khớp với drawCueStick)
    
    // Tính toán vector vuông góc với hướng gậy để xóa độ dày
    int dx = oldStickEndScreenX - screenX;
    int dy = oldStickEndScreenY - screenY;
    float length = sqrt((float)(dx * dx + dy * dy));
    
    if (length > 0) {
        // Vector đơn vị vuông góc (perpendicular vector)
        float perpX = -dy / length;
        float perpY = dx / length;
        
        // Xóa phần thân gậy - tính điểm bắt đầu phần thân (cách quả bi một chút)
        int shaftStartOffset = 6;  // Khoảng cách từ quả bi đến điểm bắt đầu thân gậy
        int oldShaftStartX = screenX + (int)(dx * shaftStartOffset / length);
        int oldShaftStartY = screenY + (int)(dy * shaftStartOffset / length);
        
        // Xóa thân gậy - vẽ lại nhiều đường line song song với màu nền
        for (int i = -halfThickness; i <= halfThickness; i++) {
            int offsetX = (int)(perpX * i);
            int offsetY = (int)(perpY * i);
            
            // Sử dụng Bresenham line algorithm để vẽ lại từng pixel với màu nền đúng
            int x0 = oldShaftStartX + offsetX;
            int y0 = oldShaftStartY + offsetY;
            int x1 = oldStickEndScreenX + offsetX;
            int y1 = oldStickEndScreenY + offsetY;
            
            int dx_line = abs(x1 - x0);
            int dy_line = abs(y1 - y0);
            int sx = (x0 < x1) ? 1 : -1;
            int sy = (y0 < y1) ? 1 : -1;
            int err = dx_line - dy_line;
            
            int x = x0;
            int y = y0;
            
            while (true) {
                // Xác định màu nền tại vị trí (x, y)
                uint16_t bgColor = getBackgroundColorAt(x, y);
                tft->drawPixel(x, y, bgColor);
                
                if (x == x1 && y == y1) break;
                
                int e2 = 2 * err;
                if (e2 > -dy_line) {
                    err -= dy_line;
                    x += sx;
                }
                if (e2 < dx_line) {
                    err += dx_line;
                    y += sy;
                }
            }
        }
        
        // Xóa phần đầu gậy (tip) - phần tiếp xúc với bi
        int tipLength = 6;  // Chiều dài phần đầu gậy (phải khớp với drawCueStick)
        for (int i = -tipSize/2; i <= tipSize/2; i++) {
            int tipOffsetX = (int)(perpX * i);
            int tipOffsetY = (int)(perpY * i);
            int tipStartX = screenX + tipOffsetX;
            int tipStartY = screenY + tipOffsetY;
            int tipEndX = screenX + (int)(dx * tipLength / length) + tipOffsetX;
            int tipEndY = screenY + (int)(dy * tipLength / length) + tipOffsetY;
            
            int dx_tip = abs(tipEndX - tipStartX);
            int dy_tip = abs(tipEndY - tipStartY);
            int sx_tip = (tipStartX < tipEndX) ? 1 : -1;
            int sy_tip = (tipStartY < tipEndY) ? 1 : -1;
            int err_tip = dx_tip - dy_tip;
            
            int x = tipStartX;
            int y = tipStartY;
            
            while (true) {
                uint16_t bgColor = getBackgroundColorAt(x, y);
                tft->drawPixel(x, y, bgColor);
                
                if (x == tipEndX && y == tipEndY) break;
                
                int e2 = 2 * err_tip;
                if (e2 > -dy_tip) {
                    err_tip -= dy_tip;
                    x += sx_tip;
                }
                if (e2 < dx_tip) {
                    err_tip += dx_tip;
                    y += sy_tip;
                }
            }
        }
    } else {
        // Fallback: xóa đường line đơn giản nếu không tính được vector
        for (int i = -halfThickness; i <= halfThickness; i++) {
            int x0 = screenX;
            int y0 = screenY + i;
            int x1 = oldStickEndScreenX;
            int y1 = oldStickEndScreenY + i;
            
            int dx_line = abs(x1 - x0);
            int dy_line = abs(y1 - y0);
            int sx = (x0 < x1) ? 1 : -1;
            int sy = (y0 < y1) ? 1 : -1;
            int err = dx_line - dy_line;
            
            int x = x0;
            int y = y0;
            
            while (true) {
                uint16_t bgColor = getBackgroundColorAt(x, y);
                tft->drawPixel(x, y, bgColor);
                
                if (x == x1 && y == y1) break;
                
                int e2 = 2 * err;
                if (e2 > -dy_line) {
                    err -= dy_line;
                    x += sx;
                }
                if (e2 < dx_line) {
                    err += dx_line;
                    y += sy;
                }
            }
        }
    }
}

void BilliardGame::eraseCueStickAtPosition(float angle, float power) {
    // Hàm xóa gậy dựa trên góc và lực cụ thể, không phụ thuộc vào isAiming
    Ball& cueBall = balls[activeBallIndex];
    int screenX = (int)cueBall.x;
    int screenY = SCREEN_HEIGHT - (int)cueBall.y;
    
    // Calculate cue stick position
    float stickLength = 40.0f + power * 0.3f;
    float stickEndX = cueBall.x - cos(angle) * (BALL_RADIUS + stickLength);
    float stickEndY = cueBall.y - sin(angle) * (BALL_RADIUS + stickLength);
    
    int oldStickEndScreenX = (int)stickEndX;
    int oldStickEndScreenY = SCREEN_HEIGHT - (int)stickEndY;
    
    // Xóa gậy - tương tự như eraseCueStick() nhưng không kiểm tra isAiming
    int stickThickness = 4;
    int halfThickness = stickThickness / 2;
    int tipSize = 2;
    
    int dx = oldStickEndScreenX - screenX;
    int dy = oldStickEndScreenY - screenY;
    float length = sqrt((float)(dx * dx + dy * dy));
    
    if (length > 0) {
        float perpX = -dy / length;
        float perpY = dx / length;
        
        // Xóa phần thân gậy
        int shaftStartOffset = 6;
        int oldShaftStartX = screenX + (int)(dx * shaftStartOffset / length);
        int oldShaftStartY = screenY + (int)(dy * shaftStartOffset / length);
        
        for (int i = -halfThickness; i <= halfThickness; i++) {
            int offsetX = (int)(perpX * i);
            int offsetY = (int)(perpY * i);
            
            int x0 = oldShaftStartX + offsetX;
            int y0 = oldShaftStartY + offsetY;
            int x1 = oldStickEndScreenX + offsetX;
            int y1 = oldStickEndScreenY + offsetY;
            
            int dx_line = abs(x1 - x0);
            int dy_line = abs(y1 - y0);
            int sx = (x0 < x1) ? 1 : -1;
            int sy = (y0 < y1) ? 1 : -1;
            int err = dx_line - dy_line;
            
            int x = x0;
            int y = y0;
            
            while (true) {
                uint16_t bgColor = getBackgroundColorAt(x, y);
                tft->drawPixel(x, y, bgColor);
                
                if (x == x1 && y == y1) break;
                
                int e2 = 2 * err;
                if (e2 > -dy_line) {
                    err -= dy_line;
                    x += sx;
                }
                if (e2 < dx_line) {
                    err += dx_line;
                    y += sy;
                }
            }
        }
        
        // Xóa phần đầu gậy
        int tipLength = 6;
        for (int i = -tipSize/2; i <= tipSize/2; i++) {
            int tipOffsetX = (int)(perpX * i);
            int tipOffsetY = (int)(perpY * i);
            int tipStartX = screenX + tipOffsetX;
            int tipStartY = screenY + tipOffsetY;
            int tipEndX = screenX + (int)(dx * tipLength / length) + tipOffsetX;
            int tipEndY = screenY + (int)(dy * tipLength / length) + tipOffsetY;
            
            int dx_tip = abs(tipEndX - tipStartX);
            int dy_tip = abs(tipEndY - tipStartY);
            int sx_tip = (tipStartX < tipEndX) ? 1 : -1;
            int sy_tip = (tipStartY < tipEndY) ? 1 : -1;
            int err_tip = dx_tip - dy_tip;
            
            int x = tipStartX;
            int y = tipStartY;
            
            while (true) {
                uint16_t bgColor = getBackgroundColorAt(x, y);
                tft->drawPixel(x, y, bgColor);
                
                if (x == tipEndX && y == tipEndY) break;
                
                int e2 = 2 * err_tip;
                if (e2 > -dy_tip) {
                    err_tip -= dy_tip;
                    x += sx_tip;
                }
                if (e2 < dx_tip) {
                    err_tip += dx_tip;
                    y += sy_tip;
                }
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
    
    // Vẽ gậy bida giống thật - phần thân và đầu gậy màu trắng
    int stickThickness = 4;  // Độ dày phần thân gậy (4 pixel - mỏng hơn để giống thật)
    int halfThickness = stickThickness / 2;
    uint16_t shaftColor = 0xFFFF;  // Màu trắng cho phần thân gậy
    
    // Tính toán vector vuông góc với hướng gậy để vẽ độ dày
    float dx = stickEndScreenX - stickStartX;
    float dy = stickEndScreenY - stickStartY;
    float length = sqrt(dx * dx + dy * dy);
    
    if (length > 0) {
        // Vector đơn vị vuông góc (perpendicular vector)
        float perpX = -dy / length;
        float perpY = dx / length;
        
        // Vẽ phần thân gậy (shaft) - màu trắng, mỏng và dài
        // Tính điểm bắt đầu phần thân (cách quả bi một chút)
        int shaftStartOffset = 6;  // Khoảng cách từ quả bi đến điểm bắt đầu thân gậy
        int shaftStartX = stickStartX + (int)(dx * shaftStartOffset / length);
        int shaftStartY = stickStartY + (int)(dy * shaftStartOffset / length);
        
        // Vẽ thân gậy bằng cách vẽ nhiều đường line song song màu trắng
        for (int i = -halfThickness; i <= halfThickness; i++) {
            int offsetX = (int)(perpX * i);
            int offsetY = (int)(perpY * i);
            
            tft->drawLine(
                shaftStartX + offsetX, 
                shaftStartY + offsetY, 
                stickEndScreenX + offsetX, 
                stickEndScreenY + offsetY, 
                shaftColor  // Màu trắng
            );
        }
        
        // Vẽ đầu gậy (tip) - phần tiếp xúc với bi, màu trắng, nhỏ hơn
        uint16_t tipColor = 0xFFFF;  // Màu trắng cho đầu gậy
        int tipSize = 2;  // Kích thước đầu gậy nhỏ hơn (2 pixel)
        int tipLength = 6;  // Chiều dài phần đầu gậy
        
        for (int i = -tipSize/2; i <= tipSize/2; i++) {
            int tipOffsetX = (int)(perpX * i);
            int tipOffsetY = (int)(perpY * i);
            // Vẽ phần đầu gậy (gần quả bi)
            int tipStartX = stickStartX + tipOffsetX;
            int tipStartY = stickStartY + tipOffsetY;
            int tipEndX = stickStartX + (int)(dx * tipLength / length) + tipOffsetX;
            int tipEndY = stickStartY + (int)(dy * tipLength / length) + tipOffsetY;
            tft->drawLine(tipStartX, tipStartY, tipEndX, tipEndY, tipColor);
        }
    } else {
        // Fallback: vẽ đường line đơn giản nếu không tính được vector
        for (int i = -halfThickness; i <= halfThickness; i++) {
            tft->drawLine(stickStartX, stickStartY + i, stickEndScreenX, stickEndScreenY + i, shaftColor);
        }
    }
    
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
    
    auto renderBallOptimized = [&](int i) {
        if (balls[i].x < 0) return;  // Pocketed
        
        if (balls[i].isActive) {
            eraseBall(i);
            drawBall(i);
        } else if (balls[i].x != oldBallX[i] || balls[i].y != oldBallY[i]) {
            eraseBall(i);
            drawBall(i);
        }
    };
    
    if (collisionThisFrame) {
        // Safe path on collision: erase all old ball areas first, then draw all balls with cue last
        int eraseRadius = BALL_RADIUS + 2;
        for (int i = 0; i < BALL_COUNT; i++) {
            if (oldBallX[i] < 0 || oldBallY[i] < 0) continue;  // Already pocketed sentinel
            eraseBallAtPosition(oldBallX[i], oldBallY[i], eraseRadius);
        }
        // Ensure pockets stay on top after erasing near edges
        redrawAllPockets();
        
        // Draw non-cue balls first
        for (int i = 0; i < BALL_COUNT; i++) {
            if (i == activeBallIndex) continue;
            if (balls[i].x >= 0) {
                drawBall(i);
            }
        }
        // Draw cue ball last for visual priority
        if (balls[activeBallIndex].x >= 0) {
            drawBall(activeBallIndex);
        }
        
        // Collision handled this frame
        collisionThisFrame = false;
    } else {
        // Normal path: draw all, cue ball last to avoid clipped highlight
        for (int i = 0; i < BALL_COUNT; i++) {
            if (i == activeBallIndex) continue;
            renderBallOptimized(i);
        }
        renderBallOptimized(activeBallIndex);
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
        drawPowerBar();
        oldCuePower = cuePower;
    } else if (!isCharging && oldCuePower > 0) {
        // Clear power bar when not charging - xóa hết và vẽ lại phần bàn bị đè
        int barX = 10;
        int barY = SCREEN_HEIGHT - 30;
        int barWidth = 100;
        int barHeight = 12;
        int textWidth = 30;
        int clearHeight = barHeight + 12;  // Chiều cao vùng cần xóa (bao gồm text)
        
        // Tính toán vùng bị đè bởi thanh lực
        int clearX = barX;
        int clearY = barY - 2;
        int clearWidth = barWidth + textWidth;
        
        // Xác định vùng nào nằm trên bàn, vùng nào nằm ngoài bàn
        int tableScreenY = SCREEN_HEIGHT - TABLE_Y;  // Top của bàn (trên màn hình)
        int tableBottomScreenY = SCREEN_HEIGHT - (TABLE_Y + TABLE_HEIGHT);  // Bottom của bàn (trên màn hình)
        int tableMinX = TABLE_X;
        int tableMaxX = TABLE_X + TABLE_WIDTH;
        
        // Xóa và vẽ lại từng phần
        for (int py = clearY; py < clearY + clearHeight; py++) {
            for (int px = clearX; px < clearX + clearWidth; px++) {
                // Kiểm tra xem pixel này có nằm trên bàn không
                bool insideTable = (px >= tableMinX && px <= tableMaxX && 
                                   py >= tableBottomScreenY && py <= tableScreenY);
                
                if (insideTable) {
                    // Nằm trên bàn - vẽ lại màu xanh
                    tft->drawPixel(px, py, COLOR_TABLE);
                } else {
                    // Nằm ngoài bàn - vẽ lại màu đen
                    tft->drawPixel(px, py, 0x0000);
                }
            }
        }
        
        // Vẽ lại thành bàn nếu thanh lực che mất thành bàn
        // Kiểm tra xem thanh lực có che mất thành bàn dưới không
        if (clearY <= tableBottomScreenY && (clearY + clearHeight) >= tableBottomScreenY) {
            // Vẽ lại thành bàn dưới
            int railThickness = 4;
            for (int i = 0; i < railThickness; i++) {
                // Chỉ vẽ phần không bị che bởi thanh lực
                int drawStartX = (clearX < tableMinX) ? tableMinX : clearX + clearWidth;
                int drawEndX = tableMaxX;
                if (drawStartX < drawEndX) {
                    tft->drawLine(drawStartX, tableBottomScreenY - i, drawEndX, tableBottomScreenY - i, COLOR_TABLE_BORDER);
                }
            }
        }
        
        // QUAN TRỌNG: Vẽ lại các quả bi bị đè bởi thanh lực
        for (int i = 0; i < BALL_COUNT; i++) {
            // Bỏ qua quả bi đã vào lỗ
            if (balls[i].x < 0) continue;
            
            // Kiểm tra xem quả bi có nằm trong vùng thanh lực không
            int ballScreenX = (int)balls[i].x;
            int ballScreenY = SCREEN_HEIGHT - (int)balls[i].y;
            int ballRadius = BALL_RADIUS;
            
            // Kiểm tra xem quả bi có giao với vùng thanh lực không
            bool ballInPowerBarArea = (ballScreenX + ballRadius >= clearX && 
                                       ballScreenX - ballRadius <= clearX + clearWidth &&
                                       ballScreenY + ballRadius >= clearY && 
                                       ballScreenY - ballRadius <= clearY + clearHeight);
            
            if (ballInPowerBarArea) {
                // Vẽ lại quả bi này
                drawBall(i);
            }
        }
        
        oldCuePower = 0;
    }
}

void BilliardGame::drawPowerBar() {
    int barX = 10;
    int barY = SCREEN_HEIGHT - 30;
    int barWidth = 100;
    int barHeight = 12;  // Tăng chiều cao thanh lực để dễ nhìn hơn
    
    // Clear old fill (xóa cả vùng text) - khôi phục nền đúng màu
    int clearX = barX;
    int clearY = barY - 2;
    int clearWidth = barWidth + 30;
    int clearHeight = barHeight + 12;
    
    // Khôi phục nền từng pixel với màu đúng
    for (int py = clearY; py < clearY + clearHeight; py++) {
        for (int px = clearX; px < clearX + clearWidth; px++) {
            uint16_t bgColor = getBackgroundColorAt(px, py);
            tft->drawPixel(px, py, bgColor);
        }
    }
    
    // Draw background (viền trắng)
    tft->drawRect(barX, barY, barWidth, barHeight, 0xFFFF);
    
    // Power fill (màu đỏ)
    int fillWidth = (int)(cuePower * barWidth / 100.0f);
    if (fillWidth > 0) {
        tft->fillRect(barX + 1, barY + 1, fillWidth - 2, barHeight - 2, 0xF800);
    }
    
    // Hiển thị số lực bên cạnh thanh (0-100)
    tft->setTextSize(1);
    tft->setTextColor(0xFFFF, COLOR_TABLE);  // Chữ trắng, nền xanh
    tft->setCursor(barX + barWidth + 5, barY + 2);
    tft->print((int)cuePower);
    tft->print("%");
}

void BilliardGame::update() {
    // Reset collision flag each frame; will be set during physics when collisions occur
    collisionThisFrame = false;
    
    // Auto-increase power while charging (tăng mạnh hơn khi giữ nút)
    if (isCharging && isAiming) {
        cuePower += 2.0f;  // Tự động tăng lực mạnh hơn (tăng từ 0.5f lên 2.0f)
        if (cuePower > 100.0f) {
            cuePower = 100.0f;  // Cap at 100
        }
        // Vẽ lại thanh lực mỗi frame khi đang charge để hiển thị rõ ràng
        drawPowerBar();
        oldCuePower = cuePower;
    }
    
    updatePhysics();
}

void BilliardGame::updatePhysics() {
    bool anyBallMoving = false;
    
    // Update ball positions
    for (int i = 0; i < BALL_COUNT; i++) {
        if (!balls[i].isActive) {
            // Reactivate slow/stuck balls that are already in the pocket mouth
            if (balls[i].x >= 0 && isBallInPocketAttractionZone(balls[i])) {
                balls[i].isActive = true;
            } else {
                continue;
            }
        }
        
        // Apply friction
        balls[i].vx *= FRICTION;
        balls[i].vy *= FRICTION;
        
        bool inPocketAttraction = (balls[i].x >= 0) ? isBallInPocketAttractionZone(balls[i]) : false;
        
        // Stop ball if velocity is too low (but never stop inside pocket attraction)
        float speed = sqrt(balls[i].vx * balls[i].vx + balls[i].vy * balls[i].vy);
        if (speed < MIN_VELOCITY) {
            if (inPocketAttraction) {
                // Keep the ball moving gently toward the pocket
                balls[i].vx *= 0.5f;
                balls[i].vy *= 0.5f;
                balls[i].isActive = true;
                anyBallMoving = true;
            } else {
                balls[i].vx = 0.0f;
                balls[i].vy = 0.0f;
                balls[i].isActive = false;
            }
        } else {
            anyBallMoving = true;
        }
        
        if (inPocketAttraction) {
            applyPocketAttraction(balls[i]);
        }
        
        // Update position (use smaller steps to prevent tunneling)
        float timeStep = 0.5f;
        balls[i].x += balls[i].vx * timeStep;
        balls[i].y += balls[i].vy * timeStep;
        
        // Check if ball fell into pocket FIRST (before wall collision check)
        // This prevents ball from going too far outside before being detected
        if (balls[i].x >= 0 && isBallInPocket(balls[i])) {
            bool isCueBall = (i == activeBallIndex);
            
            // QUAN TRỌNG: Xóa quả bi khỏi màn hình TRƯỚC KHI đặt x = -100
            // Điều này đảm bảo quả bi được xóa ở đúng vị trí cũ
            eraseBall(i);
            
            // SAU KHI xóa xong, mới đánh dấu quả bi đã rơi vào lỗ (x = -100)
            // Điều này đảm bảo quả bi không bao giờ được vẽ lại hoặc xử lý tiếp
            if (!isCueBall) {
                // Quả bi khác rơi vào lỗ - đánh dấu NGAY LẬP TỨC
                balls[i].x = -100;
                balls[i].y = -100;
                oldBallX[i] = -100;
                oldBallY[i] = -100;
            }
            
            // Dừng quả bi
            balls[i].isActive = false;
            balls[i].vx = 0.0f;
            balls[i].vy = 0.0f;
            
            if (isCueBall) {
                // Cập nhật oldBallX/Y cho cue ball (sẽ được set lại ở dưới)
                oldBallX[i] = balls[i].x;
                oldBallY[i] = balls[i].y;
            }
            
            // Xử lý đặc biệt cho quả bi trắng (cue ball) - index 0
            if (isCueBall) {
                // Quả bi trắng rơi vào lỗ - đặt lại ở vị trí ban đầu
                Serial.println("Cue ball pocketed! Resetting cue ball position...");
                
                // Đặt lại quả bi trắng ở vị trí ban đầu (góc dưới trái)
                balls[i].x = TABLE_X + 80;
                balls[i].y = TABLE_Y + TABLE_HEIGHT - 40;
                oldBallX[i] = balls[i].x;
                oldBallY[i] = balls[i].y;
                
                // Vẽ lại quả bi trắng ở vị trí mới
                drawBall(i);
                
                // Cho phép ngắm lại
                isAiming = true;
                isCharging = false;
                cuePower = 0.0f;
                oldCuePower = 0.0f;
                
                Serial.println("Cue ball reset to starting position.");
            } else {
                // Quả bi khác rơi vào lỗ - MẤT VĨNH VIỄN, không bao giờ lên lại
                // Đảm bảo x = -100 để đánh dấu đã rơi vào lỗ
                balls[i].x = -100;
                balls[i].y = -100;
                oldBallX[i] = -100;
                oldBallY[i] = -100;
                
                Serial.print("Ball ");
                Serial.print(i);
                Serial.println(" pocketed! (permanently removed)");
            }
            continue;  // Skip wall collision check for pocketed ball
        }
        
        // Check wall collisions ONLY if ball hasn't been pocketed AND not in pocket mouth
        if (balls[i].x >= 0 && !isBallInPocketAttractionZone(balls[i])) {
            checkWallCollisions(balls[i]);
        } else if (balls[i].x >= 0) {
            // If inside attraction zone after moving, keep pulling toward pocket
            applyPocketAttraction(balls[i]);
            anyBallMoving = true;
        }
    }
    
    // Check ball-to-ball collisions (multiple times to handle fast collisions)
    // Nhưng bỏ qua các quả bi đã rơi vào lỗ hoặc đang gần lỗ
    checkBallCollisions();
    
    // Apply remaining movement
    for (int i = 0; i < BALL_COUNT; i++) {
        if (balls[i].x < 0 || !balls[i].isActive) continue;
        
        bool inPocketAttraction = isBallInPocketAttractionZone(balls[i]);
        if (inPocketAttraction) {
            applyPocketAttraction(balls[i]);
        }
        
        float timeStep = 0.5f;
        balls[i].x += balls[i].vx * timeStep;
        balls[i].y += balls[i].vy * timeStep;
        
        // Check pocket again after second movement
        if (isBallInPocket(balls[i])) {
            bool isCueBall = (i == activeBallIndex);
            
            // QUAN TRỌNG: Xóa quả bi khỏi màn hình TRƯỚC KHI đặt x = -100
            // Điều này đảm bảo quả bi được xóa ở đúng vị trí cũ
            eraseBall(i);
            
            // SAU KHI xóa xong, mới đánh dấu quả bi đã rơi vào lỗ (x = -100)
            // Điều này đảm bảo quả bi không bao giờ được vẽ lại hoặc xử lý tiếp
            if (!isCueBall) {
                // Quả bi khác rơi vào lỗ - đánh dấu NGAY LẬP TỨC
                balls[i].x = -100;
                balls[i].y = -100;
                oldBallX[i] = -100;
                oldBallY[i] = -100;
            }
            
            // Dừng quả bi
            balls[i].isActive = false;
            balls[i].vx = 0.0f;
            balls[i].vy = 0.0f;
            
            if (isCueBall) {
                // Cập nhật oldBallX/Y cho cue ball (sẽ được set lại ở dưới)
                oldBallX[i] = balls[i].x;
                oldBallY[i] = balls[i].y;
            }
            
            // Xử lý đặc biệt cho quả bi trắng (cue ball) - index 0
            if (isCueBall) {
                // Quả bi trắng rơi vào lỗ - đặt lại ở vị trí ban đầu
                Serial.println("Cue ball pocketed! Resetting cue ball position...");
                
                // Đặt lại quả bi trắng ở vị trí ban đầu (góc dưới trái)
                balls[i].x = TABLE_X + 80;
                balls[i].y = TABLE_Y + TABLE_HEIGHT - 40;
                oldBallX[i] = balls[i].x;
                oldBallY[i] = balls[i].y;
                
                // Vẽ lại quả bi trắng ở vị trí mới
                drawBall(i);
                
                // Cho phép ngắm lại
                isAiming = true;
                isCharging = false;
                cuePower = 0.0f;
                oldCuePower = 0.0f;
                
                Serial.println("Cue ball reset to starting position.");
            } else {
                // Quả bi khác rơi vào lỗ - MẤT VĨNH VIỄN, không bao giờ lên lại
                // Đảm bảo x = -100 để đánh dấu đã rơi vào lỗ
                balls[i].x = -100;
                balls[i].y = -100;
                oldBallX[i] = -100;
                oldBallY[i] = -100;
                
                Serial.print("Ball ");
                Serial.print(i);
                Serial.println(" pocketed! (permanently removed)");
            }
            continue;  // Bỏ qua wall collision check
        }
        
        // Check wall collisions after second movement - CHỈ nếu quả bi chưa rơi vào lỗ
        if (balls[i].x >= 0 && !isBallInPocket(balls[i]) && !isBallInPocketAttractionZone(balls[i])) {
            checkWallCollisions(balls[i]);
        }
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
    // Kiểm tra NGAY LẬP TỨC nếu quả bi đã rơi vào lỗ (x < 0) - KHÔNG BAO GIỜ xử lý
    if (ball.x < 0) {
        return;  // Quả bi đã rơi vào lỗ, không xử lý wall collision
    }
    
    // Pocket and attraction zones have absolute priority over rail collisions
    // Skip any wall handling once the ball has entered the pocket mouth area
    if (isBallInPocket(ball) || isBallInPocketAttractionZone(ball)) {
        return;
    }
    
    // XỬ LÝ ĐẶC BIỆT CHO GÓC: Kiểm tra xem quả bi có đang ở góc giao thành bàn và viền lỗ không
    // Đảm bảo quả bi không bị biến dạng khi kẹt ở góc
    int cornerPockets[4][2] = {
        {TABLE_X, TABLE_Y},  // Top-left corner
        {TABLE_X + TABLE_WIDTH, TABLE_Y},  // Top-right corner
        {TABLE_X, TABLE_Y + TABLE_HEIGHT},  // Bottom-left corner
        {TABLE_X + TABLE_WIDTH, TABLE_Y + TABLE_HEIGHT}  // Bottom-right corner
    };
    
    bool isInCornerArea = false;
    float cornerPocketDistance = 0;
    int cornerPocketIndex = -1;
    
    // Kiểm tra xem quả bi có đang ở trong vùng góc không (vùng có thể bị kẹt)
    for (int c = 0; c < 4; c++) {
        float dx = ball.x - cornerPockets[c][0];
        float dy = ball.y - cornerPockets[c][1];
        float distance = sqrt(dx * dx + dy * dy);
        
        // Vùng góc: từ POCKET_RADIUS - 2 đến POCKET_RADIUS + BALL_RADIUS + 5
        // Đây là vùng quả bi có thể bị kẹt giữa thành bàn và viền lỗ
        if (distance >= POCKET_RADIUS - 2.0f && distance <= POCKET_RADIUS + BALL_RADIUS + 5.0f) {
            isInCornerArea = true;
            cornerPocketDistance = distance;
            cornerPocketIndex = c;
            break;
        }
    }
    
    // Nếu quả bi đang ở trong vùng góc, xử lý đặc biệt
    if (isInCornerArea) {
        float currentSpeed = sqrt(ball.vx * ball.vx + ball.vy * ball.vy);
        float minDistanceFromEdge = BALL_RADIUS + 1.0f;  // Đảm bảo quả bi không bị "ăn" vào thành bàn
        
        // QUAN TRỌNG: LUÔN đảm bảo quả bi cách thành bàn đủ xa khi ở góc (bất kể vận tốc)
        // Điều này ngăn quả bi bị "ăn" vào thành bàn ở góc
        if (cornerPocketIndex == 0) {  // Top-left
            if (ball.x < TABLE_X + minDistanceFromEdge) {
                ball.x = TABLE_X + minDistanceFromEdge;
            }
            if (ball.y < TABLE_Y + minDistanceFromEdge) {
                ball.y = TABLE_Y + minDistanceFromEdge;
            }
        } else if (cornerPocketIndex == 1) {  // Top-right
            if (ball.x > TABLE_X + TABLE_WIDTH - minDistanceFromEdge) {
                ball.x = TABLE_X + TABLE_WIDTH - minDistanceFromEdge;
            }
            if (ball.y < TABLE_Y + minDistanceFromEdge) {
                ball.y = TABLE_Y + minDistanceFromEdge;
            }
        } else if (cornerPocketIndex == 2) {  // Bottom-left
            if (ball.x < TABLE_X + minDistanceFromEdge) {
                ball.x = TABLE_X + minDistanceFromEdge;
            }
            if (ball.y > TABLE_Y + TABLE_HEIGHT - minDistanceFromEdge) {
                ball.y = TABLE_Y + TABLE_HEIGHT - minDistanceFromEdge;
            }
        } else if (cornerPocketIndex == 3) {  // Bottom-right
            if (ball.x > TABLE_X + TABLE_WIDTH - minDistanceFromEdge) {
                ball.x = TABLE_X + TABLE_WIDTH - minDistanceFromEdge;
            }
            if (ball.y > TABLE_Y + TABLE_HEIGHT - minDistanceFromEdge) {
                ball.y = TABLE_Y + TABLE_HEIGHT - minDistanceFromEdge;
            }
        }
        
        // Tính vector từ tâm lỗ đến tâm quả bi
        float dx = ball.x - cornerPockets[cornerPocketIndex][0];
        float dy = ball.y - cornerPockets[cornerPocketIndex][1];
        float distToCorner = sqrt(dx * dx + dy * dy);
        if (distToCorner > 0.01f) {
            // Tính vector vận tốc hướng vào lỗ hay ra ngoài
            float dotProduct = (ball.vx * dx + ball.vy * dy) / distToCorner;
            bool movingTowardPocket = dotProduct < 0;  // Vận tốc hướng vào lỗ nếu dotProduct < 0
            
            // Nếu quả bi đi chậm và ở góc
            if (currentSpeed < MIN_VELOCITY * 3.0f) {
                // Dừng quả bi để tránh bị kẹt và biến dạng
                ball.vx = 0.0f;
                ball.vy = 0.0f;
                return;  // Không xử lý wall collision khi ở góc và đi chậm
            } else if (movingTowardPocket) {
                // Quả bi đi đủ nhanh và hướng vào lỗ - để rơi vào lỗ tự nhiên
                // Không bounce, chỉ giảm vận tốc một chút
                ball.vx *= 0.95f;
                ball.vy *= 0.95f;
                return;  // Không xử lý wall collision
            }
            // Nếu quả bi đi đủ nhanh và hướng ra ngoài, tiếp tục xử lý wall collision bình thường
            // Nhưng vị trí đã được đảm bảo cách thành bàn đủ xa ở trên
        }
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
    
    // Đảm bảo quả bi không bị "ăn" vào thành bàn - phải cách mép bàn đủ xa
    // Thành bàn có độ dày, nên cần thêm margin để quả bi hoàn toàn nằm trên bàn
    int railThickness = 4;  // Độ dày thành bàn
    float minDistanceFromEdge = BALL_RADIUS + 1.0f;  // Khoảng cách tối thiểu từ mép bàn (BALL_RADIUS + 1 để đảm bảo không bị ăn vào)
    
    // Tính vận tốc hiện tại để quyết định có nên bounce hay dừng lại
    float currentSpeed = sqrt(ball.vx * ball.vx + ball.vy * ball.vy);
    float bounceThreshold = MIN_VELOCITY * 2.0f;  // Nếu vận tốc < threshold * 2, dừng lại thay vì bounce
    
    // Left wall (but not near top-left or bottom-left pockets)
    // Đặt quả bi cách mép bàn đủ xa để toàn bộ quả bi nằm trên bàn
    if (ball.x - BALL_RADIUS < TABLE_X && !nearTopLeftPocket && !nearBottomLeftPocket) {
        ball.x = TABLE_X + minDistanceFromEdge;  // Cách mép bàn đủ xa
        // Nếu vận tốc quá thấp, dừng lại thay vì bounce
        if (currentSpeed < bounceThreshold) {
            ball.vx = 0.0f;
            ball.vy = 0.0f;
        } else {
            ball.vx = -ball.vx * 0.8f;  // Bounce with some energy loss
        }
    }
    // Right wall (but not near top-right or bottom-right pockets)
    if (ball.x + BALL_RADIUS > TABLE_X + TABLE_WIDTH && !nearTopRightPocket && !nearBottomRightPocket) {
        ball.x = TABLE_X + TABLE_WIDTH - minDistanceFromEdge;  // Cách mép bàn đủ xa
        // Nếu vận tốc quá thấp, dừng lại thay vì bounce
        if (currentSpeed < bounceThreshold) {
            ball.vx = 0.0f;
            ball.vy = 0.0f;
        } else {
            ball.vx = -ball.vx * 0.8f;
        }
    }
    // Top wall (but not near top pockets)
    if (ball.y - BALL_RADIUS < TABLE_Y && !nearTopLeftPocket && !nearTopRightPocket && !nearTopMiddlePocket) {
        ball.y = TABLE_Y + minDistanceFromEdge;  // Cách mép bàn đủ xa
        // Nếu vận tốc quá thấp, dừng lại thay vì bounce
        if (currentSpeed < bounceThreshold) {
            ball.vx = 0.0f;
            ball.vy = 0.0f;
        } else {
            ball.vy = -ball.vy * 0.8f;
        }
    }
    // Bottom wall (but not near bottom pockets)
    if (ball.y + BALL_RADIUS > TABLE_Y + TABLE_HEIGHT && !nearBottomLeftPocket && !nearBottomRightPocket && !nearBottomMiddlePocket) {
        ball.y = TABLE_Y + TABLE_HEIGHT - minDistanceFromEdge;  // Cách mép bàn đủ xa
        // Nếu vận tốc quá thấp, dừng lại thay vì bounce
        if (currentSpeed < bounceThreshold) {
            ball.vx = 0.0f;
            ball.vy = 0.0f;
        } else {
            ball.vy = -ball.vy * 0.8f;
        }
    }
    
    // Đảm bảo quả bi không đi sâu vào cạnh bàn (thêm kiểm tra an toàn)
    // Nhưng không áp dụng nếu quả bi đang gần lỗ (để quả bi có thể rơi vào lỗ)
    bool nearAnyPocket = nearTopLeftPocket || nearTopRightPocket || nearBottomLeftPocket || 
                         nearBottomRightPocket || nearTopMiddlePocket || nearBottomMiddlePocket;
    
    if (!nearAnyPocket) {
        // Chỉ đẩy quả bi ra nếu không gần lỗ
        // Đảm bảo bi không đi ra ngoài bàn quá xa và không bị "ăn" vào thành bàn
        float minDistanceFromEdge = BALL_RADIUS + 1.0f;  // Khoảng cách tối thiểu từ mép bàn
        float currentSpeed = sqrt(ball.vx * ball.vx + ball.vy * ball.vy);
        float bounceThreshold = MIN_VELOCITY * 2.0f;  // Nếu vận tốc < threshold * 2, dừng lại
        
        if (ball.x < TABLE_X + minDistanceFromEdge) {
            ball.x = TABLE_X + minDistanceFromEdge;
            // Nếu vận tốc quá thấp, dừng lại thay vì bounce
            if (currentSpeed < bounceThreshold) {
                ball.vx = 0.0f;
                ball.vy = 0.0f;
            } else {
                ball.vx = -ball.vx * 0.8f;  // Bounce if not near pocket
            }
        }
        if (ball.x > TABLE_X + TABLE_WIDTH - minDistanceFromEdge) {
            ball.x = TABLE_X + TABLE_WIDTH - minDistanceFromEdge;
            // Nếu vận tốc quá thấp, dừng lại thay vì bounce
            if (currentSpeed < bounceThreshold) {
                ball.vx = 0.0f;
                ball.vy = 0.0f;
            } else {
                ball.vx = -ball.vx * 0.8f;
            }
        }
        if (ball.y < TABLE_Y + minDistanceFromEdge) {
            ball.y = TABLE_Y + minDistanceFromEdge;
            // Nếu vận tốc quá thấp, dừng lại thay vì bounce
            if (currentSpeed < bounceThreshold) {
                ball.vx = 0.0f;
                ball.vy = 0.0f;
            } else {
                ball.vy = -ball.vy * 0.8f;
            }
        }
        if (ball.y > TABLE_Y + TABLE_HEIGHT - minDistanceFromEdge) {
            ball.y = TABLE_Y + TABLE_HEIGHT - minDistanceFromEdge;
            // Nếu vận tốc quá thấp, dừng lại thay vì bounce
            if (currentSpeed < bounceThreshold) {
                ball.vx = 0.0f;
                ball.vy = 0.0f;
            } else {
                ball.vy = -ball.vy * 0.8f;
            }
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

bool BilliardGame::isBallInPocketAttractionZone(const Ball& ball) {
    if (ball.x < 0) {
        return false;  // Already pocketed sentinel
    }
    
    float attractionRadius = POCKET_RADIUS + BALL_RADIUS + POCKET_ATTRACTION_MARGIN;
    float attractionRadiusSq = attractionRadius * attractionRadius;
    
    int pockets[6][2] = {
        {TABLE_X, TABLE_Y},  // Top-left
        {TABLE_X + TABLE_WIDTH, TABLE_Y},  // Top-right
        {TABLE_X, TABLE_Y + TABLE_HEIGHT},  // Bottom-left
        {TABLE_X + TABLE_WIDTH, TABLE_Y + TABLE_HEIGHT},  // Bottom-right
        {TABLE_X + TABLE_WIDTH / 2, TABLE_Y},  // Top middle
        {TABLE_X + TABLE_WIDTH / 2, TABLE_Y + TABLE_HEIGHT}  // Bottom middle
    };
    
    for (int p = 0; p < 6; p++) {
        float dx = ball.x - pockets[p][0];
        float dy = ball.y - pockets[p][1];
        float distSq = dx * dx + dy * dy;
        if (distSq <= attractionRadiusSq) {
            return true;
        }
    }
    return false;
}

void BilliardGame::applyPocketAttraction(Ball& ball) {
    // Only apply when inside the attraction zone
    float attractionRadius = POCKET_RADIUS + BALL_RADIUS + POCKET_ATTRACTION_MARGIN;
    float attractionRadiusSq = attractionRadius * attractionRadius;
    
    int pockets[6][2] = {
        {TABLE_X, TABLE_Y},  // Top-left
        {TABLE_X + TABLE_WIDTH, TABLE_Y},  // Top-right
        {TABLE_X, TABLE_Y + TABLE_HEIGHT},  // Bottom-left
        {TABLE_X + TABLE_WIDTH, TABLE_Y + TABLE_HEIGHT},  // Bottom-right
        {TABLE_X + TABLE_WIDTH / 2, TABLE_Y},  // Top middle
        {TABLE_X + TABLE_WIDTH / 2, TABLE_Y + TABLE_HEIGHT}  // Bottom middle
    };
    
    int targetPocket = -1;
    float closestDistSq = attractionRadiusSq;
    
    for (int p = 0; p < 6; p++) {
        float dx = ball.x - pockets[p][0];
        float dy = ball.y - pockets[p][1];
        float distSq = dx * dx + dy * dy;
        if (distSq < closestDistSq) {
            closestDistSq = distSq;
            targetPocket = p;
        }
    }
    
    if (targetPocket == -1) {
        return;
    }
    
    float dirX = pockets[targetPocket][0] - ball.x;
    float dirY = pockets[targetPocket][1] - ball.y;
    float dist = sqrt(dirX * dirX + dirY * dirY);
    if (dist < 0.001f) {
        return;
    }
    
    float normX = dirX / dist;
    float normY = dirY / dist;
    
    ball.vx += normX * POCKET_ATTRACTION;
    ball.vy += normY * POCKET_ATTRACTION;
    ball.isActive = true;  // Keep ball active while being pulled in
}

bool BilliardGame::isBallNearPocket(float x, float y) {
    float pocketCheckRadius = POCKET_RADIUS + BALL_RADIUS + POCKET_ATTRACTION_MARGIN;
    int pockets[6][2] = {
        {TABLE_X, TABLE_Y},  // Top-left
        {TABLE_X + TABLE_WIDTH, TABLE_Y},  // Top-right
        {TABLE_X, TABLE_Y + TABLE_HEIGHT},  // Bottom-left
        {TABLE_X + TABLE_WIDTH, TABLE_Y + TABLE_HEIGHT},  // Bottom-right
        {TABLE_X + TABLE_WIDTH / 2, TABLE_Y},  // Top middle
        {TABLE_X + TABLE_WIDTH / 2, TABLE_Y + TABLE_HEIGHT}  // Bottom middle
    };
    for (int p = 0; p < 6; p++) {
        float dx = x - pockets[p][0];
        float dy = y - pockets[p][1];
        float dist = sqrt(dx * dx + dy * dy);
        if (dist < pocketCheckRadius) {
            return true;
        }
    }
    return false;
}

void BilliardGame::checkBallCollisions() {
    // Check multiple times per frame to prevent fast balls from passing through
    const int iterations = 3;
    
    for (int iter = 0; iter < iterations; iter++) {
        for (int i = 0; i < BALL_COUNT; i++) {
            // Skip if ball is already pocketed (x < 0)
            if (balls[i].x < 0) continue;
            // Skip if ball is already in pocket (kiểm tra lại để chắc chắn)
            if (isBallInPocket(balls[i])) continue;
            // Skip attraction zone to avoid unnatural bounces when falling in
            if (isBallInPocketAttractionZone(balls[i])) continue;
            if (!balls[i].isActive && iter == 0) continue;  // Only check active balls on first iteration
            // Skip if ball is near pocket (đang rơi vào lỗ) - tránh bounce khi rơi vào lỗ
            if (isBallNearPocket(balls[i].x, balls[i].y)) continue;
            
            for (int j = i + 1; j < BALL_COUNT; j++) {
                // Skip if ball is already pocketed (x < 0)
                if (balls[j].x < 0) continue;
                // Skip if ball is already in pocket (kiểm tra lại để chắc chắn)
                if (isBallInPocket(balls[j])) continue;
                // Skip attraction zone to avoid unnatural bounces when falling in
                if (isBallInPocketAttractionZone(balls[j])) continue;
                if (!balls[j].isActive && iter == 0) continue;  // Only check active balls on first iteration
                // Skip if ball is near pocket (đang rơi vào lỗ) - tránh bounce khi rơi vào lỗ
                if (isBallNearPocket(balls[j].x, balls[j].y)) continue;
                
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
                        // Flag that a visible collision happened this frame (to force safe redraw)
                        collisionThisFrame = true;
                        // Elastic collision: exchange momentum along collision normal
                        // Assuming equal mass (all balls have same mass)
                        // Giảm hệ số va chạm để cân bằng - quả bi đánh đi không quá yếu, quả bi bị đánh không quá mạnh
                        float impulse = 0.6f * dot;  // Giảm từ 0.8f xuống 0.6f để cân bằng hơn
                        
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
        
        // Bi rơi vào lỗ khi tâm bi nằm trong bán kính lỗ
        // Sử dụng POCKET_RADIUS để cho phép bi rơi vào lỗ dễ hơn
        // Nếu tâm bi cách tâm lỗ < POCKET_RADIUS thì coi là rơi vào lỗ
        float maxDistance = POCKET_RADIUS;  // 12 pixel - cho phép bi rơi vào lỗ dễ hơn
        
        // Coi là rơi vào lỗ nếu tâm bi nằm trong phạm vi này
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

void BilliardGame::handleAimUp() {
    if (isAiming && !balls[activeBallIndex].isActive) {
        eraseCueStick();
        // Xoay lên (góc -90 độ hoặc 270 độ, tức là -π/2)
        // Trong hệ tọa độ màn hình, lên là -y, nên góc là -π/2
        cueAngle = -M_PI / 2.0f;
        if (cueAngle < 0) cueAngle += 2 * M_PI;
        drawCueStick();
    }
}

void BilliardGame::handleAimDown() {
    if (isAiming && !balls[activeBallIndex].isActive) {
        eraseCueStick();
        // Xoay xuống (góc 90 độ, tức là π/2)
        // Trong hệ tọa độ màn hình, xuống là +y, nên góc là π/2
        cueAngle = M_PI / 2.0f;
        drawCueStick();
    }
}

void BilliardGame::handleAimAngle(float angle) {
    if (isAiming && !balls[activeBallIndex].isActive) {
        eraseCueStick();
        // Xoay theo góc cụ thể (radians)
        cueAngle = angle;
        // Đảm bảo góc nằm trong khoảng [0, 2π)
        while (cueAngle < 0) cueAngle += 2 * M_PI;
        while (cueAngle >= 2 * M_PI) cueAngle -= 2 * M_PI;
        drawCueStick();
    }
}

void BilliardGame::handleAimRotate(float deltaAngle) {
    if (isAiming && !balls[activeBallIndex].isActive) {
        eraseCueStick();
        // Xoay thêm một góc từ góc hiện tại
        cueAngle += deltaAngle;
        // Đảm bảo góc nằm trong khoảng [0, 2π)
        while (cueAngle < 0) cueAngle += 2 * M_PI;
        while (cueAngle >= 2 * M_PI) cueAngle -= 2 * M_PI;
        drawCueStick();
    }
}

// Xoay theo 8 hướng chính
void BilliardGame::handleAimNorth() {
    handleAimAngle(-M_PI / 2.0f);  // Lên (Bắc)
}

void BilliardGame::handleAimNorthEast() {
    handleAimAngle(-M_PI / 4.0f);  // Đông Bắc (45 độ)
}

void BilliardGame::handleAimEast() {
    handleAimAngle(0.0f);  // Phải (Đông, 0 độ)
}

void BilliardGame::handleAimSouthEast() {
    handleAimAngle(M_PI / 4.0f);  // Đông Nam (45 độ)
}

void BilliardGame::handleAimSouth() {
    handleAimAngle(M_PI / 2.0f);  // Xuống (Nam, 90 độ)
}

void BilliardGame::handleAimSouthWest() {
    handleAimAngle(3.0f * M_PI / 4.0f);  // Tây Nam (135 độ)
}

void BilliardGame::handleAimWest() {
    handleAimAngle(M_PI);  // Trái (Tây, 180 độ)
}

void BilliardGame::handleAimNorthWest() {
    handleAimAngle(-3.0f * M_PI / 4.0f);  // Tây Bắc (225 độ)
}

void BilliardGame::handleChargeStart() {
    if (isAiming && !balls[activeBallIndex].isActive) {
        isCharging = true;
        cuePower = 0.0f;
        oldCuePower = 0.0f;
        // Vẽ thanh lực ngay khi bắt đầu charge
        drawPowerBar();
    }
}

void BilliardGame::handleChargeRelease() {
    if (isCharging && isAiming) {
        // QUAN TRỌNG: Xóa gậy TRƯỚC KHI set isAiming = false
        // Đảm bảo gậy được xóa sạch, không để lại dấu vết
        eraseCueStick();
        
        // Khôi phục nền vùng thanh lực - trả lại nền đúng màu
        int barX = 10;
        int barY = SCREEN_HEIGHT - 30;
        int barWidth = 100;
        int barHeight = 12;
        int textWidth = 30;
        int clearHeight = barHeight + 12;  // Chiều cao vùng cần xóa (bao gồm text)
        
        // Tính toán vùng bị đè bởi thanh lực
        int clearX = barX;
        int clearY = barY - 2;
        int clearWidth = barWidth + textWidth;
        
        // Xác định vùng nào nằm trên bàn, vùng nào nằm ngoài bàn
        int tableScreenY = SCREEN_HEIGHT - TABLE_Y;  // Top của bàn (trên màn hình)
        int tableBottomScreenY = SCREEN_HEIGHT - (TABLE_Y + TABLE_HEIGHT);  // Bottom của bàn (trên màn hình)
        int tableMinX = TABLE_X;
        int tableMaxX = TABLE_X + TABLE_WIDTH;
        
        // Xóa và vẽ lại từng phần với màu nền đúng
        for (int py = clearY; py < clearY + clearHeight; py++) {
            for (int px = clearX; px < clearX + clearWidth; px++) {
                // Sử dụng hàm getBackgroundColorAt để lấy màu nền chính xác
                uint16_t bgColor = getBackgroundColorAt(px, py);
                tft->drawPixel(px, py, bgColor);
            }
        }
        
        // Vẽ lại thành bàn nếu thanh lực che mất thành bàn
        // Kiểm tra xem thanh lực có che mất thành bàn dưới không
        if (clearY <= tableBottomScreenY && (clearY + clearHeight) >= tableBottomScreenY) {
            // Vẽ lại thành bàn dưới
            int railThickness = 4;
            for (int i = 0; i < railThickness; i++) {
                // Chỉ vẽ phần không bị che bởi thanh lực
                int drawStartX = (clearX < tableMinX) ? tableMinX : clearX + clearWidth;
                int drawEndX = tableMaxX;
                if (drawStartX < drawEndX) {
                    tft->drawLine(drawStartX, tableBottomScreenY - i, drawEndX, tableBottomScreenY - i, COLOR_TABLE_BORDER);
                }
            }
        }
        
        // QUAN TRỌNG: Vẽ lại các quả bi bị đè bởi thanh lực
        for (int i = 0; i < BALL_COUNT; i++) {
            // Bỏ qua quả bi đã vào lỗ
            if (balls[i].x < 0) continue;
            
            // Kiểm tra xem quả bi có nằm trong vùng thanh lực không
            int ballScreenX = (int)balls[i].x;
            int ballScreenY = SCREEN_HEIGHT - (int)balls[i].y;
            int ballRadius = BALL_RADIUS;
            
            // Kiểm tra xem quả bi có giao với vùng thanh lực không
            bool ballInPowerBarArea = (ballScreenX + ballRadius >= clearX && 
                                       ballScreenX - ballRadius <= clearX + clearWidth &&
                                       ballScreenY + ballRadius >= clearY && 
                                       ballScreenY - ballRadius <= clearY + clearHeight);
            
            if (ballInPowerBarArea) {
                // Vẽ lại quả bi này
                drawBall(i);
            }
        }
        
        // Shoot the cue ball
        // Quả bi chạy về hướng cueAngle (phía trước), gậy nằm phía sau
        Ball& cueBall = balls[activeBallIndex];
        
        // Đảm bảo có lực tối thiểu để bắn
        float finalPower = cuePower;
        if (finalPower < 5.0f) {
            finalPower = 5.0f;  // Lực tối thiểu
        }
        
        // Scale power to velocity - điều chỉnh để cân bằng hơn
        float speed = finalPower * 0.3f;  // Giảm từ 0.4f xuống 0.3f để cân bằng với va chạm
        cueBall.vx = cos(cueAngle) * speed;  // Chạy về hướng góc (phía trước)
        cueBall.vy = sin(cueAngle) * speed;  // Chạy về hướng góc (phía trước)
        cueBall.isActive = true;
        
        // Tắt aiming và charging SAU KHI đã xóa gậy và đánh quả bi
        isAiming = false;
        isCharging = false;
        cuePower = 0.0f;
        oldCuePower = 0.0f;
        
        // XÓA GẬY LẦN NỮA để đảm bảo không còn dấu vết
        // Gọi hàm xóa gậy trực tiếp dựa trên oldCueAngle và oldCuePower
        eraseCueStickAtPosition(oldCueAngle, oldCuePower);
    }
}

void BilliardGame::handlePowerUp() {
    // Tăng lực nhanh khi đang charge
    if (isCharging && isAiming) {
        cuePower += 5.0f;  // Tăng lực nhanh
        if (cuePower > 100.0f) {
            cuePower = 100.0f;  // Cap at 100
        }
        // Vẽ lại power bar ngay lập tức
        drawPowerBar();
        oldCuePower = cuePower;
    }
}

void BilliardGame::handlePowerDown() {
    // Giảm lực khi đang charge
    if (isCharging && isAiming) {
        cuePower -= 3.0f;  // Giảm lực
        if (cuePower < 0.0f) {
            cuePower = 0.0f;  // Không cho phép lực âm
        }
        // Vẽ lại power bar ngay lập tức
        drawPowerBar();
        oldCuePower = cuePower;
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

void BilliardGame::aimAtNearestPocket() {
    if (!isAiming || balls[activeBallIndex].isActive) {
        return;  // Không thể ngắm nếu đang không ở chế độ ngắm hoặc quả bi đang di chuyển
    }
    
    Ball& cueBall = balls[activeBallIndex];
    
    // Danh sách 6 lỗ trên bàn
    float pockets[6][2] = {
        {TABLE_X, TABLE_Y},  // Top-left
        {TABLE_X + TABLE_WIDTH, TABLE_Y},  // Top-right
        {TABLE_X, TABLE_Y + TABLE_HEIGHT},  // Bottom-left
        {TABLE_X + TABLE_WIDTH, TABLE_Y + TABLE_HEIGHT},  // Bottom-right
        {TABLE_X + TABLE_WIDTH / 2, TABLE_Y},  // Top middle
        {TABLE_X + TABLE_WIDTH / 2, TABLE_Y + TABLE_HEIGHT}  // Bottom middle
    };
    
    // Tìm lỗ gần nhất
    float minDistance = 10000.0f;
    int nearestPocketIndex = -1;
    
    for (int i = 0; i < 6; i++) {
        float dx = pockets[i][0] - cueBall.x;
        float dy = pockets[i][1] - cueBall.y;
        float distance = sqrt(dx * dx + dy * dy);
        
        if (distance < minDistance) {
            minDistance = distance;
            nearestPocketIndex = i;
        }
    }
    
    // Nếu tìm thấy lỗ gần nhất, ngắm thẳng vào nó
    if (nearestPocketIndex >= 0) {
        float dx = pockets[nearestPocketIndex][0] - cueBall.x;
        float dy = pockets[nearestPocketIndex][1] - cueBall.y;
        
        // Tính góc để ngắm thẳng vào lỗ
        float targetAngle = atan2(dy, dx);
        
        // Xóa gậy cũ trước khi cập nhật góc
        eraseCueStick();
        
        // Cập nhật góc
        cueAngle = targetAngle;
        
        // Vẽ gậy mới
        drawCueStick();
        
        Serial.print("Aiming at pocket ");
        Serial.print(nearestPocketIndex);
        Serial.print(" at angle: ");
        Serial.println(targetAngle * 180.0f / M_PI);
    }
}

