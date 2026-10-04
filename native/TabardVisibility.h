#pragma once
#include <cstdint>

// Build 5875's guild/tabard refreshes (5E0830/5E08E0) read the actual inventory
// item and call 5E0720, bypassing the visible-item getter used for transmogs.
// Resolve the CURRENT visible record at that final display boundary. Zero is
// explicitly hidden, including when a stale refresh arrives after a selection.
template<class Lookup,class Read,class Resolve>
unsigned tabardDisplayForUpdate(void* unit,unsigned incoming,std::uintptr_t localUnit,
        Lookup lookup,Read read,Resolve resolve){
    if(!localUnit||reinterpret_cast<std::uintptr_t>(unit)!=localUnit)return incoming;
    auto* record=lookup(unit,18);
    unsigned item=0;
    if(!record||!read(reinterpret_cast<std::uintptr_t>(record)+8,item))return incoming;
    if(!item)return 0;
    const int display=resolve(record);
    return display>0?static_cast<unsigned>(display):0;
}

// A guild-emblem response can arrive after its tabard has been hidden/replaced.
// Only a still-visible guild tabard may accept those textures. No component or
// player fields are written; previews and other players retain native behavior.
template<class Lookup,class Read,class Resolve>
bool allowGuildTabardTextures(void* component,std::uintptr_t localComponent,
        std::uintptr_t localUnit,Lookup lookup,Read read,Resolve resolve){
    if(!localUnit||!localComponent||reinterpret_cast<std::uintptr_t>(component)!=localComponent)return true;
    constexpr unsigned unknown=~0u;
    const auto display=tabardDisplayForUpdate(reinterpret_cast<void*>(localUnit),unknown,
        localUnit,lookup,read,resolve);
    if(display==unknown)return true;
    if(!display)return false;
    std::uint32_t row=0;unsigned current=0,flags=0;
    if(!read(localComponent+0x4D0,row))return true;
    if(!row)return false;
    if(!read(row,current)||!read(row+0x24,flags))return true;
    return current==display&&(flags&1)!=0;
}
