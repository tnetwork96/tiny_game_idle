#ifndef DEFAULT_SKINS_H
#define DEFAULT_SKINS_H

#include "../keyboard_skins.h"

// Enum cho các variants của Default skin
enum class DefaultVariant {
    SYNTHWAVE,  // Synthwave/vaporwave
    NEON,       // Neon
    DARK,       // Dark
    RETRO,      // Retro
    MINIMAL     // Minimal
};

namespace DefaultSkins {
    KeyboardSkin getDefaultSkin(DefaultVariant variant = DefaultVariant::SYNTHWAVE);
    
    // Các variants cụ thể
    KeyboardSkin getSynthwave();
    KeyboardSkin getNeon();
    KeyboardSkin getDark();
    KeyboardSkin getRetro();
    KeyboardSkin getMinimal();
}

#endif

