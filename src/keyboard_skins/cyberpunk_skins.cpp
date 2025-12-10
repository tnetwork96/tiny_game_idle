#include "cyberpunk_skins.h"

namespace CyberpunkSkins {

KeyboardSkin getCyberpunkSkin(CyberpunkVariant variant) {
    switch (variant) {
        case CyberpunkVariant::TRON:
            return getTron();
        case CyberpunkVariant::MATRIX:
            return getMatrix();
        case CyberpunkVariant::NEON_PURPLE:
            return getNeonPurple();
        default:
            return getTron();
    }
}

KeyboardSkin getTron() {
    KeyboardSkin skin;
    // Màu sắc cyberpunk/tron: đen, cyan neon
    skin.keyBgColor = 0x0000;        // Black (nền đen)
    skin.keySelectedColor = 0x07FF;  // Neon cyan (phím được chọn sáng)
    skin.keyTextColor = 0x07FF;      // Neon cyan (chữ cyan sáng trên nền đen)
    skin.keySelectedTextColor = 0x0000; // Black text on selected key
    skin.keyBorderColor = 0x07FF;    // Neon cyan border (viền cyan)
    skin.bgScreenColor = 0x0000;      // Black background
    skin.keyWidth = 22;
    skin.keyHeight = 22;
    skin.spacing = 2;
    skin.textSize = 1;
    skin.hasBorder = true;           // Có viền sáng
    skin.roundedCorners = false;     // Góc vuông (tron style)
    skin.cornerRadius = 0;
    skin.hasPattern = true;           // Có pattern (grid + glow)
    return skin;
}

KeyboardSkin getMatrix() {
    KeyboardSkin skin;
    // Màu sắc Matrix: đen, xanh lá neon
    skin.keyBgColor = 0x0000;        // Black
    skin.keySelectedColor = 0x07E0;  // Neon green (phím được chọn)
    skin.keyTextColor = 0x07E0;      // Neon green (chữ xanh lá)
    skin.keySelectedTextColor = 0x0000; // Black text on selected key
    skin.keyBorderColor = 0x07E0;    // Neon green border
    skin.bgScreenColor = 0x0000;     // Black background
    skin.keyWidth = 22;
    skin.keyHeight = 22;
    skin.spacing = 2;
    skin.textSize = 1;
    skin.hasBorder = true;
    skin.roundedCorners = false;
    skin.cornerRadius = 0;
    skin.hasPattern = true;
    return skin;
}

KeyboardSkin getNeonPurple() {
    KeyboardSkin skin;
    // Màu sắc neon purple: đen, tím neon
    skin.keyBgColor = 0x0000;        // Black
    skin.keySelectedColor = 0xD81F;  // Neon purple (phím được chọn)
    skin.keyTextColor = 0xD81F;      // Neon purple (chữ tím)
    skin.keySelectedTextColor = 0xFFFF; // White text on selected key
    skin.keyBorderColor = 0xD81F;    // Neon purple border
    skin.bgScreenColor = 0x0000;     // Black background
    skin.keyWidth = 22;
    skin.keyHeight = 22;
    skin.spacing = 2;
    skin.textSize = 1;
    skin.hasBorder = true;
    skin.roundedCorners = false;
    skin.cornerRadius = 0;
    skin.hasPattern = true;
    return skin;
}

} // namespace CyberpunkSkins

