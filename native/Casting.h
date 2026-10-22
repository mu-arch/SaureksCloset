#pragma once
#include <array>
#include <cstdint>
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
inline bool casterKit(const Kit& k,unsigned phase){return k[1]==0&&(phase==0||phase==4)&&(ready(k[2])||release(k[2])||channel(k[2]));}
inline bool effect(unsigned id){return id&&id!=0xffffffffu;}
inline unsigned animation(unsigned original,unsigned kind,unsigned subclass){
    const bool longWeapon=subclass==10||subclass==6;
    return release(original)?(kind==1?17:longWeapon?19:18):(kind==1?26:longWeapon?28:27);
}
inline Kit prepare(const Kit& original,const Settings& settings,unsigned kind,unsigned subclass){
    Kit out=original;
    if(settings.swing)out[2]=animation(original[2],kind,subclass);
    if(settings.effects){
        // Merge duplicate left/right charging art into one weapon effect.
        out[7]=effect(original[7])?original[7]:original[6];out[6]=0xffffffffu;
    }
    return out;
}
constexpr unsigned marker=0x7f00;
inline bool synthetic(unsigned point){return point==marker+21||point==marker+22;}
inline unsigned fallback(unsigned point){return synthetic(point)?point-marker:point;}
}
