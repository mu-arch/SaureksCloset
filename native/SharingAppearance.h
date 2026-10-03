#pragma once
#include <map>
#include "SharingProtocol.h"
struct SharedAppearance{
    sharing::Remote snapshot;
    std::uintptr_t unit=0,model=0,component=0;
    std::array<std::array<std::uint32_t,12>,19> visibleItems{};
};
static std::map<std::uint64_t,SharedAppearance> sharedAppearances;
static std::map<std::uintptr_t,std::uint64_t> sharedModelOwners;
static void sharedBind(std::uint64_t guid,SharedAppearance& entry,const Player& p){
    if(entry.model&&entry.model!=p.model)sharedModelOwners.erase(entry.model);
    entry.unit=p.unit;entry.model=p.model;entry.component=p.component;
    if(p.model)sharedModelOwners[p.model]=guid;
}
static bool sharedPlayer(std::uint64_t guid,Player& p){
    if(!guid||guid==getPlayer())return false;
    p={};p.guid=guid;p.unit=reinterpret_cast<std::uintptr_t>(objectPtr(0x10,nullptr,guid,0));
    unsigned type=0;std::uint64_t live=0;
    return p.unit&&read(p.unit+0x14,type)&&type==4&&read(p.unit+8,p.fields)&&
        read(p.fields,live)&&live==guid&&read(p.fields+0x83*4,p.display)&&read(p.fields+0x84*4,p.native)&&
        read(p.fields+0x24*4,p.identity)&&read(p.fields+0xC1*4,p.body)&&read(p.fields+0xC2*4,p.facial)&&
        read(p.unit+0xD8,p.model)&&read(p.unit+0xD30,p.component);
}
static SharedAppearance* sharedForUnit(std::uintptr_t unit){
    if(sharedAppearances.empty()||!unit)return nullptr;
    std::uintptr_t fields=0;std::uint64_t guid=0;
    if(!read(unit+8,fields)||!read(fields,guid))return nullptr;
    auto i=sharedAppearances.find(guid);if(i==sharedAppearances.end())return nullptr;
    Player p;if(!sharedPlayer(guid,p)||p.unit!=unit||p.display!=p.native||!nativeModel(p.native))return nullptr;
    return &i->second;
}
static bool sharedWorldIdentity(std::uint64_t guid,std::uintptr_t unit,std::uintptr_t model){
    auto i=sharedAppearances.find(guid);if(i==sharedAppearances.end()||i->second.unit!=unit||i->second.model!=model)return false;
    std::uintptr_t fields=0,liveModel=0;std::uint64_t liveGuid=0;unsigned display=0,native=0;
    return read(unit+8,fields)&&read(fields,liveGuid)&&liveGuid==guid&&read(unit+0xd8,liveModel)&&liveModel==model&&
        read(fields+0x83*4,display)&&read(fields+0x84*4,native)&&display==native&&nativeModel(native);
}
static Appearance sharedBody(const sharing::Look& look){const auto& b=look.body;return {b[1],b[2],b[3],b[4],b[5],b[6],b[7]};}
static const char* sharedName(std::uintptr_t unit){auto* entry=sharedForUnit(unit);if(entry&&entry->snapshot.look.body[0])return sharedBody(entry->snapshot.look).model()->filename;return nullptr;}
static bool sharedCompose(std::uintptr_t component,std::array<std::uint32_t,91>& copy){
    for(auto& pair:sharedAppearances){Player p;if(!sharedPlayer(pair.first,p)||p.component!=component||p.model!=copy[8]||p.display!=p.native||!nativeModel(p.native))continue;
        if(pair.second.snapshot.look.body[0])sharedBody(pair.second.snapshot.look).compose(copy);
        sharedBind(pair.first,pair.second,p);return true;
    }return false;
}
static float sharedScale(std::uintptr_t model){
    auto owner=sharedModelOwners.find(model);if(owner==sharedModelOwners.end())return 1;
    auto i=sharedAppearances.find(owner->second);if(i==sharedAppearances.end()||!i->second.snapshot.look.body[0])return 1;
    Player p;if(sharedPlayer(i->first,p)&&p.model==model&&p.display==p.native)
        if(const auto* native=nativeModel(p.native))return sharedBody(i->second.snapshot.look).model()->scale/native->scale;
    return 1;
}
// Exact visual-only callers: whole component rebuild, per-slot visual update,
// and helmet/cape visibility. Tooltip, inventory and gameplay reads stay stock.
static bool sharedArmorCaller(std::uintptr_t caller){return caller==0x5fb551||caller==0x5ed8b8||caller==0x5ee673||caller==0x5ee802;}
static void* sharedVisibleItem(void* unit,int slot,std::uintptr_t caller,void* original){
    if(slot<0||slot>=19||!sharedArmorCaller(caller))return original;
    auto* entry=sharedForUnit(reinterpret_cast<std::uintptr_t>(unit));if(!entry)return original;
    const auto id=entry->snapshot.look.items[slot];if(id==sharing::inherit)return original;
    auto& copy=entry->visibleItems[slot];copy={};copy[2]=id;return copy.data();
}
