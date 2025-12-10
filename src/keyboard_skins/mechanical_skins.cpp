#include "mechanical_skins.h"

namespace MechanicalSkins {

KeyboardSkin getMechanicalSkin(MechanicalVariant variant) {
    switch (variant) {
        case MechanicalVariant::STEAMPUNK:
            return getSteampunk();
        case MechanicalVariant::RUSTY:
            return getRusty();
        case MechanicalVariant::BRONZE:
            return getBronze();
        default:
            return getSteampunk();
    }
}

KeyboardSkin getSteampunk() {
    KeyboardSkin skin;
    // Màu sắc steampunk: đỏ gỉ, cam, vàng đồng, bronze
    skin.keyBgColor = 0x8000;        // Dark red/rusty (đỏ gỉ sắt)
    skin.keySelectedColor = 0xFC00;  // Golden orange/copper (vàng cam đồng)
    skin.keyTextColor = 0xFFE0;      // Yellow (vàng sáng cho chữ)
    skin.keySelectedTextColor = 0x0000; // Black text on selected key
    skin.keyBorderColor = 0xFD20;    // Copper/Bronze border (viền đồng)
    skin.bgScreenColor = 0x4000;     // Dark red-brown background
    skin.keyWidth = 22;
    skin.keyHeight = 22;
    skin.spacing = 2;
    skin.textSize = 1;
    skin.hasBorder = true;           // Có viền đồng
    skin.roundedCorners = false;     // Góc vuông (cơ khí)
    skin.cornerRadius = 0;
    skin.hasPattern = true;          // Có pattern (bánh răng)
    return skin;
}

KeyboardSkin getRusty() {
    KeyboardSkin skin;
    // Màu gỉ sắt: đỏ nâu, cam gỉ
    skin.keyBgColor = 0x6000;        // Dark rusty red (đỏ gỉ đậm)
    skin.keySelectedColor = 0xFA00;  // Rust orange (cam gỉ)
    skin.keyTextColor = 0xFFE0;      // Yellow
    skin.keySelectedTextColor = 0x0000; // Black text
    skin.keyBorderColor = 0xFA00;    // Rust orange border
    skin.bgScreenColor = 0x3000;     // Very dark red-brown
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

KeyboardSkin getBronze() {
    KeyboardSkin skin;
    // Màu đồng thau: vàng đồng, bronze
    skin.keyBgColor = 0x8200;        // Dark bronze (đồng thau đậm)
    skin.keySelectedColor = 0xFE20;  // Bright bronze (đồng thau sáng)
    skin.keyTextColor = 0xFFE0;      // Yellow
    skin.keySelectedTextColor = 0x0000; // Black text
    skin.keyBorderColor = 0xFE20;    // Bright bronze border
    skin.bgScreenColor = 0x4100;     // Dark bronze-brown
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

} // namespace MechanicalSkins

