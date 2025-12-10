#include "default_skins.h"

namespace DefaultSkins {

KeyboardSkin getDefaultSkin(DefaultVariant variant) {
    switch (variant) {
        case DefaultVariant::SYNTHWAVE:
            return getSynthwave();
        case DefaultVariant::NEON:
            return getNeon();
        case DefaultVariant::DARK:
            return getDark();
        case DefaultVariant::RETRO:
            return getRetro();
        case DefaultVariant::MINIMAL:
            return getMinimal();
        default:
            return getSynthwave();
    }
}

KeyboardSkin getSynthwave() {
    KeyboardSkin skin;
    skin.keyBgColor = 0x8010;        // Dark purple
    skin.keySelectedColor = 0xFE20;  // Yellow-orange
    skin.keyTextColor = 0x07E0;      // Neon green
    skin.keySelectedTextColor = 0x0000; // Black text on selected key
    skin.keyBorderColor = 0x07FF;    // Neon cyan
    skin.bgScreenColor = 0x0000;     // Black
    skin.keyWidth = 22;
    skin.keyHeight = 22;
    skin.spacing = 2;
    skin.textSize = 1;
    skin.hasBorder = false;
    skin.roundedCorners = false;
    skin.cornerRadius = 0;
    skin.hasPattern = false;
    return skin;
}

KeyboardSkin getNeon() {
    KeyboardSkin skin;
    skin.keyBgColor = 0x0000;        // Black
    skin.keySelectedColor = 0xD81F;  // Neon purple
    skin.keyTextColor = 0x07FF;     // Neon cyan
    skin.keySelectedTextColor = 0xFFFF; // White text on selected key
    skin.keyBorderColor = 0x07E0;    // Neon green
    skin.bgScreenColor = 0x0000;     // Black
    skin.keyWidth = 22;
    skin.keyHeight = 22;
    skin.spacing = 2;
    skin.textSize = 1;
    skin.hasBorder = true;
    skin.roundedCorners = true;
    skin.cornerRadius = 4;
    skin.hasPattern = false;
    return skin;
}

KeyboardSkin getDark() {
    KeyboardSkin skin;
    skin.keyBgColor = 0x2104;        // Dark grey
    skin.keySelectedColor = 0x5AEB;  // Light grey
    skin.keyTextColor = 0xFFFF;      // White
    skin.keySelectedTextColor = 0x0000; // Black text on selected key
    skin.keyBorderColor = 0x4208;    // Medium grey
    skin.bgScreenColor = 0x0000;     // Black
    skin.keyWidth = 22;
    skin.keyHeight = 22;
    skin.spacing = 2;
    skin.textSize = 1;
    skin.hasBorder = false;
    skin.roundedCorners = false;
    skin.cornerRadius = 0;
    skin.hasPattern = false;
    return skin;
}

KeyboardSkin getRetro() {
    KeyboardSkin skin;
    skin.keyBgColor = 0x7BEF;        // Light grey
    skin.keySelectedColor = 0xF800;  // Red
    skin.keyTextColor = 0x0000;      // Black
    skin.keySelectedTextColor = 0xFFFF; // White text on selected key
    skin.keyBorderColor = 0x0000;    // Black
    skin.bgScreenColor = 0xFFFF;     // White
    skin.keyWidth = 22;
    skin.keyHeight = 22;
    skin.spacing = 2;
    skin.textSize = 1;
    skin.hasBorder = true;
    skin.roundedCorners = false;
    skin.cornerRadius = 0;
    skin.hasPattern = false;
    return skin;
}

KeyboardSkin getMinimal() {
    KeyboardSkin skin;
    skin.keyBgColor = 0x4208;        // Medium grey
    skin.keySelectedColor = 0x8410;  // Lighter grey
    skin.keyTextColor = 0xFFFF;      // White
    skin.keySelectedTextColor = 0x0000; // Black text on selected key
    skin.keyBorderColor = 0x0000;    // Black
    skin.bgScreenColor = 0x0000;     // Black
    skin.keyWidth = 22;
    skin.keyHeight = 22;
    skin.spacing = 2;
    skin.textSize = 1;
    skin.hasBorder = false;
    skin.roundedCorners = true;
    skin.cornerRadius = 3;
    skin.hasPattern = false;
    return skin;
}

} // namespace DefaultSkins

