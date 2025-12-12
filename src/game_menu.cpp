#include <Arduino.h>
#include "game_menu.h"
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

// Synthwave/Vaporwave color palette
#define NEON_PURPLE 0xD81F        // Neon purple
#define NEON_GREEN 0x07E0         // Neon green
#define NEON_CYAN 0x07FF          // Neon cyan
#define YELLOW_ORANGE 0xFE20       // Yellow-orange
#define DARK_PURPLE 0x8010        // Dark purple background
#define SOFT_WHITE 0xCE79         // Soft white
#define NEON_PINK 0xF81F          // Neon pink

const char* menuItems[] = {
    "START GAME",
    "SETTINGS",
    "ABOUT"
};

GameMenuScreen::GameMenuScreen(Adafruit_ST7789* tft) {
    this->tft = tft;
    this->selectedItem = MENU_START_GAME;
    this->animationFrame = 0;
}

void GameMenuScreen::drawBackground() {
    // Vẽ nền gradient synthwave (từ đen đến tím đậm)
    uint16_t screenWidth = 320;
    uint16_t screenHeight = 240;
    
    // Gradient từ đen (dưới) đến tím đậm (trên)
    for (uint16_t y = 0; y < screenHeight; y++) {
        uint16_t color = 0x0000;  // Đen
        if (y < screenHeight / 2) {
            // Phần trên: gradient từ tím đậm đến đen
            uint16_t ratio = (screenHeight / 2 - y) * 255 / (screenHeight / 2);
            uint8_t r = (DARK_PURPLE >> 11) & 0x1F;
            uint8_t g = (DARK_PURPLE >> 5) & 0x3F;
            uint8_t b = DARK_PURPLE & 0x1F;
            
            r = (r * ratio) / 255;
            g = (g * ratio) / 255;
            b = (b * ratio) / 255;
            
            color = (r << 11) | (g << 5) | b;
        }
        tft->drawFastHLine(0, y, screenWidth, color);
    }
    
    // Vẽ grid pattern (đường kẻ ngang mờ)
    tft->setTextColor(0x2104, 0x0000);  // Màu xám rất đậm
    for (uint16_t y = 0; y < screenHeight; y += 20) {
        tft->drawFastHLine(0, y, screenWidth, 0x2104);
    }
}

void GameMenuScreen::drawTitle() {
    // Vẽ tiêu đề "TINY GAME"
    tft->setTextSize(3);
    tft->setTextColor(NEON_PURPLE, 0x0000);
    
    // Vẽ shadow (để tạo hiệu ứng neon)
    tft->setCursor(42, 22);
    tft->setTextColor(0x0000, 0x0000);
    tft->print("TINY GAME");
    
    // Vẽ text chính
    tft->setCursor(40, 20);
    tft->setTextColor(NEON_PURPLE, 0x0000);
    tft->print("TINY GAME");
}

void GameMenuScreen::drawMenuItem(uint16_t index, bool isSelected) {
    if (index >= MENU_COUNT) return;
    
    uint16_t yPos = 90 + index * 40;  // Khoảng cách giữa các item
    uint16_t xPos = 60;
    
    // Vẽ background cho item được chọn
    if (isSelected) {
        // Vẽ highlight box với màu neon
        uint16_t boxColor = NEON_PURPLE;
        tft->fillRoundRect(xPos - 10, yPos - 5, 200, 30, 5, boxColor);
        
        // Vẽ border glow
        tft->drawRoundRect(xPos - 10, yPos - 5, 200, 30, 5, NEON_CYAN);
    }
    
    // Vẽ text
    tft->setTextSize(2);
    if (isSelected) {
        tft->setTextColor(0x0000, NEON_PURPLE);  // Chữ đen trên nền tím
    } else {
        tft->setTextColor(SOFT_WHITE, 0x0000);  // Chữ trắng mờ
    }
    
    tft->setCursor(xPos, yPos);
    tft->print(menuItems[index]);
    
    // Vẽ mũi tên chỉ vào item được chọn
    if (isSelected) {
        tft->setTextSize(2);
        tft->setTextColor(NEON_CYAN, NEON_PURPLE);
        tft->setCursor(xPos - 25, yPos);
        tft->print(">");
    }
}

void GameMenuScreen::draw() {
    drawBackground();
    drawTitle();
    
    // Vẽ tất cả các menu items
    for (uint16_t i = 0; i < MENU_COUNT; i++) {
        drawMenuItem(i, i == selectedItem);
    }
    
    // Vẽ footer với thông tin WiFi
    tft->setTextSize(1);
    tft->setTextColor(SOFT_WHITE, 0x0000);
    tft->setCursor(10, 220);
    tft->print("WiFi: Connected");
}

void GameMenuScreen::selectNext() {
    selectedItem = (MenuItem)((selectedItem + 1) % MENU_COUNT);
    draw();
}

void GameMenuScreen::selectPrevious() {
    selectedItem = (MenuItem)((selectedItem - 1 + MENU_COUNT) % MENU_COUNT);
    draw();
}

void GameMenuScreen::selectItem(MenuItem item) {
    if (item < MENU_COUNT) {
        selectedItem = item;
        draw();
    }
}

void GameMenuScreen::update() {
    // Cập nhật animation frame cho hiệu ứng
    animationFrame++;
    if (animationFrame > 1000) animationFrame = 0;
}

