#pragma once
// Included after CastingRenderer. Replace only mount-spell sound IDs in private
// process-lifetime kits. Native effects retain kit pointers after dispatch.
#include "MountSoundCatalog.h"
static std::uint64_t mountSoundOwner=0;
static unsigned mountSoundDisplay=0;
static std::map<weaponCasting::Kit,weaponCasting::Kit> mountSoundKits;
static bool mountedSpell(std::uintptr_t spell){
    for(unsigned i=0;i<3;++i){unsigned effect=0,aura=0;
        if(read(spell+4*(61+i),effect)&&effect==6&&read(spell+4*(91+i),aura)&&aura==78)return true;
    }
    return false;
}
static unsigned mountPrimaryKit(const weaponCasting::Visual& visual){
    for(unsigned i:{2u,3u,4u,5u})if(visual[i])return i;
    return 0;
}
static const unsigned* mountSoundKit(void* unit,const unsigned* spell,const unsigned* kit,unsigned phase){
    if(!mountSoundDisplay||mountSoundOwner!=getPlayer())return kit;
    Player p;unsigned id=0;const auto source=reinterpret_cast<std::uintptr_t>(spell);
    const unsigned selected=mountSoundSpell(mountSoundDisplay);
    if(!selected||!spell||!kit||!snapshot(p)||p.guid!=mountSoundOwner||p.unit!=reinterpret_cast<std::uintptr_t>(unit)||
       p.display!=p.native||!read(source,id)||projectileDBRow(0xC0D788,id)!=source||!mountedSpell(source))return kit;
    weaponCasting::Kit original{};weaponCasting::Visual from{},to{};
    const auto target=projectileDBRow(0xC0D788,selected);
    if(!target||!mountedSpell(target)||!read(reinterpret_cast<std::uintptr_t>(kit),original)||!original[0]||
       projectileDBRow(0xC0D74C,original[0])!=reinterpret_cast<std::uintptr_t>(kit)||
       unitSpellVisualOriginal(unit,spell,from.data())!=from.data()||
       unitSpellVisualOriginal(unit,reinterpret_cast<const unsigned*>(target),to.data())!=to.data())return kit;
    unsigned role=0;
    if(phase==4&&from[1]==original[0])role=1;
    else if(phase==0&&from[2]==original[0])role=2;
    else if(phase==1&&from[3]==original[0])role=3;
    else if(phase==2){for(unsigned i:{4u,5u})if(from[i]==original[0]){role=i;break;}}
    if(!role)return kit;
    // Some event mounts put their summon sound in the aura kit, not the cast
    // kit. Route the source's primary arrival event to the selected primary.
    // Suppress obsolete secondary sounds instead of playing the old mount.
    unsigned targetRole=role;
    const unsigned primary=mountPrimaryKit(from),selectedPrimary=mountPrimaryKit(to);
    if(role==primary)targetRole=selectedPrimary;
    else if(role!=1&&role==selectedPrimary&&primary!=selectedPrimary)targetRole=0;
    unsigned sound=~0u;
    if(targetRole&&to[targetRole]){
        const auto row=projectileDBRow(0xC0D74C,to[targetRole]);
        if(!row||!read(row+13*4,sound))return kit;
    }
    if(sound==original[13])return kit;
    original[13]=sound;
    const auto stored=mountSoundKits.emplace(original,original);
    return stored.first->second.data();
}
static unsigned __fastcall mountSoundKitHook(void* unit,void*,const unsigned* spell,const unsigned* kit,unsigned phase,unsigned arg4){
    const auto* audible=mountSoundKit(unit,spell,kit,phase);
    if(audible!=kit)return castingKitOriginal(unit,spell,audible,phase,arg4);
    return castingKitHook(unit,nullptr,spell,kit,phase,arg4);
}
static int __fastcall setMountSoundLua(void* L){
    if(!isNumber(L,1))return result(L,-2);
    const double value=toNumber(L,1);
    if(!std::isfinite(value)||value<0||value>2147483647.||value!=std::floor(value))return result(L,-2);
    const auto display=static_cast<unsigned>(value);
    if(display&&!mountSoundSpell(display))return result(L,-2);
    const auto owner=getPlayer();if(display&&!owner)return result(L,-1);
    mountSoundDisplay=display;mountSoundOwner=display?owner:0;return result(L,1);
}
