#pragma once
#include <array>
#include "WeaponAssets.h"
// Physical locations, independent of equipment slots and player update fields.
static constexpr std::array<unsigned,7> weaponPoints{{32,33,30,31,28,27,26}};
inline int rangedAnimationFamily(unsigned subclass){
    return subclass==2?0:(subclass==3||subclass==18)?1:(subclass==16||subclass==19)?2:-1;
}
// 5875 AnimationData IDs: ready, attack, load and hold, respectively.
// Convert only the equipped family's ranged phases, never spell casts, melee,
// movement or arbitrary emotes. A bow release has no separate rifle phase.
inline unsigned rangedAppearanceAnimation(unsigned animation,unsigned equipped,unsigned selected){
    const int from=rangedAnimationFamily(equipped),to=rangedAnimationFamily(selected);
    if(from<0||to<0||from==to)return animation;
    static constexpr unsigned phases[3][4]={{29,46,105,109},{48,49,106,110},{108,107,112,111}};
    for(unsigned i=0;i<4;++i)if(animation==phases[from][i])return phases[to][i];
    if(from==0&&animation==47)return phases[to][1];
    return animation;
}
inline bool acceptsWeapon(unsigned position,const WeaponAsset* asset){
    if(!asset||position>=7)return false;
    if(position<2)return asset->kind==1;
    if(position<4)return asset->kind==1||asset->kind==2;
    return asset->kind==position-1;
}
struct WeaponSelection {
    std::array<unsigned,7> items{};
    std::array<unsigned,3> equipped{};
    bool empty()const{for(auto id:items)if(id)return false;return true;}
    bool operator==(const WeaponSelection& b)const{return items==b.items&&equipped==b.equipped;}
    bool valid()const{for(unsigned i=0;i<7;++i)if(items[i]&&!acceptsWeapon(i,weaponAsset(items[i])))return false;return true;}
    std::array<int,3> routes()const{
        std::array<int,3> routes{{-1,-1,-1}};std::array<bool,7> used{};
        // Right waist prefers main hand; left waist prefers off hand. Exact type
        // takes priority for melee. The ranged appearance replaces any equipped
        // ranged family; an empty real slot still cannot acquire a drawn weapon.
        const unsigned order[3][7]={{1,0,2,3,4,5,6},{0,1,3,2,4,5,6},{5,0,1,2,3,4,6}};
        for(unsigned role=0;role<3;++role){
            const auto* real=weaponAsset(equipped[role]);if(!real)continue;
            for(unsigned pass=0;pass<2&&routes[role]<0;++pass)for(auto i:order[role]){
                const auto* a=weaponAsset(items[i]);
                if(!used[i]&&a&&a->kind==real->kind&&(pass||a->subclass==real->subclass)){
                    routes[role]=static_cast<int>(i);used[i]=true;break;
                }
            }
        }
        return routes;
    }
};
