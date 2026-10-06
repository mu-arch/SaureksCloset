#include "../native/HairMask.h"
#include "../native/Haircraft.h"
#include "../native/HatPlacement.h"
#include "../native/ArmorInspection.h"
#include <cassert>
#include <filesystem>
#include <iostream>
#include <map>
#include <cstring>
#include <unistd.h>
#define __fastcall
struct Player {std::uintptr_t model=1000,component=2000;unsigned display=50,native=50;std::uint64_t guid=7;};
static Player player;
static std::map<std::uintptr_t,std::vector<unsigned char>> memory;
template<class T>void store(std::uintptr_t p,const T& v){const auto* a=reinterpret_cast<const unsigned char*>(&v);memory[p]={a,a+sizeof(T)};}
template<class T>bool read(std::uintptr_t p,T& v){auto it=memory.find(p);if(it==memory.end()||it->second.size()!=sizeof(T))return false;std::memcpy(&v,it->second.data(),sizeof(T));return true;}
static bool snapshot(Player& p){p=player;return p.guid!=0;}
static const RaceModel* nativeModel(unsigned id){for(const auto& r:raceModels)if(r.display==id)return &r;return nullptr;}
static unsigned selectedHair=2;
static unsigned hairGroup(void*){return selectedHair;}
static unsigned reloads=0;
static bool allowLoad=true;
static void refresh(Player& p,bool model){assert(model);++reloads;
    if(allowLoad&&!hairMask::activePath.empty()){
        std::uintptr_t resource=0;assert(read(p.model+0x30,resource));std::array<char,260> name{};
        auto path=hairMask::activePath;path.resize(path.size()-3);std::strcpy(name.data(),path.c_str());store(resource+0x20,name);store(p.model+0x10,1u);
    }
}
static const char* localCapeMotionModel(const char* p){return p;}
struct {bool busy=false;}state;
struct Lua {std::vector<double> input,output;};
static bool isNumber(void* p,int i){return unsigned(i)<=static_cast<Lua*>(p)->input.size();}
static double toNumber(void* p,int i){return static_cast<Lua*>(p)->input[i-1];}
static void pushNumber(void* p,double v){static_cast<Lua*>(p)->output.push_back(v);}
static int result(void* p,int v){pushNumber(p,v);return 1;}
#include "../native/HairMaskRuntime.h"
static BagMatrix identity(){return {{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1}};}
int main(){
    namespace fs=std::filesystem;
    auto original=fs::current_path();char temp[]="/tmp/closet-hairmask-runtime-XXXXXX";assert(mkdtemp(temp));
    auto folder=fs::path(temp)/"Interface/AddOns/SaureksCloset/CapeMotion";fs::create_directories(folder);
    fs::copy_file(original/"addon/SaureksCloset/CapeMotion/B02.m2",folder/"B02.m2");fs::current_path(temp);
    const std::uintptr_t child=3000,resource=4000,bodyResource=5000,header=6000,hatHeader=7000,lookup=8000,attachments=9000,palette=10000,vertices=20000;
    store(player.component+0x10,0u);store(player.component+8,(unsigned char)0);store(player.component+0x18,1u);store(player.component+0x1c,1u);store(player.component+0x34,0u);store(player.component+0x4a8,600u);store(600,123u);
    store(child+0x1cc,player.model);store(child+0x1d0,11u);store(child+0x10,1u);store(child+0x30,resource);
    std::array<char,260> path{};std::strcpy(path.data(),"Item\\ObjectComponents\\Head\\TestHat");store(resource+0x20,path);
    store(player.model+0x30,bodyResource);store(bodyResource+0x130,header);store(header+0x10c,12u);store(header+0x110,lookup);store(lookup+22,(std::uint16_t)0);store(header+0x104,1u);store(header+0x108,attachments);store(attachments+4,(std::uint16_t)3);store(header+0x34,117u);store(player.model+0x94,palette);store(resource+0x130,hatHeader);store(hatHeader+0x44,8u);store(hatHeader+0x48,vertices);
    unsigned index=0;for(float x:{-.12f,.12f})for(float y:{-.12f,.12f})for(float z:{0.f,.3f})store(vertices+48*index++,hairMask::Point{{x,y,z}});
    auto head=identity(),hat=identity();hat[14]=1.6f;store(palette+64*3,head);store(reinterpret_cast<std::uintptr_t>(hat.data()),hat);
    haircraft::owner=7;haircraft::enabled=true;bagTuningOwner=7;
    auto call=[&](std::vector<double> args){Lua lua{args,{}};setHairMaskLua(&lua);return lua.output;};
    assert(call({1,90,90,95,35})[0]==0);assert(!reloads);
    hairMaskRuntime::capture(reinterpret_cast<void*>(child),hat.data());assert(hairMaskRuntime::ready);
    assert(call({1,90,90,95,35})[0]==1&&reloads==1);assert(!hairMask::activePath.empty());assert(fs::exists(hairMask::activePath));
    assert(hairMask::cachePath(hairMask::activePath.c_str())&&!hairMask::cachePath("Interface/AddOns/SaureksCloset/CapeMotion/Cache/H1_bad.m2"));
    for(unsigned i=0;i<10;++i){hairMaskRuntime::capture(reinterpret_cast<void*>(child),hat.data());assert(call({1,90,90,95,35})[0]==1);}assert(reloads==1);
    auto oldPath=hairMask::activePath;assert(call({1,75,90,95,35})[0]==1&&reloads==2&&hairMask::activePath!=oldPath);
    // Moving the camera/head together must not change the rest-space bake.
    auto transformed=identity();transformed[12]=7;transformed[14]=-2;
    auto movedHat=bagMatrixProduct(transformed,hat);store(palette+64*3,transformed);store(reinterpret_cast<std::uintptr_t>(movedHat.data()),movedHat);
    hairMaskRuntime::ready=false;hairMaskRuntime::capture(reinterpret_cast<void*>(child),movedHat.data());assert(call({1,75,90,95,35})[0]==1);assert(reloads==2);
    auto stable=hairMask::activePath;assert(stable.find("H1_")!=std::string::npos);
    // Fit edits request one new capture; completed fits do no render-hook work.
    auto& fit=hatTuningEntries[1];fit.enabled=true;fit.values.scale=100;fit.values.up=.03f;
    assert(call({1,75,90,95,35})[0]==0);
    hairMaskRuntime::capture(reinterpret_cast<void*>(child),movedHat.data());assert(call({1,75,90,95,35})[0]==1&&reloads==4&&hairMask::activePath!=stable);
    assert(call({1,75,90,20,35})[0]==-2&&reloads==4);
    assert(hairMask::model("original",7,2,0)==hairMask::activePath.c_str());assert(std::string(hairMask::model("original",8,2,0))=="original");assert(std::string(hairMask::model("original",7,1,0))=="original");
    // A newly equipped/hidden hat cannot retain the previous mask.
    store(600,124u);assert(call({1,75,90,95,35})[0]==0&&hairMask::activePath.empty());
    hairMaskRuntime::capture(reinterpret_cast<void*>(child),movedHat.data());assert(call({1,75,90,95,35})[0]==1);
    store(player.component+0x4a8,0u);assert(call({1,75,90,95,35})[0]==2&&hairMask::activePath.empty());
    store(player.component+0x4a8,600u);hairMaskRuntime::capture(reinterpret_cast<void*>(child),movedHat.data());assert(call({1,75,90,95,35})[0]==1);
    // Direct plane mode must be a distinct bake and report real resource loading.
    assert(call({1,90,90,95,35,1,35,0,0})[0]==1);auto flat=hairMask::activePath;
    assert(call({1,90,90,95,35,1,35,15,-10})[0]==1&&hairMask::activePath!=flat);
    assert(call({1,90,90,95,35,1,101,0,0})[0]==-2);
    allowLoad=false;assert(call({1,90,90,95,35,1,20,0,0})[0]==3);
    hairMaskRuntime::loadDeadline=std::chrono::steady_clock::now()-std::chrono::seconds(1);
    assert(call({1,90,90,95,35,1,20,0,0})[0]==-6);
    assert(call({0,75,90,95,35})[0]==1&&hairMask::activePath.empty());unsigned last=reloads;assert(call({0,75,90,95,35})[0]==1&&reloads==last);
    // Other owners, effects and previews cannot supply a mask capture.
    hairMaskRuntime::requested=true;hairMaskRuntime::ready=false;store(child+0x1cc,std::uintptr_t(999));hairMaskRuntime::capture(reinterpret_cast<void*>(child),hat.data());assert(!hairMaskRuntime::ready);
    fs::current_path(original);fs::remove_all(temp);
    std::cout<<"PASS: actual native capture/bake dispatch, isolated disk cache, idempotent sync, fit/hat changes, hidden/disabled restore and owner isolation\n";
}
