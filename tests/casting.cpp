// Actual casting hooks, simulated client memory/callbacks. Does not validate
// the game's ABI or rendered results; see native/CASTING.md for binary audit.
#define __fastcall
#define __thiscall
#include <array>
#include <map>
#include <vector>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <cassert>
#include <iostream>
#include <limits>
#include "../native/Casting.h"
#include "../native/WeaponAssets.h"
using HasPoint=bool (*)(void*,unsigned);
using AttachChild=void (*)(void*,void*,unsigned);
struct Player {std::uintptr_t unit=0x1000,model=0x2000;std::uint64_t guid=123;unsigned display=49,native=49;};
static Player player;
static bool snapshotOK=true,childMatches=true;
static std::uintptr_t child=0x3000;
static std::map<std::uintptr_t,std::vector<unsigned char>> memory;
template<class T> static void put(std::uintptr_t at,const T& v){
    auto& bytes=memory[at];bytes.resize(sizeof(v));std::memcpy(bytes.data(),&v,sizeof(v));
}
template<class T> static bool read(std::uintptr_t at,T& v){
    const auto it=memory.find(at);if(it==memory.end()||it->second.size()!=sizeof(v))return false;
    std::memcpy(&v,it->second.data(),sizeof(v));return true;
}
static bool snapshot(Player& p){p=player;return snapshotOK;}
static std::uint64_t getPlayer(){return player.guid;}
static void* ptr(std::uintptr_t at){return reinterpret_cast<void*>(at);}
static std::uintptr_t address(const void* p){return reinterpret_cast<std::uintptr_t>(p);}
static void* findChildHook(void* parent,void*,unsigned point){return address(parent)==player.model&&point==1?ptr(child):nullptr;}
static bool weaponModelMatches(void*,const char*){return childMatches;}
static std::uintptr_t projectileDBRow(std::uintptr_t table,unsigned id){return table==0xC0D788&&id==133?0x8000:0;}
struct Lua {std::vector<double> args;int status=0;};
static bool isNumber(void* p,int index){return static_cast<Lua*>(p)->args.size()>=static_cast<unsigned>(index);}
static double toNumber(void* p,int index){return static_cast<Lua*>(p)->args.at(index-1);}
static int result(void* p,int code){static_cast<Lua*>(p)->status=code;return 1;}
#include "../native/CastingRenderer.h"
static const unsigned* receivedKit=nullptr;
static const unsigned* receivedSpell=nullptr;
static unsigned receivedPhase=0,receivedArg=0,sheathCalls=0,kitCalls=0,effectCalls=0;
static unsigned receivedPoint=0;
static std::uintptr_t receivedModel=0,receivedChild=0;
static bool simulateKit=false;
static unsigned flags=7,effectRow=99;
static void sheath(void* unit,unsigned mode,unsigned immediate,unsigned notify){
    assert(immediate==1);assert(notify==0||notify==1);++sheathCalls;put(address(unit)+0xD40,mode);
}
static void effect(void*,unsigned point,unsigned expires,unsigned spell,const unsigned* kit,const unsigned* row,unsigned* f,std::uintptr_t callback){
    assert(expires==1100&&spell==133&&row==&effectRow&&f==&flags&&callback==0x1234);
    ++effectCalls;receivedPoint=point;assert(kit==receivedKit);
}
static unsigned kit(void* unit,const unsigned* spell,const unsigned* data,unsigned phase,unsigned arg){
    ++kitCalls;receivedKit=data;receivedSpell=spell;receivedPhase=phase;receivedArg=arg;
    if(simulateKit){
        castingSheathHook(unit,nullptr,2,1,1);
        castingEffectHook(unit,nullptr,22,1100,133,data,&effectRow,&flags,0x1234);
    }
    return 77;
}
static bool hasPoint(void* model,unsigned point){receivedModel=address(model);receivedPoint=point;return point<=4||point==21||point==22;}
static float* position(void* model,float* out,unsigned point){
    receivedModel=address(model);receivedPoint=point;out[0]=10;out[1]=20;out[2]=30;return out;
}
static void attach(void* effectModel,void* parent,unsigned point){receivedChild=address(effectModel);receivedModel=address(parent);receivedPoint=point;}
static float* origin(float* out,void* unit,unsigned spell){receivedModel=address(unit);receivedPoint=spell;out[0]=1;return out;}
static unsigned call(const weaponCasting::Kit& source,unsigned phase=4,void* unit=nullptr){
    put(0x9000,source);
    return castingKitHook(unit?unit:ptr(player.unit),nullptr,reinterpret_cast<const unsigned*>(0x8000),reinterpret_cast<const unsigned*>(0x9000),phase,987);
}
static void settings(bool on=true,unsigned item=35,bool effects=true,bool swing=true){
    Lua lua{{double(on),double(effects),double(swing),double(item)}};
    assert(setCastingLua(&lua)==1&&lua.status==1);
}
int main(){
    castingKitOriginal=kit;castingEffectOriginal=effect;castingSheathOriginal=sheath;
    castingHasPointOriginal=hasPoint;castingPointOriginal=position;castingAttachOriginal=attach;castingOriginOriginal=origin;
    put(0x8000,133u);put(0x8018,0u);for(unsigned i=0;i<3;++i)put(0x8000+4*(91+i),0u);
    put(player.unit+0xD40,0u);put(child+0x1CC,player.model);put(child+0x1D0,1u);put(child+0x10,1u);
    put(child+0x30,std::uintptr_t(0x4000));put(0x4130,std::uintptr_t(0x5000));
    put(0x5104,3u);put(0x5108,std::uintptr_t(0x6000));
    for(unsigned i=0;i<3;++i){put(0x6000+48*i,i);put(0x6008+48*i,std::array<float,3>{{float(i)*.5f,0,0}});}
    weaponCasting::Kit source{};for(unsigned i=0;i<source.size();++i)source[i]=1000+i;
    source[0]=30;source[1]=0;source[2]=51;source[6]=287;source[7]=287;
    assert(call(source)==77&&receivedKit==reinterpret_cast<const unsigned*>(0x9000)&&sheathCalls==0);
    settings();simulateKit=true;
    assert(call(source)==77&&receivedKit!=reinterpret_cast<const unsigned*>(0x9000));
    assert(receivedSpell==reinterpret_cast<const unsigned*>(0x8000)&&receivedPhase==4&&receivedArg==987);
    assert(receivedKit[2]==28&&receivedKit[6]==0xffffffffu&&receivedKit[7]==287);
    for(unsigned i=0;i<35;++i)if(i!=2&&i!=6&&i!=7)assert(receivedKit[i]==source[i]);
    weaponCasting::Kit untouched{};assert(read(0x9000,untouched)&&untouched==source);
    assert(receivedPoint==weaponCasting::marker+22&&effectCalls==1);
    unsigned mode=0;assert(read(player.unit+0xD40,mode)&&mode==1&&castingScope==0);
    // Effect callbacks retain these pointers after kit dispatch returns.
    const auto* retained=receivedKit;call(source);assert(receivedKit==retained&&retained[7]==287);
    simulateKit=false;
    assert(castingHasPointHook(ptr(player.model),nullptr,weaponCasting::marker+22));
    float output[3]{};assert(castingPointHook(ptr(player.model),nullptr,output,weaponCasting::marker+22)==output);
    assert(receivedModel==child&&receivedPoint==2&&output[2]==30);
    castingAttachHook(ptr(0xABCD),nullptr,ptr(player.model),weaponCasting::marker+22);
    assert(receivedChild==0xABCD&&receivedModel==child&&receivedPoint==2);
    assert(castingOriginHook(output,ptr(player.unit),133)==output&&receivedModel==child&&receivedPoint==2);
    // Unrelated attachments, units, targets and missile data are never rewritten.
    castingPointHook(ptr(0x9999),nullptr,output,2);assert(receivedModel==0x9999&&receivedPoint==2);
    castingPointHook(ptr(0x9999),nullptr,output,weaponCasting::marker+21);assert(receivedModel==0x9999&&receivedPoint==21);
    castingOriginHook(output,ptr(0x9999),133);assert(receivedModel==0x9999&&receivedPoint==133);
    const auto before=sheathCalls;call(source,4,ptr(0x9999));assert(receivedKit==reinterpret_cast<const unsigned*>(0x9000)&&sheathCalls==before);
    for(unsigned phase:{1,2,3,5}){call(source,phase);assert(receivedKit==reinterpret_cast<const unsigned*>(0x9000));}
    auto target=source;target[1]=1;call(target);assert(receivedKit==reinterpret_cast<const unsigned*>(0x9000));
    auto invalid=source;invalid[2]=16;call(invalid);assert(receivedKit==reinterpret_cast<const unsigned*>(0x9000));
    for(unsigned attributes:{2u,0x40u}){put(0x8018,attributes);call(source);assert(receivedKit==reinterpret_cast<const unsigned*>(0x9000));}
    put(0x8018,0u);put(0x8000+91*4,78u);call(source);assert(receivedKit==reinterpret_cast<const unsigned*>(0x9000));put(0x8000+91*4,0u);
    player.display=1;call(source);assert(receivedKit==reinterpret_cast<const unsigned*>(0x9000));player.display=49;
    childMatches=false;call(source);assert(receivedKit==reinterpret_cast<const unsigned*>(0x9000));childMatches=true;
    put(child+0x1CC,std::uintptr_t(0x9999));call(source);assert(receivedKit==reinterpret_cast<const unsigned*>(0x9000));put(child+0x1CC,player.model);
    put(child+0x10,0u);call(source);assert(receivedKit==reinterpret_cast<const unsigned*>(0x9000));put(child+0x10,1u);
    put(0x5104,65u);call(source);assert(receivedKit==reinterpret_cast<const unsigned*>(0x9000));put(0x5104,3u);
    // Each animation family preserves every other kit field.
    source[0]=38;source[2]=53;settings(true,25);call(source);assert(receivedKit[2]==17);
    settings(true,647);call(source);assert(receivedKit[2]==18);
    settings(true,35);call(source);assert(receivedKit[2]==19);
    source[0]=64;source[2]=124;call(source);assert(receivedKit[2]==28);
    settings(true,35,false,false);call(source);assert(std::equal(source.begin(),source.end(),receivedKit));
    castingOriginHook(output,ptr(player.unit),133);assert(receivedModel==player.unit&&receivedPoint==133);
    // Disable restores the original visual sheath and safely resolves markers
    // still present on effects that the native client has not expired yet.
    settings(false);assert(read(player.unit+0xD40,mode)&&mode==0&&!castingDrewWeapon);
    castingPointHook(ptr(player.model),nullptr,output,weaponCasting::marker+22);assert(receivedModel==player.model&&receivedPoint==22);
    castingAttachHook(ptr(0xABCD),nullptr,ptr(player.model),weaponCasting::marker+22);assert(receivedModel==player.model&&receivedPoint==22);
    const auto disabledCalls=sheathCalls;settings(false);assert(sheathCalls==disabledCalls);
    // A failed first cast must not leave the real weapon drawn. If the player
    // switches to ranged after casting, disabling must respect that choice.
    settings();childMatches=false;call(source);assert(read(player.unit+0xD40,mode)&&mode==0&&!castingDrewWeapon);
    childMatches=true;call(source);put(player.unit+0xD40,2u);settings(false);
    assert(read(player.unit+0xD40,mode)&&mode==2&&!castingDrewWeapon);
    settings(true,0);call(source);assert(receivedKit==reinterpret_cast<const unsigned*>(0x9000));
    Lua bad{{1,1,1,999999}};setCastingLua(&bad);assert(bad.status==-2);
    bad.args={1,1};setCastingLua(&bad);assert(bad.status==-2);
    bad.args={1,1,1,std::numeric_limits<double>::quiet_NaN()};setCastingLua(&bad);assert(bad.status==-2);
    bad.args={1,1,1,35.5};setCastingLua(&bad);assert(bad.status==-2);
    snapshotOK=false;bad.args={1,1,1,35};setCastingLua(&bad);assert(bad.status==-1);snapshotOK=true;
    assert(retained[2]==28&&retained[7]==287);
    std::cout<<"PASS: casting ownership, immutable/retained kits, poses, weapon effects/origin, exclusions, fallback, disable and bridge validation\n";
}
