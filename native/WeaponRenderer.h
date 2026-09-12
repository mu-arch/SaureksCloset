#pragma once
#include "WeaponState.h"
#include "BowPlacement.h"
// Included after the bridge's player reader, registry and Lua result helpers.
using WeaponCompose=int (__fastcall *)(void*,void*,unsigned,unsigned,unsigned,unsigned,unsigned);
using SheathPoint=int (__fastcall *)(unsigned,unsigned);
using MoveWeapon=void (__thiscall *)(void*,unsigned,unsigned);
using RebuildWeapon=void (__thiscall *)(void*,unsigned);
using FindChild=void* (__thiscall *)(void*,unsigned);
using ClearChildren=void (__thiscall *)(void*,unsigned);
using AttachChild=void (__thiscall *)(void*,void*,unsigned);
using HasPoint=bool (__thiscall *)(void*,unsigned);
using LoadChild=void (__fastcall *)(void*,unsigned,const char*,const char*,unsigned);
using UpdateAttachedModel=void (__thiscall *)(void*,const float*,const float*,const float*,float);
static WeaponCompose weaponComposeOriginal=nullptr;
static SheathPoint sheathPointOriginal=nullptr;
static MoveWeapon moveWeaponOriginal=nullptr;
static RebuildWeapon rebuildWeaponOriginal=nullptr;
static FindChild findChildOriginal=nullptr;
static ClearChildren clearChildrenOriginal=nullptr;
static UpdateAttachedModel updateAttachedOriginal=nullptr;
#ifndef SAUREKS_WEAPON_TEST
template<typename T> static T weaponFunction(std::uintptr_t address){return reinterpret_cast<T>(address);}
#endif
static const auto retainChild=weaponFunction<DestroyModel>(0x710390);
static const auto detachChild=weaponFunction<DestroyModel>(0x713020);
static const auto attachChild=weaponFunction<AttachChild>(0x712F70);
static const auto hasPoint=weaponFunction<HasPoint>(0x712CB0);
static const auto loadChild=weaponFunction<LoadChild>(0x4798C0);
static const auto refreshMelee=weaponFunction<void (__thiscall *)(void*,unsigned)>(0x605DA0);
static const auto refreshRanged=weaponFunction<void (__thiscall *)(void*,unsigned)>(0x611E10);
struct WeaponContext {
    std::uintptr_t parent=0,unit=0;std::uint64_t guid=0;unsigned token=0;
    WeaponSelection selection;
    std::array<int,3> routes{{-1,-1,-1}};
    std::array<void*,7> extra{};
};
static std::array<WeaponContext,9> weaponContexts{};
static int scopedSheathPoint=-1;
static std::uintptr_t loadingExtraParent=0;
static WeaponContext* weaponContext(std::uintptr_t model){
    for(auto& c:weaponContexts)if(model&&c.parent==model)return &c;
    return nullptr;
}
static bool ownedExtra(const WeaponContext& c,void* child){for(auto p:c.extra)if(p&&p==child)return true;return false;}
static bool positionStoredBow(void* child,const float* attachment,std::array<float,16>& adjusted){
    std::uintptr_t parent=0;unsigned point=0;
    const auto model=reinterpret_cast<std::uintptr_t>(child);
    if(!read(model+0x1CC,parent)||!read(model+0x1D0,point)||point!=weaponPoints[5])return false;
    const auto* c=weaponContext(parent);
    if(!c||c->guid!=getPlayer())return false;
    const auto* asset=weaponAsset(c->selection.items[5]);
    if(!asset||asset->kind!=4||asset->subclass!=2)return false;
    if(c->extra[5]!=child&&(c->token||c->routes[2]!=5||findChildOriginal(reinterpret_cast<void*>(parent),point)!=child))return false;
    std::array<float,3> back;
    if(!animatedBackPosition(parent,back)||!read(reinterpret_cast<std::uintptr_t>(attachment),adjusted))return false;
    // Keep the ranged sheath's animated orientation, but center the bow's grip
    // on this model's authored back anchor. Logical point 27 stays independent
    // of shields at 28, preserving ownership and native draw/sheath callbacks.
    for(unsigned axis=0;axis<3;++axis)adjusted[12+axis]=back[axis];
    return true;
}
static bool positionStoredBackWeapon(void* child,std::array<float,16>& adjusted){
    std::uintptr_t parent=0;unsigned point=0;
    const auto model=reinterpret_cast<std::uintptr_t>(child);
    if(!read(model+0x1CC,parent)||!read(model+0x1D0,point)||(point!=30&&point!=31))return false;
    const auto* c=weaponContext(parent);
    if(!c||c->guid!=getPlayer())return false;
    const unsigned position=point==30?2:3;
    const auto* asset=weaponAsset(c->selection.items[position]);
    if(!asset||asset->kind!=2||asset->sheath!=1)return false;
    if(c->extra[position]!=child){
        if(c->token)return false;
        bool routed=false;for(auto route:c->routes)if(route==static_cast<int>(position))routed=true;
        if(!routed||findChildOriginal(reinterpret_cast<void*>(parent),point)!=child)return false;
    }
    // Points 30/31 have a staff-style pose. Keep those independent logical
    // homes, but draw type-1 two-handers using the model's sword pose at 26/27.
    // The full animated transform preserves both the grip position and angle.
    return animatedAttachmentMatrix(parent,point==30?26:27,adjusted);
}
static void __fastcall updateAttachedHook(void* model,void*,const float* matrix,const float* scale,const float* lighting,float alpha){
    std::array<float,16> adjusted;
    // Exact recursive child update after the animated attachment is resolved.
    if(reinterpret_cast<std::uintptr_t>(__builtin_return_address(0))==0x718761&&
       (positionStoredBackWeapon(model,adjusted)||positionStoredBow(model,matrix,adjusted)))
        updateAttachedOriginal(model,adjusted.data(),scale,lighting,alpha);
    else updateAttachedOriginal(model,matrix,scale,lighting,alpha);
}
static void releaseExtras(WeaponContext& c){
    const auto extra=c.extra;c.extra.fill(nullptr);
    for(auto child:extra)if(child){
        std::uintptr_t parent=0;
        if(read(reinterpret_cast<std::uintptr_t>(child)+0x1CC,parent)&&parent==c.parent)detachChild(child);
        releaseModel(child);
    }
}
static void forgetWeapons(std::uintptr_t model){
    if(auto* c=weaponContext(model)){releaseExtras(*c);*c={};}
}
static void* __fastcall findChildHook(void* parent,void*,unsigned point){
    const auto* c=weaponContext(reinterpret_cast<std::uintptr_t>(parent));
    if(!c)return findChildOriginal(parent,point);
    std::uintptr_t child=0;read(c->parent+0x1DC,child);
    for(unsigned n=0;child&&n<128;++n){
        unsigned id=0;std::uintptr_t next=0;
        if(!read(child+0x1D0,id)||!read(child+0x1E4,next))return nullptr;
        if(id==point&&!ownedExtra(*c,reinterpret_cast<void*>(child)))return reinterpret_cast<void*>(child);
        if(next==child)break;
        child=next;
    }
    return nullptr;
}
static void __fastcall clearChildrenHook(void* parent,void*,unsigned point){
    const auto address=reinterpret_cast<std::uintptr_t>(parent);
    if(loadingExtraParent==address)return;
    const auto* c=weaponContext(address);
    if(!c){clearChildrenOriginal(parent,point);return;}
    std::uintptr_t child=0;read(address+0x1DC,child);
    for(unsigned n=0;child&&n<128;++n){
        unsigned id=0;std::uintptr_t next=0;
        if(!read(child+0x1D0,id)||!read(child+0x1E4,next))return;
        if(id==point&&!ownedExtra(*c,reinterpret_cast<void*>(child)))detachChild(reinterpret_cast<void*>(child));
        if(next==child)break;
        child=next;
    }
}
static int __fastcall sheathPointHook(unsigned sheath,unsigned side){
    return scopedSheathPoint>=0?scopedSheathPoint:sheathPointOriginal(sheath,side);
}
static void* weaponDisplay(const WeaponAsset* asset){
    std::uintptr_t table=0,row=0;unsigned max=0,id=0;
    if(asset&&read(0xC0DC10,table)&&read(0xC0DC14,max)&&asset->display<=max&&
       read(table+4*asset->display,row)&&read(row,id)&&id==asset->display)return reinterpret_cast<void*>(row);
    return nullptr;
}
static int __fastcall weaponComposeHook(void* parent,void* display,unsigned slot,unsigned sheath,unsigned stored,unsigned shield,unsigned rangedRight){
    auto* c=weaponContext(reinterpret_cast<std::uintptr_t>(parent));
    const int old=scopedSheathPoint;
    scopedSheathPoint=-1;
    if(c&&!c->token&&c->guid==getPlayer()&&slot>=15&&slot<=17){
        const int route=c->routes[slot-15];
        if(route>=0){
            const auto* a=weaponAsset(c->selection.items[route]);
            if(void* row=weaponDisplay(a)){display=row;scopedSheathPoint=weaponPoints[route];sheath=a->sheath;}
        }
    }
    const int result=weaponComposeOriginal(parent,display,slot,sheath,stored,shield,rangedRight);
    scopedSheathPoint=old;return result;
}
static void __fastcall moveWeaponHook(void* unit,void*,unsigned role,unsigned stored){
    std::uintptr_t parent=0;read(reinterpret_cast<std::uintptr_t>(unit)+0xD8,parent);
    auto* c=weaponContext(parent);const int old=scopedSheathPoint;
    // A ranged move can rebuild melee weapons recursively. Each role must
    // choose its own home instead of inheriting the outer ranged override.
    scopedSheathPoint=-1;
    if(c&&!c->token&&c->unit==reinterpret_cast<std::uintptr_t>(unit)&&c->guid==getPlayer()&&role<3&&c->routes[role]>=0)
        scopedSheathPoint=weaponPoints[c->routes[role]];
    moveWeaponOriginal(unit,role,stored);scopedSheathPoint=old;
}
static void __fastcall rebuildWeaponHook(void* unit,void*,unsigned role){
    std::uintptr_t parent=0;read(reinterpret_cast<std::uintptr_t>(unit)+0xD8,parent);
    auto* c=weaponContext(parent);const int old=scopedSheathPoint;
    scopedSheathPoint=-1;
    if(c&&!c->token&&c->unit==reinterpret_cast<std::uintptr_t>(unit)&&c->guid==getPlayer()&&role<3&&c->routes[role]>=0)
        scopedSheathPoint=weaponPoints[c->routes[role]];
    // The client's missing-child fallback also computes the sheath point for
    // weapon effects before composing. Scope that entire role's rebuild.
    rebuildWeaponOriginal(unit,role);scopedSheathPoint=old;
}
static bool ensureExtras(WeaponContext& c){
    bool complete=true;
    for(unsigned i=0;i<7;++i){
        const auto* a=weaponAsset(c.selection.items[i]);if(!a)continue;
        if(!hasPoint(reinterpret_cast<void*>(c.parent),weaponPoints[i])){complete=false;continue;}
        bool routed=false;for(auto route:c.routes)if(route==static_cast<int>(i))routed=true;
        if(!c.token&&routed)continue;
        if(c.extra[i]){
            std::uintptr_t parent=0;
            if(read(reinterpret_cast<std::uintptr_t>(c.extra[i])+0x1CC,parent)&&!parent)
                attachChild(c.extra[i],reinterpret_cast<void*>(c.parent),weaponPoints[i]);
            continue;
        }
        // Keep preexisting children, then identify the factory's newly attached
        // child by its parent-list head. Our own reference survives stock clears.
        std::uintptr_t before=0,after=0;read(c.parent+0x1DC,before);
        loadingExtraParent=c.parent;
        loadChild(reinterpret_cast<void*>(c.parent),weaponPoints[i],a->model,a->texture,0);
        loadingExtraParent=0;read(c.parent+0x1DC,after);
        unsigned point=0;std::uintptr_t parent=0;
        if(after&&after!=before&&read(after+0x1D0,point)&&point==weaponPoints[i]&&read(after+0x1CC,parent)&&parent==c.parent){
            c.extra[i]=reinterpret_cast<void*>(after);retainChild(c.extra[i]);
        }else complete=false;
    }
    return complete;
}
static int __fastcall setWeapons(void* L){
    unsigned values[11]{};
    for(int i=0;i<11;++i){
        if(!isNumber(L,i+1))return result(L,-2);
        const double v=toNumber(L,i+1);
        if(!std::isfinite(v)||v<0||v>2147483647||v!=static_cast<unsigned>(v))return result(L,-2);
        values[i]=static_cast<unsigned>(v);
    }
    WeaponSelection selection;
    for(unsigned i=0;i<7;++i)selection.items[i]=values[i+1];
    for(unsigned i=0;i<3;++i)selection.equipped[i]=values[i+8];
    if(!selection.valid())return result(L,-2);
    Player p;if(!snapshot(p))return result(L,-1);
    std::uintptr_t parent=p.model;const unsigned token=values[0];
    if(token){
        parent=0;
        for(auto& e:previews.entries)if(e.token==token&&e.guid==p.guid&&e.status==1)parent=e.model;
        if(!parent)return result(L,-1);
    }else if(p.display!=p.native){
        if(auto* c=weaponContext(parent)){releaseExtras(*c);*c={};}
        return result(L,0);
    }
    unsigned loaded=0;if(!parent||!read(parent+0x10,loaded)||!loaded)return result(L,0);
    auto* c=weaponContext(parent);
    if(!c&&selection.empty())return result(L,1);
    if(!c){
        for(auto& entry:weaponContexts)if(!entry.parent){c=&entry;break;}
        if(!c)return result(L,-1);
        c->parent=parent;c->unit=token?0:p.unit;c->guid=p.guid;c->token=token;
    }
    if(c->guid!=p.guid||c->token!=token)return result(L,-1);
    if(!(c->selection==selection)){
        releaseExtras(*c);
        // Remove the old routed weapons at their old homes before rerouting.
        if(!token)for(auto route:c->routes)if(route>=0)clearChildrenHook(reinterpret_cast<void*>(parent),nullptr,weaponPoints[route]);
        if(!token){
            // A first override may use a different home than the stock weapon.
            // Clear both generations before rebuilding, or its old child remains.
            for(const auto* settings:{&c->selection,&selection})for(unsigned role=0;role<3;++role){
                const auto* a=weaponAsset(settings->equipped[role]);if(!a)continue;
                const unsigned side=role==0||(role==2&&(a->inventory==25||a->inventory==26));
                const int point=sheathPointOriginal(a->sheath,side);
                if(point>=0)clearChildrenHook(reinterpret_cast<void*>(parent),nullptr,static_cast<unsigned>(point));
            }
        }
        c->selection=selection;c->routes=selection.routes();
        for(auto& route:c->routes)if(route>=0&&(!weaponDisplay(weaponAsset(selection.items[route]))||!hasPoint(reinterpret_cast<void*>(parent),weaponPoints[route])))route=-1;
        if(!token){
            unsigned mode=0;read(p.unit+0xD40,mode);
            refreshMelee(reinterpret_cast<void*>(p.unit),0);refreshMelee(reinterpret_cast<void*>(p.unit),1);
            if(mode==2){moveWeaponHook(reinterpret_cast<void*>(p.unit),nullptr,0,1);moveWeaponHook(reinterpret_cast<void*>(p.unit),nullptr,1,1);}
            refreshRanged(reinterpret_cast<void*>(p.unit),mode!=2);
        }
    }
    const bool complete=ensureExtras(*c);
    if(selection.empty())*c={};
    return result(L,complete?1:0);
}
