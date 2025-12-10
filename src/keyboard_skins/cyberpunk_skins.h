#ifndef CYBERPUNK_SKINS_H
#define CYBERPUNK_SKINS_H

#include "../keyboard_skins.h"

// Enum cho các variants của Cyberpunk skin
enum class CyberpunkVariant {
    TRON,        // Tron style (cyan)
    MATRIX,      // Matrix style (green)
    NEON_PURPLE  // Neon purple
};

namespace CyberpunkSkins {
    KeyboardSkin getCyberpunkSkin(CyberpunkVariant variant = CyberpunkVariant::TRON);
    
    // Các variants cụ thể
    KeyboardSkin getTron();
    KeyboardSkin getMatrix();
    KeyboardSkin getNeonPurple();
}

#endif

