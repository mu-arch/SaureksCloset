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
struct Player {std::uintptr_t model=0,unit=0;std::uint64_t guid=0;unsigned display=0,native=0;};
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
static std::array<std::array<unsigned char,8>,3> realInfo{};
static const unsigned char* info(void*,unsigned role,unsigned){
    const auto* a=role<3?weaponAsset(realIDs[role]):nullptr;
    if(!a)return nullptr;
    realInfo[role]={{2,static_cast<unsigned char>(a->subclass),7,static_cast<unsigned char>(a->inventory),static_cast<unsigned char>(a->sheath),9,10,11}};
    return realInfo[role].data();
}
static const WeaponAsset* visualWeapon(unsigned role,std::uintptr_t caller){
    if(!weaponInfoOriginal)return weaponAsset(realIDs[role]);
    const auto* result=weaponInfoAt(pointer(player.unit),role,0,caller);
    const auto* c=weaponContext(player.model);
    if(c&&result==c->rangedInfo.data())return weaponAsset(c->selection.items[5]);
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
static void observeAttachment(void* model,const float* matrix,const float* color,const float* lighting,float alpha){
    attachmentUpdate={model,matrix,color,lighting,alpha,attachmentUpdate.calls+1};
}
struct BowStringSubmission {void* model=nullptr;void* renderState=nullptr;void* unit=nullptr;unsigned calls=0;};
static BowStringSubmission bowStringSubmission;
static void observeBowString(void* model,void* renderState,void* unit){
    bowStringSubmission={model,renderState,unit,bowStringSubmission.calls+1};
}
static Lua request(unsigned token,const WeaponSelection& s,int quiverHorizontal=-1,int hideRanged=-1,int hideMelee=-1,int actualQuiver=-1,int backBag=-1){
    Lua L;L.values.push_back(token);for(auto v:s.items)L.values.push_back(v);for(auto v:s.equipped)L.values.push_back(v);
    const int optional[]={quiverHorizontal,hideRanged,hideMelee,actualQuiver,backBag};
    for(unsigned i=0;i<5;++i){
        bool later=false;for(unsigned j=i;j<5;++j)if(optional[j]>=0)later=true;
        if(later)L.values.push_back(optional[i]<0?0:optional[i]);
    }
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
            const double minimum=index<7?-1:(index<10?-180:25),maximum=index<7?1:(index<10?180:200);
            for(double value:{std::nextafter(minimum,-std::numeric_limits<double>::infinity()),
                              std::nextafter(maximum,std::numeric_limits<double>::infinity())}){
                auto invalid=valid;invalid.values[index]=value;assert(setBagFit(&invalid)==-2);
            }
        }
        for(double value:{-.1,.5,2.}){auto invalid=valid;invalid.values[11]=value;assert(setBagFit(&invalid)==-2);}
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
        assert(setWeapons(&bag)==1&&memory[address(pack)+0x1CC]==player.model&&loads==beforeLoads+1+tuningLoads);
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
        for(auto caller:{0x611E24u,0x60B5BBu,0x60B797u,0x61183Bu,0x624B35u}){
            auto* metadata=weaponInfoAt(pointer(player.unit),2,0,caller);
            assert(metadata==context->rangedInfo.data()&&metadata[1]==a->subclass&&metadata[3]==a->inventory&&metadata[4]==a->sheath);
            assert(metadata[2]==7&&metadata[5]==9&&metadata[6]==10&&metadata[7]==11);
            assert(realInfo[2][1]==weaponAsset(actual)->subclass);
        }
        assert(weaponInfoAt(pointer(player.unit),2,0,0x12345)==realInfo[2].data());
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
    std::cout<<"PASS: native hook simulation, cross-family ranged drawing/sheathing, real metadata isolation and placement tuning\n";
}
