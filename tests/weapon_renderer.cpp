// Exercise the actual renderer hooks with an in-memory native-object simulator.
// This tests ownership/routing, not the game's ABI or rendered appearance.
#define __fastcall
#define __thiscall
#define SAUREKS_WEAPON_TEST
#include <cassert>
#include <cmath>
#include <cstdint>
#include <map>
#include <limits>
#include <vector>
#include <string>
#include <iostream>
#include <type_traits>
#include "../native/PreviewState.h"
#include "../native/WeaponState.h"
#include "bow_attachment_fixtures.h"
#include "bag_hip_fixtures.h"
#include "quiver_attachment_fixtures.h"
#include "sword_attachment_fixtures.h"
using DestroyModel=void (*)(void*);
static std::map<std::uintptr_t,std::uint64_t> memory;
static std::map<std::uintptr_t,int> refs;
static unsigned loads=0,meleeRefreshes=0,rangedRefreshes=0;
static bool factoryModelsLoaded=true;
static unsigned bagTestTime=1000;
static unsigned bagClockMilliseconds(){return bagTestTime;}
static std::uintptr_t nextModel=0x100000,rangeHolder=0;
static void forgetWeapons(std::uintptr_t);
static void* pointer(std::uintptr_t p){return reinterpret_cast<void*>(p);}
static std::uintptr_t address(void* p){return reinterpret_cast<std::uintptr_t>(p);}
static void ref(void* p){assert(refs[address(p)]>0);++refs[address(p)];}
static void unref(void* p){assert(refs[address(p)]>0);if(--refs[address(p)]==0)forgetWeapons(address(p));}
static const auto releaseModel=&unref;
template<typename T> static bool read(std::uintptr_t a,T& out){auto i=memory.find(a);if(i==memory.end())return false;out=static_cast<T>(i->second);return true;}
static std::map<std::uintptr_t,std::array<float,16>> matrices;
static bool writeBagResponseMatrices(std::uintptr_t address,const std::array<std::array<float,16>,61>& values){
    for(unsigned i=0;i<values.size();++i)if(!matrices.count(address+64*i))return false;
    for(unsigned i=0;i<values.size();++i)matrices[address+64*i]=values[i];
    return true;
}
static bool read(std::uintptr_t a,std::array<float,16>& out){auto i=matrices.find(a);if(i==matrices.end())return false;out=i->second;return true;}
static std::map<std::uintptr_t,std::array<float,3>> positions;
static bool read(std::uintptr_t a,std::array<float,3>& out){auto i=positions.find(a);if(i==positions.end())return false;out=i->second;return true;}
static std::map<std::uintptr_t,float> scalars;
static bool read(std::uintptr_t a,float& out){auto i=scalars.find(a);if(i==scalars.end())return false;out=i->second;return true;}
static std::map<std::uintptr_t,std::array<char,260>> resourceNames;
static bool read(std::uintptr_t a,std::array<char,260>& out){auto i=resourceNames.find(a);if(i==resourceNames.end())return false;out=i->second;return true;}
static std::map<std::string,std::uintptr_t> modelResources;
static void modelName(void* child,const char* filename){
    std::string name=filename;
    const auto dot=name.rfind('.');if(dot!=std::string::npos)name.resize(dot);
    auto& resource=modelResources[name];if(!resource)resource=0xE00000+0x1000*modelResources.size();
    std::array<char,260> buffer{};assert(name.size()<buffer.size());
    for(unsigned i=0;i<name.size();++i)buffer[i]=name[i];
    resourceNames[resource+0x20]=buffer;
    memory[address(child)+0x30]=resource;memory[address(child)+0x10]=1;
}
struct Player {std::uintptr_t model=0,unit=0;std::uint64_t guid=0;unsigned display=0,native=0;std::uintptr_t fields=0,component=0;unsigned identity=0,body=0,facial=0;};
static Player player{0x1000,0x2000,123,49,49};
static PreviewRegistry previews;
static bool snapshot(Player& p){p=player;return p.guid!=0;}
static std::uint64_t getPlayer(){return player.guid;}
struct Lua {std::vector<double> values;};
static bool isNumber(void* L,int i){return static_cast<unsigned>(i)<=static_cast<Lua*>(L)->values.size();}
static double toNumber(void* L,int i){return static_cast<Lua*>(L)->values[i-1];}
static std::vector<double> luaOutput;
static void pushNumber(void*,double value){luaOutput.push_back(value);}
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
struct SequenceCall { void* model;unsigned animation,time,blend;bool full; };
static std::vector<SequenceCall> sequenceCalls;
static void sequenceTime(void* model,int key,unsigned animation,int variation,unsigned time,float speed,unsigned blend,unsigned primary){
    assert(key==-1&&variation==0&&speed==1.f&&blend<=1&&primary==1);
    sequenceCalls.push_back({model,animation,time,blend,true});
    std::uintptr_t state=0;
    if(read(address(model)+0x90,state)&&state){
        std::uintptr_t resource=0,header=0,lookup=0;unsigned index=animation==5?1:0;
        if(read(address(model)+0x30,resource)&&read(resource+0x130,header)&&read(header+0x28,lookup))read(lookup+animation*2,index);
        memory[state+0xF8]=animation;memory[state+0xA4]=index;
    }
}
static void sequenceOffset(void* model,int key,unsigned time){
    std::uintptr_t state=0;unsigned animation=0;
    assert(key==-1&&read(address(model)+0x90,state)&&state&&read(state+0xF8,animation));
    sequenceCalls.push_back({model,animation,time,0,false});
}
template<typename T> static T weaponFunction(std::uintptr_t a){
    if constexpr(std::is_same_v<T,decltype(&ref)>){
        if(a==0x710390)return &ref;
        if(a==0x713020)return &detach;
    }else if constexpr(std::is_same_v<T,decltype(&attach)>){if(a==0x712F70)return &attach;}
    else if constexpr(std::is_same_v<T,decltype(&supports)>){if(a==0x712CB0)return &supports;}
    else if constexpr(std::is_same_v<T,decltype(&factory)>){if(a==0x4798C0)return &factory;}
    else if constexpr(std::is_same_v<T,decltype(&melee)>){if(a==0x605DA0)return &melee;if(a==0x611E10)return &ranged;}
    else if constexpr(std::is_same_v<T,decltype(&sequenceTime)>){if(a==0x7121A0)return &sequenceTime;}
    else if constexpr(std::is_same_v<T,decltype(&sequenceOffset)>){if(a==0x7127F0)return &sequenceOffset;}
    assert(false);return nullptr;
}
#ifdef SAUREKS_SHARING_TEST
#include "sharing_weapon_fixture.h"
#endif
#include "../native/WeaponRenderer.h"
static unsigned lastBagGeneration=0;
static int setBagsStatus(void* L){
    luaOutput.clear();const auto results=setBags(L);
    if(results!=2)return results; // The test result() shim returns error statuses directly.
    assert(luaOutput.size()==2);lastBagGeneration=static_cast<unsigned>(luaOutput[1]);
    return static_cast<int>(luaOutput[0]);
}
static unsigned realIDs[3]={35,0,0};
static std::array<std::array<unsigned char,8>,3> realInfo{};
static unsigned serverRangedAppearance=0;
static const unsigned char* info(void*,unsigned role,unsigned){
    const auto* a=role<3?weaponAsset(realIDs[role]):nullptr;
    if(!a&&role==2&&serverRangedAppearance)a=weaponAsset(serverRangedAppearance);
    if(!a)return nullptr;
    realInfo[role]={{2,static_cast<unsigned char>(a->subclass),7,static_cast<unsigned char>(a->inventory),static_cast<unsigned char>(a->sheath),9,10,11}};
    return realInfo[role].data();
}
static const WeaponAsset* visualWeapon(unsigned role,std::uintptr_t caller){
    if(!weaponInfoOriginal)return weaponAsset(realIDs[role]);
    const auto* result=weaponInfoAt(pointer(player.unit),role,0,caller);
    const auto* c=weaponContext(player.model);
    if(c&&result==c->rangedInfo.data())return weaponAsset(c->selection.items[c->routes[2]]);
    return weaponAsset(realIDs[role]);
}
static int sheath(unsigned type,unsigned side){
    switch(type){case 1:return side?26:27;case 2:return side?30:31;case 3:return side?32:33;case 4:return 28;}return -1;
}
static void* find(void* par,unsigned point){
    for(auto child=memory[address(par)+0x1DC];child;child=memory[child+0x1E4])if(memory[child+0x1D0]==point)return pointer(child);
    return nullptr;
}
static void clear(void* par,unsigned point){while(auto p=find(par,point))detach(p);}
static void factory(void* parent,unsigned point,const char* filename,const char*,unsigned){
    clearChildrenHook(parent,nullptr,point);
    auto child=nextModel;nextModel+=0x1000;refs[child]=1;memory[child+0x1CC]=0;
    attach(pointer(child),parent,point);unref(pointer(child));++loads;
    modelName(pointer(child),filename);
    if(!factoryModelsLoaded)memory[child+0x10]=0;
    matrices[child+0xBC]={{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1}};
}
static int compose(void* parent,void* display,unsigned slot,unsigned type,unsigned stored,unsigned shield,unsigned right){
    if(!parent||!display)return -1;
    auto home=sheathPointHook(type,slot==15||(slot==17&&right));
    unsigned hand=slot==15||right?1:(shield?0:2);
    clearChildrenHook(parent,nullptr,hand);if(home>=0)clearChildrenHook(parent,nullptr,home);
    int point=stored?home:static_cast<int>(hand);if(point<0)return -1;
    const char* filename="";
    for(const auto& asset:weaponAssets)if(asset.display==memory[address(display)]){filename=asset.model;break;}
    factory(parent,point,filename,"",0);return point;
}
static void melee(void* unit,unsigned role){
    ++meleeRefreshes;auto* a=weaponAsset(realIDs[role]);if(!a)return;
    weaponComposeHook(pointer(memory[address(unit)+0xD8]),weaponDisplay(a),15+role,a->sheath,memory[address(unit)+0xD40]==0,a->kind==3,0);
}
static void ranged(void* unit,unsigned stored){
    ++rangedRefreshes;
    if(rangeHolder){unref(pointer(rangeHolder));rangeHolder=0;}
    auto* a=visualWeapon(2,0x611E24);if(!a)return;
    auto* parent=pointer(memory[address(unit)+0xD8]);
    if(!stored){clearChildrenHook(parent,nullptr,0);clearChildrenHook(parent,nullptr,1);clearChildrenHook(parent,nullptr,2);}
    const int point=weaponComposeHook(parent,weaponDisplay(a),17,a->sheath,stored,0,a->inventory==25||a->inventory==26);
    if(point>=0){auto p=findChildHook(parent,nullptr,point);if(p){rangeHolder=address(p);ref(p);}}
}
static void move(void* unit,unsigned role,unsigned stored){
    auto* a=visualWeapon(role,0x60B5BB);if(!a)return;
    auto* parent=pointer(memory[address(unit)+0xD8]);
    const bool right=role==0||(role==2&&(a->inventory==25||a->inventory==26));
    const unsigned hand=right?1:(a->kind==3?0:2);
    const int home=sheathPointHook(a->sheath,right);
    auto p=findChildHook(parent,nullptr,stored?hand:static_cast<unsigned>(home));
    if(p){
        ref(p);detach(p);
        if(!stored||home>=0)attach(p,parent,stored?static_cast<unsigned>(home):hand);
        unref(p);
        if(role==2&&stored){rebuildWeaponHook(unit,nullptr,0);rebuildWeaponHook(unit,nullptr,1);}
    }else rebuildWeaponHook(unit,nullptr,role);
}
static int effectHomes[3]={-1,-1,-1};
static void rebuild(void* unit,unsigned role){
    const auto* a=visualWeapon(role,0x60B797);if(!a)return;
    const bool right=role==0||(role==2&&(a->inventory==25||a->inventory==26));
    effectHomes[role]=sheathPointHook(a->sheath,right);
    const unsigned mode=memory[address(unit)+0xD40];
    // 60B992 only rebuilds ranged while drawn, through 611E10. A generic
    // rebuild cannot restore a ranged weapon deleted after it was put away.
    if(role==2){if(mode==2)ranged(unit,0);return;}
    const bool stored=mode==0||(mode==2&&role!=2);
    auto* parent=pointer(memory[address(unit)+0xD8]);
    // 60B907 retains a drawn ranged child while rebuilding this melee slot.
    auto* held=mode==2?findChildHook(parent,nullptr,role==0?1:2):nullptr;
    if(held){ref(held);detach(held);}
    weaponComposeHook(parent,weaponDisplay(a),15+role,a->sheath,stored,a->kind==3,right);
    if(held){attach(held,parent,role==0?1:2);unref(held);}
}
static void sheathTransition(void* unit){
    // Native 611770 uses D3C as the prior mode and D40 as the requested mode.
    // Its caller copies D40 to D3C only after this routine returns.
    const auto base=address(unit);
    const unsigned oldMode=memory[base+0xD3C],mode=memory[base+0xD40];
    auto* parent=pointer(memory[base+0xD8]);
    if(mode==2){
        if(oldMode==1){moveWeaponHook(unit,nullptr,0,1);moveWeaponHook(unit,nullptr,1,1);}
        ranged(unit,0);
    }else if(mode==1){
        if(oldMode==2)ranged(unit,1);
        moveWeaponHook(unit,nullptr,0,0);moveWeaponHook(unit,nullptr,1,0);
    }else if(mode==0&&oldMode==1){
        moveWeaponHook(unit,nullptr,0,1);moveWeaponHook(unit,nullptr,1,1);
    }else if(mode==0&&oldMode==2){
        // NPC talking and loot crouching take this immediate clear-only path.
        // The child's D24 reference survives; no move, rebuild or ranged
        // refresh occurs, even for a bow with no stock sheath point.
        clearChildrenHook(parent,nullptr,35);
        const auto* a=visualWeapon(2,0x61183B);
        if(a)clearChildrenHook(parent,nullptr,a->inventory==25||a->inventory==26?1:2);
    }
}
struct AttachmentUpdate {
    void* model=nullptr;const float* matrix=nullptr;const float* color=nullptr;const float* lighting=nullptr;
    float alpha=0;unsigned calls=0;
};
static AttachmentUpdate attachmentUpdate;
static bool captureAttachmentMatrix=false;
static bool evaluateAttachmentBones=false;
static std::array<float,16> capturedAttachmentMatrix;
static void observeAttachment(void* model,const float* matrix,const float* color,const float* lighting,float alpha){
    attachmentUpdate={model,matrix,color,lighting,alpha,attachmentUpdate.calls+1};
    if(captureAttachmentMatrix)for(unsigned i=0;i<16;++i)capturedAttachmentMatrix[i]=matrix[i];
    if(evaluateAttachmentBones){
        BagMatrix attachment;for(unsigned i=0;i<16;++i)attachment[i]=matrix[i];
        const auto base=address(model);const auto pose=bagMatrixProduct(attachment,matrices.at(base+0xBC));
        matrices[base+0xFC]=pose;
        // Build 5875's update writes each untracked control from the root pose.
        // A later hook must replace this before either skinning route reads it.
        for(unsigned i=0;i<61;++i)matrices[memory.at(base+0x94)+64*i]=pose;
    }
}
struct BowStringSubmission {void* model=nullptr;void* renderState=nullptr;void* unit=nullptr;unsigned calls=0;};
static BowStringSubmission bowStringSubmission;
static void observeBowString(void* model,void* renderState,void* unit){
    bowStringSubmission={model,renderState,unit,bowStringSubmission.calls+1};
}
static Lua request(unsigned token,const WeaponSelection& s,int quiverHorizontal=-1,int hideRanged=-1,int hideMelee=-1,int actualQuiver=-1,int backBag=-1){
    Lua L;L.values.push_back(token);for(unsigned i=0;i<7;++i)L.values.push_back(s.items[i]);for(auto v:s.equipped)L.values.push_back(v);
    const int optional[]={quiverHorizontal,hideRanged,hideMelee,actualQuiver,backBag};
    for(unsigned i=0;i<5;++i){
        bool later=false;for(unsigned j=i;j<5;++j)if(optional[j]>=0)later=true;
        if(later)L.values.push_back(optional[i]<0?0:optional[i]);
    }
    if(s.independent){
        L.values.resize(16,0);
        for(unsigned i=7;i<10;++i)L.values.push_back(s.items[i]);
        L.values.push_back(1);
    }
    if(s.carriedMode>=0){
        L.values.resize(21,0);
        for(unsigned i=7;i<10;++i)L.values[9+i]=s.items[i];
        L.values[19]=s.independent?1:0;
        L.values.push_back(s.carriedMode);
    }
    if(s.stowedMask>=0){L.values.resize(23,0);L.values[22]=s.stowedMask;}
    return L;
}
int main(){
    {
        // Exercise the actual Lua bridge, including validation before narrowing
        // numeric values and the four-argument clear operation after reload.
        for(unsigned race=1;race<=8;++race)for(unsigned sex=0;sex<2;++sex){
            Lua defaults{{1.,double(race),double(sex)}};luaOutput.clear();
            assert(getBagFitDefaults(&defaults)==8&&luaOutput.size()==8&&luaOutput[0]==1);
            BagTuningValues expected;assert(bagTuningDefaults(1,race,sex,expected));
            assert(luaOutput[1]==expected.left&&luaOutput[2]==expected.inset&&luaOutput[3]==expected.up&&
                luaOutput[4]==expected.pitch&&luaOutput[5]==expected.roll&&luaOutput[6]==0&&luaOutput[7]==85);
        }
        Lua valid{{1,2,0,1,.15,.05,-.02,16,14,8,90,1}};
        const auto loggedInGuid=player.guid;
        player.guid=0;
        const auto beforeLoginOwner=bagTuningOwner;
        const auto beforeLoginRevision=bagTuningEntries[2].revision;
        assert(setBagFit(&valid)==-1&&!bagTuningEntries[2].enabled&&
            bagTuningEntries[2].revision==beforeLoginRevision&&bagTuningOwner==beforeLoginOwner);
        auto invalidBeforeLogin=valid;invalidBeforeLogin.values[4]=2;
        assert(setBagFit(&invalidBeforeLogin)==-2);
        Lua clearBeforeLogin{{1,2,0,0}};assert(setBagFit(&clearBeforeLogin)==1);
        player.guid=loggedInGuid;
        assert(setBagFit(&valid)==1&&bagTuningEntries[2].enabled);
        const auto revision=bagTuningEntries[2].revision;
        assert(setBagFit(&valid)==1&&bagTuningEntries[2].revision==revision);
        for(unsigned index=0;index<12;++index){
            for(double value:{std::numeric_limits<double>::quiet_NaN(),std::numeric_limits<double>::infinity(),-std::numeric_limits<double>::infinity()}){
                auto invalid=valid;invalid.values[index]=value;
                assert(setBagFit(&invalid)==-2&&bagTuningEntries[2].revision==revision);
            }
            auto missing=valid;missing.values.resize(index);
            assert(setBagFit(&missing)==-2&&bagTuningEntries[2].revision==revision);
        }
        for(unsigned index=0;index<4;++index){
            auto invalid=valid;invalid.values[index]=.5;
            assert(setBagFit(&invalid)==-2);
        }
        for(unsigned index=4;index<11;++index){
            const double minimum=index==6?-3:(index<7?-1:(index<10?-180:25)),maximum=index<7?1:(index<10?180:200);
            for(double value:{std::nextafter(minimum,-std::numeric_limits<double>::infinity()),
                              std::nextafter(maximum,std::numeric_limits<double>::infinity())}){
                auto invalid=valid;invalid.values[index]=value;assert(setBagFit(&invalid)==-2);
            }
        }
        for(double value:{-.1,.5,2.}){auto invalid=valid;invalid.values[11]=value;assert(setBagFit(&invalid)==-2);}
        auto footFit=valid;footFit.values[6]=-3;
        assert(setBagFit(&footFit)==1&&bagTuningEntries[2].values.up==-3);
        footFit.values[0]=101;assert(setBagFit(&footFit)==-2); // Weapon limits remain unchanged.
        for(auto key:std::vector<std::vector<double>>{{0,2,0},{2,2,0},{1,0,0},{1,9,0},{1,2,2},{1,2,-1},{1,.5,0}}){
            Lua invalid{key};assert(getBagFitDefaults(&invalid)==-2);
            invalid.values.push_back(0);assert(setBagFit(&invalid)==-2);
        }
        Lua clear{{1,2,0,0}};assert(setBagFit(&clear)==1&&!bagTuningEntries[2].enabled);
        const auto cleared=bagTuningEntries[2].revision;
        assert(setBagFit(&clear)==1&&bagTuningEntries[2].revision==cleared);
        // Login ownership is checked before drawing as well as on API writes.
        assert(setBagFit(&valid)==1);
        bagTuningUseOwner(player.guid+1);assert(!bagTuningEntries[2].enabled);
        bagTuningUseOwner(player.guid);
    }
    (void)&updateAttachedHook;
    sheathPointOriginal=&sheath;weaponComposeOriginal=&compose;moveWeaponOriginal=&move;findChildOriginal=&find;clearChildrenOriginal=&clear;
    rebuildWeaponOriginal=&rebuild;
    sheathTransitionOriginal=&sheathTransition;updateAttachedOriginal=&observeAttachment;
    bowStringDrawOriginal=&observeBowString;
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
    // A stock clone carries both native hand/sheath children and addon-owned
    // storage children, but the copied children have no preview ownership entry.
    std::vector<std::uintptr_t> inherited;
    for(unsigned point:{0u,1u,2u,26u,27u,28u,29u,30u,31u,32u,33u}){
        factory(pointer(0x6000),point,"","",0);
        inherited.push_back(address(find(pointer(0x6000),point)));
    }
    factory(pointer(0x6000),5,"","",0);
    const auto ornament=find(pointer(0x6000),5);
    const auto worldHead=memory[player.model+0x1DC];const int worldStaffRefs=refs[address(staff)];
    discardInheritedPreviewWeapons(player.model); // Never operate on the world.
    assert(memory[player.model+0x1DC]==worldHead&&refs[address(staff)]==worldStaffRefs);
    discardInheritedPreviewWeapons(0x6000);
    for(auto child:inherited)assert(!refs[child]);
    assert(find(pointer(0x6000),5)==ornament&&refs[address(ornament)]==1);
    assert(memory[player.model+0x1DC]==worldHead&&refs[address(staff)]==worldStaffRefs);
    // An unexpected shared source-list pointer must not detach world children.
    previews.entries[1].model=0x7000;previews.entries[1].guid=player.guid;
    memory[0x71DC]=worldHead;
    discardInheritedPreviewWeapons(0x7000);
    assert(memory[player.model+0x1DC]==worldHead&&refs[address(staff)]==worldStaffRefs);
    previews.entries[1]={};memory[0x71DC]=0;
    auto preview=request(8,s);assert(setWeapons(&preview)==1);auto* pc=weaponContext(0x6000);assert(pc);
    for(unsigned i=0;i<7;i++)assert(pc->extra[i]&&pc->extra[i]!=c->extra[i]);
    discardInheritedPreviewWeapons(0x6000); // Repeated cleanup preserves owned extras.
    for(unsigned i=0;i<7;i++){
        unsigned children=0;
        for(auto child=memory[0x61DC];child;child=memory[child+0x1E4])
            if(memory[child+0x1D0]==weaponPoints[i])++children;
        assert(children==1&&refs[address(pc->extra[i])]==2);
    }

    // Use each race/gender's actual authored anchor, with independent world
    // and preview model data. Animated bone matrices include scale/orientation.
    const std::array<float,16> identity{{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1}};
    matrices[0xA00000]={{0,0,1,0,1,0,0,0,0,1,0,0,20,30,40,1}};
    const auto input=matrices[0xA00000];
    const auto* attachment=reinterpret_cast<const float*>(0xA00000);
    std::array<float,16> shifted;
    // Point-26 bone rotations read from all 16 installed build-5875 character
    // models. Quiver_A's mesh runs along X; its thin Y axis faces out of the back.
    const std::array<float,4> quiverRotations[]={
        {{-0.329552650f,0.365415573f,0.627129555f,0.603800476f}}, // Human Male
        {{-0.415333271f,0.381701469f,0.590476036f,0.577183068f}}, // Human Female
        {{-0.399986267f,0.307284355f,0.588998795f,0.631401420f}}, // Orc Male
        {{-0.385741711f,0.404605865f,0.612889290f,0.558447957f}}, // Orc Female
        {{-0.304881573f,0.388266563f,0.644009590f,0.584421039f}}, // Dwarf Male
        {{-0.329553127f,0.365415573f,0.627128601f,0.603801191f}}, // Dwarf Female
        {{-0.357254982f,0.388986588f,0.573322296f,0.626386344f}}, // NightElf Male
        {{-0.342073441f,0.391391754f,0.629943848f,0.577034652f}}, // NightElf Female
        {{-0.356030464f,0.382049561f,0.686529160f,0.505923092f}}, // Scourge Male
        {{-0.422152519f,0.339923859f,0.567375183f,0.619939029f}}, // Scourge Female
        {{-0.335517883f,0.363302231f,0.626076698f,0.602882445f}}, // Tauren Male
        {{-0.291043282f,0.360037804f,0.690954208f,0.555201650f}}, // Tauren Female
        {{-0.322676182f,0.382793427f,0.624661446f,0.599289060f}}, // Gnome Male
        {{-0.277092934f,0.519509315f,0.639954567f,0.493748665f}}, // Gnome Female
        {{-0.442594528f,0.366827965f,0.551461220f,0.604514539f}}, // Troll Male
        {{-0.371366501f,0.379602432f,0.661606789f,0.529400945f}}, // Troll Female
    };
    // The visible center is the midpoint of the actual mesh bounds, not its
    // offset model origin. Compare the composed rendered center to back point 28.
    const std::array<float,3> meshCenter{{.3518670499f,.0128780054f,.0149712414f}};
    const auto transformPoint=[](const std::array<float,16>& m,const std::array<float,3>& p){
        std::array<float,3> out;
        for(unsigned i=0;i<3;++i)out[i]=m[12+i]+p[0]*m[i]+p[1]*m[4+i]+p[2]*m[8+i];
        return out;
    };
    const auto setBack=[&](WeaponContext* context,const BowAttachmentFixture& fixture){
        const auto base=context==c?0x2000000u:0x3000000u;
        matrices[context->parent+0xFC]=identity;
        memory[context->parent+0x2C]=base+0xA000;matrices[base+0xA000+0x9C]=identity;
        memory[context->parent+0x30]=base;memory[base+0x130]=base+0x1000;
        memory[base+0x1000+0x10C]=37;memory[base+0x1000+0x110]=base+0x2000;
        memory[base+0x2000+2*28]=fixture.index;
        memory[base+0x1000+0x104]=34;memory[base+0x1000+0x108]=base+0x3000;
        const auto record=base+0x3000+48*fixture.index;
        memory[record]=28;memory[record+4]=fixture.bone;positions[record+8]=fixture.position;
        memory[base+0x1000+0x34]=128;memory[context->parent+0x94]=base+0x5000;
        memory[base+0x1000+0x38]=base+0x7000;
        const auto torso=quiverBackParents[2*(fixture.race-1)+fixture.sex];
        memory[base+0x7000+108*fixture.bone+8]=torso;
        matrices[base+0x5000+64*torso]=identity;
        const auto boneAddress=base+0x5000+64*fixture.bone;
        matrices[boneAddress]=identity;
        return boneAddress;
    };
    const auto assertCentered=[&](WeaponContext* context,const std::array<float,16>& adjusted,
        const BowAttachmentFixture& fixture,std::uintptr_t bone){
        const auto localCenter=transformPoint(matrices[address(context->extra[6])+0xBC],meshCenter);
        const auto renderedCenter=transformPoint(adjusted,localCenter);
        const auto back=transformPoint(matrices[bone],fixture.position);
        const auto base=context==c?0x2000000u:0x3000000u;
        const auto torso=quiverBackParents[2*(fixture.race-1)+fixture.sex];
        const auto& torsoFrame=matrices[base+0x5000+64*torso];
        for(unsigned i=0;i<3;++i){
            const float expected=back[i]-.105362409f*torsoFrame[4+i];
            assert(std::fabs(renderedCenter[i]-expected)<.00001f);
        }
    };
    const unsigned beforeAngleLoads=loads,beforeAngleMelee=meleeRefreshes,beforeAngleRanged=rangedRefreshes;
    const auto angleRefs=refs;
    for(auto context:{c,pc}){
        const auto children=context->extra;
        auto horizontal=request(context->token,s,1);
        matrices[address(context->extra[6])+0xBC]=identity;
        auto backBone=setBack(context,bowFixtures[0]);
        assert(!context->quiverHorizontal&&positionStoredQuiver(context->extra[6],attachment,shifted));
        for(unsigned i=0;i<12;++i)assert(shifted[i]==input[i]);
        assertCentered(context,shifted,bowFixtures[0],backBone);
        assert(setWeapons(&horizontal)==1&&context->quiverHorizontal);
        assert(context->extra==children&&refs==angleRefs);
        assert(loads==beforeAngleLoads&&meleeRefreshes==beforeAngleMelee&&rangedRefreshes==beforeAngleRanged);
        for(unsigned i=0;i<6;++i){
            void* child=context->extra[i];
            if(!child)child=findChildOriginal(pointer(context->parent),weaponPoints[i]);
            assert(child&&!positionStoredQuiver(child,attachment,shifted));
        }
        unsigned fixtureIndex=0;
        for(const auto& q:quiverRotations){
            const auto& fixture=bowFixtures[fixtureIndex++];
            backBone=setBack(context,fixture);
            const float x=q[0],y=q[1],z=q[2],w=q[3];
            const std::array<float,16> authored{{
                1-2*(y*y+z*z),2*(x*y+z*w),2*(x*z-y*w),0,
                2*(x*y-z*w),1-2*(x*x+z*z),2*(y*z+x*w),0,
                2*(x*z+y*w),2*(y*z-x*w),1-2*(x*x+y*y),0,
                7,-8,9,1}};
            assert(authored[4]<-.8f); // outward +Y is toward a rear viewer (-X)
            matrices[0xA00000]=authored;
            assert(positionStoredQuiver(context->extra[6],attachment,shifted));
            // Opposite sign from the prior build, as requested after in-game
            // testing. The center stays on the back instead of orbiting a shoulder.
            assert(-authored[1]*shifted[2]+authored[2]*shifted[1]>.5f);
            assertCentered(context,shifted,fixture,backBone);
            const float angleCos=authored[0]*shifted[0]+authored[1]*shifted[1]+authored[2]*shifted[2];
            assert(std::fabs(angleCos-.70710678f)<.00002f);
            for(float scale:{.55f,1.f,1.3f}){
                auto transform=authored;
                for(unsigned i=0;i<12;++i)transform[i]*=scale;
                // A further parent animation turns the model and mixes axes;
                // nonuniform scaling exercises preservation of actual lengths.
                for(unsigned column:{0u,4u,8u}){
                    const float oldX=transform[column],oldY=transform[column+1];
                    transform[column]=-oldY;
                    transform[column+1]=oldX;
                    transform[column+2]*=2;
                }
                matrices[0xA00000]=transform;
                assert(positionStoredQuiver(context->extra[6],attachment,shifted));
                for(unsigned i:{3u,4u,5u,6u,7u,11u,15u})assert(shifted[i]==transform[i]);
                assertCentered(context,shifted,fixture,backBone);
                // A proper rotation preserves the complete basis Gram matrix,
                // including nonuniform scale and nonorthogonal animated axes.
                for(unsigned left:{0u,4u,8u})for(unsigned right:{0u,4u,8u}){
                    float before=0,after=0;
                    for(unsigned i=0;i<3;++i){before+=transform[left+i]*transform[right+i];after+=shifted[left+i]*shifted[right+i];}
                    assert(std::fabs(before-after)<.00001f);
                }
                const auto once=shifted;
                for(unsigned frame=0;frame<10;++frame)assert(positionStoredQuiver(context->extra[6],attachment,shifted)&&shifted==once);
                assert(matrices[0xA00000]==transform);
                // Keep the same center with animated back bones and independent
                // child-local scale, rotation and translation, in both modes.
                matrices[backBone]={{0,scale,0,0,-scale,0,0,0,0,0,scale*2,0,3,-4,5,1}};
                const auto base=context==c?0x2000000u:0x3000000u;
                const auto torso=quiverBackParents[2*(fixture.race-1)+fixture.sex];
                const auto torsoAddress=base+0x5000+64*torso;
                matrices[torsoAddress]={{scale,0,0,0,0,0,scale,0,0,-scale,0,0,8,-3,7,1}};
                const auto localAddress=address(context->extra[6])+0xBC;
                matrices[localAddress]={{0,1.4f,0,0,-.8f,0,0,0,0,0,.6f,0,.1f,-.2f,.3f,1}};
                for(bool horizontal:{false,true}){
                    context->quiverHorizontal=horizontal;
                    assert(positionStoredQuiver(context->extra[6],attachment,shifted));
                    assertCentered(context,shifted,fixture,backBone);
                    if(!horizontal)for(unsigned i=0;i<12;++i)assert(shifted[i]==transform[i]);
                }
                matrices[localAddress]=identity;matrices[backBone]=identity;matrices[torsoAddress]=identity;
            }
        }
        auto* forced=context->extra[6];
        context->extra[6]=nullptr;
        assert(!positionStoredQuiver(forced,attachment,shifted)); // unowned/native quiver
        context->extra[6]=forced;
        auto originalGuid=context->guid;context->guid=999;
        assert(!positionStoredQuiver(forced,attachment,shifted));context->guid=originalGuid;
        const auto originalItem=context->selection.items[6];context->selection.items[6]=4939;
        assert(!positionStoredQuiver(forced,attachment,shifted));context->selection.items[6]=originalItem;
        const auto originalParent=memory[address(forced)+0x1CC];memory[address(forced)+0x1CC]=0x5000;
        assert(!positionStoredQuiver(forced,attachment,shifted));memory[address(forced)+0x1CC]=originalParent;
        memory[address(forced)+0x1D0]=27;assert(!positionStoredQuiver(forced,attachment,shifted));memory[address(forced)+0x1D0]=26;
        matrices[0xA00000]=identity;matrices[0xA00000][15]=0;
        assert(!positionStoredQuiver(forced,attachment,shifted));
        matrices[0xA00000]=identity;matrices[0xA00000][5]=0;
        assert(!positionStoredQuiver(forced,attachment,shifted));
        matrices[0xA00000]=identity;matrices[0xA00000][0]=std::numeric_limits<float>::quiet_NaN();
        assert(!positionStoredQuiver(forced,attachment,shifted));
        matrices[0xA00000]=input;
        auto local=matrices[address(forced)+0xBC];matrices.erase(address(forced)+0xBC);
        assert(!positionStoredQuiver(forced,attachment,shifted));matrices[address(forced)+0xBC]=local;
        matrices[address(forced)+0xBC][12]=std::numeric_limits<float>::infinity();
        assert(!positionStoredQuiver(forced,attachment,shifted));matrices[address(forced)+0xBC]=local;
        auto bone=matrices[backBone];matrices.erase(backBone);
        assert(!positionStoredQuiver(forced,attachment,shifted));matrices[backBone]=bone;
        const auto base=context==c?0x2000000u:0x3000000u;
        const auto parentIndex=base+0x7000+108*bowFixtures[15].bone+8;
        const auto torso=memory[parentIndex];
        memory[parentIndex]=0xFFFF;assert(!positionStoredQuiver(forced,attachment,shifted));
        memory[parentIndex]=128;assert(!positionStoredQuiver(forced,attachment,shifted));memory[parentIndex]=torso;
        const auto torsoAddress=base+0x5000+64*torso;
        auto torsoFrame=matrices[torsoAddress];matrices.erase(torsoAddress);
        assert(!positionStoredQuiver(forced,attachment,shifted));matrices[torsoAddress]=torsoFrame;
        matrices[torsoAddress][5]=0;assert(!positionStoredQuiver(forced,attachment,shifted));
        matrices[torsoAddress]=torsoFrame;
        for(int flag:{0,1,-1}){
            auto toggle=request(context->token,s,flag);
            assert(setWeapons(&toggle)==1&&context->quiverHorizontal==(flag==1));
            assert(positionStoredQuiver(forced,attachment,shifted));
            assertCentered(context,shifted,bowFixtures[15],backBone);
            if(flag!=1)for(unsigned i=0;i<12;++i)assert(shifted[i]==input[i]);
            assert(context->extra==children&&refs==angleRefs&&matrices[0xA00000]==input);
            assert(loads==beforeAngleLoads&&meleeRefreshes==beforeAngleMelee&&rangedRefreshes==beforeAngleRanged);
        }
    }
    for(double flag:{-1.,.5,2.,std::numeric_limits<double>::infinity(),std::numeric_limits<double>::quiet_NaN()}){
        auto invalid=request(0,s);invalid.values.push_back(flag);
        assert(setWeapons(&invalid)==-2&&!c->quiverHorizontal&&refs==angleRefs);
    }
    // Per-hand stow visibility must use the same authored back pose as the
    // older decorative slots. Exercise both real equipment and appearances;
    // previews own their explicit hand-role children instead of native ones.
    const auto assertModernStoredPose=[&](WeaponContext* context,void* child,unsigned role,
        unsigned item,const std::array<float,16>& expected){
        const auto saved=*context;
        const auto childAddress=address(child);
        const auto savedResource=memory[childAddress+0x30],savedLoaded=memory[childAddress+0x10];
        const auto savedPoint=memory[childAddress+0x1D0];
        const auto savedLocal=matrices[childAddress+0xBC];
        auto local=identity;local[0]=local[5]=local[10]=.8f;
        local[12]=.13f;local[13]=-.07f;local[14]=.09f;
        matrices[childAddress+0xBC]=local;
        const auto positioned=[&](){return role==2?positionStoredBow(child,attachment,shifted):positionStoredBackWeapon(child,shifted);};
        const auto close=[](const std::array<float,16>& actual,const std::array<float,16>& wanted){
            for(unsigned i=0;i<16;++i)assert(std::fabs(actual[i]-wanted[i])<.0001f);
        };
        for(int advanced:{0,1})for(bool appearance:{false,true}){
            if(context->token&&!appearance)continue; // Native preview TryOn is not an owned extra.
            context->selection={};context->selection.independent=true;
            context->selection.carriedMode=advanced;context->selection.stowedMask=7;
            const auto alternate=role==2?gun:weaponAsset(item)->kind==1?25u:35u;
            context->selection.equipped[role]=appearance?alternate:item;
            context->routes.fill(-1);context->extra.fill(nullptr);context->nativeChildren.fill(nullptr);
            if(appearance){context->selection.items[7+role]=item;context->routes[role]=7+role;}
            if(context->token)context->extra[7+role]=child;else context->nativeChildren[role]=child;
            modelName(child,weaponAsset(item)->model);
            memory[childAddress+0x1D0]=savedPoint;
            assert(positioned());close(shifted,expected);
            // Check the actual renderer submission, not only its routing
            // decision. The child-local scale and offset are composed once.
            captureAttachmentMatrix=true;
            updateAttachmentForCaller(child,attachment,nullptr,nullptr,1.f,0x718761);
            captureAttachmentMatrix=false;
            close(capturedAttachmentMatrix,expected);
            close(bagMatrixProduct(capturedAttachmentMatrix,local),bagMatrixProduct(expected,local));
            assert(matrices[childAddress+0xBC]==local);
            assert(memory[childAddress+0x1D0]==savedPoint);
            memory[childAddress+0x1D0]=role==0?1:2;
            assert(!positioned()); // Drawn weapons retain the hand pose.
            updateAttachmentForCaller(child,attachment,nullptr,nullptr,1.f,0x718761);
            assert(attachmentUpdate.matrix==attachment);
            memory[childAddress+0x1D0]=savedPoint;
            context->extra.fill(nullptr);context->nativeChildren.fill(nullptr);
            assert(!positioned()); // A matching mesh at the same point is not enough.
            if(context->token)context->extra[7+role]=child;else context->nativeChildren[role]=child;
            modelName(child,weaponAsset(role==2?gun:35)->model);
            assert(!positioned()); // Stale ownership cannot reposition a different mesh.
            modelName(child,weaponAsset(item)->model);
            context->selection.stowedMask=-1;
            assert(!positioned()); // This test has no legacy decoration route.
        }
        *context=saved;memory[childAddress+0x30]=savedResource;memory[childAddress+0x10]=savedLoaded;
        memory[childAddress+0x1D0]=savedPoint;matrices[childAddress+0xBC]=savedLocal;
    };
    assert(!positionStoredBow(c->extra[5],attachment,shifted)); // gun unchanged
    c->selection.items[5]=2507;pc->selection.items[5]=2507;
    for(const auto& fixture:bowFixtures)for(auto context:{c,pc}){
        // Deliberately unrelated to player race or any fixed attachment index.
        const auto base=context==c?0x2000000u:0x3000000u;
        matrices[context->parent+0xFC]=identity;
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
            assertModernStoredPose(context,context->extra[5],2,2507,once);
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
            assertModernStoredPose(context,child,position-2,4939,once);
            // One-handed weapons can also author a type-1 back sheath.
            assert(weaponAsset(778)->kind==1&&weaponAsset(778)->sheath==1);
            assertModernStoredPose(context,child,position-2,778,once);
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
    // The game can add its own quiver while ours is already present. Suppress
    // only that matching native mesh at draw time; keep its callbacks/ownership.
    for(auto* context:{c,pc}){
        auto* custom=context->extra[6];const auto customAddress=address(custom);
        factory(pointer(context->parent),26,"native quiver","",0);
        auto* native=findChildOriginal(pointer(context->parent),26);assert(native&&native!=custom);
        const auto nativeAddress=address(native);const auto resource=0xD00000u;
        memory[nativeAddress+0x10]=1;memory[nativeAddress+0x30]=resource;
        memory[customAddress+0x10]=1;memory[customAddress+0x30]=resource;
        const float color[]={.4f,.5f,.6f},lighting[]={.1f,.2f,.3f};
        const auto beforeRefs=refs;const auto head=memory[context->parent+0x1DC];
        const auto beforeLoads=loads,beforeMelee=meleeRefreshes,beforeRanged=rangedRefreshes;
        for(bool horizontal:{false,true}){
            context->quiverHorizontal=horizontal;
            for(unsigned frame=0;frame<4;++frame){
                const auto beforeCalls=attachmentUpdate.calls;
                updateWeaponAttachment(native,attachment,color,lighting,.65f);
                assert(attachmentUpdate.calls==beforeCalls+1&&attachmentUpdate.model==native);
                assert(attachmentUpdate.alpha==0&&attachmentUpdate.matrix==attachment);
                assert(attachmentUpdate.color==color&&attachmentUpdate.lighting==lighting);
                updateWeaponAttachment(custom,attachment,color,lighting,.65f);
                assert(attachmentUpdate.alpha==.65f&&attachmentUpdate.model==custom);
            }
        }
        assert(refs==beforeRefs&&head==memory[context->parent+0x1DC]);
        assert(loads==beforeLoads&&meleeRefreshes==beforeMelee&&rangedRefreshes==beforeRanged);
        // Swords can use point 26 too; a different mesh must remain visible.
        memory[nativeAddress+0x30]=resource+1;
        updateWeaponAttachment(native,attachment,color,lighting,.4f);assert(attachmentUpdate.alpha==.4f);
        memory[nativeAddress+0x30]=resource;
        memory[nativeAddress+0x1D0]=28;assert(!hideNativeQuiver(native));memory[nativeAddress+0x1D0]=26;
        const auto guid=context->guid;context->guid=999;assert(!hideNativeQuiver(native));context->guid=guid;
        memory[nativeAddress+0x1CC]=0x5000;assert(!hideNativeQuiver(native));memory[nativeAddress+0x1CC]=context->parent;
        memory[nativeAddress+0x10]=0;assert(!hideNativeQuiver(native));memory[nativeAddress+0x10]=1;
        memory[customAddress+0x10]=0;assert(!hideNativeQuiver(native));memory[customAddress+0x10]=1;
        memory[customAddress+0x1D0]=27;assert(!hideNativeQuiver(native));memory[customAddress+0x1D0]=26;
        const auto selected=context->selection.items[6];context->selection.items[6]=4939;
        assert(!hideNativeQuiver(native));context->selection.items[6]=selected;
        // A stock detach temporarily makes the native quiver visible until our
        // existing recovery reattaches the custom, without loading another one.
        detach(custom);updateWeaponAttachment(native,attachment,color,lighting,.7f);
        assert(attachmentUpdate.alpha==.7f&&refs[customAddress]==1);
        auto restore=request(context->token,s);
        assert(setWeapons(&restore)==1&&context->extra[6]==custom&&loads==beforeLoads);
        updateWeaponAttachment(native,attachment,color,lighting,.7f);assert(attachmentUpdate.alpha==0);
        // Clearing through the actual API restores the native mesh on the next
        // update even if other customized weapon slots keep the context alive.
        auto withoutQuiver=s;withoutQuiver.items[6]=0;auto clearQuiver=request(context->token,withoutQuiver);
        assert(setWeapons(&clearQuiver)==1&&!context->extra[6]);
        assert(refs[nativeAddress]==1&&memory[nativeAddress+0x1CC]==context->parent);
        updateWeaponAttachment(native,attachment,color,lighting,.8f);assert(attachmentUpdate.alpha==.8f);
        assert(setWeapons(&restore)==1&&context->extra[6]);
        detach(native);
    }
    // Stored visibility is independent of horizontal orientation and changes
    // alpha without rebuilding custom or routed weapon instances.
    memory[player.unit+0xD40]=0;moveWeaponHook(pointer(player.unit),nullptr,0,1);
    for(auto* context:{c,pc}){
        const auto beforeRefs=refs;const auto beforeLoads=loads;
        const auto beforeMelee=meleeRefreshes,beforeRanged=rangedRefreshes;
        for(int rangedFlag:{0,1})for(int meleeFlag:{0,1}){
            auto options=request(context->token,s,0,rangedFlag,meleeFlag);
            assert(setWeapons(&options)==1);
            for(unsigned position=0;position<7;++position){
                auto* child=context->extra[position];
                if(!child)child=findChildOriginal(pointer(context->parent),weaponPoints[position]);
                assert(child);
                const auto* asset=weaponAsset(s.items[position]);assert(asset);
                const bool hidden=(asset->kind==4&&rangedFlag)||((asset->kind==1||asset->kind==2)&&meleeFlag);
                updateWeaponAttachment(child,attachment,nullptr,nullptr,.6f);
                assert(attachmentUpdate.alpha==(hidden?0:.6f));
                const auto originalPoint=memory[address(child)+0x1D0];
                for(unsigned hand:{0u,1u,2u}){
                    memory[address(child)+0x1D0]=hand;
                    updateWeaponAttachment(child,attachment,nullptr,nullptr,.6f);
                    assert(attachmentUpdate.alpha==.6f); // drawn weapons always visible
                }
                memory[address(child)+0x1D0]=originalPoint;
            }
        }
        assert(refs==beforeRefs&&loads==beforeLoads&&meleeRefreshes==beforeMelee&&rangedRefreshes==beforeRanged);
        // A same-point native child must match the actual selected weapon mesh.
        auto* child=context==c?findChildOriginal(pointer(context->parent),30):nullptr;
        if(child){
            const auto resource=memory[address(child)+0x30];
            modelName(child,"Item\\ObjectComponents\\Weapon\\Unrelated_Prop.m2");
            assert(!hideStoredWeapon(child));memory[address(child)+0x30]=resource;
            assert(hideStoredWeapon(child));
            const auto guid=context->guid;context->guid=999;assert(!hideStoredWeapon(child));context->guid=guid;
        }
        auto resetOptions=request(context->token,s);assert(setWeapons(&resetOptions)==1);
        assert(!context->hideMeleeWhenStored&&!context->hideRangedWhenStored);
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
    unsigned alternateBow=0;
    for(const auto& a:weaponAssets)if(a.kind==4&&a.subclass==2&&a.item!=2507)alternateBow=a.item;
    assert(alternateBow);
    for(unsigned selectedBow:{2507u,alternateBow})for(unsigned selectedQuiver:{0u,quiver}){
        s.items[5]=selectedBow;s.items[6]=selectedQuiver;L=request(0,s);
        memory[player.unit+0xD3C]=0;memory[player.unit+0xD40]=0;
        assert(setWeapons(&L)==1);c=weaponContext(player.model);
        assert(c&&c->routes[2]==5&&!c->extra[5]);
        // Exercise successive NPC-talk and loot-crouch interruptions. Both
        // clear the held bow through the same immediate native transition.
        for(unsigned interruption=0;interruption<2;++interruption){
            memory[player.unit+0xD40]=2;
            sheathTransitionHook(pointer(player.unit),nullptr);
            memory[player.unit+0xD3C]=2;
            auto* heldBow=findChildHook(pointer(player.model),nullptr,2);
            assert(heldBow&&address(heldBow)==rangeHolder&&refs[rangeHolder]==2);
            assert(!findChildHook(pointer(player.model),nullptr,27));
            auto* storedSword=findChildHook(pointer(player.model),nullptr,30);
            auto* forcedQuiver=c->extra[6];
            assert(storedSword);
            factory(pointer(player.model),35,"arrow","",0);
            const auto arrow=address(find(pointer(player.model),35));
            const unsigned beforeLoads=loads,beforeRangedRefreshes=rangedRefreshes;
            memory[player.unit+0xD40]=0;
            sheathTransitionHook(pointer(player.unit),nullptr);
            memory[player.unit+0xD3C]=0;
            assert(!findChildHook(pointer(player.model),nullptr,2)&&!find(pointer(player.model),35)&&!refs[arrow]);
            const auto* storedBow=findChildHook(pointer(player.model),nullptr,27);
            assert(storedBow&&storedBow!=heldBow&&!refs[address(heldBow)]);
            assert(memory[rangeHolder+0x1CC]==player.model&&refs[rangeHolder]==2);
            assert(reinterpret_cast<std::uintptr_t>(storedBow)==rangeHolder);
            assert(findChildHook(pointer(player.model),nullptr,30)==storedSword);
            assert(c->extra[6]==forcedQuiver);
            if(forcedQuiver)assert(memory[address(forcedQuiver)+0x1CC]==player.model&&refs[address(forcedQuiver)]==2);
            assert(loads==beforeLoads+1&&rangedRefreshes==beforeRangedRefreshes+1);
            // The completed transition and identical addon updates stay idle.
            sheathTransitionHook(pointer(player.unit),nullptr);
            assert(setWeapons(&L)==1&&loads==beforeLoads+1&&rangedRefreshes==beforeRangedRefreshes+1);
            unsigned storedBows=0;
            for(auto child=memory[player.model+0x1DC];child;child=memory[child+0x1E4])
                if(memory[child+0x1D0]==27)++storedBows;
            assert(storedBows==1);
        }
    }
    // A normal ranged-to-melee draw already stores ranged through 611E10;
    // the immediate-transition repair must not add a second refresh.
    memory[player.unit+0xD40]=2;
    sheathTransitionHook(pointer(player.unit),nullptr);
    memory[player.unit+0xD3C]=2;memory[player.unit+0xD40]=1;
    const unsigned beforeMeleeDrawLoads=loads,beforeMeleeDrawRanged=rangedRefreshes;
    sheathTransitionHook(pointer(player.unit),nullptr);
    assert(loads==beforeMeleeDrawLoads+1&&rangedRefreshes==beforeMeleeDrawRanged+1);
    assert(findChildHook(pointer(player.model),nullptr,1)&&findChildHook(pointer(player.model),nullptr,27));
    assert(!findChildHook(pointer(player.model),nullptr,2)&&!findChildHook(pointer(player.model),nullptr,30));
    memory[player.unit+0xD3C]=1;memory[player.unit+0xD40]=0;
    sheathTransitionHook(pointer(player.unit),nullptr);memory[player.unit+0xD3C]=0;
    assert(!findChildHook(pointer(player.model),nullptr,1)&&findChildHook(pointer(player.model),nullptr,30));
    // Unrelated players' rebuilds do not inherit the local player's route.
    memory[0x4000+0xD8]=0x5000;memory[0x4000+0xD40]=0;
    memory[0x4000+0xD3C]=2;
    factory(pointer(0x5000),2,"foreign bow","",0);
    const auto foreignBow=address(find(pointer(0x5000),2));
    const auto localBow=findChildHook(pointer(player.model),nullptr,27);
    sheathTransitionHook(pointer(0x4000),nullptr);
    assert(!refs[foreignBow]&&!find(pointer(0x5000),2)&&!find(pointer(0x5000),27));
    assert(findChildHook(pointer(player.model),nullptr,27)==localBow);
    scopedSheathPoint=27;rebuildWeaponHook(pointer(0x4000),nullptr,0);
    assert(effectHomes[0]==26&&scopedSheathPoint==27);scopedSheathPoint=-1;
    forgetWeapons(player.model);
    if(rangeHolder){unref(pointer(rangeHolder));rangeHolder=0;}
    s={};L=request(0,s);
    L.values[1]=999999;assert(setWeapons(&L)==-2);
    L=request(99,s);assert(setWeapons(&L)==-1);
    // Options work with no custom selections. Merely registering the actual
    // equipment must not rebuild stock weapons or force a world quiver to exist.
    realIDs[0]=35;realIDs[1]=143;realIDs[2]=2507;
    memory[player.unit+0xD40]=0;
    melee(pointer(player.unit),0);melee(pointer(player.unit),1);
    const auto* quiverAsset=weaponAsset(quiver);assert(quiverAsset&&quiverAsset->kind==5);
    factory(pointer(player.model),26,quiverAsset->model,quiverAsset->texture,0);
    auto* nativeQuiver=findChildOriginal(pointer(player.model),26);assert(nativeQuiver);
    WeaponSelection plain;plain.equipped={{35,143,2507}};
    const auto optionLoads=loads,optionMelee=meleeRefreshes,optionRanged=rangedRefreshes;
    auto horizontalNative=request(0,plain,1,1,1,quiver);
    assert(setWeapons(&horizontalNative)==1);c=weaponContext(player.model);
    assert(c&&c->selection.empty()&&!c->passthroughQuiver&&!c->extra[6]);
    assert(loads==optionLoads&&meleeRefreshes==optionMelee&&rangedRefreshes==optionRanged);
    auto backBone=setBack(c,bowFixtures[0]);
    matrices[0xA00000]=input;
    assert(nativeQuiverModel(nativeQuiver)&&positionStoredQuiver(nativeQuiver,attachment,shifted));
    const auto actualCenter=transformPoint(shifted,meshCenter);
    const auto expectedBack=transformPoint(matrices[backBone],bowFixtures[0].position);
    for(unsigned i=0;i<3;++i)assert(std::fabs(actualCenter[i]-(expectedBack[i]-(i==1?.105362409f:0)))<.00001f);
    assert(!hideStoredWeapon(nativeQuiver)&&!hideNativeQuiver(nativeQuiver));
    auto* nativeStaff=findChildOriginal(pointer(player.model),30);
    auto* nativeShield=findChildOriginal(pointer(player.model),28);
    assert(nativeStaff&&nativeShield&&hideStoredWeapon(nativeStaff)&&!hideStoredWeapon(nativeShield));
    const auto resource=memory[address(nativeQuiver)+0x30];
    modelName(nativeQuiver,"item/objectcomponents/quiver/quiver_a.m2");
    assert(nativeQuiverModel(nativeQuiver));
    modelName(nativeQuiver,"Item\\ObjectComponents\\Quiver\\Quiver_A_Prop.m2");
    assert(!positionStoredQuiver(nativeQuiver,attachment,shifted));
    memory[address(nativeQuiver)+0x30]=resource;
    for(unsigned hand:{0u,1u,2u}){
        memory[address(nativeQuiver)+0x1D0]=hand;
        assert(!positionStoredQuiver(nativeQuiver,attachment,shifted));
    }
    memory[address(nativeQuiver)+0x1D0]=26;
    auto noOrientation=request(0,plain,0,1,1,quiver);assert(setWeapons(&noOrientation)==1);
    assert(!positionStoredQuiver(nativeQuiver,attachment,shifted));
    updateWeaponAttachment(nativeQuiver,attachment,nullptr,nullptr,.7f);
    assert(attachmentUpdate.matrix==attachment&&attachmentUpdate.alpha==.7f);
    assert(loads==optionLoads&&meleeRefreshes==optionMelee&&rangedRefreshes==optionRanged);
    auto optionsOff=request(0,plain,0,0,0,quiver);
    assert(setWeapons(&optionsOff)==1&&!weaponContext(player.model));
    updateWeaponAttachment(nativeStaff,attachment,nullptr,nullptr,.8f);assert(attachmentUpdate.alpha==.8f);
    assert(!positionStoredQuiver(nativeQuiver,attachment,shifted));
    assert(setWeapons(&optionsOff)==1&&!weaponContext(player.model)&&loads==optionLoads);
    // A rare stock ranged model with a real sheath home is identified by its
    // actual equipped mesh, while weapons at unrelated points remain visible.
    auto rangedPlain=plain;rangedPlain.equipped[2]=1046;
    auto hideStockRange=request(0,rangedPlain,0,1,0,quiver);
    assert(setWeapons(&hideStockRange)==1);c=weaponContext(player.model);
    auto* stockGun=weaponAsset(1046);assert(stockGun&&stockGun->kind==4);
    modelName(nativeQuiver,stockGun->model); // point 26 is this gun's stock home
    assert(hideStoredWeapon(nativeQuiver)&&!positionStoredQuiver(nativeQuiver,attachment,shifted));
    memory[address(nativeQuiver)+0x1D0]=1;assert(!hideStoredWeapon(nativeQuiver));
    memory[address(nativeQuiver)+0x1D0]=26;modelName(nativeQuiver,quiverAsset->model);
    assert(!hideStoredWeapon(nativeQuiver));
    assert(setWeapons(&optionsOff)==1&&!weaponContext(player.model));
    // Bow strings are submitted by a separate callback with fixed alpha 255.
    // Hide its submission with the mesh, without stopping attachment updates,
    // changing ownership, or suppressing drawn / other players' bows.
    for(unsigned token:{0u,8u}){
        auto bowSelection=plain;bowSelection.items[5]=2507;
        auto hideBow=request(token,bowSelection,0,1,0);
        assert(setWeapons(&hideBow)==1);
        auto* context=weaponContext(token?0x6000:player.model);assert(context);
        auto* bow=token?context->extra[5]:findChildOriginal(pointer(context->parent),27);
        assert(bow&&ownedExtra(*context,bow)==(token!=0));
        auto* renderState=pointer(0xEF1000);auto* callbackUnit=pointer(player.unit);
        const auto stableRefs=refs;const auto stableLoads=loads;
        const auto checkString=[&](bool hidden){
            const auto calls=bowStringSubmission.calls,updates=attachmentUpdate.calls;
            bowStringDrawHook(bow,renderState,callbackUnit);
            assert(bowStringSubmission.calls==calls+(hidden?0u:1u));
            if(!hidden)assert(bowStringSubmission.model==bow&&bowStringSubmission.renderState==renderState&&
                bowStringSubmission.unit==callbackUnit);
            updateWeaponAttachment(bow,attachment,nullptr,nullptr,.8f);
            assert(attachmentUpdate.calls==updates+1&&attachmentUpdate.alpha==(hidden?0:.8f));
        };
        for(unsigned frame=0;frame<8;++frame){
            checkString(true);
            for(unsigned hand:{0u,1u,2u}){
                memory[address(bow)+0x1D0]=hand;checkString(false);
            }
            memory[address(bow)+0x1D0]=27;checkString(true);
            auto visibleBow=request(token,bowSelection,0,0,1);
            assert(setWeapons(&visibleBow)==1);checkString(false); // melee-only never hides a bow
            assert(setWeapons(&hideBow)==1);checkString(true);
        }
        context->guid=999;checkString(false);context->guid=player.guid;
        const auto parent=memory[address(bow)+0x1CC];
        memory[address(bow)+0x1CC]=0xDEAD;checkString(false);
        memory[address(bow)+0x1CC]=parent;
        if(!token){
            const auto modelResource=memory[address(bow)+0x30];
            modelName(bow,"Item\\ObjectComponents\\Weapon\\Unrelated_Prop.m2");checkString(false);
            memory[address(bow)+0x30]=modelResource;
        }
        checkString(true);assert(refs==stableRefs&&loads==stableLoads);
        auto clearBow=request(token,plain,0,0,0);
        assert(setWeapons(&clearBow)==1&&!weaponContext(parent));
    }
    // A fresh/race-changed preview gets the actual quiver explicitly, independent
    // of inherited custom skins or the world's current draw/sheath state.
    auto previewActual=request(8,plain,0,0,0,quiver);
    const auto beforePreviewLoads=loads,beforePreviewMelee=meleeRefreshes,beforePreviewRanged=rangedRefreshes;
    factoryModelsLoaded=false;
    assert(setWeapons(&previewActual)==0);pc=weaponContext(0x6000);
    factoryModelsLoaded=true;
    assert(pc&&pc->selection.empty()&&pc->passthroughQuiver&&!pc->extra[6]);
    auto* passthrough=pc->passthroughQuiver;
    assert(!nativeQuiverModel(passthrough)&&refs[address(passthrough)]==2&&loads==beforePreviewLoads+1);
    assert(setWeapons(&previewActual)==0&&pc->passthroughQuiver==passthrough&&loads==beforePreviewLoads+1);
    memory[address(passthrough)+0x10]=1;
    assert(setWeapons(&previewActual)==1&&pc->passthroughQuiver==passthrough&&loads==beforePreviewLoads+1);
    assert(nativeQuiverModel(passthrough));
    assert(!positionStoredQuiver(passthrough,attachment,shifted));
    assert(meleeRefreshes==beforePreviewMelee&&rangedRefreshes==beforePreviewRanged);
    backBone=setBack(pc,bowFixtures[1]);
    const auto previewRefs=refs;
    for(int angle:{1,0,1,0}){
        auto toggle=request(8,plain,angle,1,1,quiver);
        assert(setWeapons(&toggle)==1&&pc->passthroughQuiver==passthrough);
        assert(positionStoredQuiver(passthrough,attachment,shifted)==(angle==1));
        if(angle==1){
            const auto center=transformPoint(shifted,meshCenter);
            const auto back=transformPoint(matrices[backBone],bowFixtures[1].position);
            for(unsigned i=0;i<3;++i)assert(std::fabs(center[i]-(back[i]-(i==1?.105362409f:0)))<.00001f);
        }
        assert(!hideStoredWeapon(passthrough)&&!hideNativeQuiver(passthrough));
        assert(refs==previewRefs&&loads==beforePreviewLoads+1);
    }
    discardInheritedPreviewWeapons(pc->parent); // never discard the owned fallback
    clearChildrenHook(pointer(pc->parent),nullptr,26);
    assert(pc->passthroughQuiver==passthrough&&refs[address(passthrough)]==2);
    detach(passthrough);assert(refs[address(passthrough)]==1);
    assert(setWeapons(&previewActual)==1&&pc->passthroughQuiver==passthrough&&refs[address(passthrough)]==2);
    assert(loads==beforePreviewLoads+1);
    factory(pointer(pc->parent),26,quiverAsset->model,quiverAsset->texture,0);
    auto* inheritedQuiver=findChildOriginal(pointer(pc->parent),26);assert(inheritedQuiver!=passthrough);
    assert(hideNativeQuiver(inheritedQuiver));
    discardInheritedPreviewWeapons(pc->parent);
    assert(!refs[address(inheritedQuiver)]&&refs[address(passthrough)]==2);
    unsigned otherQuiver=0;for(const auto& asset:weaponAssets)if(asset.kind==5&&asset.item!=quiver){otherQuiver=asset.item;break;}
    auto newActual=request(8,plain,0,0,0,otherQuiver);const auto beforeSwapLoads=loads;
    assert(setWeapons(&newActual)==1&&pc->passthroughQuiver!=passthrough&&!refs[address(passthrough)]);
    passthrough=pc->passthroughQuiver;assert(passthrough&&loads==beforeSwapLoads+1);
    assert(meleeRefreshes==beforePreviewMelee&&rangedRefreshes==beforePreviewRanged);
    WeaponSelection customized=plain;customized.items[6]=quiver;
    auto customPreview=request(8,customized,0,0,0,otherQuiver);
    assert(setWeapons(&customPreview)==1&&pc->extra[6]&&!pc->passthroughQuiver&&!refs[address(passthrough)]);
    auto* customPreviewQuiver=pc->extra[6];
    assert(setWeapons(&newActual)==1&&pc->passthroughQuiver&&!pc->extra[6]&&!refs[address(customPreviewQuiver)]);
    passthrough=pc->passthroughQuiver;
    const auto beforeInvalidRefs=refs;const auto beforeInvalidLoads=loads;
    for(unsigned argument:{13u,14u,15u}){
        auto invalid=newActual;invalid.values[argument-1]=-1;
        assert(setWeapons(&invalid)==-2&&pc->passthroughQuiver==passthrough&&pc->actualQuiver==otherQuiver);
        assert(!pc->quiverHorizontal&&!pc->hideRangedWhenStored&&!pc->hideMeleeWhenStored);
        assert(refs==beforeInvalidRefs&&loads==beforeInvalidLoads);
    }
    auto noActual=request(8,plain,1,0,0,0);
    assert(setWeapons(&noActual)==1&&!pc->passthroughQuiver&&!refs[address(passthrough)]&&weaponContext(pc->parent));
    auto clearPreview=request(8,plain,0,0,0,0);
    assert(setWeapons(&clearPreview)==1&&!weaponContext(0x6000));
    for(unsigned argument:{12u,13u,14u})for(double value:{-1.,.5,2.,std::numeric_limits<double>::infinity(),std::numeric_limits<double>::quiet_NaN()}){
        auto invalid=request(0,plain,0,0,0,0);invalid.values[argument-1]=value;
        assert(setWeapons(&invalid)==-2&&!weaponContext(player.model));
    }
    for(double value:{-1.,.5,4939.,999999.,std::numeric_limits<double>::infinity(),std::numeric_limits<double>::quiet_NaN()}){
        auto invalid=request(8,plain,0,0,0,0);invalid.values[14]=value;
        assert(setWeapons(&invalid)==-2&&!weaponContext(0x6000));
    }
    {
        WeaponSelection empty;
        auto bag=request(0,empty,0,0,0,0,1);
        factoryModelsLoaded=false;
        assert(setWeapons(&bag)==0);
        auto* bc=weaponContext(player.model);assert(bc&&bc->backBag==1&&bc->backpack);
        void* pack=bc->backpack;const auto beforeLoads=loads;
        assert(setWeapons(&bag)==0&&loads==beforeLoads&&bc->backpack==pack);
        memory[address(pack)+0x10]=1;factoryModelsLoaded=true;
        assert(setWeapons(&bag)==1&&loads==beforeLoads);
        modelName(pack,"World\\Generic\\PassiveDoodads\\ErrorCube\\ErrorCube.mdx");
        assert(setWeapons(&bag)==0&&!bc->backpack&&!refs[address(pack)]);
        assert(setWeapons(&bag)==1);pack=bc->backpack;
        assert(refs[address(pack)]==2&&ownedExtra(*bc,pack));
        assert(findChildHook(pointer(player.model),nullptr,28)!=pack);
        clearChildrenHook(pointer(player.model),nullptr,28);
        assert(memory[address(pack)+0x1CC]==player.model);
        for(const auto& fixture:bowFixtures){
            const auto bone=setBack(bc,fixture);matrices[address(pack)+0xBC]=identity;
            std::array<float,16> placed;
            assert(positionBackpack(pack,placed));
            const float scale=.45f*.85f;
            for(unsigned col:{0u,4u,8u}){float length=0;for(unsigned i=0;i<3;++i)length+=placed[col+i]*placed[col+i];assert(std::fabs(std::sqrt(length)-scale)<.00001f);}
            const auto& fit=bagFits[2*(fixture.race-1)+fixture.sex];
            if(fixture.race==2){
                // Orc male's centered pivot moves up/in; other fits stay put.
                const float adjustment=fixture.sex==0?.0375f:0;
                assert(std::fabs(placed[12]-fixture.position[0]-fit.depth-adjustment)<.00001f);
                assert(std::fabs(placed[13]-fixture.position[1]-.28f*.45f)<.00001f);
                assert(std::fabs(placed[14]-fixture.position[2]-(.20f*.45f-.15f+adjustment))<.00001f);
                assert(placed[8]<0&&placed[9]>0); // Bottom goes inward (+X), right (-Y).
                assert(std::fabs(std::asin(-placed[8]/scale)*57.2957795f-(fixture.sex==0?15.f:10.f))<.001f);
                assert(std::fabs(std::atan2(placed[9],placed[10])*57.2957795f-15.f)<.001f);
            }else{
                assert(std::fabs(placed[0]-scale)<.00001f&&std::fabs(placed[5]-scale)<.00001f&&std::fabs(placed[10]-scale)<.00001f);
                assert(std::fabs(placed[12]-fixture.position[0]-fit.depth)<.00001f&&placed[13]>fixture.position[1]);
                assert(std::fabs(placed[14]-fixture.position[2]-(.20f*.45f-.15f))<.00001f);
            }
            auto animated=identity;animated[0]=.8f;animated[2]=-.6f;animated[8]=.6f;animated[10]=.8f;animated[12]=.4f;animated[14]=-.3f;
            const auto base=bc==c?0x2000000u:0x3000000u;
            const auto torso=quiverBackParents[2*(fixture.race-1)+fixture.sex];
            matrices[bone]=animated;matrices[base+0x5000+64*torso]=animated;
            std::array<float,16> bent,expected{};assert(positionBackpack(pack,bent));
            for(unsigned row=0;row<4;++row)for(unsigned col=0;col<4;++col)for(unsigned k=0;k<4;++k)
                expected[col*4+row]+=animated[k*4+row]*placed[col*4+k];
            for(unsigned i=0;i<16;++i)assert(std::fabs(bent[i]-expected[i])<.00001f);
            // Different skeleton scales must not resize the same pack.
            for(float characterScale:{.55f,1.f,1.5f}){
                auto scaled=animated;
                for(unsigned column:{0u,4u,8u})for(unsigned row=0;row<3;++row)
                    scaled[column+row]*=characterScale*(column==4?1.2f:1.f);
                matrices[bone]=scaled;matrices[base+0x5000+64*torso]=scaled;
                std::array<float,16> fixed;assert(positionBackpack(pack,fixed));
                for(unsigned column:{0u,4u,8u}){
                    float squared=0;for(unsigned row=0;row<3;++row)squared+=fixed[column+row]*fixed[column+row];
                    assert(std::fabs(std::sqrt(squared)-scale)<.00001f);
                }
            }
            setBack(bc,fixture);
            // A factory-local rotation/scale/translation must not tilt the bag,
            // change its contact surface or move it around the attachment pivot.
            auto local=identity;local[0]=0;local[1]=2;local[4]=-2;local[5]=0;local[10]=2;local[12]=.3f;local[14]=-.2f;
            matrices[address(pack)+0xBC]=local;
            std::array<float,16> compensated;assert(positionBackpack(pack,compensated));
            std::array<float,16> product{};
            for(unsigned row=0;row<4;++row)for(unsigned col=0;col<4;++col)for(unsigned k=0;k<4;++k)
                product[col*4+row]+=compensated[k*4+row]*local[col*4+k];
            for(unsigned i=0;i<16;++i)assert(std::fabs(product[i]-placed[i])<.00001f);
            local[0]=local[1]=0;matrices[address(pack)+0xBC]=local;
            assert(!positionBackpack(pack,compensated));
        }
        const auto beforeStockCloneLoads=loads;
        // The ordinary character sheet is an unregistered stock clone, not
        // an addon preview token. Its inherited pack needs our fit at draw time.
        {
            constexpr std::uintptr_t stockModel=0x9000,nestedModel=0xA000;
            WeaponContext stock;stock.parent=stockModel;
            factory(pointer(stockModel),28,"Interface\\AddOns\\SaureksCloset\\Models\\DarkSchoolbag.mdx","",0);
            auto* stockPack=find(pointer(stockModel),28);
            const auto references=refs;
            const auto previewEntries=previews.entries;
            assert(!weaponContext(stockModel)&&!previews.find(stockModel));
            BagMatrix placed;
            assert(!positionBackpack(stockPack,placed)); // Unrecognized clone used the raw shield pose.
            rememberClonedBagPreview(player.model,stockModel);
            assert(clonedBagOwner(stockModel)==player.guid);
            rememberClonedBagPreview(stockModel,nestedModel);
            assert(clonedBagOwner(nestedModel)==player.guid);
            rememberClonedBagPreview(0xDEAD,0xB000);
            assert(!clonedBagOwner(0xB000));
            captureAttachmentMatrix=true;
            for(const auto& fixture:bowFixtures){
                setBack(bc,fixture);matrices[address(pack)+0xBC]=identity;
                BagMatrix worldPose;assert(positionBackpack(pack,worldPose));
                const auto bone=setBack(&stock,fixture);
                const auto torso=0x3000000+0x5000+64*quiverBackParents[2*(fixture.race-1)+fixture.sex];
                for(float zoom:{.4f,1.f,3.f}){
                    auto view=identity;
                    view[0]=0;view[1]=zoom;view[4]=-zoom;view[5]=0;view[10]=zoom;
                    view[12]=7;view[13]=-9;view[14]=4;
                    matrices[stockModel+0xFC]=view;matrices[bone]=view;matrices[torso]=view;
                    auto local=identity;local[0]=2;local[5]=.8f;local[10]=1.3f;local[12]=.4f;local[14]=-.3f;
                    matrices[address(stockPack)+0xBC]=local;
                    const auto expected=bagMatrixProduct(view,worldPose);
                    assert(positionBackpack(stockPack,placed,true));
                    auto actual=bagMatrixProduct(placed,local);
                    for(unsigned i=0;i<16;++i)assert(std::fabs(actual[i]-expected[i])<.00005f);
                    memory[player.unit+0x9E8]=0x2001; // World running/jumping must not animate the sheet bag.
                    for(unsigned frame=0;frame<5;++frame){
                        bagTestTime+=16;updateWeaponAttachment(stockPack,identity.data(),nullptr,nullptr,.8f);
                        assert(attachmentUpdate.alpha==.8f);
                        actual=bagMatrixProduct(capturedAttachmentMatrix,local);
                        for(unsigned i=0;i<16;++i)assert(std::fabs(actual[i]-expected[i])<.00005f);
                    }
                }
            }
            // Current saved placement values apply to the stock clone as well.
            setBack(bc,bowFixtures[2]);setBack(&stock,bowFixtures[2]);
            matrices[address(stockPack)+0xBC]=identity;
            BagTuningValues fit;assert(bagTuningDefaults(1,2,0,fit));fit.scale=60;fit.inset=.08f;
            assert(bagTuningSet(1,2,0,true,fit));
            BagMatrix worldPose;assert(positionBackpack(pack,worldPose));
            assert(positionBackpack(stockPack,placed));
            for(unsigned i=0;i<16;++i)assert(std::fabs(placed[i]-worldPose[i])<.00005f);
            assert(bagTuningSet(1,2,0,false));
            const auto savedGuid=player.guid;++player.guid;
            updateWeaponAttachment(stockPack,identity.data(),nullptr,nullptr,1);assert(attachmentUpdate.alpha==0);
            player.guid=savedGuid;
            memory[stockModel+0x94]=0;
            updateWeaponAttachment(stockPack,identity.data(),nullptr,nullptr,1);assert(attachmentUpdate.alpha==0);
            setBack(&stock,bowFixtures[2]);
            updateWeaponAttachment(stockPack,identity.data(),nullptr,nullptr,1);assert(attachmentUpdate.alpha==1);
            assert(refs==references); // No retention, detach, reload or world-child mutation.
            for(unsigned i=0;i<previews.entries.size();++i)assert(previews.entries[i].model==previewEntries[i].model);
            captureAttachmentMatrix=false;memory[player.unit+0x9E8]=0;
            // A shield on the same attachment remains completely native.
            modelName(stockPack,"Item\\ObjectComponents\\Shield\\Shield_Test.mdx");
            updateWeaponAttachment(stockPack,identity.data(),nullptr,nullptr,.6f);
            assert(attachmentUpdate.matrix==identity.data()&&attachmentUpdate.alpha==.6f);
            forgetWeapons(stockModel);forgetWeapons(nestedModel);
            assert(!clonedBagOwner(stockModel)&&!clonedBagOwner(nestedModel));
            assert(refs==references);detach(stockPack);
            // Repeated character-sheet opens release weak entries for reuse.
            for(unsigned i=0;i<100;++i){
                rememberClonedBagPreview(player.model,stockModel);assert(clonedBagOwner(stockModel)==player.guid);
                forgetWeapons(stockModel);assert(!clonedBagOwner(stockModel));
            }
            std::cout<<"PASS: stock character-screen bags use fitted size/position across all bodies and zooms; world motion, native shields and ownership stay isolated\n";
        }
        const auto stockCloneLoads=loads-beforeStockCloneLoads;
        unsigned tuningLoads=0;
        {
            const auto beforeTuningLoads=loads;
            // The real bridge changes only the matching displayed race/sex,
            // simultaneously on the world bag and independently owned preview.
            setBack(bc,bowFixtures[2]);matrices[address(pack)+0xBC]=identity;
            BagMatrix originalPose;assert(positionBackpack(pack,originalPose));
            Lua tune{{1,2,0,1,.2,.075,.02,20,10,12,70,0}};
            assert(setBagFit(&tune)==1);
            BagMatrix worldPose;assert(positionBackpack(pack,worldPose,true));
            assert(!bc->bagMotion.ready&&std::fabs(worldPose[13]-originalPose[13]-(.2f-.126f))<.00001f);
            auto previewBag=request(8,empty,0,0,0,0,1);assert(setWeapons(&previewBag)==1);
            auto* tuningPreview=weaponContext(0x6000);assert(tuningPreview&&tuningPreview->backpack);
            setBack(tuningPreview,bowFixtures[2]);matrices[address(tuningPreview->backpack)+0xBC]=identity;
            BagMatrix previewPose;assert(positionBackpack(tuningPreview->backpack,previewPose,true));
            for(unsigned i=0;i<16;++i)assert(std::fabs(worldPose[i]-previewPose[i])<.00001f);
            setBack(tuningPreview,bowFixtures[3]);assert(positionBackpack(tuningPreview->backpack,previewPose,true));
            assert(tuningPreview->bagMotion.ready); // Female retained default enabled motion.
            assert(!bc->bagMotion.ready);
            forgetWeapons(tuningPreview->parent);
            Lua clear{{1,2,0,0}};assert(setBagFit(&clear)==1);
            setBack(bc,bowFixtures[2]);assert(positionBackpack(pack,worldPose,true));
            for(unsigned i=0;i<16;++i)assert(std::fabs(worldPose[i]-originalPose[i])<.00001f);
            tuningLoads=loads-beforeTuningLoads;
        }
        // Production rendering owns persistent motion; walking, jumping, swimming,
        // turning in place and a stationary preview must not trigger run sway.
        for(unsigned moving:{1u,2u,4u,8u}){memory[bc->unit+0x9E8]=moving;assert(bagIsRunning(*bc));}
        for(unsigned stopped:{0u,0x10u,0x20u,0x101u,0x2001u,0x4001u,0x200001u,0x8000001u}){
            memory[bc->unit+0x9E8]=stopped;assert(!bagIsRunning(*bc));
        }
        auto previewContext=*bc;previewContext.token=8;memory[bc->unit+0x9E8]=1;assert(!bagIsRunning(previewContext));
        for(unsigned moving:{1u,2u,4u,8u,0x101u,0x200001u,0x2000u,0x4000u}){
            memory[bc->unit+0x9E8]=moving;assert(bagMotionActive(*bc)&&!bagMotionActive(previewContext));
        }
        for(unsigned stopped:{0u,0x10u,0x20u,0x100u,0x200000u,0x801u}){
            memory[bc->unit+0x9E8]=stopped;assert(!bagMotionActive(*bc));
        }
        for(unsigned airborne:{0x2000u,0x4000u,0x6000u,0x2101u,0x2200u,0x20002000u}){
            memory[bc->unit+0x9E8]=airborne;assert(bagIsAirborne(*bc)&&!bagIsAirborne(previewContext));
        }
        for(unsigned grounded:{0u,1u,0x100u,0x200u,0x200000u,0x202000u,0x2400u,0x2800u,0x802000u,0x1002000u,0x8002000u}){
            memory[bc->unit+0x9E8]=grounded;assert(!bagIsAirborne(*bc));
        }
        memory.erase(bc->unit+0x9E8);assert(!bagIsAirborne(*bc));
        // Native downward velocity: no lift on ascent, gradual lift after the
        // apex, and no use of stale fall data after landing or in a preview.
        memory[bc->unit+0x9E8]=0x2000;scalars[bc->unit+0xA48]=-8.f;
        for(unsigned elapsed:{0u,100u,400u}){memory[bc->unit+0xA20]=elapsed;assert(bagAirLiftTarget(*bc)==0);}
        memory[bc->unit+0xA20]=416;assert(bagAirLiftTarget(*bc)>0&&bagAirLiftTarget(*bc)<.02f);
        memory[bc->unit+0xA20]=600;assert(bagAirLiftTarget(*bc)>.4f&&bagAirLiftTarget(*bc)<.6f);
        memory[bc->unit+0xA20]=1200;assert(bagAirLiftTarget(*bc)==1&&bagAirLiftTarget(previewContext)==0);
        memory[bc->unit+0xA20]=400;assert(bagAirLiftTarget(*bc)==0); // Native collision time can rewind.
        scalars[bc->unit+0xA48]=0;memory[bc->unit+0xA20]=0;assert(bagAirLiftTarget(*bc)==0);
        memory[bc->unit+0xA20]=2000;assert(bagAirLiftTarget(*bc)==1); // Walk-off fall.
        memory[bc->unit+0x9E8]=0x20002000;assert(std::fabs(bagAirLiftTarget(*bc)-.875f)<.000001f);
        memory[bc->unit+0xA20]=0;scalars[bc->unit+0xA48]=5;assert(bagAirLiftTarget(*bc)==.625f); // Feather-fall rebases its clock.
        memory[bc->unit+0xA20]=2000;
        for(unsigned blocked:{0u,0x4000u,0x202000u,0x2400u}){memory[bc->unit+0x9E8]=blocked;assert(bagAirLiftTarget(*bc)==0);}
        memory[bc->unit+0x9E8]=0x2000;
        for(float invalid:{std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN()}){
            scalars[bc->unit+0xA48]=invalid;assert(bagAirLiftTarget(*bc)==0);
        }
        scalars.erase(bc->unit+0xA48);assert(bagAirLiftTarget(*bc)==0);
        scalars[bc->unit+0xA48]=0;memory.erase(bc->unit+0xA20);assert(bagAirLiftTarget(*bc)==0);
        memory[bc->unit+0x9E8]=0;
        const auto motionBone=setBack(bc,bowFixtures[2]);matrices[address(pack)+0xBC]=identity;
        std::array<float,16> stillPose;assert(positionBackpack(pack,stillPose));
        const auto torsoAddress=(bc==c?0x2000000u:0x3000000u)+0x5000+64*quiverBackParents[2];
        bc->bagMotion={};
        for(unsigned frame=0;frame<60;++frame){
            auto camera=identity;const float angle=frame*.1f;
            camera[0]=-std::cos(angle);camera[1]=-std::sin(angle);camera[4]=-std::sin(angle);camera[5]=std::cos(angle);
            camera[12]=10+frame*.5f;camera[14]=-5;
            matrices[bc->parent+0xFC]=camera;matrices[motionBone]=camera;matrices[torsoAddress]=camera;
            matrices[memory[bc->parent+0x2C]+0x9C]=camera;
            bagTestTime=1000+frame*16;std::array<float,16> rendered;assert(positionBackpack(pack,rendered,true));
            const auto expected=bagMatrixProduct(camera,stillPose);
            for(unsigned i=0;i<16;++i)assert(std::fabs(rendered[i]-expected[i])<.00005f);
        }
        setBack(bc,bowFixtures[2]);
        bc->bagMotion={};bagTestTime=1000;
        updateWeaponAttachment(pack,identity.data(),nullptr,nullptr,1);
        assert(bc->bagMotion.ready&&attachmentUpdate.alpha==1);
        const float startX=bc->bagMotion.position[0];
        matrices[motionBone][12]+=.008f;bagTestTime+=16;
        updateWeaponAttachment(pack,identity.data(),nullptr,nullptr,1);
        assert(std::fabs(bc->bagMotion.position[0]-(startX+.008f))<.000001f);
        matrices[motionBone][12]+=.04f;bagTestTime+=16;
        updateWeaponAttachment(pack,identity.data(),nullptr,nullptr,1);
        assert(std::fabs(bc->bagMotion.position[0]-(startX+.048f))<.000001f); // The bag never trails its anchor.
        memory[bc->unit+0x9E8]=1;
        for(unsigned i=0;i<60;++i){bagTestTime+=16;updateWeaponAttachment(pack,identity.data(),nullptr,nullptr,1);}
        assert(bc->bagMotion.runWeight>.99f);
        std::array<float,16> runningPose,rigidPose;
        assert(positionBackpack(pack,runningPose,true)&&positionBackpack(pack,rigidPose,false));
        for(unsigned i=0;i<16;++i)assert(std::fabs(runningPose[i]-rigidPose[i])<.00001f); // No free-running sway on a static torso.
        // Read world up through the actor's scene, including a sideways actor
        // and a camera basis unrelated to the attachment or bag orientation.
        auto actor=identity;actor[0]=0;actor[2]=-1;actor[8]=1;actor[10]=0;
        auto camera=identity;camera[5]=0;camera[6]=1;camera[9]=-1;camera[10]=0;camera[12]=6;camera[14]=9;
        const auto modelView=bagMatrixProduct(camera,actor);
        BagMatrix renderToWorld;assert(bagAffineInverse(camera,renderToWorld));
        bc->bagMotion={};float largestGive=0;
        for(unsigned frame=0;frame<100;++frame){
            auto body=identity;body[12]=.04f*std::sin(frame*.29f);
            matrices[bc->parent+0xFC]=modelView;matrices[memory[bc->parent+0x2C]+0x9C]=camera;
            matrices[motionBone]=bagMatrixProduct(modelView,body);matrices[torsoAddress]=matrices[motionBone];
            bagTestTime+=16;
            BagMatrix moving,raw;assert(positionBackpack(pack,moving,true)&&positionBackpack(pack,raw,false));
            moving=bagMatrixProduct(renderToWorld,moving);raw=bagMatrixProduct(renderToWorld,raw);
            assert(std::fabs(moving[12]-raw[12])<.00001f&&std::fabs(moving[13]-raw[13])<.00001f);
            largestGive=std::fmax(largestGive,std::fabs(moving[14]-raw[14]));
        }
        assert(largestGive>.003f&&largestGive<1.239f*.45f*.85f*.0401f);
        const auto scene=memory[bc->parent+0x2C];memory[bc->parent+0x2C]=0;
        BagMatrix noScene,raw;assert(positionBackpack(pack,noScene,true)&&positionBackpack(pack,raw,false));
        assert(noScene==raw&&!bc->bagMotion.ready);memory[bc->parent+0x2C]=scene;
        // A real jump keeps the bag settled throughout ascent, then lifts its
        // bottom outward/up as downward momentum builds. Landing settles it.
        setBack(bc,bowFixtures[2]);bc->bagMotion={};memory[bc->unit+0x9E8]=0;
        BagMatrix resting,airPose;assert(positionBackpack(pack,resting,true));
        memory[bc->unit+0x9E8]=0x2000;scalars[bc->unit+0xA48]=-8.f;
        for(unsigned frame=0;frame<100;++frame){
            memory[bc->unit+0xA20]=(frame+1)*16;bagTestTime+=16;assert(positionBackpack(pack,airPose,true));
            if(frame<25){assert(bc->bagMotion.airborneWeight==0);assert(airPose==resting);}
        }
        assert(bc->bagMotion.airborneWeight>.99f);
        const auto restBottom=transformPoint(resting,{{0,0,-.6195f}}),liftedBottom=transformPoint(airPose,{{0,0,-.6195f}});
        assert(liftedBottom[0]<restBottom[0]-.08f&&liftedBottom[2]>restBottom[2]+.001f);
        BagTuningValues paused;assert(bagTuningDefaults(1,2,0,paused));paused.motion=false;
        assert(bagTuningSet(1,2,0,true,paused));assert(positionBackpack(pack,airPose,true));
        assert(!bc->bagMotion.ready);for(unsigned i=0;i<16;++i)assert(std::fabs(airPose[i]-resting[i])<.00001f);
        assert(bagTuningSet(1,2,0,false));bagTestTime+=16;assert(positionBackpack(pack,airPose,true));
        assert(bc->bagMotion.airborneWeight==0);
        for(unsigned frame=0;frame<40;++frame){bagTestTime+=16;assert(positionBackpack(pack,airPose,true));}
        memory[bc->unit+0x9E8]=0;
        // The same decay now starts from double the swing; allow one second
        // to reach this strict absolute pose tolerance after landing.
        for(unsigned frame=0;frame<60;++frame){bagTestTime+=16;assert(positionBackpack(pack,airPose,true));}
        for(unsigned i=0;i<16;++i)assert(std::fabs(airPose[i]-resting[i])<.00001f);
        setBack(bc,bowFixtures[3]);bagTestTime+=16;
        updateWeaponAttachment(pack,identity.data(),nullptr,nullptr,1);
        assert(bc->bagMotion.runWeight==0); // Changing race/gender discards old history.
        matrices[address(pack)+0xBC][0]=0;bagTestTime+=16;
        updateWeaponAttachment(pack,identity.data(),nullptr,nullptr,1);
        assert(!bc->bagMotion.ready&&attachmentUpdate.alpha==0);
        matrices[address(pack)+0xBC]=identity;memory[bc->unit+0x9E8]=0;
        detach(pack);assert(refs[address(pack)]==1);
        assert(setWeapons(&bag)==1&&memory[address(pack)+0x1CC]==player.model&&loads==beforeLoads+1+tuningLoads+stockCloneLoads);
        for(double invalidValue:{-1.,2.,.5,std::numeric_limits<double>::quiet_NaN()}){
            auto invalid=bag;invalid.values[15]=invalidValue;
            assert(setWeapons(&invalid)==-2&&bc->backpack==pack);
        }
        WeaponSelection withShield;for(const auto& asset:weaponAssets)if(asset.kind==3){withShield.items[4]=asset.item;break;}
        auto shieldBag=request(0,withShield,0,0,0,0,1);assert(setWeapons(&shieldBag)==1&&bc->backpack==pack&&bc->extra[4]);
        auto* shieldChild=bc->extra[4];auto withoutBag=request(0,withShield);
        assert(setWeapons(&withoutBag)==1&&!bc->backpack&&!bc->bagMotion.ready&&bc->extra[4]==shieldChild&&!refs[address(pack)]);
        auto off=request(0,empty);assert(setWeapons(&off)==1&&!weaponContext(player.model));
        auto previewBag=request(8,empty,0,0,0,0,1);assert(setWeapons(&previewBag)==1);
        auto* pc=weaponContext(0x6000);assert(pc&&pc->backpack&&pc->token==8);
        pack=pc->backpack;discardInheritedPreviewWeapons(pc->parent);
        assert(memory[address(pack)+0x1CC]==pc->parent);
        setBack(pc,bowFixtures[2]);memory[player.unit+0x9E8]=0x2000;
        BagMatrix previewRest,previewJump;assert(positionBackpack(pack,previewRest,false));
        for(unsigned frame=0;frame<40;++frame){bagTestTime+=16;assert(positionBackpack(pack,previewJump,true));}
        assert(pc->bagMotion.airborneWeight==0);
        for(unsigned i=0;i<16;++i)assert(std::fabs(previewRest[i]-previewJump[i])<.00001f);
        memory[player.unit+0x9E8]=0;
        forgetWeapons(pc->parent);assert(!refs[address(pack)]&&!weaponContext(0x6000));
        assert(setWeapons(&bag)==1);bc=weaponContext(player.model);pack=bc->backpack;
        player.display=999;assert(setWeapons(&bag)==0&&!refs[address(pack)]&&!weaponContext(player.model));
        player.display=player.native;assert(setWeapons(&bag)==1);
        assert(setWeapons(&off)==1&&!weaponContext(player.model));
    }
    // Cross-family ranged appearances use the selected mesh AND its hand and
    // bow callback metadata, retaining ordinary native transition ownership.
    weaponInfoOriginal=&info;
    unsigned rangedKinds[4]{};
    for(const auto& a:weaponAssets){
        if(a.kind==4){
            const int i=a.subclass==2?0:a.subclass==3?1:a.subclass==18?2:a.subclass==19?3:-1;
            if(i>=0)rangedKinds[i]=a.item;
        }
    }
    for(auto actual:rangedKinds)for(auto cosmetic:rangedKinds){
        assert(actual&&cosmetic);realIDs[0]=realIDs[1]=0;realIDs[2]=actual;
        WeaponSelection selection;selection.equipped[2]=actual;selection.items[5]=cosmetic;
        memory[player.unit+0xD40]=0;
        auto on=request(0,selection);assert(setWeapons(&on)==1);
        auto* context=weaponContext(player.model);assert(context&&context->routes[2]==5&&!context->extra[5]);
        const auto* a=weaponAsset(cosmetic);
        for(unsigned animation=0;animation<160;++animation){
            assert(weaponAnimation(pointer(player.unit),animation)==rangedAppearanceAnimation(animation,weaponAsset(actual)->subclass,a->subclass));
            assert(weaponAnimation(pointer(0xDEAD),animation)==animation);
        }
        context->token=1;assert(weaponAnimation(pointer(player.unit),107)==107);context->token=0;
        player.display=999;assert(weaponAnimation(pointer(player.unit),107)==107);player.display=player.native;
        for(auto caller:{0x611E24u,0x60B5BBu,0x60B797u,0x61183Bu,0x624B35u,0x5FD4A5u,0x5FE019u}){
            auto* metadata=weaponInfoAt(pointer(player.unit),2,0,caller);
            assert(metadata==context->rangedInfo.data()&&metadata[1]==a->subclass&&metadata[3]==a->inventory&&metadata[4]==a->sheath);
            assert(metadata[2]==7&&metadata[5]==9&&metadata[6]==10&&metadata[7]==11);
            assert(realInfo[2][1]==weaponAsset(actual)->subclass);
        }
        assert(weaponInfoAt(pointer(player.unit),2,0,0x12345)==realInfo[2].data());
        assert(!scopedProjectileAppearance);
        assert(weaponInfoAt(pointer(player.unit),2,0,0x60A4FE)==realInfo[2].data());
        scopedProjectileAppearance=true;
        const auto* projectileInfo=weaponInfoAt(pointer(player.unit),2,0,0x60A4FE);
        assert(projectileInfo==context->rangedInfo.data()&&projectileInfo[1]==a->subclass);
        assert(projectileInfo[3]==a->inventory&&projectileInfo[4]==a->sheath);
        assert(weaponInfoAt(pointer(player.unit),2,0,0x12345)==realInfo[2].data());
        assert(weaponInfoAt(pointer(0xDEAD),2,0,0x60A4FE)==realInfo[2].data());
        assert(weaponInfoAt(pointer(player.unit),2,1,0x60A4FE)==realInfo[2].data());
        scopedProjectileAppearance=false;
        assert(weaponInfoAt(pointer(player.unit),2,0,0x60A4FE)==realInfo[2].data());
        assert(weaponInfoAt(pointer(player.unit),2,1,0x611E24)==realInfo[2].data());
        assert(weaponInfoAt(pointer(0xDEAD),2,0,0x611E24)==realInfo[2].data());
        const unsigned hand=a->inventory==25||a->inventory==26?1:2;
        memory[player.unit+0xD3C]=0;memory[player.unit+0xD40]=2;sheathTransitionHook(pointer(player.unit),nullptr);
        auto* drawn=findChildOriginal(pointer(player.model),hand);assert(drawn&&weaponModelMatches(drawn,a->model));
        assert(rangeHolder==address(drawn));
        memory[player.unit+0xD3C]=2;memory[player.unit+0xD40]=0;sheathTransitionHook(pointer(player.unit),nullptr);
        auto* stored=findChildOriginal(pointer(player.model),27);assert(stored&&weaponModelMatches(stored,a->model));
        assert(!findChildOriginal(pointer(player.model),hand));
        const auto before=loads;assert(setWeapons(&on)==1&&loads==before);
        // Every race/sex uses its own fit, and drawn models are never tuned.
        for(const auto& fixture:bowFixtures){
            setBack(context,fixture);
            BagTuningValues values;values.scale=100;values.left=.1f;
            assert(bagTuningSet(106,fixture.race,fixture.sex,true,values));
            BagMatrix tuned;assert(tuneStoredPlacement(stored,identity,tuned));
            assert(std::fabs(tuned[13]-.1f)<.00001f);
            memory[address(stored)+0x1D0]=hand;assert(!tuneStoredPlacement(stored,identity,tuned));
            memory[address(stored)+0x1D0]=27;
            assert(bagTuningSet(106,fixture.race,fixture.sex,false));
        }
        WeaponSelection empty;empty.equipped[2]=actual;auto off=request(0,empty);
        assert(setWeapons(&off)==1&&!weaponContext(player.model));
        for(unsigned animation=0;animation<160;++animation)assert(weaponAnimation(pointer(player.unit),animation)==animation);
    }
    // A carried choice replaces the matching stock stowed weapon even with no
    // explicit in-hand appearance. The stock child still owns draw/sheath.
    struct CarriedReplacementCase {unsigned actual,role,position,home;};
    for(const auto& test:std::array<CarriedReplacementCase,9>{{
        {25,0,0,32},{25,1,1,33},{35,0,2,30},{1117,1,3,31},
        {778,0,2,26},{778,1,3,27},{143,1,4,28},
        {1046,2,5,26},{5259,2,5,28}}}){
        realIDs[0]=realIDs[1]=realIDs[2]=0;realIDs[test.role]=test.actual;
        WeaponSelection selection;selection.independent=true;
        selection.equipped[test.role]=test.actual;
        selection.items={{25,25,35,35,143,rangedKinds[0],quiver}};
        memory[player.unit+0xD3C]=0;memory[player.unit+0xD40]=0;
        auto on=request(0,selection);assert(setWeapons(&on)==1);
        auto* context=weaponContext(player.model);assert(context&&context->routes[test.role]==-1);
        const auto* actual=weaponAsset(test.actual);
        auto* native=findChildHook(pointer(player.model),nullptr,test.home);
        auto* carried=context->extra[test.position];assert(native&&carried&&native!=carried);
        assert(weaponModelMatches(native,actual->model)&&hideStoredWeapon(native)&&!hideStoredWeapon(carried));
        const auto beforeRefs=refs;const auto beforeLoads=loads;
        updateWeaponAttachment(native,attachment,nullptr,nullptr,.8f);assert(attachmentUpdate.alpha==0);
        updateWeaponAttachment(carried,attachment,nullptr,nullptr,.8f);assert(attachmentUpdate.alpha==.8f);
        if(actual->kind==4&&actual->subclass==2){
            const auto calls=bowStringSubmission.calls;
            bowStringDrawHook(native,nullptr,pointer(player.unit));assert(bowStringSubmission.calls==calls);
        }
        assert(refs==beforeRefs&&loads==beforeLoads);
        // A shared attachment point is not enough to hide quivers or props.
        modelName(native,"Item\\ObjectComponents\\Quiver\\Quiver_A.mdx");
        assert(!hideStoredWeapon(native));modelName(native,actual->model);
        for(unsigned point:{0u,1u,2u,29u}){
            memory[address(native)+0x1D0]=point;assert(!hideStoredWeapon(native));
        }
        memory[address(native)+0x1D0]=test.home;
        memory[address(carried)+0x10]=0;assert(!hideStoredWeapon(native));
        memory[address(carried)+0x10]=1;assert(hideStoredWeapon(native));
        detach(carried);assert(!hideStoredWeapon(native));
        assert(setWeapons(&on)==1&&context->extra[test.position]==carried&&hideStoredWeapon(native));
        // Every carried child stays in place as real equipment is drawn.
        const auto carriedChildren=context->extra;
        memory[player.unit+0xD40]=test.role==2?2:1;
        sheathTransitionHook(pointer(player.unit),nullptr);
        const unsigned hand=test.role==0||(test.role==2&&(actual->inventory==25||actual->inventory==26))?1:actual->kind==3?0:2;
        auto* held=findChildHook(pointer(player.model),nullptr,hand);
        assert(held&&weaponModelMatches(held,actual->model)&&!hideStoredWeapon(held));
        if(actual->kind==4&&actual->subclass==2){
            const auto calls=bowStringSubmission.calls;
            bowStringDrawHook(held,nullptr,pointer(player.unit));assert(bowStringSubmission.calls==calls+1);
        }
        assert(context->extra==carriedChildren);
        for(unsigned position=0;position<7;++position)
            assert(memory[address(context->extra[position])+0x1D0]==weaponPoints[position]);
        memory[player.unit+0xD3C]=test.role==2?2:1;
        memory[player.unit+0xD40]=test.role==2?1:0;
        sheathTransitionHook(pointer(player.unit),nullptr);
        native=findChildHook(pointer(player.model),nullptr,test.home);
        assert(native&&weaponModelMatches(native,actual->model)&&hideStoredWeapon(native));
        assert(context->extra==carriedChildren);
        // Clearing only this location restores the real stowed appearance;
        // custom appearances at all other locations must not suppress it.
        selection.items[test.position]=0;on=request(0,selection);
        memory[player.unit+0xD40]=0;assert(setWeapons(&on)==1);
        native=findChildHook(pointer(player.model),nullptr,test.home);
        assert(native&&weaponModelMatches(native,actual->model)&&!hideStoredWeapon(native));
        updateWeaponAttachment(native,attachment,nullptr,nullptr,.8f);assert(attachmentUpdate.alpha==.8f);
        WeaponSelection empty;empty.equipped=selection.equipped;auto off=request(0,empty);
        assert(setWeapons(&off)==1&&!weaponContext(player.model));
    }
    // Explicit in-use appearances and carried decorations are separate children.
    for(auto actual:rangedKinds)for(auto cosmetic:rangedKinds){
        realIDs[0]=realIDs[1]=0;realIDs[2]=actual;
        WeaponSelection selection;selection.independent=true;selection.equipped[2]=actual;
        selection.items[5]=rangedKinds[0];selection.items[9]=cosmetic;
        memory[player.unit+0xD40]=0;
        auto on=request(0,selection);assert(setWeapons(&on)==1);
        auto* c=weaponContext(player.model);assert(c&&c->routes[2]==9&&c->extra[5]);
        auto* carried=c->extra[5];const auto* chosen=weaponAsset(cosmetic);
        assert(!hideStoredWeapon(carried));
        auto* stowed=findChildHook(pointer(player.model),nullptr,27);
        assert(stowed&&weaponModelMatches(stowed,chosen->model)&&hideStoredWeapon(stowed));
        memory[player.unit+0xD3C]=0;memory[player.unit+0xD40]=2;
        sheathTransitionHook(pointer(player.unit),nullptr);
        const unsigned hand=chosen->inventory==25||chosen->inventory==26?1:2;
        auto* held=findChildHook(pointer(player.model),nullptr,hand);
        assert(held&&weaponModelMatches(held,chosen->model)&&!hideStoredWeapon(held));
        assert(c->extra[5]==carried&&memory[address(carried)+0x1D0]==27&&rangeHolder==address(held));
        auto before=loads;assert(setWeapons(&on)==1&&loads==before);
        memory[player.unit+0xD3C]=2;memory[player.unit+0xD40]=0;
        sheathTransitionHook(pointer(player.unit),nullptr);
        assert(c->extra[5]==carried&&refs[address(carried)]==2);
        // Clearing the in-use choice restores real equipment; back decoration remains.
        selection.items[9]=0;on=request(0,selection);assert(setWeapons(&on)==1);
        assert(c->routes[2]==-1&&c->extra[5]);
        WeaponSelection empty;empty.equipped[2]=actual;auto off=request(0,empty);
        assert(setWeapons(&off)==1&&!weaponContext(player.model));
    }
    // A server-provided real wand is valid equipment even without a catalog ID.
    {
        realIDs[2]=999999;serverRangedAppearance=rangedKinds[3];
        WeaponSelection s;s.independent=true;s.equipped[2]=999999;s.items[9]=rangedKinds[1];
        auto on=request(0,s);assert(setWeapons(&on)==1);
        auto* c=weaponContext(player.model);assert(c&&c->routes[2]==9);
        assert(weaponInfoAt(pointer(player.unit),2,0,0x5FE019)[1]==3);
        assert(weaponAnimation(pointer(player.unit),107)==49);
        memory[player.unit+0xD3C]=0;memory[player.unit+0xD40]=2;
        sheathTransitionHook(pointer(player.unit),nullptr);
        auto* held=findChildHook(pointer(player.model),nullptr,1);
        assert(held&&weaponModelMatches(held,weaponAsset(rangedKinds[1])->model));
        memory[player.unit+0xD3C]=2;memory[player.unit+0xD40]=0;
        sheathTransitionHook(pointer(player.unit),nullptr);
        WeaponSelection empty;auto off=request(0,empty);assert(setWeapons(&off)==1);
        serverRangedAppearance=0;realIDs[2]=rangedKinds[3];
    }
    // Independent preview poses own their children and do not leak into the world.
    {
        memory[0x6000+0x10]=1;previews.entries[0]={};
        previews.entries[0].model=0x6000;previews.entries[0].guid=player.guid;
        previews.entries[0].token=77;previews.entries[0].status=1;
        WeaponSelection s;s.independent=true;s.items[5]=rangedKinds[0];s.items[9]=rangedKinds[1];s.equipped[2]=rangedKinds[3];
        auto on=request(77,s);on.values.push_back(2);assert(setWeapons(&on)==1);
        auto* c=weaponContext(0x6000);assert(c&&c->extra[5]&&c->extra[9]);
        assert(memory[address(c->extra[9])+0x1D0]==1);
        on.values[20]=0;assert(setWeapons(&on)==1&&c->extra[5]&&!c->extra[9]);
        auto invalid=on;invalid.values[16]=-1;assert(setWeapons(&invalid)==-2);
        invalid=on;invalid.values[20]=3;assert(setWeapons(&invalid)==-2);
        forgetWeapons(0x6000);previews.entries[0]={};
    }
    // Explicit carried mode controls the entire stored loadout. An empty
    // carried section deliberately means an empty back, not native fallback.
    for(const auto& test:std::array<CarriedReplacementCase,9>{{
        {25,0,0,32},{25,1,1,33},{35,0,2,30},{1117,1,3,31},
        {778,0,2,26},{778,1,3,27},{143,1,4,28},
        {1046,2,5,26},{5259,2,5,28}}}){
        realIDs[0]=realIDs[1]=realIDs[2]=0;realIDs[test.role]=test.actual;
        WeaponSelection selection;selection.independent=true;selection.carriedMode=1;
        selection.equipped[test.role]=test.actual;
        memory[player.unit+0xD3C]=0;memory[player.unit+0xD40]=0;
        auto on=request(0,selection,0,1,1);assert(setWeapons(&on)==1);
        auto* context=weaponContext(player.model);assert(context&&context->selection.empty());
        const auto* actual=weaponAsset(test.actual);
        auto* native=findChildHook(pointer(player.model),nullptr,test.home);
        assert(native&&hideStoredWeapon(native)&&!context->hideRangedWhenStored&&!context->hideMeleeWhenStored);
        assert(context->nativeChildren[test.role]==native);
        const auto idleLoads=loads,idleMelee=meleeRefreshes,idleRange=rangedRefreshes;
        for(unsigned repeat=0;repeat<3;++repeat)assert(setWeapons(&on)==1);
        assert(loads==idleLoads&&meleeRefreshes==idleMelee&&rangedRefreshes==idleRange);
        const auto guid=context->guid;context->guid=0;assert(!hideStoredWeapon(native));context->guid=guid;
        selection.items={{25,25,35,35,143,rangedKinds[0],quiver}};
        on=request(0,selection,0,1,1);assert(setWeapons(&on)==1);
        auto carried=context->extra;
        for(unsigned i=0;i<7;++i)assert(carried[i]&&!hideStoredWeapon(carried[i]));
        memory[player.unit+0xD3C]=0;memory[player.unit+0xD40]=test.role==2?2:1;
        sheathTransitionHook(pointer(player.unit),nullptr);
        const unsigned hand=test.role==0||(test.role==2&&(actual->inventory==25||actual->inventory==26))?1:actual->kind==3?0:2;
        auto* held=findChildHook(pointer(player.model),nullptr,hand);
        assert(held&&!hideStoredWeapon(held)&&weaponModelMatches(held,actual->model));
        assert(context->extra==carried);
        for(unsigned i=0;i<7;++i)assert(memory[address(carried[i])+0x1D0]==weaponPoints[i]);
        memory[player.unit+0xD3C]=test.role==2?2:1;memory[player.unit+0xD40]=test.role==2?1:0;
        sheathTransitionHook(pointer(player.unit),nullptr);
        native=findChildHook(pointer(player.model),nullptr,test.home);
        assert(native&&hideStoredWeapon(native)&&context->extra==carried);
        selection.carriedMode=0;
        auto off=request(0,selection,0,1,1);memory[player.unit+0xD40]=0;
        assert(setWeapons(&off)==1&&!weaponContext(player.model));
        native=findChildHook(pointer(player.model),nullptr,test.home);
        assert(native&&!hideStoredWeapon(native));
        for(auto extra:carried)assert(!refs[address(extra)]);
    }
    // An appearance with carrying disabled sheathes at the chosen weapon's
    // natural home. Native sheath-zero bows/wands stay valid in the hand.
    for(auto actual:rangedKinds)for(auto cosmetic:rangedKinds){
        realIDs[0]=realIDs[1]=0;realIDs[2]=actual;
        WeaponSelection selection;selection.independent=true;selection.carriedMode=0;
        selection.equipped[2]=actual;selection.items[9]=cosmetic;
        selection.items[5]=rangedKinds[0]; // Defensive suppression, even for callers retaining IDs.
        memory[player.unit+0xD3C]=0;memory[player.unit+0xD40]=0;
        auto off=request(0,selection,0,1,1);assert(setWeapons(&off)==1);
        auto* context=weaponContext(player.model);assert(context&&context->routes[2]==9&&!context->extra[5]);
        const auto* chosen=weaponAsset(cosmetic);
        const unsigned hand=chosen->inventory==25||chosen->inventory==26?1:2;
        const int home=sheath(chosen->sheath,hand==1);
        assert(selectedWeaponHome(context->selection,2,9)==home);
        if(home>=0){
            auto* stored=findChildHook(pointer(player.model),nullptr,home);
            assert(stored&&weaponModelMatches(stored,chosen->model)&&!hideStoredWeapon(stored));
        }
        memory[player.unit+0xD40]=2;sheathTransitionHook(pointer(player.unit),nullptr);
        auto* held=findChildHook(pointer(player.model),nullptr,hand);
        assert(held&&weaponModelMatches(held,chosen->model)&&!hideStoredWeapon(held));
        selection.carriedMode=1;auto on=request(0,selection);
        assert(setWeapons(&on)==1&&context->extra[5]);
        held=findChildHook(pointer(player.model),nullptr,hand);assert(held&&!hideStoredWeapon(held));
        auto* carried=context->extra[5];assert(!hideStoredWeapon(carried));
        memory[player.unit+0xD3C]=2;memory[player.unit+0xD40]=0;sheathTransitionHook(pointer(player.unit),nullptr);
        auto* stored=findChildHook(pointer(player.model),nullptr,27);
        assert(stored&&hideStoredWeapon(stored)&&!hideStoredWeapon(carried));
        assert(setWeapons(&off)==1&&!context->extra[5]&&!refs[address(carried)]);
        if(home>=0){
            stored=findChildHook(pointer(player.model),nullptr,home);
            assert(stored&&weaponModelMatches(stored,chosen->model)&&!hideStoredWeapon(stored));
        }
        WeaponSelection empty;empty.carriedMode=0;empty.equipped[2]=actual;
        auto clearRequest=request(0,empty);assert(setWeapons(&clearRequest)==1&&!weaponContext(player.model));
    }
    // Catalog-independent composition ownership hides a server-only weapon,
    // while an unrelated prop at the same point and every in-hand point stay visible.
    {
        realIDs[0]=realIDs[1]=realIDs[2]=0;
        WeaponSelection selection;selection.independent=true;selection.carriedMode=1;
        selection.equipped[0]=999999;
        auto on=request(0,selection);assert(setWeapons(&on)==1);
        auto* context=weaponContext(player.model);assert(context);
        const auto* sample=weaponAsset(35);
        weaponComposeHook(pointer(player.model),weaponDisplay(sample),15,sample->sheath,1,0,0);
        auto* unknown=findChildHook(pointer(player.model),nullptr,30);
        assert(unknown&&context->nativeChildren[0]==unknown&&hideStoredWeapon(unknown));
        for(unsigned hand:{0u,1u,2u}){memory[address(unknown)+0x1D0]=hand;assert(!hideStoredWeapon(unknown));}
        memory[address(unknown)+0x1D0]=30;
        selection.items[7]=35;on=request(0,selection);assert(setWeapons(&on)==1);
        assert(!refs[address(unknown)]&&!findChildHook(pointer(player.model),nullptr,30));
        weaponComposeHook(pointer(player.model),weaponDisplay(sample),15,sample->sheath,1,0,0);
        auto* replacement=findChildHook(pointer(player.model),nullptr,33);
        assert(replacement&&hideStoredWeapon(replacement));
        selection.items[7]=0;on=request(0,selection);assert(setWeapons(&on)==1&&!refs[address(replacement)]);
        factory(pointer(player.model),30,"Unrelated\\Prop.mdx","",0);
        assert(!refs[address(unknown)]&&!context->nativeChildren[0]);
        auto* prop=findChildHook(pointer(player.model),nullptr,30);assert(prop&&!hideStoredWeapon(prop));
        assert(weaponComposeHook(pointer(player.model),nullptr,15,sample->sheath,1,0,0)==-1);
        assert(!context->nativeChildren[0]&&!hideStoredWeapon(prop)&&refs[address(prop)]==1);
        factory(pointer(player.model),26,weaponAsset(quiver)->model,"",0);
        auto* nativeQuiver=findChildHook(pointer(player.model),nullptr,26);
        assert(nativeQuiver&&hideNativeQuiver(nativeQuiver)&&!hideStoredWeapon(nativeQuiver));
        context->guid=0;assert(!hideNativeQuiver(nativeQuiver));context->guid=player.guid;
        selection.carriedMode=0;auto off=request(0,selection);assert(setWeapons(&off)==1&&!weaponContext(player.model));
        assert(!hideNativeQuiver(nativeQuiver));
    }
    // Preview carrying has the same all-or-nothing semantics, with held
    // appearance poses separate and actual quivers restored when switched off.
    {
        memory[0x6000+0x10]=1;previews.entries[0]={};
        previews.entries[0].model=0x6000;previews.entries[0].guid=player.guid;
        previews.entries[0].token=88;previews.entries[0].status=1;
        WeaponSelection selection;selection.independent=true;selection.carriedMode=1;
        selection.equipped[2]=rangedKinds[3];selection.items[9]=1046;
        selection.items[5]=rangedKinds[0];selection.items[6]=quiver;
        auto on=request(88,selection,0,1,1,quiver,1);on.values[20]=2;
        assert(setWeapons(&on)==1);auto* context=weaponContext(0x6000);
        assert(context&&context->extra[5]&&context->extra[6]&&context->extra[9]&&context->backpack&&!context->passthroughQuiver);
        const auto* chosen=weaponAsset(selection.items[9]);
        const int naturalHome=sheath(chosen->sheath,chosen->inventory==25||chosen->inventory==26);
        assert(naturalHome>=0&&naturalHome!=static_cast<int>(weaponPoints[9]));
        factory(pointer(context->parent),naturalHome,chosen->model,"",0);
        auto* nativePreview=findChildHook(pointer(context->parent),nullptr,naturalHome);
        assert(nativePreview&&hideStoredWeapon(nativePreview)&&!hideStoredWeapon(context->extra[9]));
        auto* bag=context->backpack;auto* ownedQuiver=context->extra[6];
        assert(!hideNativeQuiver(ownedQuiver)&&!hideStoredWeapon(ownedQuiver));
        selection.items[5]=selection.items[6]=0;
        auto empty=request(88,selection,0,1,1,quiver,1);empty.values[20]=2;
        assert(setWeapons(&empty)==1&&!context->extra[5]&&!context->extra[6]&&!context->passthroughQuiver);
        assert(context->backpack==bag&&context->extra[9]&&!hideStoredWeapon(context->extra[9]));
        for(double flag:{-1.,.5,2.,std::numeric_limits<double>::infinity(),std::numeric_limits<double>::quiet_NaN()}){
            auto invalid=empty;invalid.values[21]=flag;
            const auto beforeRefs=refs;assert(setWeapons(&invalid)==-2&&refs==beforeRefs&&context->selection.carriedMode==1);
        }
        selection.carriedMode=0;selection.independent=false;
        auto off=request(88,selection,0,1,1,quiver,1);off.values[20]=2;
        assert(setWeapons(&off)==1&&context->passthroughQuiver&&context->extra[9]&&context->backpack==bag);
        assert(!hideNativeQuiver(context->passthroughQuiver));
        off.values[20]=0;assert(setWeapons(&off)==1&&context->extra[9]);
        assert(memory[address(context->extra[9])+0x1D0]==static_cast<unsigned>(naturalHome));
        assert(!hideStoredWeapon(context->extra[9])&&context->backpack==bag);
        forgetWeapons(0x6000);previews.entries[0]={};
    }
    // Selected staves fit the body's actual authored staff mount in Simple
    // mode, while passthrough equipment and all held weapons remain native.
    {
        const StaffFit* fit=nullptr;
        for(const auto& candidate:staffFits)
            if(candidate.race==4&&candidate.sex==1&&candidate.point==30)fit=&candidate;
        assert(fit&&fit->inward>0);
        const auto* asset=weaponAsset(6215);assert(asset&&asset->subclass==10);
        const auto* shaft=staffShaftFor(asset->model);assert(shaft);
        const auto close=[](const BagMatrix& actual,const BagMatrix& expected){
            for(unsigned i=0;i<16;++i)assert(std::fabs(actual[i]-expected[i])<.00008f);
        };
        // A staff's long X axis lies along the torso's Z axis. Rotate about
        // the authored grip pivot, retaining its original position exactly.
        BagMatrix staffBone{{0,0,-1,0,0,1,0,0,1,0,0,0,0,0,0,1}};
        for(unsigned row=0;row<3;++row){
            staffBone[12+row]=fit->anchor[row];
            for(unsigned axis=0;axis<3;++axis)
                staffBone[12+row]-=staffBone[axis*4+row]*fit->anchor[axis];
        }
        auto authored=staffBone;
        for(unsigned row=0;row<3;++row)authored[12+row]=fit->anchor[row];
        auto local=identity;local[0]=1.1f;local[5]=.95f;local[10]=.9f;local[14]=.004f;
        const float inward=fit->inward-shaft->maxZ*.9f-.004f-.012f;
        assert(inward>.005f&&inward<.12f);
        auto expected=authored;expected[12]+=inward;
        const auto skeleton=[&](WeaponContext* context,std::uintptr_t base,const BagMatrix& view){
            memory[context->parent+0x30]=base;memory[base+0x130]=base+0x1000;
            memory[base+0x1000+0x10C]=37;memory[base+0x1000+0x110]=base+0x2000;
            memory[base+0x1000+0x104]=34;memory[base+0x1000+0x108]=base+0x3000;
            memory[base+0x1000+0x34]=128;memory[base+0x1000+0x38]=base+0x7000;
            memory[context->parent+0x94]=base+0x5000;
            for(unsigned point:{28u,30u}){
                const unsigned bone=point==30?63:62;
                memory[base+0x2000+2*point]=point;
                const auto record=base+0x3000+48*point;
                memory[record]=point;memory[record+4]=bone;
                positions[record+8]=point==30?fit->anchor:bagFits[7].anchor;
                memory[base+0x7000+108*bone+8]=18;
                matrices[base+0x5000+64*bone]=bagMatrixProduct(view,point==30?staffBone:identity);
            }
            matrices[base+0x5000+64*18]=view;
            matrices[context->parent+0xFC]=view;
        };

        realIDs[0]=35;realIDs[1]=realIDs[2]=0;
        WeaponSelection selection;selection.carriedMode=0;selection.equipped[0]=35;selection.items[7]=6215;
        memory[player.unit+0xD3C]=0;memory[player.unit+0xD40]=0;
        auto on=request(0,selection);assert(setWeapons(&on)==1);
        auto* context=weaponContext(player.model);assert(context&&context->routes[0]==7);
        auto* child=findChildHook(pointer(player.model),nullptr,30);
        assert(child&&context->nativeChildren[0]==child&&weaponModelMatches(child,asset->model));
        const std::uintptr_t worldData=0xA100000;
        skeleton(context,worldData,identity);matrices[address(child)+0xBC]=local;
        BagMatrix fitted;assert(positionStoredStaff(child,fitted));close(fitted,expected);
        const auto originalBones=matrices[worldData+0x5000+64*63];
        const auto originalLocal=matrices[address(child)+0xBC];
        const auto originalRefs=refs;const auto originalLoads=loads;
        const float color[]={.3f,.5f,.7f,1},lighting[]={1,1,1,1};
        captureAttachmentMatrix=true;
        for(unsigned frame=0;frame<20;++frame){
            const auto updates=attachmentUpdate.calls;
            updateWeaponAttachment(child,authored.data(),color,lighting,.75f);
            assert(attachmentUpdate.calls==updates+1&&attachmentUpdate.model==child&&attachmentUpdate.alpha==.75f);
            assert(attachmentUpdate.color==color&&attachmentUpdate.lighting==lighting);
            close(capturedAttachmentMatrix,expected);
            assert(memory[address(child)+0x1D0]==30&&memory[address(child)+0x1CC]==player.model);
        }
        assert(refs==originalRefs&&loads==originalLoads&&matrices[worldData+0x5000+64*63]==originalBones);
        assert(matrices[address(child)+0xBC]==originalLocal);

        // The update receives a model/view matrix already present in bones.
        // A reflected, zoomed and translated preview must transform once.
        auto view=identity;view[0]=0;view[1]=1.6f;view[4]=.8f;view[5]=0;view[10]=1.3f;
        view[12]=8;view[13]=-6;view[14]=3;
        skeleton(context,worldData,view);
        assert(positionStoredStaff(child,fitted));close(fitted,bagMatrixProduct(view,expected));
        skeleton(context,worldData,identity);

        for(unsigned hand:{0u,1u,2u}){
            memory[address(child)+0x1D0]=hand;assert(!positionStoredStaff(child,fitted));
            updateWeaponAttachment(child,authored.data(),color,lighting,.75f);
            close(capturedAttachmentMatrix,authored);assert(attachmentUpdate.alpha==.75f);
        }
        memory[address(child)+0x1D0]=30;
        const auto owner=context->guid;context->guid=owner+1;
        assert(!positionStoredStaff(child,fitted));context->guid=owner;
        context->routes[0]=-1;assert(!positionStoredStaff(child,fitted));context->routes[0]=7;
        modelName(child,"Unrelated\\UnknownStaff.mdx");assert(!positionStoredStaff(child,fitted));
        modelName(child,asset->model);
        const auto staffRecord=worldData+0x3000+48*30;
        positions[staffRecord+8][0]+=.025f;assert(!positionStoredStaff(child,fitted));
        updateWeaponAttachment(child,authored.data(),color,lighting,.75f);
        close(capturedAttachmentMatrix,authored);assert(attachmentUpdate.alpha==.75f);
        positions[staffRecord+8]=fit->anchor;
        memory[worldData+0x2000+2*30]=0xFFFF;assert(!positionStoredStaff(child,fitted));
        memory[worldData+0x2000+2*30]=30;
        matrices[player.model+0xFC][0]=0;assert(!positionStoredStaff(child,fitted));
        matrices[player.model+0xFC]=identity;
        assert(positionStoredStaff(child,fitted));close(fitted,expected);
        captureAttachmentMatrix=false;
        WeaponSelection empty;empty.carriedMode=0;empty.equipped[0]=35;
        auto off=request(0,empty);assert(setWeapons(&off)==1&&!weaponContext(player.model));

        // Addon previews own independent staff children. Advanced placement
        // tuning is additive to the contact fit and cannot accumulate per frame.
        memory[0x6000+0x10]=1;previews.entries[0]={};
        previews.entries[0].model=0x6000;previews.entries[0].guid=player.guid;
        previews.entries[0].token=89;previews.entries[0].status=1;
        selection={};selection.independent=true;selection.carriedMode=1;selection.items[2]=6215;
        on=request(89,selection);assert(setWeapons(&on)==1);
        context=weaponContext(0x6000);assert(context&&context->extra[2]);child=context->extra[2];
        skeleton(context,0xA200000,identity);matrices[address(child)+0xBC]=local;
        assert(positionStoredStaff(child,fitted));close(fitted,expected);
        const auto previewRefs=refs;const auto previewLoads=loads;
        BagTuningValues adjustment;adjustment.scale=100;adjustment.inset=.01f;adjustment.left=.03f;adjustment.up=.02f;
        assert(bagTuningSet(103,4,1,true,adjustment));
        auto tunedExpected=expected;tunedExpected[12]+=.01f;tunedExpected[13]+=.03f;tunedExpected[14]+=.02f;
        captureAttachmentMatrix=true;
        for(unsigned frame=0;frame<20;++frame){
            updateWeaponAttachment(child,authored.data(),color,lighting,.8f);
            close(capturedAttachmentMatrix,tunedExpected);assert(attachmentUpdate.alpha==.8f);
        }
        assert(refs==previewRefs&&loads==previewLoads&&context->extra[2]==child);
        assert(bagTuningSet(103,4,1,false));
        updateWeaponAttachment(child,authored.data(),color,lighting,.8f);
        close(capturedAttachmentMatrix,expected);
        memory[address(child)+0x1D0]=1;assert(!positionStoredStaff(child,fitted));
        memory[address(child)+0x1D0]=30;
        context->selection.items[2]=999999;assert(!positionStoredStaff(child,fitted));
        context->selection.items[2]=6215;
        captureAttachmentMatrix=false;forgetWeapons(0x6000);previews.entries[0]={};
    }
    {
        const auto bagRequest=[](unsigned token){Lua call;call.values.resize(17,0);call.values[0]=token;return call;};
        auto choices=bagRequest(0);
        for(unsigned i=0;i<8;++i){choices.values[1+i*2]=2;choices.values[2+i*2]=i%3;}
        const auto initialLoads=loads;
        assert(setBagsStatus(&choices)==1&&loads==initialLoads+8);
        auto* context=weaponContext(player.model);assert(context);
        const auto firstGeneration=lastBagGeneration;assert(firstGeneration&&context->bagGeneration==firstGeneration);
        std::array<void*,8> children{};
        for(unsigned i=0;i<8;++i){
            const auto& bag=context->bags[i];children[i]=bag.child;
            assert(bag.model==2&&bag.mount==i%3&&bag.child&&refs[address(bag.child)]==2);
            assert(ownedExtra(*context,bag.child)&&memory[address(bag.child)+0x1D0]==bagAttachment(i%3));
            assert(findChildHook(pointer(context->parent),nullptr,bagAttachment(i%3))!=bag.child);
            for(unsigned j=0;j<i;++j)assert(children[j]!=children[i]);
        }
        assert(setBagsStatus(&choices)==1&&loads==initialLoads+8&&lastBagGeneration==firstGeneration);
        for(unsigned point:{28u,32u,33u})clearChildrenHook(pointer(context->parent),nullptr,point);
        for(auto child:children)assert(memory[address(child)+0x1CC]==context->parent&&refs[address(child)]==2);
        WeaponSelection empty;auto weapons=request(0,empty);
        assert(setWeapons(&weapons)==1&&weaponContext(player.model)==context);
        for(unsigned i=0;i<8;++i)assert(context->bags[i].child==children[i]);
        detach(children[0]);assert(refs[address(children[0])]==1);
        assert(setWeapons(&weapons)==1&&context->bags[0].child==children[0]&&refs[address(children[0])]==2);

        // Real build5875 hip bones for all 16 body types and both sides.
        for(const auto& fixture:bowFixtures){
            setBack(context,fixture);
            const auto base=context==c?0x2000000u:0x3000000u;
            for(const auto& hip:bagHipFixtures)if(hip.race==fixture.race&&hip.sex==fixture.sex){
                memory[base+0x2000+2*hip.point]=hip.index;
                const auto record=base+0x3000+48*hip.index;
                memory[record]=hip.point;memory[record+4]=hip.bone;positions[record+8]=hip.position;
                memory[base+0x7000+108*hip.bone+8]=hip.parent;
                matrices[base+0x5000+64*hip.bone]=identity;matrices[base+0x5000+64*hip.parent]=identity;
            }
            for(unsigned i=0;i<8;++i){
                BagMatrix placed;assert(positionBackpack(children[i],placed));
                BagTuningValues defaults;assert(bagInstanceTuningDefaults(i%3,fixture.race,fixture.sex,defaults));
                Lua query{{double(201+i),double(fixture.race),double(fixture.sex),double(i%3)}};
                luaOutput.clear();assert(getBagFitDefaults(&query)==8&&luaOutput[7]==defaults.scale);
                for(unsigned column:{0u,4u,8u}){
                    float length=0;for(unsigned axis=0;axis<3;++axis)length+=placed[column+axis]*placed[column+axis];
                    assert(std::fabs(std::sqrt(length)-.45f*defaults.scale/100)<.00001f);
                }
                if(i%3){
                    const auto& hip=bagHipFixtures[4*(fixture.race-1)+2*fixture.sex+(i%3-1)];
                    assert(std::fabs(placed[12]-hip.position[0])<.00001f&&std::fabs(placed[13]-hip.position[1])<.00001f);
                    assert(std::fabs(placed[14]-hip.position[2]+.15f)<.00001f);
                    assert(i%3==1?placed[1]<0:placed[1]>0); // Bag faces away from the selected hip.
                }
            }
        }
        setBack(context,bowFixtures[0]);
        Lua firstFit{{0,1,1,0,1,.22,.03,.04,5,6,7,90,0}};
        Lua otherFit{{0,4,1,0,1,-.22,.02,.08,-5,-6,-7,110,1}};
        assert(setBagInstanceFit(&firstFit)==1&&setBagInstanceFit(&otherFit)==1);
        assert(context->bags[0].fits[0].values.left==.22f&&context->bags[3].fits[0].values.left==-.22f);
        const auto revision=context->bags[0].fits[0].revision;
        assert(setBagInstanceFit(&firstFit)==1&&context->bags[0].fits[0].revision==revision);
        BagMatrix firstPose,otherPose;
        assert(positionBackpack(children[0],firstPose)&&positionBackpack(children[3],otherPose)&&firstPose!=otherPose);
        for(unsigned index:{1u,2u,3u,4u,5u,8u,11u,12u}){
            auto invalid=firstFit;invalid.values[index]=std::numeric_limits<double>::quiet_NaN();
            assert(setBagInstanceFit(&invalid)==-2&&context->bags[0].fits[0].revision==revision);
        }
        auto badFit=firstFit;badFit.values[5]=1.0000000001;
        assert(setBagInstanceFit(&badFit)==-2&&context->bags[0].fits[0].revision==revision);
        badFit=firstFit;badFit.values[7]=std::nextafter(-3.,-std::numeric_limits<double>::infinity());
        assert(setBagInstanceFit(&badFit)==-2&&context->bags[0].fits[0].revision==revision);
        auto footRange=firstFit;footRange.values[7]=-3;
        assert(setBagInstanceFit(&footRange)==1&&context->bags[0].fits[0].values.up==-3);
        assert(setBagInstanceFit(&firstFit)==1);
        for(double value:{-1.,.5,17.,999999.,std::numeric_limits<double>::infinity()}){
            auto invalid=choices;invalid.values[1]=value;
            assert(setBagsStatus(&invalid)==-2&&context->bags[0].child==children[0]);
        }
        auto invalid=choices;invalid.values[2]=3;assert(setBagsStatus(&invalid)==-2);
        invalid=choices;invalid.values.pop_back();assert(setBagsStatus(&invalid)==-2);
        invalid=choices;invalid.values.push_back(0);assert(setBagsStatus(&invalid)==-2);
        invalid=choices;invalid.values[1]=0;invalid.values[2]=1;assert(setBagsStatus(&invalid)==-2);

        // Each addon preview has its own fits even when model and instance IDs match.
        const auto previewParent=std::uintptr_t(0x6000);
        previews.entries[0]={};previews.entries[0].model=previewParent;previews.entries[0].guid=player.guid;
        previews.entries[0].token=87;previews.entries[0].status=1;memory[previewParent+0x10]=1;
        auto previewChoices=choices;previewChoices.values[0]=87;assert(setBagsStatus(&previewChoices)==1);
        auto* preview=weaponContext(previewParent);assert(preview&&preview!=context);
        auto previewFit=firstFit;previewFit.values[0]=87;previewFit.values[5]=.55;
        assert(setBagInstanceFit(&previewFit)==1&&preview->bags[0].fits[0].values.left==.55f);
        assert(context->bags[0].fits[0].values.left==.22f);

        // The clone callback supplies exact child identities before parent registration.
        // Two copies of one mesh retain different placement after world choices change.
        const auto stockParent=std::uintptr_t(0xD1000);WeaponContext stock;stock.parent=stockParent;
        setBack(&stock,bowFixtures[0]);
        std::array<void*,2> copies{};
        for(unsigned n=0;n<2;++n){
            const unsigned source=n?3:0;
            loadingExtraParent=stockParent;factory(pointer(stockParent),28,bagAsset(2)->model,bagAsset(2)->texture,0);loadingExtraParent=0;
            copies[n]=pointer(memory[stockParent+0x1DC]);
            rememberClonedBagPreview(address(children[source]),address(copies[n]));
        }
        rememberClonedBagPreview(context->parent,stockParent);
        assert(clonedBagOwner(stockParent)==player.guid);
        for(unsigned n=0;n<2;++n){
            const auto* clone=clonedBagChild(address(copies[n]));assert(clone&&clone->identity==(n?204u:201u));
            BagMatrix pose;assert(positionBackpack(copies[n],pose));
            const auto& expected=n?otherPose:firstPose;for(unsigned i=0;i<16;++i)assert(std::fabs(pose[i]-expected[i])<.00001f);
        }
        auto cloneFit=firstFit;cloneFit.values[5]=.8;assert(setBagInstanceFit(&cloneFit)==1);
        assert(clonedBagChild(address(copies[0]))->bag.fits[0].values.left==.22f);
        const auto destroyed=address(copies[0]);detach(copies[0]);assert(!clonedBagChild(destroyed));
        detach(copies[1]);forgetWeapons(stockParent);
        // Deletion and mounting rebuild only the selected instance; all others survive.
        const auto beforeDeleteLoads=loads;
        choices.values[7]=0;choices.values[8]=0;
        assert(setBagsStatus(&choices)==1&&!context->bags[3].child&&!refs[address(children[3])]&&loads==beforeDeleteLoads);
        const auto deletedGeneration=lastBagGeneration;assert(deletedGeneration>firstGeneration);
        for(unsigned i=0;i<8;++i)if(i!=3)assert(context->bags[i].child==children[i]);
        choices.values[4]=2;assert(setBagsStatus(&choices)==1&&loads==beforeDeleteLoads+1&&lastBagGeneration>deletedGeneration);
        assert(context->bags[1].child!=children[1]&&!refs[address(children[1])]);
        // A failed mesh loads once, retries readiness and replaces only itself on failure.
        auto* failed=context->bags[2].child;modelName(failed,"World\\ErrorCube.mdx");
        assert(setBagsStatus(&choices)==0&&!context->bags[2].child&&!refs[address(failed)]);
        factoryModelsLoaded=false;assert(setBagsStatus(&choices)==0&&context->bags[2].child);
        failed=context->bags[2].child;const auto pendingLoads=loads;
        assert(setBagsStatus(&choices)==0&&loads==pendingLoads&&context->bags[2].child==failed);
        memory[address(failed)+0x10]=1;factoryModelsLoaded=true;assert(setBagsStatus(&choices)==1);
        // Unexpected native reattachment is repaired without losing saved fits.
        auto* misplaced=context->bags[0].child;
        const auto preservedGeneration=context->bagGeneration;
        const auto preservedFit=context->bags[0].fits[0].values;
        detach(misplaced);attach(misplaced,pointer(context->parent),33);
        const auto beforeRepairLoads=loads;
        assert(setBagsStatus(&choices)==1&&context->bags[0].child==misplaced&&loads==beforeRepairLoads);
        assert(memory[address(misplaced)+0x1D0]==28&&refs[address(misplaced)]==2);
        assert(lastBagGeneration==preservedGeneration&&context->bags[0].fits[0].values==preservedFit);
        const auto foreignParent=std::uintptr_t(0xD2000);
        detach(misplaced);attach(misplaced,pointer(foreignParent),28);
        assert(setBagsStatus(&choices)==1&&context->bags[0].child!=misplaced&&loads==beforeRepairLoads+1);
        assert(memory[address(misplaced)+0x1CC]==foreignParent&&refs[address(misplaced)]==1);
        assert(lastBagGeneration==preservedGeneration&&context->bags[0].fits[0].values==preservedFit);
        detach(misplaced);
        auto* destroyedChild=context->bags[0].child;
        forgetWeapons(address(destroyedChild));assert(!context->bags[0].child);
        detach(destroyedChild);unref(destroyedChild);
        assert(setBagsStatus(&choices)==1&&context->bags[0].child!=destroyedChild&&loads==beforeRepairLoads+2);
        assert(lastBagGeneration==preservedGeneration&&context->bags[0].fits[0].values==preservedFit);
        const auto worldOff=bagRequest(0);auto off=worldOff;assert(setBagsStatus(&off)==1&&!weaponContext(player.model)&&lastBagGeneration==0);
        assert(setBagsStatus(&choices)==1&&lastBagGeneration>deletedGeneration);
        const auto recreatedGeneration=lastBagGeneration;
        assert(setBagsStatus(&off)==1&&!weaponContext(player.model)&&lastBagGeneration==0);
        assert(setWeapons(&weapons)==1&&!weaponContext(player.model));
        assert(setBagsStatus(&choices)==1&&lastBagGeneration>recreatedGeneration);
        assert(setBagsStatus(&off)==1&&!weaponContext(player.model));
        off=bagRequest(87);assert(setBagsStatus(&off)==1&&!weaponContext(previewParent));previews.entries[0]={};
        assert(setBagInstanceFit(&firstFit)==-1);firstFit.values[4]=0;assert(setBagInstanceFit(&firstFit)==1);
        std::cout<<"PASS: eight independent bags, all body hip anchors, private preview fits, duplicate clone identity, deletion and load recovery\n";
    }
    // Placement response dispatch: old cloth assets stay in Stand while every
    // body/mount uses its own local response alongside the original rigid motion.
    {
        const auto oldContexts=weaponContexts;
        auto& c=weaponContexts[0];c={};c.parent=0xC1A000;c.unit=player.unit;c.guid=player.guid;
        auto& bag=c.bags[0];bag.model=12;bag.child=pointer(0xC1B000);
        auto* child=bag.child;const auto model=address(child);
        modelName(pointer(c.parent),"Character\\Human\\Female\\HumanFemale.mdx");
        modelName(child,bagAsset(12)->model);
        const auto parentData=memory[c.parent+0x30],childData=memory[model+0x30];
        const std::uintptr_t ph=0xC20000,ch=0xC21000,ps=0xC22000,cs=0xC23000,al=0xC24000,records=0xC25000;
        memory[parentData+0x130]=ph;memory[childData+0x130]=ch;
        memory[model+0x1CC]=c.parent;memory[model+0x1D0]=28;matrices[model+0xBC]=identity;
        memory[ph+0x10C]=34;memory[ph+0x110]=al;memory[al+56]=0;memory[al+64]=1;
        memory[ph+0x104]=2;memory[ph+0x108]=records;
        memory[records]=28;memory[records+4]=1;positions[records+8]=bagFits[1].anchor;
        memory[records+48]=32;memory[records+52]=1;positions[records+56]=bagFits[1].anchor;
        memory[ph+0x34]=2;memory[ph+0x38]=0xC26000;memory[0xC26000+108+8]=0;
        memory[c.parent+0x94]=0xC27000;matrices[0xC27000]=identity;matrices[0xC27000+64]=identity;
        matrices[c.parent+0xFC]=identity;memory[c.parent+0x2C]=0xC28000;matrices[0xC28000+0x9C]=identity;
        memory[c.parent+0x90]=0xC29000;memory[0xC29000+0x98]=1333;memory[0xC29000+0x9c]=1;
        memory[ph+0x1c]=2;memory[ph+0x20]=ps;memory[ps+68]=5;memory[ps+72]=1000;memory[ps+76]=1666;
        static constexpr char marker[]="ClosetClothV1";
        memory[ch+8]=sizeof(marker);memory[ch+12]=0xC2A000;
        for(unsigned i=0;i<sizeof(marker);++i)memory[0xC2A000+i]=marker[i];
        memory[ch+0x34]=129;memory[ch+0x1c]=2;memory[ch+0x20]=cs;
        memory[ch+0x24]=6;memory[ch+0x28]=0xC2B000;memory[0xC2B000]=0;memory[0xC2B000+10]=1;
        memory[cs]=0;memory[cs+68]=5;memory[cs+72]=1000;memory[cs+76]=1666;
        memory[cs+4]=0;memory[cs+8]=1;memory[cs+16]=0;memory[cs+84]=0;
        memory[cs+32]=120;memory[cs+100]=120;
        memory[model+0x90]=0xC2C000;memory[0xC2C000+0xF8]=0;memory[0xC2C000+0xA4]=0;
        memory[player.unit+0x9E8]=1;
        BagMatrix output,rest;
        assert(positionBackpack(child,rest));
        bag.motion.ready=true;bag.motion.verticalOffset=.1f;bag.motion.airborneWeight=1;
        assert(positionBackpack(child,output,true));
        assert(sequenceCalls.back().animation==0&&sequenceCalls.back().blend==0);
        assert(bag.motion.ready&&bag.motion.verticalOffset==0&&bag.motion.airborneWeight==0);
        assert(!bag.response.tracking); // Old baked deformation stays disabled; original motion remains.
        ClothAnimationResource resource;
        for(unsigned id:{12u,13u,14u,16u})assert(clothAnimationResource(model,id,resource));
        for(unsigned animation:{5u,37u,38u,39u,40u,187u}){
            memory[ps+68]=animation;bagTestTime+=16;assert(positionBackpack(child,output,true));
            assert(sequenceCalls.back().animation==0&&bag.motion.ready);
        }
        // Install the lightweight V3 rig: rest pivots define the field, even
        // though culling bounds are expanded beyond the rest bag.
        static constexpr char rigMarker[]="ClosetBagV3";
        memory[ch+8]=sizeof(rigMarker);memory[ch+0x34]=61;memory[ch+0x38]=0xC30000;
        for(unsigned i=0;i<sizeof(rigMarker);++i)memory[0xC2A000+i]=rigMarker[i];
        // Mageweave's real rest footprint is compact; generic unit-cube
        // bounds would incorrectly activate the heavy-backpack response.
        positions[0xC30000+108+96]={{-.422f,-.5925f,-.6195f}};
        positions[0xC30000+108*60+96]={{0.f,.5925f,.6195f}};
        positions[ch+0xB4]={{-2,-2,-2}};positions[ch+0xC0]={{2,2,2}};
        memory[model+0x94]=0xC40000;matrices[model+0xFC]=rest;
        for(unsigned i=0;i<61;++i)matrices[0xC40000+64*i]=rest;
        const auto clipCalls=sequenceCalls.size();
        assert(positionBackpack(child,output,true));assert(bag.response.tracking&&!bag.response.ready);
        for(unsigned frame=0;frame<150;++frame){
            bagTestTime+=16;assert(positionBackpack(child,output,true));
            assert(output==rest&&bag.motion.ready); // A static torso has no synthetic bounce.
        }
        assert(sequenceCalls.size()==clipCalls&&bag.response.ready&&bag.response.builds==1&&bag.response.offset[2]<-.03f);
        assert(std::fabs(bag.response.profile.height-1.239f)<.00001f);
        assert(applyBagResponseBones(child));
        assert(matrices[0xC40000]==rest&&matrices[0xC40000+64*5]==rest&&matrices[0xC40000+64*41]==rest);
        assert(matrices[0xC40000+64][14]<rest[14]-.02f); // Lower/front fabric sags.
        assert(matrices[0xC27000]==identity&&matrices[0xC27000+64]==identity); // Player bones untouched.
        {
            const auto savedBag=bag;
            for(const auto& asset:bagCatalog){
                bag=savedBag;bag.model=asset.id;modelName(child,asset.model);
                memory[memory[model+0x30]+0x130]=ch;
                const bool soft=asset.id>=12&&asset.id<=16;
                // Consumer rejects stale cloth deformation immediately, even
                // before placement has had a chance to clear the old state.
                assert(applyBagResponseBones(child));
                if(!soft)for(unsigned bone=0;bone<61;++bone)assert(matrices[0xC40000+64*bone]==rest);
                else assert(matrices[0xC40000+64]!=rest);
                assert(positionBackpack(child,output,true));
                assert(bag.response.ready==soft);
                assert(applyBagResponseBones(child));
                if(!soft)for(unsigned bone=0;bone<61;++bone)assert(matrices[0xC40000+64*bone]==rest);
            }
            bag=savedBag;memory[model+0x30]=childData;assert(applyBagResponseBones(child));
        }
        const auto drawn=matrices;
        evaluateAttachmentBones=true;
        // Real lazy updates overwrite +0x94 just like the recursive route.
        // Consume the resulting matrices as the CPU/GPU skinners do: a weighted
        // vertex changes after native evaluation, including a duplicate update.
        const std::array<float,3> lowerVertex{{-.4f,0,-.5f}};
        auto skinLower=[&](){
            std::array<float,3> result{};
            for(auto influence:std::array<std::pair<unsigned,float>,4>{{{1,.4f},{6,.3f},{11,.2f},{16,.1f}}}){
                const auto point=transformPoint(matrices.at(0xC40000+64*influence.first),lowerVertex);
                for(unsigned axis=0;axis<3;++axis)result[axis]+=point[axis]*influence.second;
            }
            return result;
        };
        for(auto caller:{0x718761u,0x71415Du,0x714183u}){
            updateAttachmentForCaller(child,identity.data(),nullptr,nullptr,1,caller);
            assert(matrices==drawn);
            const auto rigid=transformPoint(matrices.at(model+0xFC),lowerVertex);
            assert(skinLower()[2]<rigid[2]-.02f);
        }
        // Original mounting-point bounce and local cloth motion both respond to
        // an animated torso. Basis lengths remain constant: no whole-bag stretch.
        float minFabric=1,maxFabric=-1,maxBounce=0;
        for(unsigned frame=0;frame<160;++frame){
            auto animated=identity;animated[14]=.05f*std::sin(frame*.29f);
            matrices[0xC27000]=matrices[0xC27000+64]=animated;bagTestTime+=16;
            updateAttachmentForCaller(child,identity.data(),nullptr,nullptr,1,frame%2?0x71415D:0x718761);
            BagMatrix raw;assert(positionBackpack(child,raw));
            const auto& pose=matrices.at(model+0xFC);
            maxBounce=std::fmax(maxBounce,std::fabs(pose[14]-raw[14]));
            for(unsigned column:{0u,4u,8u}){
                float actual=0,expected=0;for(unsigned axis=0;axis<3;++axis){actual+=pose[column+axis]*pose[column+axis];expected+=rest[column+axis]*rest[column+axis];}
                assert(std::fabs(actual-expected)<.00001f);
            }
            const auto rigid=transformPoint(pose,lowerVertex);
            const float fabric=skinLower()[2]-rigid[2];
            minFabric=std::fmin(minFabric,fabric);maxFabric=std::fmax(maxFabric,fabric);
        }
        assert(maxBounce>.003f&&maxFabric-minFabric>.003f);
        // Restore the established outward/upward airborne lift while the lower
        // controls still deform relative to that rigid pose; no clip switching.
        matrices[0xC27000]=matrices[0xC27000+64]=identity;
        memory[player.unit+0x9E8]=0x2000;scalars[player.unit+0xA48]=-8;
        for(unsigned frame=0;frame<100;++frame){
            memory[player.unit+0xA20]=(frame+1)*16;bagTestTime+=16;
            updateAttachmentForCaller(child,identity.data(),nullptr,nullptr,1,0x714183);
        }
        const auto restBottom=transformPoint(rest,{{0,0,-.6195f}});
        const auto liftedBottom=transformPoint(matrices.at(model+0xFC),{{0,0,-.6195f}});
        assert(bag.motion.airborneWeight>.99f&&liftedBottom[0]<restBottom[0]-.08f&&liftedBottom[2]>restBottom[2]+.001f);
        assert(matrices.at(0xC40000+64)!=matrices.at(0xC40000)&&sequenceCalls.size()==clipCalls);
        memory[player.unit+0x9E8]=1;
        for(unsigned frame=0;frame<100;++frame){bagTestTime+=16;updateAttachmentForCaller(child,identity.data(),nullptr,nullptr,1,0x714183);}
        assert(bag.motion.airborneWeight<.00001f);
        evaluateAttachmentBones=false;matrices[model+0xFC]=rest;
        // Parenting on another body or hip does not select a special bake.
        bag.mount=1;memory[model+0x1D0]=32;
        assert(positionBackpack(child,output,true));assert(!bag.response.ready&&bag.motion.ready);
        for(unsigned frame=0;frame<70;++frame){bagTestTime+=16;assert(positionBackpack(child,output,true));}
        assert(bag.response.ready&&bag.response.builds==2&&bag.response.offset[2]<-.025f);
        auto& parentName=resourceNames[parentData+0x20];const auto femaleName=parentName;parentName[0]='X';
        assert(positionBackpack(child,output,true));assert(bag.response.ready);parentName=femaleName;
        // Turning off the option restores every control to rest, including a
        // duplicate native update that did not recalculate its bones this frame.
        bag.fits[1].enabled=true;bag.fits[1].values.motion=false;
        assert(positionBackpack(child,output,true));assert(!bag.response.tracking&&!bag.motion.ready);
        assert(applyBagResponseBones(child));for(unsigned i=0;i<61;++i)assert(matrices[0xC40000+64*i]==rest);
        bag.fits[1]={};bag.mount=0;memory[model+0x1D0]=28;
        memory[player.unit+0x9E8]=0x2000;scalars[player.unit+0xA48]=-8;
        memory[player.unit+0xA20]=0;assert(bagResponseFlight(c)==-1);
        memory[player.unit+0xA20]=416;assert(bagResponseFlight(c)>0&&bagResponseFlight(c)<.02f);
        memory[player.unit+0xA20]=2000;assert(bagResponseFlight(c)==1);
        memory[player.unit+0x9E8]=0x4000;assert(bagResponseFlight(c)==1);
        c.token=77;assert(bagResponseFlight(c)==0);c.token=0;
        memory[player.unit+0x9E8]=0;assert(bagResponseFlight(c)==0);
        // World mount samples include root travel but exclude camera movement.
        memory[player.unit+0x9E8]=1;
        for(unsigned frame=0;frame<50;++frame){
            const float time=frame*.016f;auto camera=identity;camera[12]=frame*.4f;camera[13]=-frame*.2f;
            auto actor=identity;actor[14]=.08f*std::sin(time*12);
            matrices[c.parent+0xFC]=bagMatrixProduct(camera,actor);
            matrices[0xC27000]=matrices[0xC27000+64]=matrices[c.parent+0xFC];matrices[0xC28000+0x9C]=camera;
            bagTestTime+=16;assert(positionBackpack(child,output,true));
            assert(std::fabs(bag.response.pin[0]-(rest[12]+rest[8]*.6195f))<.00001);
        }
        assert(std::fabs(bag.response.driver[2])>.03f); // Root vertical travel excites this mounting point.
        // Duplicate bags retain independent caches and local fields.
        auto& second=c.bags[1];second=bag;second.child=pointer(model+0x100);second.response={};
        memory[model+0x100+0x30]=childData;memory[model+0x100+0x10]=1;
        memory[model+0x100+0x1CC]=c.parent;memory[model+0x100+0x1D0]=28;matrices[model+0x100+0xBC]=identity;
        assert(positionBackpack(second.child,output,true));assert(second.response.tracking&&!second.response.ready);
        const auto safeMatrices=matrices;
        memory[model+0x94]=memory[c.parent+0x94];assert(!applyBagResponseBones(child)&&matrices==safeMatrices);
        memory[model+0x94]=0xC40000;memory[ch+0x34]=62;
        assert(!applyBagResponseBones(child)&&matrices==safeMatrices);memory[ch+0x34]=61;
        // Five copies driven by the SAME torso must visibly separate in the
        // final native pose, not merely in a tiny secondary spring offset.
        // Exercise recursive and both lazy native-update routes, including a
        // duplicate evaluation of each child at the same timestamp.
        {
            constexpr unsigned count=5,step=8;
            std::array<std::vector<float>,count> angles;
            matrices[c.parent+0xFC]=identity;matrices[0xC28000+0x9C]=identity;
            memory[player.unit+0x9E8]=1;
            for(unsigned slot=0;slot<count;++slot){
                auto& item=c.bags[slot];item={};item.model=12;item.child=pointer(model+slot*0x1000);
                const auto address=::address(item.child),palette=std::uintptr_t(0xC40000+slot*0x10000);
                memory[address+0x30]=childData;memory[address+0x10]=1;
                memory[address+0x1CC]=c.parent;memory[address+0x1D0]=28;
                memory[address+0x94]=palette;matrices[address+0xBC]=identity;matrices[address+0xFC]=rest;
                for(unsigned bone=0;bone<61;++bone)matrices[palette+64*bone]=rest;
            }
            evaluateAttachmentBones=true;
            const std::array<std::uintptr_t,3> callers{{0x718761,0x71415D,0x714183}};
            for(unsigned frame=0;frame<560;++frame){
                const float wave=std::sin(frame*step*(2*3.14159265358979323846f/640));
                const float angle=7*.01745329252f*wave;
                auto animated=identity;animated[5]=animated[10]=std::cos(angle);
                animated[6]=std::sin(angle);animated[9]=-animated[6];animated[14]=.025f*wave;
                matrices[0xC27000]=matrices[0xC27000+64]=animated;bagTestTime+=step;
                for(unsigned slot=0;slot<count;++slot){
                    const auto object=c.bags[slot].child;
                    updateAttachmentForCaller(object,identity.data(),nullptr,nullptr,1,callers[(frame+slot)%callers.size()]);
                    const auto pose=matrices.at(address(object)+0xFC);
                    updateAttachmentForCaller(object,identity.data(),nullptr,nullptr,1,callers[(frame+slot+1)%callers.size()]);
                    assert(matrices.at(address(object)+0xFC)==pose);
                    if(frame>=160)angles[slot].push_back(std::atan2(pose[6],pose[5]));
                }
            }
            const auto correlation=[&](unsigned slot,unsigned lag){
                double sx=0,sy=0,sxx=0,syy=0,sxy=0;
                constexpr unsigned samples=320; // Four full gait cycles.
                for(unsigned i=0;i<samples;++i){
                    const double x=angles[0][i],y=angles[slot][i+lag];
                    sx+=x;sy+=y;sxx+=x*x;syy+=y*y;sxy+=x*y;
                }
                const double variance=(sxx-sx*sx/samples)*(syy-sy*sy/samples);
                assert(variance>1.e-6);
                return (sxy-sx*sy/samples)/std::sqrt(variance);
            };
            for(unsigned slot=1;slot<count;++slot){
                unsigned bestLag=0;double best=-2;
                for(unsigned lag=0;lag<=30;++lag){const double value=correlation(slot,lag);if(value>best){best=value;bestLag=lag;}}
                assert(best>.97&&correlation(slot,0)<.96);
                assert(std::abs(int(bestLag*step)-int(bagJiggleDelay(201+slot)))<=16);
            }
            // Stopping locomotion must remove extra jiggle even while native
            // idle breathing keeps the mount moving. It must not restart from
            // delayed samples. The base fitted bag continues to follow the body.
            memory[player.unit+0x9E8]=0;
            for(unsigned frame=0;frame<200;++frame){
                const float angle=.035f*std::sin(frame*.12f);
                auto idle=identity;idle[5]=idle[10]=std::cos(angle);
                idle[6]=std::sin(angle);idle[9]=-idle[6];idle[14]=.008f*std::sin(frame*.08f);
                matrices[0xC27000]=matrices[0xC27000+64]=idle;bagTestTime+=step;
                for(unsigned slot=0;slot<count;++slot){
                    auto& item=c.bags[slot];BagMatrix fitted;
                    updateAttachmentForCaller(item.child,identity.data(),nullptr,nullptr,1,callers[slot%callers.size()]);
                    assert(positionBackpack(item.child,fitted));
                    if(frame>=100){
                        const auto& pose=matrices.at(address(item.child)+0xFC);
                        for(unsigned element=0;element<16;++element)assert(std::fabs(pose[element]-fitted[element])<.00001f);
                        assert(bagResponseLength(item.response.driver)==0);
                        assert(std::fabs(item.response.offset[2]+.1f*1.239f*.45f*.85f)<.0001f);
                    }
                }
            }
            memory[player.unit+0x9E8]=1;
            std::array<float,count> resumed{};
            for(unsigned frame=0;frame<120;++frame){
                const float angle=.12f*std::sin(frame*.12f);
                auto moving=identity;moving[5]=moving[10]=std::cos(angle);
                moving[6]=std::sin(angle);moving[9]=-moving[6];moving[14]=.025f*std::sin(frame*.12f);
                matrices[0xC27000]=matrices[0xC27000+64]=moving;bagTestTime+=step;
                for(unsigned slot=0;slot<count;++slot){
                    auto& item=c.bags[slot];BagMatrix fitted;
                    updateAttachmentForCaller(item.child,identity.data(),nullptr,nullptr,1,callers[slot%callers.size()]);
                    assert(positionBackpack(item.child,fitted));
                    const auto& pose=matrices.at(address(item.child)+0xFC);
                    resumed[slot]=std::fmax(resumed[slot],std::fabs(pose[6]-fitted[6]));
                }
            }
            for(float movement:resumed)assert(movement>.001f);
            // Hold the torso still to isolate the original upward/outward
            // jump lift. Every bag receives one native descent signal; each
            // begins at its own time and eventually reaches the same full lift.
            matrices[0xC27000]=matrices[0xC27000+64]=identity;
            for(unsigned frame=0;frame<150;++frame){
                bagTestTime+=step;
                for(auto& item:c.bags)if(item.child)updateAttachmentForCaller(item.child,identity.data(),nullptr,nullptr,1,0x714183);
            }
            std::array<unsigned,count> halfLift{};
            memory[player.unit+0x9E8]=0x2000;scalars[player.unit+0xA48]=-8;
            for(unsigned frame=0;frame<300;++frame){
                memory[player.unit+0xA20]=(frame+1)*step;bagTestTime+=step;
                for(unsigned slot=0;slot<count;++slot){
                    auto& item=c.bags[slot];
                    updateAttachmentForCaller(item.child,identity.data(),nullptr,nullptr,1,callers[(frame+slot)%callers.size()]);
                    if(!halfLift[slot]&&item.motion.airborneWeight>=.5f)halfLift[slot]=(frame+1)*step;
                }
            }
            const auto restBottom=transformPoint(rest,{{0,0,-.6195f}});
            for(unsigned slot=0;slot<count;++slot){
                assert(halfLift[slot]&&c.bags[slot].motion.airborneWeight>.99f);
                if(slot)assert(halfLift[slot]>=halfLift[slot-1]+24);
                const auto bottom=transformPoint(matrices.at(address(c.bags[slot].child)+0xFC),{{0,0,-.6195f}});
                assert(bottom[0]<restBottom[0]-.08f&&bottom[2]>restBottom[2]+.001f);
            }
            evaluateAttachmentBones=false;
            std::cout<<"PASS: five rendered bag poses have distinct gait phases, idle settling, restart and staggered original jump lift\n";
        }
        // NPCs retain their native state and never inherit player adjustments.
        c.guid=player.guid+1;
        const auto npcMemory=memory;const auto npcMatrices=matrices;const auto npcOutput=output;const auto npcCalls=sequenceCalls.size();
        assert(!positionBackpack(child,output,true));
        assert(memory==npcMemory&&matrices==npcMatrices&&output==npcOutput&&sequenceCalls.size()==npcCalls);
        updateAttachmentForCaller(child,identity.data(),nullptr,nullptr,.73f,0x71415D);
        assert(attachmentUpdate.matrix==identity.data()&&attachmentUpdate.alpha==.73f);
        assert(memory==npcMemory&&matrices==npcMatrices&&sequenceCalls.size()==npcCalls);
        weaponContexts=oldContexts;
        std::cout<<"PASS: local control response on back/hips, world mounting motion, old-clip suppression and player isolation\n";
    }
    {
        // Exercise the real positionBackpack path, including its reconstruction
        // of the saved neutral fit. A bag dragged down from a hip preset must
        // follow the foot's skin rather than the original hip attachment.
        const auto savedContexts=weaponContexts;
        auto& context=weaponContexts[0];context={};context.parent=0xD4000;context.guid=player.guid;
        auto& bag=context.bags[0];bag.model=12;bag.mount=1;bag.child=pointer(0xD5000);
        const auto child=address(bag.child);
        modelName(bag.child,bagAsset(12)->model);
        memory[memory[child+0x30]+0x130]=0xD460000; // No deformation metadata: use the legacy top height.
        const std::uintptr_t data=0xD400000,header=data+0x1000,lookup=data+0x2000,records=data+0x3000;
        const std::uintptr_t palette=data+0x4000,definitions=data+0x5000,vertices=data+0x6000;
        const std::uintptr_t views=data+0x7000,indices=data+0x8000,sections=data+0x9000;
        memory[context.parent+0x30]=data;memory[data+0x130]=header;
        memory[context.parent+0x94]=palette;matrices[context.parent+0xFC]=identity;
        memory[child+0x1CC]=context.parent;memory[child+0x1D0]=32;
        auto local=identity;local[0]=local[5]=local[10]=.7f;local[12]=.04f;local[14]=-.08f;
        matrices[child+0xBC]=local;
        memory[header+0x10C]=34;memory[header+0x110]=lookup;memory[lookup+56]=0;memory[lookup+64]=1;
        memory[header+0x104]=2;memory[header+0x108]=records;
        memory[records]=28;memory[records+4]=1;positions[records+8]=bagFits[1].anchor;
        memory[records+48]=32;memory[records+52]=1;positions[records+56]={{.01f,.17f,.9f}};
        memory[header+0x34]=4;memory[header+0x38]=definitions;memory[definitions+108+8]=0;
        for(unsigned i=0;i<4;++i)matrices[palette+64*i]=identity;
        auto& fit=bag.fits[1];fit.enabled=true;fit.revision=9;fit.values.up=-.9f;
        fit.values.left=.03f;fit.values.inset=.01f;fit.values.pitch=13;fit.values.yaw=60;
        fit.values.scale=40;fit.values.motion=false;
        const auto savedFit=fit.values;
        BagMatrix neutral,output;assert(positionBackpack(bag.child,neutral)&&!bag.bodyBound);
        const auto neutralModel=bagMatrixProduct(neutral,local);
        const auto contact=transformPoint(neutralModel,{{0,0,.6195f}});
        memory[header]=0x3032444D;memory[header+4]=256;
        memory[header+0x44]=3;memory[header+0x48]=vertices;
        memory[header+0x4C]=1;memory[header+0x50]=views;
        memory[views]=3;memory[views+4]=indices;memory[views+24]=2;memory[views+28]=sections;
        memory[sections]=0;memory[sections+4]=0;memory[sections+6]=2;
        memory[sections+32]=1501;memory[sections+36]=2;memory[sections+38]=1;
        positions[vertices]=positions[records+56];positions[vertices+48]=contact;
        positions[vertices+48][0]+=.005f;positions[vertices+96]=contact;
        for(unsigned vertex=0;vertex<3;++vertex){
            memory[indices+2*vertex]=vertex;
            for(unsigned influence=0;influence<4;++influence){
                memory[vertices+48*vertex+12+influence]=influence?0:255;
                memory[vertices+48*vertex+16+influence]=influence?0:vertex+1;
            }
        }
        const auto close=[](const BagMatrix& a,const BagMatrix& b){
            for(unsigned i=0;i<16;++i)assert(std::fabs(a[i]-b[i])<.00002f);
        };
        BagMatrix foot=identity;
        for(unsigned frame=0;frame<30;++frame){
            const float angle=.9f*std::sin(frame*.23f);foot=identity;
            foot[0]=foot[10]=std::cos(angle);foot[2]=-std::sin(angle);foot[8]=std::sin(angle);
            foot[12]=.2f*std::sin(frame*.31f);foot[14]=.15f*std::sin(frame*.19f);
            auto hip=identity;hip[13]=.3f*std::cos(frame*.2f);hip[14]=.1f;
            matrices[palette]=matrices[palette+64]=hip;matrices[palette+128]=foot;
            bagTestTime+=16;assert(positionBackpack(bag.child,output));
            assert(bag.bodyBound&&bag.bodyBinding.bones[0]==2&&bag.bodyBinding.builds==1);
            close(output,bagMatrixProduct(foot,neutral));
            const auto pinned=transformPoint(bagMatrixProduct(output,local),{{0,0,.6195f}});
            const auto expected=transformPoint(foot,contact);
            for(unsigned axis=0;axis<3;++axis)assert(std::fabs(pinned[axis]-expected[axis])<.00002f);
        }
        assert(fit.values==savedFit&&fit.revision==9);
        // A Character-window clone must bind to its own live skeleton and view,
        // even when it shares the source's mesh resource and saved placement.
        const std::uintptr_t cloneParent=0xD6000,cloneChild=0xD7000,clonePalette=0xD500000;
        modelName(pointer(cloneChild),bagAsset(12)->model);
        memory[cloneChild+0x1CC]=cloneParent;memory[cloneChild+0x1D0]=32;matrices[cloneChild+0xBC]=local;
        memory[cloneParent+0x30]=data;memory[cloneParent+0x94]=clonePalette;
        auto view=identity;view[0]=0;view[1]=1.5f;view[4]=-1.5f;view[5]=0;view[10]=1.5f;view[12]=8;
        matrices[cloneParent+0xFC]=view;
        auto cloneFoot=identity;cloneFoot[13]=-.4f;cloneFoot[14]=.3f;
        for(unsigned i=0;i<4;++i)matrices[clonePalette+64*i]=bagMatrixProduct(view,i==2?cloneFoot:identity);
        rememberClonedBagPreview(child,cloneChild);rememberClonedBagPreview(context.parent,cloneParent);
        auto* clone=clonedBagChild(cloneChild);assert(clone&&!clone->bag.bodyBinding.ready);
        assert(positionBackpack(pointer(cloneChild),output));
        close(output,bagMatrixProduct(view,bagMatrixProduct(cloneFoot,neutral)));
        assert(clone->bag.bodyBound&&clone->bag.bodyBinding.builds==1&&bag.bodyBinding.builds==1);
        assert(clone->bag.fits[1].values==savedFit&&positionBackpack(bag.child,output));
        close(output,bagMatrixProduct(foot,neutral));
        for(auto& copy:clonedBagChildren)if(copy.child==cloneChild)copy={};
        for(auto& copy:clonedBagPreviews)if(copy.model==cloneParent)copy={};
        weaponContexts=savedContexts;
        std::cout<<"PASS: real bag placement binds saved hip-to-foot fits, pins live skin contact and uses independent cloned skeletons\n";
    }
    // Modern per-hand storage is independent of decorative carried items.
    // Cover passthrough and explicit appearances, all visibility combinations,
    // then the clear-only ranged -> NPC/loot transition with a missing 2H child.
    for(int advanced:{0,1})for(bool appearance:{false,true}){
        WeaponSelection clear;auto off=request(0,clear);assert(setWeapons(&off)==1);
        realIDs[0]=35;realIDs[1]=0;realIDs[2]=2507;
        assert(weaponAsset(35)->kind==2);
        WeaponSelection selection;selection.independent=true;selection.carriedMode=advanced;selection.stowedMask=7;
        selection.equipped={{35,0,2507}};
        if(appearance){selection.items[7]=35;selection.items[9]=2507;}
        if(advanced){selection.items[2]=35;selection.items[5]=2507;}
        memory[player.unit+0xD3C]=0;memory[player.unit+0xD40]=0;
        auto on=request(0,selection);assert(setWeapons(&on)==1);
        auto* context=weaponContext(player.model);assert(context);
        Lua noBags;noBags.values.resize(17,0);assert(setBags(&noBags)>0);
        assert(weaponContext(player.model)==context&&context->selection.stowedMask==7);
        const int mainHome=selectedWeaponHome(selection,0,context->routes[0]);
        const int bowHome=selectedWeaponHome(selection,2,context->routes[2]);
        assert(mainHome>=0&&bowHome==27);
        const auto decoration=context->extra;
        const auto baselineLoads=loads;
        for(int mask=0;mask<8;++mask){
            selection.stowedMask=mask;on=request(0,selection);assert(setWeapons(&on)==1);
            auto main=findChildHook(pointer(player.model),nullptr,mainHome);
            auto bow=findChildHook(pointer(player.model),nullptr,bowHome);
            assert(main&&bow&&hideStoredWeapon(main)==!(mask&1)&&hideStoredWeapon(bow)==!(mask&4));
            for(unsigned i=0;i<7;++i)if(decoration[i])assert(context->extra[i]==decoration[i]&&!hideStoredWeapon(decoration[i]));
            const auto submissions=bowStringSubmission.calls;
            bowStringDrawHook(bow,nullptr,pointer(player.unit));
            assert(bowStringSubmission.calls==submissions+((mask&4)?1:0));
        }
        assert(loads==baselineLoads); // Visibility itself changes no weapon instances.
        selection.stowedMask=7;on=request(0,selection);assert(setWeapons(&on)==1);
        for(unsigned interruption=0;interruption<2;++interruption){
            memory[player.unit+0xD3C]=0;memory[player.unit+0xD40]=2;
            sheathTransitionHook(pointer(player.unit),nullptr);
            auto held=findChildHook(pointer(player.model),nullptr,2);assert(held&&!hideStoredWeapon(held));
            clearChildrenHook(pointer(player.model),nullptr,mainHome); // Lost during another animation/composition.
            memory[player.unit+0xD3C]=2;memory[player.unit+0xD40]=0;
            sheathTransitionHook(pointer(player.unit),nullptr);
            auto main=findChildHook(pointer(player.model),nullptr,mainHome);
            auto bow=findChildHook(pointer(player.model),nullptr,bowHome);
            assert(main&&bow&&!hideStoredWeapon(main)&&!hideStoredWeapon(bow));
            assert(weaponModelMatches(main,weaponAsset(35)->model));
            const auto restoredLoads=loads;
            memory[player.unit+0xD3C]=0;sheathTransitionHook(pointer(player.unit),nullptr);
            assert(setWeapons(&on)==1&&loads==restoredLoads);
            assert(context->extra==decoration);
        }
        if(!appearance){
            realIDs[0]=25;selection.equipped[0]=25;on=request(0,selection);
            assert(setWeapons(&on)==1);
            const int newHome=selectedWeaponHome(selection,0,context->routes[0]);
            auto main=findChildHook(pointer(player.model),nullptr,newHome);
            assert(main&&weaponModelMatches(main,weaponAsset(25)->model)&&newHome!=mainHome);
            assert(!findChildHook(pointer(player.model),nullptr,mainHome));
            realIDs[0]=35;selection.equipped[0]=35;
        }
        // Ready previews show exactly one movable hand-role object plus decor.
        memory[0x6000+0x10]=1;previews.entries[0]={};
        previews.entries[0].model=0x6000;previews.entries[0].guid=player.guid;
        previews.entries[0].token=77;previews.entries[0].status=1;
        selection.items[7]=35;selection.items[9]=2507;
        auto preview=request(77,selection);assert(setWeapons(&preview)==1);
        auto* pc=weaponContext(0x6000);assert(pc&&pc->extra[7]&&pc->extra[9]);
        assert(!hideStoredWeapon(pc->extra[7])&&!hideStoredWeapon(pc->extra[9]));
        for(double invalidMask:{-1.,8.,1.5}){
            auto invalid=preview;invalid.values[22]=invalidMask;assert(setWeapons(&invalid)==-2);
        }
        preview.values[22]=0;assert(setWeapons(&preview)==1);
        assert(hideStoredWeapon(pc->extra[7])&&hideStoredWeapon(pc->extra[9]));
        preview.values[20]=2;assert(setWeapons(&preview)==1);
        assert(pc->extra[9]&&!hideStoredWeapon(pc->extra[9]));
        forgetWeapons(0x6000);previews.entries[0]={};
        assert(setWeapons(&off)==1);
    }
    // Modern wands share the user's ranged storage choice, including native
    // sheath-zero assets and cross-family appearances. Drawn wands stay visible.
    for(const auto& asset:weaponAssets)if(asset.kind==4&&asset.subclass==19){
        WeaponSelection s;s.stowedMask=7;s.equipped[2]=asset.item;
        assert(selectedWeaponHome(s,2,-1)==27);
        s.items[9]=asset.item;assert(selectedWeaponHome(s,2,9)==27);
    }
    for(int advanced:{0,1})for(unsigned actual:rangedKinds)for(bool appearance:{false,true}){
        if(!appearance&&actual!=rangedKinds[3])continue;
        WeaponSelection clear;auto off=request(0,clear);assert(setWeapons(&off)==1);
        realIDs[0]=35;realIDs[1]=0;realIDs[2]=actual;
        WeaponSelection s;s.independent=true;s.carriedMode=advanced;s.stowedMask=7;
        s.equipped={{35,0,actual}};if(appearance)s.items[9]=rangedKinds[3];
        if(advanced)s.items[5]=rangedKinds[3]; // Identical decoration stays independent.
        memory[player.unit+0xD3C]=0;memory[player.unit+0xD40]=0;
        auto on=request(0,s);assert(setWeapons(&on)==1);
        auto* c=weaponContext(player.model);assert(c&&c->nativeChildren[2]);
        const auto decoration=c->extra[5];
        const auto initialWand=c->nativeChildren[2];
        const auto baselineLoads=loads;
        for(int mask=0;mask<8;++mask){
            s.stowedMask=mask;on=request(0,s);assert(setWeapons(&on)==1);
            auto stowed=findChildHook(pointer(player.model),nullptr,27);
            assert(stowed==initialWand&&weaponModelMatches(stowed,weaponAsset(rangedKinds[3])->model));
            assert(hideStoredWeapon(stowed)==!(mask&4));
            if(decoration)assert(c->extra[5]==decoration&&!hideStoredWeapon(decoration));
        }
        assert(loads==baselineLoads); // Eye toggle changes visibility, not instances.
        for(int mask:{7,3}){
            s.stowedMask=mask;on=request(0,s);assert(setWeapons(&on)==1);
            memory[player.unit+0xD3C]=0;memory[player.unit+0xD40]=2;
            sheathTransitionHook(pointer(player.unit),nullptr);
            auto held=findChildHook(pointer(player.model),nullptr,1);
            assert(held&&weaponModelMatches(held,weaponAsset(rangedKinds[3])->model)&&!hideStoredWeapon(held));
            // NPC/loot transition restores both the missing melee model and
            // the wand, preserving the user's stowed visibility choice.
            const int mainHome=selectedWeaponHome(s,0,c->routes[0]);
            clearChildrenHook(pointer(player.model),nullptr,mainHome);
            memory[player.unit+0xD3C]=2;memory[player.unit+0xD40]=0;
            sheathTransitionHook(pointer(player.unit),nullptr);
            assert(findChildHook(pointer(player.model),nullptr,mainHome));
            assert(!findChildHook(pointer(player.model),nullptr,1));
            auto stowed=findChildHook(pointer(player.model),nullptr,27);
            assert(stowed&&weaponModelMatches(stowed,weaponAsset(rangedKinds[3])->model));
            assert(hideStoredWeapon(stowed)==!(mask&4));
            assert(c->extra[5]==decoration);
            if(decoration)assert(!hideStoredWeapon(decoration));
        }
        memory[0x6000+0x10]=1;previews.entries[0]={};
        previews.entries[0].model=0x6000;previews.entries[0].guid=player.guid;
        previews.entries[0].token=77;previews.entries[0].status=1;
        s.items[9]=rangedKinds[3];auto preview=request(77,s);
        assert(setWeapons(&preview)==1);auto* pc=weaponContext(0x6000);
        assert(pc&&pc->extra[9]);
        const auto previewDecoration=pc->extra[5];
        for(int mask:{7,3}){
            preview.values[22]=mask;assert(setWeapons(&preview)==1);
            assert(pc->extra[9]&&hideStoredWeapon(pc->extra[9])==!(mask&4));
            if(pc->extra[5])assert(!hideStoredWeapon(pc->extra[5]));
            preview.values[20]=2;assert(setWeapons(&preview)==1);
            assert(pc->extra[9]&&!hideStoredWeapon(pc->extra[9]));
            preview.values[20]=0;assert(setWeapons(&preview)==1);
            assert(pc->extra[9]&&hideStoredWeapon(pc->extra[9])==!(mask&4));
            if(previewDecoration)assert(pc->extra[5]&&!hideStoredWeapon(pc->extra[5]));
        }
        forgetWeapons(0x6000);previews.entries[0]={};assert(setWeapons(&off)==1);
    }
    std::cout<<"PASS: wand stow eye choice in both modes, all ranged appearance families, previews, drawing and NPC/loot transitions\n";
    // Equipped-role tuning affects stowed placement only; drawing restores native transforms.
    for(int advanced:{0,1})for(bool appearance:{false,true})for(unsigned rangedItem:rangedKinds){
        realIDs[0]=25;realIDs[1]=143;realIDs[2]=rangedItem;
        WeaponSelection s;s.independent=true;s.carriedMode=advanced;s.stowedMask=7;
        s.equipped={{25,143,rangedItem}};
        if(appearance){s.items[7]=25;s.items[8]=143;s.items[9]=rangedItem;}
        memory[player.unit+0xD3C]=0;memory[player.unit+0xD40]=0;
        auto on=request(0,s);assert(setWeapons(&on)==1);
        auto* ctx=weaponContext(player.model);assert(ctx);
        for(unsigned role=0;role<3;++role){
            memory[player.unit+0xD3C]=0;memory[player.unit+0xD40]=role==2?2:1;
            sheathTransitionHook(pointer(player.unit),nullptr);
            const auto* a=weaponAsset(s.equipped[role]);
            const unsigned hand=role==0?1:role==1?0:(a->inventory==25||a->inventory==26?1:2);
            auto drawn=findChildHook(pointer(player.model),nullptr,hand);assert(drawn);
            memory[player.unit+0xD3C]=role==2?2:1;memory[player.unit+0xD40]=0;
            sheathTransitionHook(pointer(player.unit),nullptr);
            const int home=selectedWeaponHome(s,role,ctx->routes[role]);
            auto child=home>=0?findChildHook(pointer(player.model),nullptr,home):nullptr;
            assert(home>=0&&child);
            for(const auto& fixture:bowFixtures){
                setBack(ctx,fixture);
                BagTuningValues v;v.scale=125;v.left=.1f;v.yaw=35;
                assert(bagTuningSet(108+role,fixture.race,fixture.sex,true,v));
                BagMatrix result,expected;assert(tuneStoredPlacement(child,identity,result));
                assert(placementTuning(identity,matrices[address(child)+0xBC],identity,identity,v,expected));
                for(unsigned axis=0;axis<16;++axis)assert(std::fabs(result[axis]-expected[axis])<.00001f);
                // Actual draw/stow transition with the fit still enabled.
                memory[player.unit+0xD3C]=0;memory[player.unit+0xD40]=role==2?2:1;
                sheathTransitionHook(pointer(player.unit),nullptr);
                drawn=findChildHook(pointer(player.model),nullptr,hand);assert(drawn);
                assert(!tuneStoredPlacement(drawn,identity,result));
                memory[player.unit+0xD3C]=role==2?2:1;memory[player.unit+0xD40]=0;
                sheathTransitionHook(pointer(player.unit),nullptr);
                child=findChildHook(pointer(player.model),nullptr,home);assert(child);
                assert(tuneStoredPlacement(child,identity,result));
                assert(bagTuningSet(108+role,fixture.race,fixture.sex,false));
                assert(!tuneStoredPlacement(child,identity,result));
            }
        }
        WeaponSelection clear;auto off=request(0,clear);assert(setWeapons(&off)==1);
    }
    for(int advanced:{0,1})for(unsigned rangedItem:{2507u,rangedKinds[3]}){
        WeaponSelection s;s.independent=true;s.carriedMode=advanced;s.stowedMask=7;
        s.equipped={{25,143,rangedItem}};s.items[7]=25;s.items[8]=143;s.items[9]=rangedItem;
        memory[0x6000+0x10]=1;previews.entries[0]={};
        previews.entries[0].model=0x6000;previews.entries[0].guid=player.guid;
        previews.entries[0].token=77;previews.entries[0].status=1;
        for(unsigned role=0;role<3;++role){
            auto preview=request(77,s);preview.values[20]=0;
            assert(setWeapons(&preview)==1);auto* ctx=weaponContext(0x6000);
            auto child=ctx->extra[7+role];assert(child);
            const auto& fixture=bowFixtures[0];setBack(ctx,fixture);
            BagTuningValues v;v.scale=120;v.yaw=25;
            assert(bagTuningSet(108+role,fixture.race,fixture.sex,true,v));
            BagMatrix result,expected;
            assert(tuneStoredPlacement(child,identity,result));
            assert(placementTuning(identity,matrices[address(child)+0xBC],identity,identity,v,expected));
            for(unsigned axis=0;axis<16;++axis)assert(std::fabs(result[axis]-expected[axis])<.00001f);
            preview.values[20]=role==2?2:1;assert(setWeapons(&preview)==1);
            assert(!tuneStoredPlacement(ctx->extra[7+role],identity,result));
            assert(bagTuningSet(108+role,fixture.race,fixture.sex,false));
        }
        forgetWeapons(0x6000);previews.entries[0]={};
    }
    // A preview using real equipment (no cosmetic hand override) retains the
    // client's native sheath point. Back swords use 26/27 here, while the world
    // renderer deliberately owns them at 30/31. Matching decorations may share
    // a point and mesh, but must keep their own fit instead of the hand-slot fit.
    for(int advanced:{0,1})for(unsigned role:{0u,1u,2u})for(unsigned item:{25u,35u,647u,778u,143u,1047u,4899u,5259u}){
        const auto* a=weaponAsset(item);
        if(!acceptsWeapon(7+role,a))continue;
        WeaponSelection s;s.independent=true;s.carriedMode=advanced;s.stowedMask=7;
        s.equipped[role]=item;
        const bool right=role==0||(role==2&&(a->inventory==25||a->inventory==26));
        const int nativeHome=sheathPointOriginal(a->sheath,right);assert(nativeHome>=0);
        // Pick a legal decorative location at the same home when possible.
        const unsigned decoration=a->kind==4?5:a->kind==3?4:nativeHome==32?0:nativeHome==33?1:role==0?2:3;
        if(advanced)s.items[decoration]=item;
        memory[0x6000+0x10]=1;previews.entries[0]={};
        previews.entries[0].model=0x6000;previews.entries[0].guid=player.guid;
        previews.entries[0].token=77;previews.entries[0].status=1;
        factory(pointer(0x6000),nativeHome,a->model,a->texture,0);
        auto child=findChildOriginal(pointer(0x6000),nativeHome);assert(child);
        auto preview=request(77,s);assert(setWeapons(&preview)==1);
        auto* ctx=weaponContext(0x6000);assert(ctx&&ctx->routes[role]<0);
        for(const auto& fixture:bowFixtures){
            setBack(ctx,fixture);
            BagTuningValues v;v.scale=120;v.left=.13f;v.yaw=25;
            assert(bagTuningSet(108+role,fixture.race,fixture.sex,true,v));
            BagMatrix result,expected;
            assert(tuneStoredPlacement(child,identity,result));
            assert(placementTuning(identity,matrices[address(child)+0xBC],identity,identity,v,expected));
            for(unsigned axis=0;axis<16;++axis)assert(std::fabs(result[axis]-expected[axis])<.00001f);
            if(advanced){
                auto extra=ctx->extra[decoration];assert(extra);
                assert(!tuneStoredPlacement(extra,identity,result));
                BagTuningValues separate;separate.scale=85;separate.left=-.17f;
                assert(bagTuningSet(101+decoration,fixture.race,fixture.sex,true,separate));
                assert(tuneStoredPlacement(extra,identity,result));
                assert(placementTuning(identity,matrices[address(extra)+0xBC],identity,identity,separate,expected));
                for(unsigned axis=0;axis<16;++axis)assert(std::fabs(result[axis]-expected[axis])<.00001f);
                assert(bagTuningSet(101+decoration,fixture.race,fixture.sex,false));
            }
            memory[address(child)+0x1D0]=right?1:a->kind==3?0:2;
            assert(!tuneStoredPlacement(child,identity,result));
            memory[address(child)+0x1D0]=nativeHome;
            modelName(child,weaponAsset(a->kind==3?25:143)->model);
            assert(!tuneStoredPlacement(child,identity,result));
            modelName(child,a->model);
            assert(bagTuningSet(108+role,fixture.race,fixture.sex,false));
        }
        forgetWeapons(0x6000);clear(pointer(0x6000),nativeHome);previews.entries[0]={};
    }
    std::cout<<"PASS: stowed main/off/ranged fits restore native drawn transforms, all bodies, both modes, previews and passthrough\n";
    for(int advanced:{0,1})for(unsigned offhand:{25u,143u}){
        realIDs[0]=25;realIDs[1]=offhand;realIDs[2]=0;
        WeaponSelection selection;selection.independent=true;selection.carriedMode=advanced;
        selection.stowedMask=0;selection.equipped={{25,offhand,0}};
        selection.items[7]=25;selection.items[8]=offhand;
        memory[player.unit+0xD3C]=0;memory[player.unit+0xD40]=0;
        auto on=request(0,selection);assert(setWeapons(&on)==1);
        auto* context=weaponContext(player.model);assert(context);
        for(int mask:{1,2,3,0}){
            selection.stowedMask=mask;on=request(0,selection);assert(setWeapons(&on)==1);
            for(unsigned role=0;role<2;++role){
                auto child=findChildHook(pointer(player.model),nullptr,selectedWeaponHome(selection,role,context->routes[role]));
                assert(child&&hideStoredWeapon(child)==!(mask&(1<<role)));
            }
        }
        memory[player.unit+0xD40]=1;sheathTransitionHook(pointer(player.unit),nullptr);
        auto main=findChildHook(pointer(player.model),nullptr,1);
        auto offhandChild=findChildHook(pointer(player.model),nullptr,offhand==143?0:2);
        assert(main&&offhandChild&&!hideStoredWeapon(main)&&!hideStoredWeapon(offhandChild));
        WeaponSelection clear;auto off=request(0,clear);assert(setWeapons(&off)==1);
    }
    std::cout<<"PASS: per-hand stow options in both modes, passthrough bows, decorative isolation, bow strings, previews and missing two-hand recovery after ranged NPC/loot transitions\n";
    std::cout<<"PASS: native hook simulation, cross-family ranged drawing/sheathing, real metadata isolation, staff body contact and placement tuning\n";
    return 0;
}
