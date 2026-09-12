// Exercise the actual renderer hooks with an in-memory native-object simulator.
// This tests ownership/routing, not the game's ABI or rendered appearance.
#define __fastcall
#define __thiscall
#define SAUREKS_WEAPON_TEST
#include <cassert>
#include <cmath>
#include <cstdint>
#include <map>
#include <vector>
#include <iostream>
#include <type_traits>
#include "../native/PreviewState.h"
#include "../native/WeaponState.h"
#include "bow_attachment_fixtures.h"
#include "sword_attachment_fixtures.h"
using DestroyModel=void (*)(void*);
static std::map<std::uintptr_t,std::uint64_t> memory;
static std::map<std::uintptr_t,int> refs;
static unsigned loads=0,meleeRefreshes=0,rangedRefreshes=0;
static std::uintptr_t nextModel=0x100000,rangeHolder=0;
static void forgetWeapons(std::uintptr_t);
static void* pointer(std::uintptr_t p){return reinterpret_cast<void*>(p);}
static std::uintptr_t address(void* p){return reinterpret_cast<std::uintptr_t>(p);}
static void ref(void* p){assert(refs[address(p)]>0);++refs[address(p)];}
static void unref(void* p){assert(refs[address(p)]>0);if(--refs[address(p)]==0)forgetWeapons(address(p));}
static const auto releaseModel=&unref;
template<typename T> static bool read(std::uintptr_t a,T& out){auto i=memory.find(a);if(i==memory.end())return false;out=static_cast<T>(i->second);return true;}
static std::map<std::uintptr_t,std::array<float,16>> matrices;
static bool read(std::uintptr_t a,std::array<float,16>& out){auto i=matrices.find(a);if(i==matrices.end())return false;out=i->second;return true;}
static std::map<std::uintptr_t,std::array<float,3>> positions;
static bool read(std::uintptr_t a,std::array<float,3>& out){auto i=positions.find(a);if(i==positions.end())return false;out=i->second;return true;}
struct Player {std::uintptr_t model=0,unit=0;std::uint64_t guid=0;unsigned display=0,native=0;};
static Player player{0x1000,0x2000,123,49,49};
static PreviewRegistry previews;
static bool snapshot(Player& p){p=player;return p.guid!=0;}
static std::uint64_t getPlayer(){return player.guid;}
struct Lua {std::vector<double> values;};
static bool isNumber(void* L,int i){return static_cast<unsigned>(i)<=static_cast<Lua*>(L)->values.size();}
static double toNumber(void* L,int i){return static_cast<Lua*>(L)->values[i-1];}
static int result(void*,int status){return status;}
static void detach(void* p){
    auto child=address(p);auto parent=memory[child+0x1CC];assert(parent);
    auto link=parent+0x1DC;
    while(memory[link]&&memory[link]!=child)link=memory[link]+0x1E4;
    assert(memory[link]==child);memory[link]=memory[child+0x1E4];
    memory[child+0x1CC]=0;memory[child+0x1E4]=0;memory[child+0x1D0]=0xffffffff;
    unref(p);
}
static void attach(void* p,void* par,unsigned point){
    auto child=address(p),parent=address(par);assert(!memory[child+0x1CC]);
    memory[child+0x1CC]=parent;memory[child+0x1D0]=point;
    memory[child+0x1E4]=memory[parent+0x1DC];memory[parent+0x1DC]=child;ref(p);
}
static bool supports(void*,unsigned point){return point<34;}
static void factory(void*,unsigned,const char*,const char*,unsigned);
static void melee(void*,unsigned);
static void ranged(void*,unsigned);
template<typename T> static T weaponFunction(std::uintptr_t a){
    if constexpr(std::is_same_v<T,decltype(&ref)>){
        if(a==0x710390)return &ref;
        if(a==0x713020)return &detach;
    }else if constexpr(std::is_same_v<T,decltype(&attach)>){if(a==0x712F70)return &attach;}
    else if constexpr(std::is_same_v<T,decltype(&supports)>){if(a==0x712CB0)return &supports;}
    else if constexpr(std::is_same_v<T,decltype(&factory)>){if(a==0x4798C0)return &factory;}
    else if constexpr(std::is_same_v<T,decltype(&melee)>){if(a==0x605DA0)return &melee;if(a==0x611E10)return &ranged;}
    assert(false);return nullptr;
}
#include "../native/WeaponRenderer.h"
static unsigned realIDs[3]={35,0,0};
static int sheath(unsigned type,unsigned side){
    switch(type){case 1:return side?26:27;case 2:return side?30:31;case 3:return side?32:33;case 4:return 28;}return -1;
}
static void* find(void* par,unsigned point){
    for(auto child=memory[address(par)+0x1DC];child;child=memory[child+0x1E4])if(memory[child+0x1D0]==point)return pointer(child);
    return nullptr;
}
static void clear(void* par,unsigned point){while(auto p=find(par,point))detach(p);}
static void factory(void* parent,unsigned point,const char*,const char*,unsigned){
    clearChildrenHook(parent,nullptr,point);
    auto child=nextModel;nextModel+=0x1000;refs[child]=1;memory[child+0x1CC]=0;
    attach(pointer(child),parent,point);unref(pointer(child));++loads;
}
static int compose(void* parent,void*,unsigned slot,unsigned type,unsigned stored,unsigned shield,unsigned right){
    auto home=sheathPointHook(type,slot==15||(slot==17&&right));
    unsigned hand=slot==15||right?1:(shield?0:2);
    clearChildrenHook(parent,nullptr,hand);if(home>=0)clearChildrenHook(parent,nullptr,home);
    int point=stored?home:static_cast<int>(hand);if(point<0)return -1;
    factory(parent,point,"","",0);return point;
}
static void melee(void* unit,unsigned role){
    ++meleeRefreshes;auto* a=weaponAsset(realIDs[role]);if(!a)return;
    weaponComposeHook(pointer(memory[address(unit)+0xD8]),weaponDisplay(a),15+role,a->sheath,memory[address(unit)+0xD40]==0,a->kind==3,0);
}
static void ranged(void* unit,unsigned stored){
    ++rangedRefreshes;
    if(rangeHolder){unref(pointer(rangeHolder));rangeHolder=0;}
    auto* a=weaponAsset(realIDs[2]);if(!a)return;
    auto* parent=pointer(memory[address(unit)+0xD8]);
    if(!stored){clearChildrenHook(parent,nullptr,0);clearChildrenHook(parent,nullptr,1);clearChildrenHook(parent,nullptr,2);}
    const int point=weaponComposeHook(parent,weaponDisplay(a),17,a->sheath,stored,0,a->inventory==25||a->inventory==26);
    if(point>=0){auto p=findChildHook(parent,nullptr,point);if(p){rangeHolder=address(p);ref(p);}}
}
static void move(void* unit,unsigned role,unsigned stored){
    auto* a=weaponAsset(realIDs[role]);if(!a)return;
    auto* parent=pointer(memory[address(unit)+0xD8]);
    const bool right=role==0||(role==2&&(a->inventory==25||a->inventory==26));
    const unsigned hand=right?1:(a->kind==3?0:2);
    const int home=sheathPointHook(a->sheath,right);if(home<0)return;
    auto p=findChildHook(parent,nullptr,stored?hand:static_cast<unsigned>(home));
    if(p){ref(p);detach(p);attach(p,parent,stored?static_cast<unsigned>(home):hand);unref(p);}
    if(role==2&&stored){rebuildWeaponHook(unit,nullptr,0);rebuildWeaponHook(unit,nullptr,1);}
}
static int effectHomes[3]={-1,-1,-1};
static void rebuild(void* unit,unsigned role){
    const auto* a=weaponAsset(realIDs[role]);if(!a)return;
    const bool right=role==0||(role==2&&(a->inventory==25||a->inventory==26));
    effectHomes[role]=sheathPointHook(a->sheath,right);
    const unsigned mode=memory[address(unit)+0xD40];
    const bool stored=mode==0||(mode==2&&role!=2);
    weaponComposeHook(pointer(memory[address(unit)+0xD8]),weaponDisplay(a),15+role,a->sheath,stored,a->kind==3,right);
}
static Lua request(unsigned token,const WeaponSelection& s){
    Lua L;L.values.push_back(token);for(auto v:s.items)L.values.push_back(v);for(auto v:s.equipped)L.values.push_back(v);return L;
}
int main(){
    (void)&updateAttachedHook;
    sheathPointOriginal=&sheath;weaponComposeOriginal=&compose;moveWeaponOriginal=&move;findChildOriginal=&find;clearChildrenOriginal=&clear;
    rebuildWeaponOriginal=&rebuild;
    memory[player.unit+0xD8]=player.model;memory[player.unit+0xD40]=0;memory[player.model+0x10]=1;
    memory[0xC0DC10]=0x900000;memory[0xC0DC14]=100000;
    for(const auto& a:weaponAssets){const auto row=0xC00000+100*a.display;memory[0x900000+4*a.display]=row;memory[row]=a.display;}
    unsigned gun=0,quiver=0;for(const auto& a:weaponAssets){if(a.kind==4&&a.subclass==3)gun=a.item;if(a.kind==5)quiver=a.item;}
    WeaponSelection s;s.items={{25,25,35,35,143,gun,quiver}};s.equipped={{35,0,0}};
    melee(pointer(player.unit),0);auto original=find(pointer(player.model),30);assert(original);
    auto L=request(0,s);assert(setWeapons(&L)==1);auto* c=weaponContext(player.model);assert(c);
    assert(c->routes[0]==2&&c->routes[2]==-1);
    assert(!refs[address(original)]);assert(findChildHook(pointer(player.model),nullptr,30));
    for(unsigned i=0;i<7;i++){assert(find(pointer(player.model),weaponPoints[i]));if(i!=2)assert(c->extra[i]);}
    unsigned count=loads;assert(setWeapons(&L)==1&&loads==count); // no idle reload
    auto staff=findChildHook(pointer(player.model),nullptr,30);
    memory[player.unit+0xD40]=1;moveWeaponHook(pointer(player.unit),nullptr,0,0);
    assert(findChildHook(pointer(player.model),nullptr,1)==staff&&refs[address(staff)]==1);
    for(unsigned i=0;i<7;i++)if(i!=2)assert(memory[address(c->extra[i])+0x1CC]==player.model);
    moveWeaponHook(pointer(player.unit),nullptr,0,1);assert(findChildHook(pointer(player.model),nullptr,30)==staff);
    // The preview has independent instances and cannot alter world ownership.
    previews.entries[0].model=0x6000;previews.entries[0].guid=player.guid;previews.entries[0].token=8;previews.entries[0].status=1;memory[0x6010]=1;
    auto preview=request(8,s);assert(setWeapons(&preview)==1);auto* pc=weaponContext(0x6000);assert(pc);
    for(unsigned i=0;i<7;i++)assert(pc->extra[i]&&pc->extra[i]!=c->extra[i]);
    // Use each race/gender's actual authored anchor, with independent world
    // and preview model data. Animated bone matrices include scale/orientation.
    const std::array<float,16> identity{{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1}};
    matrices[0xA00000]={{0,0,1,0,1,0,0,0,0,1,0,0,20,30,40,1}};
    const auto input=matrices[0xA00000];
    const auto* attachment=reinterpret_cast<const float*>(0xA00000);
    std::array<float,16> shifted;
    assert(!positionStoredBow(c->extra[5],attachment,shifted)); // gun unchanged
    c->selection.items[5]=2507;pc->selection.items[5]=2507;
    for(const auto& fixture:bowFixtures)for(auto context:{c,pc}){
        // Deliberately unrelated to player race or any fixed attachment index.
        const auto base=context==c?0x2000000u:0x3000000u;
        memory[context->parent+0x30]=base;
        memory[base+0x130]=base+0x1000;
        memory[base+0x1000+0x10C]=37;memory[base+0x1000+0x110]=base+0x2000;
        memory[base+0x2000+2*28]=fixture.index;
        memory[base+0x1000+0x104]=34;memory[base+0x1000+0x108]=base+0x3000;
        const auto record=base+0x3000+48*fixture.index;
        memory[record]=28;memory[record+4]=fixture.bone;positions[record+8]=fixture.position;
        memory[base+0x1000+0x34]=128;memory[context->parent+0x94]=base+0x5000;
        const auto boneAddress=base+0x5000+64*fixture.bone;
        for(const auto& transform:{identity,std::array<float,16>{{0,2,0,0,-2,0,0,0,0,0,2,0,7,8,9,1}},
            std::array<float,16>{{1,0,0,0,0,0,1,0,0,-1,0,0,-3,2,5,1}}}){
            matrices[boneAddress]=transform;
            assert(positionStoredBow(context->extra[5],attachment,shifted));
            const auto& p=fixture.position;
            for(unsigned axis=0;axis<3;++axis){
                const float expected=transform[12+axis]+p[0]*transform[axis]+p[1]*transform[4+axis]+p[2]*transform[8+axis];
                assert(std::fabs(shifted[12+axis]-expected)<.0001f);
            }
            for(unsigned i=0;i<12;++i)assert(shifted[i]==input[i]); // sheath angle/scale unchanged
            const auto once=shifted;
            assert(positionStoredBow(context->extra[5],attachment,shifted)&&shifted==once);
            assert(matrices[0xA00000]==input&&matrices[boneAddress]==transform);
        }
        memory[base+0x2000+2*28]=0xFFFF;
        assert(!positionStoredBow(context->extra[5],attachment,shifted));
        memory[base+0x2000+2*28]=fixture.index;
        memory[record]=27;assert(!positionStoredBow(context->extra[5],attachment,shifted));memory[record]=28;
        memory[record+4]=128;assert(!positionStoredBow(context->extra[5],attachment,shifted));memory[record+4]=fixture.bone;
        matrices[boneAddress][15]=0;assert(!positionStoredBow(context->extra[5],attachment,shifted));matrices[boneAddress]=identity;
    }
    auto bow=c->extra[5];c->extra[5]=nullptr;c->routes[2]=5;
    assert(positionStoredBow(bow,attachment,shifted)); // native routed bow
    memory[address(bow)+0x1D0]=2;
    assert(!positionStoredBow(bow,attachment,shifted)); // drawn hand remains native
    memory[address(bow)+0x1D0]=27;c->routes[2]=-1;
    assert(!positionStoredBow(bow,attachment,shifted)); // unrelated child not adjusted
    c->extra[5]=bow;
    auto guid=c->guid;c->guid=999;assert(!positionStoredBow(bow,attachment,shifted));c->guid=guid;
    for(const auto& asset:weaponAssets)if(asset.kind==4&&asset.subclass!=2){
        c->selection.items[5]=asset.item;assert(!positionStoredBow(bow,attachment,shifted));
    }
    c->selection.items[5]=gun;pc->selection.items[5]=gun;
    // The storage repair uses logical points 30/31, but sheath-type-1 swords
    // must use the original sword bone's orientation, not the staff bone's.
    for(const auto& fixture:swordFixtures)for(auto context:{c,pc}){
        const unsigned position=fixture.point==26?2:3;
        void* child=context->extra[position];
        if(!child)child=findChildOriginal(pointer(context->parent),weaponPoints[position]);
        assert(child);
        const auto oldItem=context->selection.items[position];
        assert(!positionStoredBackWeapon(child,shifted)); // original staff unaffected
        context->selection.items[position]=4939;
        const auto base=context==c?0x4000000u:0x5000000u;
        memory[context->parent+0x30]=base;memory[base+0x130]=base+0x1000;
        memory[base+0x1000+0x10C]=37;memory[base+0x1000+0x110]=base+0x2000;
        memory[base+0x2000+2*fixture.point]=fixture.index;
        memory[base+0x1000+0x104]=34;memory[base+0x1000+0x108]=base+0x3000;
        const auto record=base+0x3000+48*fixture.index;
        memory[record]=fixture.point;memory[record+4]=fixture.bone;positions[record+8]=fixture.position;
        memory[base+0x1000+0x34]=128;memory[context->parent+0x94]=base+0x5000;
        const auto boneAddress=base+0x5000+64*fixture.bone;
        for(const auto& transform:{identity,std::array<float,16>{{0,2,0,0,-2,0,0,0,0,0,2,0,7,8,9,1}},
            std::array<float,16>{{1,0,0,0,0,0,1,0,0,-1,0,0,-3,2,5,1}}}){
            matrices[boneAddress]=transform;
            assert(positionStoredBackWeapon(child,shifted));
            for(unsigned i=0;i<12;++i)assert(shifted[i]==transform[i]);
            for(unsigned axis=0;axis<3;++axis){
                const auto& p=fixture.position;
                assert(std::fabs(shifted[12+axis]-(transform[12+axis]+p[0]*transform[axis]+p[1]*transform[4+axis]+p[2]*transform[8+axis]))<.0001f);
            }
            const auto once=shifted;assert(positionStoredBackWeapon(child,shifted)&&shifted==once);
            assert(matrices[boneAddress]==transform); // do not alter parent skeleton
            assert(memory[address(child)+0x1D0]==weaponPoints[position]); // logical home unchanged
        }
        memory[address(child)+0x1D0]=1;assert(!positionStoredBackWeapon(child,shifted));
        memory[address(child)+0x1D0]=weaponPoints[position];
        auto originalGuid=context->guid;context->guid=999;assert(!positionStoredBackWeapon(child,shifted));context->guid=originalGuid;
        memory[base+0x2000+2*fixture.point]=0xFFFF;assert(!positionStoredBackWeapon(child,shifted));
        memory[base+0x2000+2*fixture.point]=fixture.index;
        memory[record+4]=128;assert(!positionStoredBackWeapon(child,shifted));memory[record+4]=fixture.bone;
        matrices[boneAddress][15]=0;assert(!positionStoredBackWeapon(child,shifted));
        context->selection.items[position]=oldItem;
    }
    clearChildrenHook(pointer(player.model),nullptr,32);assert(c->extra[0]&&refs[address(c->extra[0])]==2);
    assert(!findChildHook(pointer(player.model),nullptr,32)); // native callers skip decor
    auto extra=c->extra[0];detach(extra);assert(refs[address(extra)]==1);
    assert(setWeapons(&L)==1&&c->extra[0]==extra&&refs[address(extra)]==2);
    // Actually equipping a gun routes that stored instance through stock callbacks.
    s.equipped[2]=gun;realIDs[2]=gun;memory[player.unit+0xD40]=2;L=request(0,s);
    assert(setWeapons(&L)==1&&c->routes[2]==5&&rangeHolder);
    assert(findChildHook(pointer(player.model),nullptr,1)==pointer(rangeHolder));
    assert(refs[rangeHolder]==2);assert(!ownedExtra(*c,pointer(rangeHolder)));
    // Disabling releases our children and restores ordinary native homes.
    const auto extrasBeforeOff=c->extra;
    WeaponSelection off;off.equipped=s.equipped;auto disable=request(0,off);
    assert(setWeapons(&disable)==1&&!weaponContext(player.model));
    for(auto child:extrasBeforeOff)if(child)assert(refs[address(child)]==0);
    assert(pc->extra[0]&&refs[address(pc->extra[0])]==2);
    assert(setWeapons(&L)==1);c=weaponContext(player.model);assert(c&&c->extra[0]);
    // Destruction invalidates the parent's identity before its address is reused.
    auto oldExtra=c->extra[0];forgetWeapons(player.model);assert(!weaponContext(player.model)&&refs[address(oldExtra)]==0);
    assert(weaponContext(0x6000)==pc&&pc->extra[0]);
    forgetWeapons(0x6000);assert(!weaponContext(0x6000));
    if(rangeHolder){unref(pointer(rangeHolder));rangeHolder=0;}
    s={};L=request(0,s);assert(setWeapons(&L)==1);
    // Reproduce the reported equipment: sword's stock point 26 is removed by
    // quiver composition; the bow's stock sheath 0 gives it no stored child.
    realIDs[0]=4939;realIDs[1]=0;realIDs[2]=2507;memory[player.unit+0xD40]=0;
    melee(pointer(player.unit),0);auto lostSword=find(pointer(player.model),26);assert(lostSword);
    factory(pointer(player.model),26,"quiver","",0);assert(!refs[address(lostSword)]);
    ranged(pointer(player.unit),1);assert(!find(pointer(player.model),27));
    // Equipped fallbacks sent by Lua are routed through native draw callbacks,
    // not extra visible copies of a weapon that is also held in the hand.
    s.items[2]=4939;s.items[5]=2507;s.equipped={{4939,0,2507}};L=request(0,s);
    assert(setWeapons(&L)==1);c=weaponContext(player.model);
    assert(c&&c->routes[0]==2&&c->routes[2]==5&&!c->extra[2]&&!c->extra[5]);
    assert(findChildHook(pointer(player.model),nullptr,30)&&findChildHook(pointer(player.model),nullptr,27));
    factory(pointer(player.model),26,"quiver","",0);
    assert(findChildHook(pointer(player.model),nullptr,30)&&findChildHook(pointer(player.model),nullptr,27));
    memory[player.unit+0xD40]=1;moveWeaponHook(pointer(player.unit),nullptr,0,0);
    assert(findChildHook(pointer(player.model),nullptr,1)&&!findChildHook(pointer(player.model),nullptr,30));
    memory[player.unit+0xD40]=0;moveWeaponHook(pointer(player.unit),nullptr,0,1);
    assert(!findChildHook(pointer(player.model),nullptr,1)&&findChildHook(pointer(player.model),nullptr,30));
    memory[player.unit+0xD40]=2;ranged(pointer(player.unit),0);
    assert(findChildHook(pointer(player.model),nullptr,2)&&!findChildHook(pointer(player.model),nullptr,27));
    memory[player.unit+0xD40]=0;moveWeaponHook(pointer(player.unit),nullptr,2,1);
    assert(effectHomes[0]==30); // nested sword rebuild never inherits bow's 27
    assert(!findChildHook(pointer(player.model),nullptr,2));
    assert(findChildHook(pointer(player.model),nullptr,30)&&findChildHook(pointer(player.model),nullptr,27));
    clearChildrenHook(pointer(player.model),nullptr,26); // stock quiver disappears when stored
    assert(findChildHook(pointer(player.model),nullptr,30)&&findChildHook(pointer(player.model),nullptr,27));
    count=loads;assert(setWeapons(&L)==1&&loads==count);
    s.items[6]=quiver;L=request(0,s);assert(setWeapons(&L)==1);
    assert(c->extra[6]&&findChildHook(pointer(player.model),nullptr,30)&&findChildHook(pointer(player.model),nullptr,27));
    // Unrelated players' rebuilds do not inherit the local player's route.
    memory[0x4000+0xD8]=0x5000;memory[0x4000+0xD40]=0;
    scopedSheathPoint=27;rebuildWeaponHook(pointer(0x4000),nullptr,0);
    assert(effectHomes[0]==26&&scopedSheathPoint==27);scopedSheathPoint=-1;
    forgetWeapons(player.model);
    if(rangeHolder){unref(pointer(rangeHolder));rangeHolder=0;}
    s={};L=request(0,s);
    L.values[1]=999999;assert(setWeapons(&L)==-2);
    L=request(99,s);assert(setWeapons(&L)==-1);
    std::cout<<"PASS: native hook simulation (routing, draw reuse, callback ownership, preview isolation, detach recovery, destruction, idempotence, input validation)\n";
}
