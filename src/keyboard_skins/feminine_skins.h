#ifndef FEMININE_SKINS_H
#define FEMININE_SKINS_H

#include "../keyboard_skins.h"

// Enum cho các variants của Feminine skin
enum class FeminineVariant {
    PINK,       // Hồng
    ROSE,       // Hoa hồng
    LAVENDER,   // Oải hương
    PEACH,      // Đào
    CORAL,      // San hô
    MINT,       // Bạc hà
    LILAC,      // Tím hoa cà
    BLUSH,      // Đỏ hồng nhạt
    CREAM,      // Kem
    SAGE        // Xanh lá cây nhạt
};

namespace FeminineSkins {
    KeyboardSkin getFeminineSkin(FeminineVariant variant = FeminineVariant::PINK);
    
    // Các variants cụ thể
    KeyboardSkin getPink();
    KeyboardSkin getRose();
    KeyboardSkin getLavender();
    KeyboardSkin getPeach();
    KeyboardSkin getCoral();
    KeyboardSkin getMint();
    KeyboardSkin getLilac();
    KeyboardSkin getBlush();
    KeyboardSkin getCream();
    KeyboardSkin getSage();
}

#endif

