#pragma once
#include <array>
#include "WeaponAssets.h"
// Physical locations, independent of equipment slots and player update fields.
static constexpr std::array<unsigned,7> weaponPoints{{32,33,30,31,28,27,26}};
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
        // takes priority over this preference. Never draw a purely cosmetic gun.
        const unsigned order[3][7]={{1,0,2,3,4,5,6},{0,1,3,2,4,5,6},{5,0,1,2,3,4,6}};
        for(unsigned role=0;role<3;++role){
            const auto* real=weaponAsset(equipped[role]);if(!real)continue;
            for(unsigned pass=0;pass<2&&routes[role]<0;++pass)for(auto i:order[role]){
                const auto* a=weaponAsset(items[i]);
                if(!used[i]&&a&&a->kind==real->kind&&(pass||a->subclass==real->subclass)){
                    // Bows use a different hand/callback from guns. Keep ranged
                    // families exact; never give a gun the bow animation callback.
                    if(role==2&&a->subclass!=real->subclass)continue;
                    routes[role]=static_cast<int>(i);used[i]=true;break;
                }
            }
        }
        return routes;
    }
};
