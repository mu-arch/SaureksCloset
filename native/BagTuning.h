#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include "BagFits.h"
#include "BagMounts.h"
// Session overrides never rewrite generated contact fits or player fields.
// Lua persists the user's drafts separately and restores them after login.
struct BagTuningValues {
    float left=0,inset=0,up=0,pitch=0,roll=0,yaw=0,scale=85;
    bool motion=true;
    bool operator==(const BagTuningValues& other) const {
        return left==other.left&&inset==other.inset&&up==other.up&&pitch==other.pitch&&
            roll==other.roll&&yaw==other.yaw&&scale==other.scale&&motion==other.motion;
    }
};
struct BagTuningEntry { bool enabled=false; unsigned revision=0; BagTuningValues values; };
inline std::array<BagTuningEntry,16> bagTuningEntries{};
inline std::uint64_t bagTuningOwner=0;
inline bool bagTuningKey(unsigned bag,unsigned race,unsigned sex) {
    return bag==1&&race>=1&&race<=8&&sex<=1;
}
inline bool bagTuningDefaults(unsigned bag,unsigned race,unsigned sex,BagTuningValues& out) {
    if(!bagTuningKey(bag,race,sex))return false;
    const auto mount=bagMount(bag,race,sex);
    out={bagModelScale*.28f,mount.backInward,bagModelScale*.20f-.15f+mount.backUp,
        mount.inwardDegrees,mount.rightDegrees,0,85,true};
    return true;
}
inline bool bagTuningValid(const BagTuningValues& values) {
    for(float offset:{values.left,values.inset,values.up})
        if(!std::isfinite(offset)||offset< -1||offset>1)return false;
    for(float angle:{values.pitch,values.roll,values.yaw})
        if(!std::isfinite(angle)||angle< -180||angle>180)return false;
    return std::isfinite(values.scale)&&values.scale>=25&&values.scale<=200;
}
inline bool bagTuningSet(unsigned bag,unsigned race,unsigned sex,bool enabled,const BagTuningValues& values={}) {
    if(!bagTuningKey(bag,race,sex)||(enabled&&!bagTuningValid(values)))return false;
    auto& entry=bagTuningEntries[(race-1)*2+sex];
    if(entry.enabled==enabled&&(!enabled||entry.values==values))return true;
    entry.enabled=enabled;
    if(enabled)entry.values=values;
    ++entry.revision;
    return true;
}
inline void bagTuningUseOwner(std::uint64_t guid) {
    if(bagTuningOwner==guid)return;
    for(auto& entry:bagTuningEntries){
        if(entry.enabled){entry.enabled=false;++entry.revision;}
    }
    bagTuningOwner=guid;
}
