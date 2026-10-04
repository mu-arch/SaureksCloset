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
    float amplitude=1;
    bool operator==(const BagTuningValues& other) const {
        return left==other.left&&inset==other.inset&&up==other.up&&pitch==other.pitch&&
            roll==other.roll&&yaw==other.yaw&&scale==other.scale&&motion==other.motion&&amplitude==other.amplitude;
    }
};
struct BagTuningEntry { bool enabled=false; unsigned revision=0; BagTuningValues values; };
inline std::array<BagTuningEntry,16> bagTuningEntries{};
inline std::array<BagTuningEntry,16> hatTuningEntries{};
inline std::array<std::array<BagTuningEntry,16>,10> weaponTuningEntries{};
inline std::uint64_t bagTuningOwner=0;
inline bool bagTuningKey(unsigned bag,unsigned race,unsigned sex) {
    return (bag==1||(bag>=101&&bag<=111)||(bag>=201&&bag<=208))&&race>=1&&race<=8&&sex<=1;
}
inline bool bagInstanceTuningDefaults(unsigned mount,unsigned race,unsigned sex,BagTuningValues& out) {
    if(mount>2||race<1||race>8||sex>1)return false;
    const auto contact=bagMount(1,race,sex);
    out={};
    if(mount==0){
        out.inset=contact.backInward;out.up=-.03f+contact.backUp;
        out.pitch=contact.inwardDegrees;out.scale=85;
    }else{
        out.up=-.15f;out.yaw=mount==1?-90.f:90.f;out.scale=70;
    }
    return true;
}
inline bool bagTuningDefaults(unsigned bag,unsigned race,unsigned sex,BagTuningValues& out) {
    if(!bagTuningKey(bag,race,sex))return false;
    if(bag>=201)return bagInstanceTuningDefaults(0,race,sex,out);
    if(bag!=1){out={};out.scale=100;return true;}
    const auto mount=bagMount(bag,race,sex);
    out={bagModelScale*.28f,mount.backInward,bagModelScale*.20f-.15f+mount.backUp,
        mount.inwardDegrees,mount.rightDegrees,0,85,true};
    return true;
}
inline bool bagTuningValid(const BagTuningValues& values,bool bag=false) {
    for(float offset:{values.left,values.inset})
        if(!std::isfinite(offset)||offset< -1||offset>1)return false;
    if(!std::isfinite(values.up)||values.up<(bag?-3.f:-1.f)||values.up>1)return false;
    for(float angle:{values.pitch,values.roll,values.yaw})
        if(!std::isfinite(angle)||angle< -180||angle>180)return false;
    return std::isfinite(values.amplitude)&&values.amplitude>=0&&values.amplitude<=2&&std::isfinite(values.scale)&&values.scale>=25&&values.scale<=200;
}
inline bool bagTuningSet(unsigned bag,unsigned race,unsigned sex,bool enabled,const BagTuningValues& values={}) {
    if(!bagTuningKey(bag,race,sex)||bag>=201||(enabled&&!bagTuningValid(values,bag==1)))return false;
    auto& entry=bag==1?bagTuningEntries[(race-1)*2+sex]:bag==111?hatTuningEntries[(race-1)*2+sex]:weaponTuningEntries[bag-101][(race-1)*2+sex];
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
    for(auto& slot:weaponTuningEntries)for(auto& entry:slot){
        if(entry.enabled){entry.enabled=false;++entry.revision;}
    }
    bagTuningOwner=guid;
    for(auto& entry:hatTuningEntries)if(entry.enabled){entry.enabled=false;++entry.revision;}
}
