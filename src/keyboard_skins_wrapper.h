#ifndef KEYBOARD_SKINS_WRAPPER_H
#define KEYBOARD_SKINS_WRAPPER_H

#include "keyboard_skins.h"
#include "keyboard_skins/feminine_skins.h"
#include "keyboard_skins/cyberpunk_skins.h"
#include "keyboard_skins/mechanical_skins.h"
#include "keyboard_skins/default_skins.h"

// Namespace chứa tất cả các skin - wrapper để dễ sử dụng
namespace KeyboardSkins {
    // Default skins - sử dụng DefaultSkins namespace
    inline KeyboardSkin getDefaultSkin(DefaultVariant variant = DefaultVariant::SYNTHWAVE) {
        return DefaultSkins::getDefaultSkin(variant);
    }
    inline KeyboardSkin getSynthwaveSkin() { return DefaultSkins::getSynthwave(); }
    inline KeyboardSkin getNeonSkin() { return DefaultSkins::getNeon(); }
    inline KeyboardSkin getDarkSkin() { return DefaultSkins::getDark(); }
    inline KeyboardSkin getRetroSkin() { return DefaultSkins::getRetro(); }
    inline KeyboardSkin getMinimalSkin() { return DefaultSkins::getMinimal(); }
    
    // Feminine skins - sử dụng FeminineSkins namespace
    inline KeyboardSkin getFeminineSkin(FeminineVariant variant = FeminineVariant::PINK) {
        return FeminineSkins::getFeminineSkin(variant);
    }
    inline KeyboardSkin getFemininePink() { return FeminineSkins::getPink(); }
    inline KeyboardSkin getFeminineRose() { return FeminineSkins::getRose(); }
    inline KeyboardSkin getFeminineLavender() { return FeminineSkins::getLavender(); }
    inline KeyboardSkin getFemininePeach() { return FeminineSkins::getPeach(); }
    inline KeyboardSkin getFeminineCoral() { return FeminineSkins::getCoral(); }
    inline KeyboardSkin getFeminineMint() { return FeminineSkins::getMint(); }
    inline KeyboardSkin getFeminineLilac() { return FeminineSkins::getLilac(); }
    inline KeyboardSkin getFeminineBlush() { return FeminineSkins::getBlush(); }
    inline KeyboardSkin getFeminineCream() { return FeminineSkins::getCream(); }
    inline KeyboardSkin getFeminineSage() { return FeminineSkins::getSage(); }
    
    // Cyberpunk skins - sử dụng CyberpunkSkins namespace
    inline KeyboardSkin getCyberpunkSkin(CyberpunkVariant variant = CyberpunkVariant::TRON) {
        return CyberpunkSkins::getCyberpunkSkin(variant);
    }
    inline KeyboardSkin getCyberpunkTron() { return CyberpunkSkins::getTron(); }
    inline KeyboardSkin getCyberpunkMatrix() { return CyberpunkSkins::getMatrix(); }
    inline KeyboardSkin getCyberpunkNeonPurple() { return CyberpunkSkins::getNeonPurple(); }
    
    // Mechanical skins - sử dụng MechanicalSkins namespace
    inline KeyboardSkin getMechanicalSkin(MechanicalVariant variant = MechanicalVariant::STEAMPUNK) {
        return MechanicalSkins::getMechanicalSkin(variant);
    }
    inline KeyboardSkin getMechanicalSteampunk() { return MechanicalSkins::getSteampunk(); }
    inline KeyboardSkin getMechanicalRusty() { return MechanicalSkins::getRusty(); }
    inline KeyboardSkin getMechanicalBronze() { return MechanicalSkins::getBronze(); }
}

#endif

