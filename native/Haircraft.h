#pragma once
#include <cstdint>

namespace haircraft {
inline std::uint64_t owner=0;
inline bool enabled=false;
inline bool owns(std::uint64_t configuredOwner,std::uint64_t player,bool active,
        std::uintptr_t component,std::uintptr_t model,std::uintptr_t playerComponent,
        std::uintptr_t playerModel,bool ordinaryBody,std::uint64_t previewOwner){
    if(!active||!player||configuredOwner!=player||!component||!model)return false;
    return (component==playerComponent&&model==playerModel&&ordinaryBody)||previewOwner==player;
}

// 4799A0 attaches the helmet, then applies HelmetGeosetVisData's race mask to
// the component's hair group (144), facial groups and ears. Restore only hair,
// resolving it from the current race/sex/style descriptor, never from the
// already-hidden group. Bald styles correctly resolve to the native group 1.
template<class Compose,class Resolve,class Store>
void compose(void* component,bool keepHair,Compose original,Resolve resolve,Store store){
    original(component);
    if(keepHair){
        const auto descriptor=reinterpret_cast<std::uintptr_t>(component)+0x18;
        const unsigned group=resolve(reinterpret_cast<void*>(descriptor));
        store(reinterpret_cast<std::uintptr_t>(component)+0x144,group);
    }
}
}
