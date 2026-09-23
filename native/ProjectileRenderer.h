#pragma once
// Included after WeaponRenderer.h. Select the local player's weapon visuals
// before precast/fire sounds and missile dispatch, then provide cosmetic ammo
// to the missile constructor. Spell, targets, speed and equipment stay native.
using UnitSpellVisual=unsigned* (__thiscall *)(void*,const unsigned*,unsigned*);
static UnitSpellVisual unitSpellVisualOriginal=nullptr;
using UnitMissile=void (__thiscall *)(void*,const void*,const void*,unsigned,unsigned,
    unsigned,unsigned,unsigned,const unsigned*,unsigned,const void*,const void*);
static UnitMissile unitMissileOriginal=nullptr;

static std::uintptr_t projectileDBRow(std::uintptr_t tableAddress,unsigned id){
    std::uintptr_t table=0,row=0;unsigned maximum=0,actual=0;
    if(!id||!read(tableAddress,table)||!table||!read(tableAddress+4,maximum)||id>maximum||
       !read(table+4*id,row)||!row||!read(row,actual)||actual!=id)return 0;
    return row;
}
static bool selectedWeaponVisual(void* unit,unsigned spellId,std::array<unsigned,16>& visual,
    unsigned& ammoDisplay){
    Player p;
    if(!snapshot(p)||p.unit!=reinterpret_cast<std::uintptr_t>(unit)||p.display!=p.native)return false;
    const auto* context=weaponContext(p.model);
    if(!context||context->token||context->unit!=p.unit||context->guid!=p.guid||context->routes[2]<0||
       static_cast<unsigned>(context->routes[2])>=context->selection.items.size())return false;
    const auto* selected=weaponAsset(context->selection.items[context->routes[2]]);
    if(!selected||selected->kind!=4)return false;

    // Build 5875 uses Attributes bit 1 for ranged weapon visuals. Its equipment
    // requirement also must consist exclusively of bow/gun/crossbow/wand types.
    // This includes ranged special attacks, but never unrelated cast spells.
    const auto spell=projectileDBRow(0xC0D788,spellId);
    unsigned attributes=0,itemClass=0,subclasses=0;
    constexpr unsigned rangedTypes=(1u<<2)|(1u<<3)|(1u<<18)|(1u<<19);
    if(!spell||!read(spell+0x18,attributes)||!(attributes&2)||
       !read(spell+0xE8,itemClass)||itemClass!=2||!read(spell+0xEC,subclasses)||
       !subclasses||(subclasses&~rangedTypes))return false;

    const auto display=reinterpret_cast<std::uintptr_t>(weaponDisplay(selected));
    unsigned visualId=0;
    if(!display||!read(display+0x28,visualId))return false;
    // Two stock decorative wand models have no firing visual. Give those the
    // native Lesser Magic Wand bolt instead of retaining an arrow or bullet.
    if(!visualId&&selected->subclass==19)visualId=2799;
    const auto weaponVisual=projectileDBRow(0xC0D738,visualId);
    std::array<unsigned,16> selectedVisual{};
    if(!weaponVisual||!read(weaponVisual,selectedVisual))return false;
    unsigned replacementAmmo=0;
    if(selected->subclass==19){
        if(!selectedVisual[6]||!projectileDBRow(0xC0D760,selectedVisual[7]))return false;
    }else if(selected->subclass==2||selected->subclass==3||selected->subclass==18){
        // Client ItemDisplayInfo: ArrowFlight_01 / BulletFlight_01 and skins.
        replacementAmmo=selected->subclass==3?2418:2414;
        if(!projectileDBRow(0xC0DC10,replacementAmmo))return false;
        selectedVisual[7]=0; // Select native ammo composition rather than the old spell bolt.
    }else return false;

    visual=selectedVisual;ammoDisplay=replacementAmmo;
    return true;
}
static unsigned* __fastcall unitSpellVisualHook(void* unit,void*,const unsigned* spellRow,
    unsigned* output){
    unsigned* result=unitSpellVisualOriginal(unit,spellRow,output);
    unsigned spellId=0,ammo=0;
    std::array<unsigned,16> selected{};
    // All four native callers own this 64-byte output until their visual work
    // finishes. Never return a hook-local array or modify a shared DBC row.
    if(!output||result!=output||!read(reinterpret_cast<std::uintptr_t>(spellRow),spellId)||
       projectileDBRow(0xC0D788,spellId)!=reinterpret_cast<std::uintptr_t>(spellRow)||
       !selectedWeaponVisual(unit,spellId,selected,ammo))return result;
    output[1]=selected[1];output[2]=selected[2];
    // Physical weapon visuals normally rely on ammo supplied by the server.
    // A real wand sends none: keep visual dispatch alive until the late hook
    // supplies the selected bullet/arrow. Zero values must also replace the
    // equipped weapon's sounds, so a gun cannot retain a wand's flight sound.
    output[6]=1;output[7]=selected[7];output[10]=selected[10];output[14]=selected[14];
    return result;
}
static bool prepareWeaponProjectile(void* unit,unsigned spellId,unsigned& ammoDisplay,
    unsigned& ammoInventory,std::array<unsigned,16>& visual){
    std::array<unsigned,16> selected{};unsigned ammo=0;
    if(!selectedWeaponVisual(unit,spellId,selected,ammo))return false;
    visual[7]=selected[7];visual[10]=selected[10];
    ammoDisplay=ammo;ammoInventory=ammo?24:0;
    return true;
}
static void __fastcall unitMissileHook(void* unit,void*,const void* targetGuid,const void* castData,
    unsigned ammoDisplay,unsigned ammoInventory,unsigned arg5,unsigned arg6,unsigned arg7,
    const unsigned* visual,unsigned spellId,const void* targets,const void* targetFlags){
    std::array<unsigned,16> copy{};
    const bool replaced=visual&&read(reinterpret_cast<std::uintptr_t>(visual),copy)&&
        prepareWeaponProjectile(unit,spellId,ammoDisplay,ammoInventory,copy);
    const bool previous=scopedProjectileAppearance;
    // The constructor's visual metadata lookup also controls whether an arrow
    // sticks into the target. Keep that choice consistent with its new model.
    scopedProjectileAppearance=replaced;
    unitMissileOriginal(unit,targetGuid,castData,ammoDisplay,ammoInventory,arg5,arg6,arg7,
        replaced?copy.data():visual,spellId,targets,targetFlags);
    scopedProjectileAppearance=previous;
}
