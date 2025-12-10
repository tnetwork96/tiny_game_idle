#include "feminine_skins.h"

namespace FeminineSkins {

KeyboardSkin getFeminineSkin(FeminineVariant variant) {
    switch (variant) {
        case FeminineVariant::PINK:
            return getPink();
        case FeminineVariant::ROSE:
            return getRose();
        case FeminineVariant::LAVENDER:
            return getLavender();
        case FeminineVariant::PEACH:
            return getPeach();
        case FeminineVariant::CORAL:
            return getCoral();
        case FeminineVariant::MINT:
            return getMint();
        case FeminineVariant::LILAC:
            return getLilac();
        case FeminineVariant::BLUSH:
            return getBlush();
        case FeminineVariant::CREAM:
            return getCream();
        case FeminineVariant::SAGE:
            return getSage();
        default:
            return getPink();
    }
}

KeyboardSkin getPink() {
    KeyboardSkin skin;
    // Màu hồng cổ điển
    skin.keyBgColor = 0xF81F;        // Pink/Magenta (màu hồng)
    skin.keySelectedColor = 0xFE1F;  // Light pink (hồng nhạt)
    skin.keyTextColor = 0xFFFF;      // White (chữ trắng nổi bật)
    skin.keySelectedTextColor = 0xF81F; // Pink text on selected key
    skin.keyBorderColor = 0xF81F;    // Pink border
    skin.bgScreenColor = 0xE73F;     // Light purple background
    skin.keyWidth = 29;
    skin.keyHeight = 28;
    skin.spacing = 2;
    skin.textSize = 2;
    skin.hasBorder = true;
    skin.roundedCorners = true;
    skin.cornerRadius = 5;
    skin.hasPattern = true;
    return skin;
}

KeyboardSkin getRose() {
    KeyboardSkin skin;
    // Màu hoa hồng: hồng đậm, đỏ hồng
    skin.keyBgColor = 0xF800;        // Red-pink (đỏ hồng)
    skin.keySelectedColor = 0xFC18;  // Rose pink (hồng hoa hồng)
    skin.keyTextColor = 0xFFFF;      // White
    skin.keySelectedTextColor = 0xF800; // Red-pink text
    skin.keyBorderColor = 0xF800;    // Red-pink border
    skin.bgScreenColor = 0xF1A0;     // Light rose background
    skin.keyWidth = 29;
    skin.keyHeight = 28;
    skin.spacing = 2;
    skin.textSize = 2;
    skin.hasBorder = true;
    skin.roundedCorners = true;
    skin.cornerRadius = 5;
    skin.hasPattern = true;
    return skin;
}

KeyboardSkin getLavender() {
    KeyboardSkin skin;
    // Màu oải hương: tím nhạt, lavender
    skin.keyBgColor = 0x915C;        // Lavender purple (tím oải hương)
    skin.keySelectedColor = 0xB5B6;  // Light lavender (oải hương nhạt)
    skin.keyTextColor = 0xFFFF;      // White
    skin.keySelectedTextColor = 0x915C; // Lavender text
    skin.keyBorderColor = 0x915C;    // Lavender border
    skin.bgScreenColor = 0xC618;     // Very light purple background
    skin.keyWidth = 29;
    skin.keyHeight = 28;
    skin.spacing = 2;
    skin.textSize = 2;
    skin.hasBorder = true;
    skin.roundedCorners = true;
    skin.cornerRadius = 5;
    skin.hasPattern = true;
    return skin;
}

KeyboardSkin getPeach() {
    KeyboardSkin skin;
    // Màu đào: cam nhạt, peach
    skin.keyBgColor = 0xFD20;        // Peach (màu đào)
    skin.keySelectedColor = 0xFF45;  // Light peach (đào nhạt)
    skin.keyTextColor = 0xFFFF;      // White
    skin.keySelectedTextColor = 0xFD20; // Peach text
    skin.keyBorderColor = 0xFD20;    // Peach border
    skin.bgScreenColor = 0xFFB0;     // Very light peach background
    skin.keyWidth = 29;
    skin.keyHeight = 28;
    skin.spacing = 2;
    skin.textSize = 2;
    skin.hasBorder = true;
    skin.roundedCorners = true;
    skin.cornerRadius = 5;
    skin.hasPattern = true;
    return skin;
}

KeyboardSkin getCoral() {
    KeyboardSkin skin;
    // Màu san hô: cam hồng, coral
    skin.keyBgColor = 0xFA20;        // Coral (màu san hô)
    skin.keySelectedColor = 0xFC50;  // Light coral (san hô nhạt)
    skin.keyTextColor = 0xFFFF;      // White
    skin.keySelectedTextColor = 0xFA20; // Coral text
    skin.keyBorderColor = 0xFA20;    // Coral border
    skin.bgScreenColor = 0xFF90;     // Very light coral background
    skin.keyWidth = 29;
    skin.keyHeight = 28;
    skin.spacing = 2;
    skin.textSize = 2;
    skin.hasBorder = true;
    skin.roundedCorners = true;
    skin.cornerRadius = 5;
    skin.hasPattern = true;
    return skin;
}

KeyboardSkin getMint() {
    KeyboardSkin skin;
    // Màu bạc hà: xanh lá nhạt, mint
    skin.keyBgColor = 0x07F0;        // Mint green (xanh bạc hà)
    skin.keySelectedColor = 0x5FF8;  // Light mint (bạc hà nhạt)
    skin.keyTextColor = 0xFFFF;      // White
    skin.keySelectedTextColor = 0x07F0; // Mint text
    skin.keyBorderColor = 0x07F0;    // Mint border
    skin.bgScreenColor = 0x9FF8;     // Very light mint background
    skin.keyWidth = 29;
    skin.keyHeight = 28;
    skin.spacing = 2;
    skin.textSize = 2;
    skin.hasBorder = true;
    skin.roundedCorners = true;
    skin.cornerRadius = 5;
    skin.hasPattern = true;
    return skin;
}

KeyboardSkin getLilac() {
    KeyboardSkin skin;
    // Màu tím hoa cà: tím nhạt hơn lavender
    skin.keyBgColor = 0xA2B5;        // Lilac (tím hoa cà)
    skin.keySelectedColor = 0xC618;  // Light lilac (tím hoa cà nhạt)
    skin.keyTextColor = 0xFFFF;      // White
    skin.keySelectedTextColor = 0xA2B5; // Lilac text
    skin.keyBorderColor = 0xA2B5;    // Lilac border
    skin.bgScreenColor = 0xD69A;     // Very light lilac background
    skin.keyWidth = 29;
    skin.keyHeight = 28;
    skin.spacing = 2;
    skin.textSize = 2;
    skin.hasBorder = true;
    skin.roundedCorners = true;
    skin.cornerRadius = 5;
    skin.hasPattern = true;
    return skin;
}

KeyboardSkin getBlush() {
    KeyboardSkin skin;
    // Màu đỏ hồng nhạt: blush pink
    skin.keyBgColor = 0xF8B6;        // Blush pink (đỏ hồng nhạt)
    skin.keySelectedColor = 0xFCD6;  // Light blush (blush nhạt)
    skin.keyTextColor = 0xFFFF;      // White
    skin.keySelectedTextColor = 0xF8B6; // Blush text
    skin.keyBorderColor = 0xF8B6;    // Blush border
    skin.bgScreenColor = 0xFED6;     // Very light blush background
    skin.keyWidth = 29;
    skin.keyHeight = 28;
    skin.spacing = 2;
    skin.textSize = 2;
    skin.hasBorder = true;
    skin.roundedCorners = true;
    skin.cornerRadius = 5;
    skin.hasPattern = true;
    return skin;
}

KeyboardSkin getCream() {
    KeyboardSkin skin;
    // Màu kem: vàng nhạt, cream
    skin.keyBgColor = 0xFFD6;        // Cream (màu kem)
    skin.keySelectedColor = 0xFFF6;  // Light cream (kem nhạt)
    skin.keyTextColor = 0x8000;      // Dark red (chữ đỏ đậm để tương phản)
    skin.keySelectedTextColor = 0xF800; // Red text
    skin.keyBorderColor = 0xFFD6;    // Cream border
    skin.bgScreenColor = 0xFFF6;     // Very light cream background
    skin.keyWidth = 29;
    skin.keyHeight = 28;
    skin.spacing = 2;
    skin.textSize = 2;
    skin.hasBorder = true;
    skin.roundedCorners = true;
    skin.cornerRadius = 5;
    skin.hasPattern = true;
    return skin;
}

KeyboardSkin getSage() {
    KeyboardSkin skin;
    // Màu xanh lá cây nhạt: sage green
    skin.keyBgColor = 0x67A0;        // Sage green (xanh lá cây nhạt)
    skin.keySelectedColor = 0x8F60;  // Light sage (sage nhạt)
    skin.keyTextColor = 0xFFFF;      // White
    skin.keySelectedTextColor = 0x67A0; // Sage text
    skin.keyBorderColor = 0x67A0;    // Sage border
    skin.bgScreenColor = 0xB7C0;     // Very light sage background
    skin.keyWidth = 29;
    skin.keyHeight = 28;
    skin.spacing = 2;
    skin.textSize = 2;
    skin.hasBorder = true;
    skin.roundedCorners = true;
    skin.cornerRadius = 5;
    skin.hasPattern = true;
    return skin;
}

} // namespace FeminineSkins

