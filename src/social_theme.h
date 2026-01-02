#ifndef SOCIAL_THEME_H
#define SOCIAL_THEME_H

#include <Arduino.h>

struct SocialTheme {
    uint16_t colorBg;
    uint16_t colorSidebarBg;
    uint16_t colorCardBg;
    uint16_t colorHighlight;
    uint16_t colorAccent;
    uint16_t colorTextMain;
    uint16_t colorTextMuted;
    uint16_t colorSuccess;
    uint16_t colorError;
    
    uint8_t  tabHeight;
    uint8_t  rowHeight;
    uint8_t  sidebarWidth;
    uint8_t  cornerRadius;
    uint8_t  itemPadding;
    uint8_t  headerHeight;
    
    bool     showTabBorders;
    bool     useFloatingTabs;
};

#endif

