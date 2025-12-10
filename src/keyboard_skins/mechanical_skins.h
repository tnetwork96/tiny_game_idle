#ifndef MECHANICAL_SKINS_H
#define MECHANICAL_SKINS_H

#include "../keyboard_skins.h"

// Enum cho các variants của Mechanical skin
enum class MechanicalVariant {
    STEAMPUNK,  // Steampunk (đồng, vàng)
    RUSTY,      // Gỉ sắt
    BRONZE      // Đồng thau
};

namespace MechanicalSkins {
    KeyboardSkin getMechanicalSkin(MechanicalVariant variant = MechanicalVariant::STEAMPUNK);
    
    // Các variants cụ thể
    KeyboardSkin getSteampunk();
    KeyboardSkin getRusty();
    KeyboardSkin getBronze();
}

#endif

