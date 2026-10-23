#pragma once
// Included after WeaponRenderer and ProjectileRenderer. DBC rows are immutable;
// cached private kit copies have process lifetime because effects retain them.
#include "Casting.h"
using CastingKit=unsigned (__thiscall *)(void*,const unsigned*,const unsigned*,unsigned,unsigned);
using CastingEffect=void (__thiscall *)(void*,unsigned,unsigned,unsigned,const unsigned*,const unsigned*,unsigned*,std::uintptr_t);
using CastingSheath=void (__thiscall *)(void*,unsigned,unsigned,unsigned);
using CastingPoint=float* (__thiscall *)(void*,float*,unsigned);
using CastingOrigin=float* (__fastcall *)(float*,void*,unsigned);
using CastingSequence=void (__thiscall *)(void*,int,unsigned,int,unsigned,float,unsigned,unsigned);
static CastingKit castingKitOriginal=nullptr;
static CastingEffect castingEffectOriginal=nullptr;
static CastingSheath castingSheathOriginal=nullptr;
static HasPoint castingHasPointOriginal=nullptr;
static CastingPoint castingPointOriginal=nullptr;
static AttachChild castingAttachOriginal=nullptr;
static CastingOrigin castingOriginOriginal=nullptr;
static CastingSequence castingSequenceOriginal=nullptr;
static weaponCasting::Settings castingSettings;
static std::uint64_t castingOwner=0;
static std::uintptr_t castingScope=0;
static unsigned castingPreviousSheath=0;
static bool castingDrewWeapon=false;
static std::map<std::array<unsigned,6>,weaponCasting::Kit> castingKits;
static std::map<std::array<std::uintptr_t,3>,unsigned> castingTips;
static bool castingPlayer(void* unit,Player& p){
    return castingSettings.enabled&&castingOwner==getPlayer()&&snapshot(p)&&p.guid==castingOwner&&
        p.unit==reinterpret_cast<std::uintptr_t>(unit)&&p.display==p.native&&p.model;
}
static const WeaponAsset* castingAsset(){
    const auto* a=weaponAsset(castingSettings.item);
    return a&&(a->kind==1||a->kind==2)?a:nullptr;
}
static bool castingSpell(unsigned spellId){
    const auto row=projectileDBRow(0xC0D788,spellId);
    unsigned attributes=0,category=0,family=0,mechanic=0;
    // The client also represents food, professions and other activities as
    // spells. Their props/gestures must never be turned into weapon casting.
    if(!row||!read(row+0x18,attributes)||(attributes&(2|0x20|0x40))||
       !read(row+4*2,category)||category==11||category==59||
       !read(row+4*160,family)||family==13||!read(row+4*5,mechanic)||mechanic==16)return false;
    if(!family){
        unsigned visual=0;
        if(!read(row+4*115,visual)||!visual)return false; // e.g. triggered Well Fed bonuses
    }
    for(unsigned i=0;i<3;++i){unsigned aura=0;
        if(!read(row+4*(91+i),aura)||aura==78)return false;
        if(aura==84||aura==85){
            unsigned interrupts=0;
            if(!read(row+4*22,interrupts)||(interrupts&0x40000))return false;
        }
        if(!family){
            unsigned effect=0;if(!read(row+4*(61+i),effect))return false;
            if(effect==33||effect==50||effect==95||effect==99||effect==118)return false;
        }
    }
    return true;
}
static unsigned castingCurrentSpell(const Player& p){
    // Build 5875 sets +C8C to the spell ID at SPELL_START and clears it at
    // success/failure/interruption. Channel spells live at descriptor+228.
    // Read the client lifecycle, not a guessed animation-duration timeout.
    unsigned spell=0;
    if(!read(p.unit+0xC8C,spell))return 0;
    if(spell)return castingSpell(spell)?spell:0;
    std::uintptr_t descriptor=0;
    return read(p.unit+0x110,descriptor)&&descriptor&&read(descriptor+0x228,spell)&&spell&&castingSpell(spell)?spell:0;
}
static bool castingInProgress(const Player& p){return castingCurrentSpell(p)!=0;}
static unsigned* castingAugmentVisual(void* unit,const unsigned* spell,unsigned* output,unsigned* original){
    Player p;unsigned id=0;
    if(!output||(original&&original!=output)||!castingSettings.swing||!castingAsset()||!castingPlayer(unit,p)||
       !read(reinterpret_cast<std::uintptr_t>(spell),id)||projectileDBRow(0xC0D788,id)!=reinterpret_cast<std::uintptr_t>(spell)||
       !castingSpell(id))return original;
    // A few spells have no caster art/animation at all. Supply the client's
    // blank cast kit, then customize its private copy at dispatch. No new art,
    // missiles or sounds are added, and existing target/ground kits survive.
    if((!original||(!output[2]&&!output[5]))&&projectileDBRow(0xC0D74C,21)){
        // No-visual generic effects include triggered Well Fed/item bonuses;
        // don't invent a weapon swing for those background updates.
        unsigned family=0;
        if(!read(reinterpret_cast<std::uintptr_t>(spell)+4*160,family)||!family)return original;
        if(!original)std::fill(output,output+16,0u);
        output[2]=21;return output;
    }
    return original;
}
static bool castingVisual(void* unit,const unsigned* spell,weaponCasting::Visual& visual){
    auto* original=unitSpellVisualOriginal(unit,spell,visual.data());
    return castingAugmentVisual(unit,spell,visual.data(),original)==visual.data();
}
static bool castingLoopAnimation(const Player& p,unsigned animation){
    const auto spell=projectileDBRow(0xC0D788,castingCurrentSpell(p));
    weaponCasting::Visual visual{};
    if(!spell||!castingVisual(reinterpret_cast<void*>(p.unit),reinterpret_cast<const unsigned*>(spell),visual))return false;
    for(unsigned index:{1u,5u,2u}){
        if(index==2&&!visual[5])continue; // A release swing is not a charge loop.
        unsigned actual=0;const auto kit=projectileDBRow(0xC0D74C,visual[index]);
        if(kit&&read(kit+8,actual)&&actual==animation)return true;
    }
    return false;
}
static void* castingWeapon(std::uintptr_t parent,unsigned& tip){
    Player p;
    if(!castingSettings.enabled||castingOwner!=getPlayer()||!snapshot(p)||p.guid!=castingOwner||
       p.display!=p.native||!parent||p.model!=parent)return nullptr;
    const auto* asset=castingAsset();if(!asset)return nullptr;
    void* child=findChildHook(reinterpret_cast<void*>(parent),nullptr,1);
    if(!child||!weaponModelMatches(child,asset->model))return nullptr;
    std::uintptr_t actualParent=0,resource=0,header=0,attachments=0;unsigned point=0,count=0,loaded=0;
    const auto address=reinterpret_cast<std::uintptr_t>(child);
    if(!read(address+0x1CC,actualParent)||actualParent!=parent||!read(address+0x1D0,point)||point!=1||
       !read(address+0x10,loaded)||!loaded||!read(address+0x30,resource)||!read(resource+0x130,header)||
       !read(header+0x104,count)||!count||count>64||!read(header+0x108,attachments)||!attachments)return nullptr;
    // Rest attachment locations are immutable for this loaded weapon resource.
    // Cache only its point ID, never a model pointer or an animated position.
    const std::array<std::uintptr_t,3> key{{resource,header,castingSettings.item}};
    const auto cached=castingTips.find(key);
    if(cached!=castingTips.end()&&castingHasPointOriginal(child,cached->second)){tip=cached->second;return child;}
    float farthest=-1;
    for(unsigned i=0;i<count;++i){
        unsigned id=0;std::array<float,3> local{};
        if(!read(attachments+48*i,id)||id>255||!read(attachments+48*i+8,local))continue;
        float length=0;bool finite=true;for(float v:local){finite=finite&&std::isfinite(v);length+=v*v;}
        if(finite&&length>farthest&&castingHasPointOriginal(child,id)){tip=id;farthest=length;}
    }
    if(farthest<0)return nullptr;
    castingTips[key]=tip;return child;
}
static void* castingDrawWeapon(void* unit,const Player& p,unsigned& tip){
    unsigned previous=0;
    if(!castingAsset()||!read(p.unit+0xD40,previous))return nullptr;
    // No sheath notification is sent to the server. Use the existing held
    // model, including for instant projectiles without a preceding charge kit.
    castingSheathOriginal(unit,1,1,0);
    void* weapon=castingWeapon(p.model,tip);
    if(!weapon){castingSheathOriginal(unit,previous,1,0);return nullptr;}
    if(!castingDrewWeapon){castingPreviousSheath=previous;castingDrewWeapon=true;}
    return weapon;
}
static bool castingForwardAttachment(void* model,const float* matrix,std::array<float,16>& output){
    Player p;std::uintptr_t parent=0;unsigned tip=0,point=0;
    // Cheap attachment checks reject ordinary equipment/effects before any
    // player snapshot or weapon-resource traversal on this render hot path.
    if(!castingSettings.enabled||!castingSettings.effects||
       !read(reinterpret_cast<std::uintptr_t>(model)+0x1CC,parent)||!parent||
       !read(parent+0x1D0,point)||point!=1||
       !weaponModelMatches(model,"Spells\\ConeofCold_Hand.mdx"))return false;
    if(castingOwner!=getPlayer()||!snapshot(p)||p.guid!=castingOwner||p.display!=p.native||!p.model||
       castingWeapon(p.model,tip)!=reinterpret_cast<void*>(parent))return false;
    std::array<float,16> root{};
    return read(p.model+0xFC,root)&&weaponCasting::forwardTransform(root,matrix,output);
}
static bool __fastcall castingHasPointHook(void* model,void*,unsigned point){
    if(!weaponCasting::synthetic(point))return castingHasPointOriginal(model,point);
    unsigned tip=0;
    if(castingWeapon(reinterpret_cast<std::uintptr_t>(model),tip))return true;
    return castingHasPointOriginal(model,weaponCasting::fallback(point));
}
static float* __fastcall castingPointHook(void* model,void*,float* output,unsigned point){
    if(weaponCasting::synthetic(point)){
        unsigned tip=0;
        if(void* weapon=castingWeapon(reinterpret_cast<std::uintptr_t>(model),tip))return castingPointOriginal(weapon,output,tip);
        point=weaponCasting::fallback(point);
    }
    return castingPointOriginal(model,output,point);
}
static void __fastcall castingAttachHook(void* child,void*,void* parent,unsigned point){
    if(weaponCasting::synthetic(point)){
        unsigned tip=0;
        if(void* weapon=castingWeapon(reinterpret_cast<std::uintptr_t>(parent),tip)){parent=weapon;point=tip;}
        else point=weaponCasting::fallback(point);
    }
    castingAttachOriginal(child,parent,point);
}
static void __fastcall castingSheathHook(void* unit,void*,unsigned mode,unsigned immediate,unsigned notify){
    Player p;unsigned tip=0;
    const bool hold=(castingScope&&castingScope==reinterpret_cast<std::uintptr_t>(unit))||
        (castingDrewWeapon&&castingAsset()&&castingPlayer(unit,p)&&castingInProgress(p)&&castingWeapon(p.model,tip));
    if(hold&&mode!=1){mode=1;notify=0;}
    castingSheathOriginal(unit,mode,immediate,notify);
}
static void __fastcall castingSequenceHook(void* model,void*,int track,unsigned animation,int variation,
    unsigned time,float speed,unsigned repeat,unsigned blend){
    // The client can schedule the sustained casting loop directly on the
    // model after the opening kit/UnitAnimation call has returned.
    if(castingSettings.enabled&&castingSettings.swing&&castingOwner==getPlayer()&&animation&&animation!=0xffffffffu){
        Player p;unsigned tip=0;const auto* asset=castingAsset();
        if(asset&&snapshot(p)&&p.guid==castingOwner&&p.display==p.native&&p.model==reinterpret_cast<std::uintptr_t>(model)&&
           castingLoopAnimation(p,animation)&&castingWeapon(p.model,tip))
            animation=weaponCasting::animation(51,asset->kind,asset->subclass);
    }
    castingSequenceOriginal(model,track,animation,variation,time,speed,repeat,blend);
}
static void __fastcall castingEffectHook(void* unit,void*,unsigned point,unsigned expires,unsigned spell,
    const unsigned* kit,const unsigned* effect,unsigned* flags,std::uintptr_t callback){
    if(castingSettings.effects&&castingScope==reinterpret_cast<std::uintptr_t>(unit)&&(point==21||point==22))point+=weaponCasting::marker;
    castingEffectOriginal(unit,point,expires,spell,kit,effect,flags,callback);
}
static unsigned __fastcall castingKitHook(void* unit,void*,const unsigned* spell,const unsigned* kit,unsigned phase,unsigned arg4){
    Player p;unsigned id=0;weaponCasting::Kit copy{};weaponCasting::Visual visual{};
    const auto* asset=castingAsset();
    if(!castingPlayer(unit,p)||!asset||!spell||!kit||!read(reinterpret_cast<std::uintptr_t>(spell),id)||
       projectileDBRow(0xC0D788,id)!=reinterpret_cast<std::uintptr_t>(spell)||!castingSpell(id)||
       !read(reinterpret_cast<std::uintptr_t>(kit),copy)||!castingVisual(unit,spell,visual))
        return castingKitOriginal(unit,spell,kit,phase,arg4);
    const auto role=weaponCasting::casterKit(copy,phase,visual);
    if(role==weaponCasting::Role::None)return castingKitOriginal(unit,spell,kit,phase,arg4);
    // State/aura dispatches also use phase 2, and a few visuals share the same
    // state/channel kit. Only the player's active channel may take that path.
    if(phase==2&&castingCurrentSpell(p)!=id)return castingKitOriginal(unit,spell,kit,phase,arg4);
    // The client's own visual sheath routine composes/moves the existing held
    // weapon; no extra weapon decoration or duplicate model is created.
    unsigned tip=0;
    if(!castingDrawWeapon(unit,p,tip))return castingKitOriginal(unit,spell,kit,phase,arg4);
    const auto prepared=weaponCasting::prepare(copy,castingSettings,asset->kind,asset->subclass,role);
    const std::array<unsigned,6> key{{copy[0],asset->kind,asset->subclass,castingSettings.effects,castingSettings.swing,static_cast<unsigned>(role)}};
    const auto inserted=castingKits.emplace(key,prepared);
    const auto previousScope=castingScope;castingScope=p.unit;
    const auto result=castingKitOriginal(unit,spell,inserted.first->second.data(),phase,arg4);
    castingScope=previousScope;return result;
}
static float* __fastcall castingOriginHook(float* output,void* unit,unsigned spellId){
    Player p;unsigned tip=0;
    if(castingSettings.effects&&castingPlayer(unit,p)&&castingSpell(spellId))
        if(void* weapon=castingDrawWeapon(unit,p,tip))return castingPointOriginal(weapon,output,tip);
    return castingOriginOriginal(output,unit,spellId);
}
static int __fastcall castingVersionLua(void* L){return result(L,1);}
static int __fastcall setCastingLua(void* L){
    unsigned values[4]{};
    for(int i=0;i<4;++i){
        if(!isNumber(L,i+1))return result(L,-2);
        const double value=toNumber(L,i+1);
        if(!std::isfinite(value)||value<0||value>(i==3?2147483647.:1.)||value!=static_cast<unsigned>(value))return result(L,-2);
        values[i]=static_cast<unsigned>(value);
    }
    weaponCasting::Settings requested{values[0]!=0,values[1]!=0,values[2]!=0,values[3]};
    const auto* asset=weaponAsset(requested.item);
    if(requested.enabled&&requested.item&&(!asset||(asset->kind!=1&&asset->kind!=2)))return result(L,-2);
    Player p;if(!snapshot(p))return result(L,-1);
    unsigned currentSheath=0;
    if(!requested.enabled&&castingDrewWeapon&&castingOwner==p.guid&&castingSheathOriginal&&
       read(p.unit+0xD40,currentSheath)&&currentSheath==1)
        castingSheathOriginal(reinterpret_cast<void*>(p.unit),castingPreviousSheath,1,0);
    if(!requested.enabled||castingOwner!=p.guid)castingDrewWeapon=false;
    castingSettings=requested;castingOwner=p.guid;
    return result(L,1);
}
