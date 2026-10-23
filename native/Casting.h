#pragma once
#include <array>
#include <cstdint>
#include <cmath>
namespace weaponCasting {
using Kit=std::array<unsigned,35>;
struct Settings {
    bool enabled=false,effects=true,swing=true;
    unsigned item=0;
    bool operator==(const Settings& b)const{return enabled==b.enabled&&effects==b.effects&&swing==b.swing&&item==b.item;}
};
inline bool ready(unsigned a){return a==31||a==51||a==52;}
inline bool release(unsigned a){return a==32||a==33||a==53||a==54;}
inline bool channel(unsigned a){return a==124||a==125;}
enum class Role:unsigned {None,Precast,Release,Channel};
using Visual=std::array<unsigned,16>;
inline Role casterKit(const Kit& k,unsigned phase,const Visual& visual){
    // Dispatch role, not an animation allowlist or the kit's metadata type.
    if(!k[0])return Role::None;
    if(phase==4&&k[0]==visual[1])return Role::Precast;
    if(phase==0&&k[0]==visual[2])return visual[5]?Role::Channel:Role::Release;
    if(phase==2&&k[0]==visual[5])return Role::Channel;
    return Role::None;
}
inline bool effect(unsigned id){return id&&id!=0xffffffffu;}
inline unsigned animation(unsigned original,unsigned kind,unsigned subclass){
    const bool longWeapon=subclass==10||subclass==6;
    return release(original)?(kind==1?17:longWeapon?19:18):(kind==1?26:longWeapon?28:27);
}
inline Kit prepare(const Kit& original,const Settings& settings,unsigned kind,unsigned subclass,Role role){
    Kit out=original;
    if(settings.swing)out[2]=animation(role==Role::Release?53:51,kind,subclass);
    if(settings.effects){
        // Merge duplicate left/right charging art into one weapon effect.
        out[7]=effect(original[7])?original[7]:original[6];out[6]=0xffffffffu;
    }
    return out;
}
constexpr unsigned marker=0x7f00;
inline bool synthetic(unsigned point){return point==marker+21||point==marker+22;}
inline unsigned fallback(unsigned point){return synthetic(point)?point-marker:point;}
inline bool forwardTransform(const std::array<float,16>& player,const float* attachment,std::array<float,16>& out){
    if(!attachment)return false;
    for(unsigned i=0;i<16;++i)if(!std::isfinite(player[i])||!std::isfinite(attachment[i]))return false;
    if(std::fabs(player[15]-1.f)>.001f||std::fabs(attachment[15]-1.f)>.001f)return false;
    for(unsigned column:{0u,4u,8u}){
        float length=0;for(unsigned axis=0;axis<3;++axis)length+=player[column+axis]*player[column+axis];
        if(length<.000001f)return false;
    }
    // ConeofCold_Hand's authored emitter faces +X. Keep the weapon-tip
    // position, but use the player's facing/scale instead of the swing bones.
    out=player;
    for(unsigned axis=0;axis<3;++axis)out[12+axis]=attachment[12+axis];
    return true;
}
}
