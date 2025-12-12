#ifndef GAME_MENU_H
#define GAME_MENU_H

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

enum MenuItem {
    MENU_START_GAME,
    MENU_SETTINGS,
    MENU_ABOUT,
    MENU_COUNT
};

class GameMenuScreen {
private:
    Adafruit_ST7789* tft;
    MenuItem selectedItem;
    uint16_t animationFrame;
    
    void drawBackground();
    void drawMenuItem(uint16_t index, bool isSelected);
    void drawTitle();
    
public:
    GameMenuScreen(Adafruit_ST7789* tft);
    
    void draw();
    void selectNext();
    void selectPrevious();
    void selectItem(MenuItem item);
    MenuItem getSelectedItem() const { return selectedItem; }
    void update();  // For animations
};

#endif

