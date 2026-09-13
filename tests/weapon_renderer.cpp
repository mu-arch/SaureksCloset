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
    const auto* a=weaponAsset(realIDs[role]);if(!a)return;
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
        const auto* a=weaponAsset(realIDs[2]);
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
static Lua request(unsigned token,const WeaponSelection& s,int quiverHorizontal=-1,int hideRanged=-1,int hideMelee=-1,int actualQuiver=-1){
    Lua L;L.values.push_back(token);for(auto v:s.items)L.values.push_back(v);for(auto v:s.equipped)L.values.push_back(v);
    const int optional[]={quiverHorizontal,hideRanged,hideMelee,actualQuiver};
    for(unsigned i=0;i<4;++i){
        bool later=false;for(unsigned j=i;j<4;++j)if(optional[j]>=0)later=true;
        if(later)L.values.push_back(optional[i]<0?0:optional[i]);
    }
    return L;
}
int main(){
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
    std::cout<<"PASS: native hook simulation (routing, draw reuse, callback ownership, preview isolation, detach recovery, destruction, idempotence, input validation)\n";
}
