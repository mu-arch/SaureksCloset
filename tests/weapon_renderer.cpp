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
}
static Lua request(unsigned token,const WeaponSelection& s){
    Lua L;L.values.push_back(token);for(auto v:s.items)L.values.push_back(v);for(auto v:s.equipped)L.values.push_back(v);return L;
}
int main(){
    sheathPointOriginal=&sheath;weaponComposeOriginal=&compose;moveWeaponOriginal=&move;findChildOriginal=&find;clearChildrenOriginal=&clear;
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
    L.values[1]=999999;assert(setWeapons(&L)==-2);
    L=request(99,s);assert(setWeapons(&L)==-1);
    std::cout<<"PASS: native hook simulation (routing, draw reuse, callback ownership, preview isolation, detach recovery, destruction, idempotence, input validation)\n";
}
