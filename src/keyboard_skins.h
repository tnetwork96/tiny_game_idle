#ifndef KEYBOARD_SKINS_H
#define KEYBOARD_SKINS_H

#include <Arduino.h>

// Struct để quản lý skin của bàn phím
struct KeyboardSkin {
    uint16_t keyBgColor;          // Màu nền phím bình thường
    uint16_t keySelectedColor;    // Màu nền phím được chọn
    uint16_t keyTextColor;        // Màu chữ trên phím bình thường
    uint16_t keySelectedTextColor; // Màu chữ trên phím được chọn
    uint16_t keyBorderColor;      // Màu viền phím (nếu có)
    uint16_t bgScreenColor;       // Màu nền màn hình
    uint16_t keyWidth;            // Chiều rộng phím
    uint16_t keyHeight;           // Chiều cao phím
    uint16_t spacing;             // Khoảng cách giữa các phím
    uint16_t textSize;            // Kích thước chữ
    bool hasBorder;               // Có viền phím không
    bool roundedCorners;          // Có bo góc không
    uint16_t cornerRadius;        // Bán kính bo góc (nếu có)
    bool hasPattern;              // Có hoa văn không
    
    // Constructor mặc định
    KeyboardSkin() {
        keyBgColor = 0x8010;
        keySelectedColor = 0xFE20;
        keyTextColor = 0x07E0;
        keySelectedTextColor = 0x0000;
        keyBorderColor = 0x07FF;
        bgScreenColor = 0x0000;
        keyWidth = 22;
        keyHeight = 22;
        spacing = 2;
        textSize = 1;
        hasBorder = false;
        roundedCorners = false;
        cornerRadius = 0;
        hasPattern = false;
    }
};

// Forward declarations - các enum và namespace được định nghĩa trong các file riêng
// Để sử dụng các skin, include file tương ứng:
// #include "keyboard_skins/feminine_skins.h"
// #include "keyboard_skins/cyberpunk_skins.h"
// #include "keyboard_skins/mechanical_skins.h"
// #include "keyboard_skins/default_skins.h"
//
// Hoặc include tất cả qua keyboard_skins_wrapper.h

#endif

