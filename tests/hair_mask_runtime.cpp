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
int main(int argc,char** argv){
    namespace fs=std::filesystem;
    auto original=fs::current_path();char temp[]="/tmp/closet-hairmask-runtime-XXXXXX";assert(mkdtemp(temp));
    auto folder=fs::path(temp)/"Interface/AddOns/SaureksCloset/CapeMotion";fs::create_directories(folder);
    fs::copy_file(original/"addon/SaureksCloset/CapeMotion/B02.m2",folder/"B02.m2");fs::current_path(temp);
    const std::uintptr_t child=3000,resource=4000,bodyResource=5000,header=6000,hatHeader=7000,lookup=8000,attachments=9000,palette=10000,vertices=20000;
    store(player.component+0x10,0u);store(player.component+8,(unsigned char)0);store(player.component+0x18,1u);store(player.component+0x1c,1u);store(player.component+0x34,0u);store(player.component+0x4a8,600u);store(600,123u);
    store(child+0x1cc,player.model);store(child+0x1d0,11u);store(child+0x10,1u);store(child+0x30,resource);
    std::array<char,260> path{};std::strcpy(path.data(),"Item\\ObjectComponents\\Head\\TestHat");store(resource+0x20,path);
    store(player.model+0x30,bodyResource);store(bodyResource+0x130,header);store(header+0x10c,12u);store(header+0x110,lookup);store(lookup+22,(std::uint16_t)0);store(header+0x104,1u);store(header+0x108,attachments);store(attachments+4,(std::uint16_t)3);store(header+0x34,117u);store(player.model+0x94,palette);store(resource+0x130,hatHeader);store(hatHeader+0x44,8u);store(hatHeader+0x48,vertices);
    const std::uintptr_t view=100000,indices=200000,triangles=300000,sections=400000,batches=500000,flags=600000;
    store(hatHeader+0x4c,1u);store(hatHeader+0x50,view);store(view,8u);store(view+4,indices);store(view+8,6u);store(view+12,triangles);store(view+24,1u);store(view+28,sections);store(view+32,1u);store(view+36,batches);store(hatHeader+0x84,1u);store(hatHeader+0x88,flags);
    // The fishing hat has a texture-replacement lookup at 0x7c/0x80
    // containing -1. It is NOT the render-flag array at 0x84/0x88.
    const std::uintptr_t replacements=700000;store(hatHeader+0x7c,3u);store(hatHeader+0x80,replacements);store(replacements+2,(std::uint16_t)0xffff);
    store(sections+8,(std::uint16_t)0);store(sections+10,(std::uint16_t)6);store(batches+4,(std::uint16_t)0);store(batches+10,(std::uint16_t)0);store(flags+2,(std::uint16_t)0);
    for(unsigned i=0;i<8;++i)store(indices+2*i,(std::uint16_t)i);
    unsigned ti=0;for(unsigned v:{1,3,5,3,7,5})store(triangles+2*ti++,(std::uint16_t)v);
    unsigned index=0;for(float x:{-.12f,.12f})for(float y:{-.12f,.12f})for(float z:{0.f,.3f})store(vertices+48*index++,hairMask::Point{{x,y,z}});
    auto head=identity(),hat=identity();hat[14]=1.6f;store(palette+64*3,head);store(reinterpret_cast<std::uintptr_t>(hat.data()),hat);
    haircraft::owner=7;haircraft::enabled=true;bagTuningOwner=7;
    auto call=[&](std::vector<double> args){Lua lua{args,{}};setHairMaskLua(&lua);return lua.output;};
    assert(call({1,90,90,95,35,1,35,0,0})[0]==-2); // retired destructive API
    assert(call({1,3})[0]==0);assert(!reloads);
    hairMaskRuntime::capture(reinterpret_cast<void*>(child),hat.data());assert(hairMaskRuntime::ready&&hairMaskRuntime::hatSurface.size()==2);
    assert(call({1,3})[0]==1&&reloads==1);assert(!hairMask::activePath.empty());assert(fs::exists(hairMask::activePath));
    assert(hairMask::cachePath(hairMask::activePath.c_str())&&!hairMask::cachePath("../../bad.m2"));
    for(unsigned i=0;i<10;++i){hairMaskRuntime::capture(reinterpret_cast<void*>(child),hat.data());assert(call({1,3})[0]==1);}assert(reloads==1);
    auto transformed=identity();transformed[12]=7;transformed[14]=-2;
    auto movedHat=bagMatrixProduct(transformed,hat);store(palette+64*3,transformed);store(reinterpret_cast<std::uintptr_t>(movedHat.data()),movedHat);
    hairMaskRuntime::ready=false;hairMaskRuntime::capture(reinterpret_cast<void*>(child),movedHat.data());assert(call({1,3})[0]==1&&reloads==1);
    auto& fit=hatTuningEntries[1];fit.enabled=true;fit.values.scale=100;fit.values.up=.03f;
    assert(call({1,3})[0]==0&&hairMask::activePath.empty());
    hairMaskRuntime::capture(reinterpret_cast<void*>(child),movedHat.data());assert(call({1,3})[0]==1);
    assert(hairMask::model("original",7,2,0)==hairMask::activePath.c_str());assert(std::string(hairMask::model("original",8,2,0))=="original");
    store(600,124u);assert(call({1,3})[0]==0&&hairMask::activePath.empty());
    hairMaskRuntime::capture(reinterpret_cast<void*>(child),movedHat.data());assert(call({1,3})[0]==1);
    store(player.component+0x4a8,0u);assert(call({1,3})[0]==2&&hairMask::activePath.empty());
    store(player.component+0x4a8,600u);allowLoad=false;
    std::array<char,260> ordinaryName{};std::strcpy(ordinaryName.data(),"Character/Human/Female/HumanFemale");store(bodyResource+0x20,ordinaryName);
    hairMaskRuntime::capture(reinterpret_cast<void*>(child),movedHat.data());assert(call({1,3})[0]==3);
    hairMaskRuntime::loadDeadline=std::chrono::steady_clock::now()-std::chrono::seconds(1);assert(call({1,3})[0]==-6);
    assert(call({0})[0]==1&&hairMask::activePath.empty());unsigned last=reloads;assert(call({0,90,90,95,35})[0]==1&&reloads==last);
    // Alpha feathers cannot masquerade as solid crown coverage.
    assert(call({1,3})[0]==0);store(flags+2,(std::uint16_t)1);hairMaskRuntime::capture(reinterpret_cast<void*>(child),movedHat.data());assert(hairMaskRuntime::ready&&hairMaskRuntime::hatSurface.empty());
    assert(call({1,3})[0]==4&&hairMask::activePath.empty());
    // Other owners, effects and previews cannot supply a mask capture.
    hairMaskRuntime::requested=true;hairMaskRuntime::ready=false;store(child+0x1cc,std::uintptr_t(999));hairMaskRuntime::capture(reinterpret_cast<void*>(child),hat.data());assert(!hairMaskRuntime::ready);
    // Optional integration against a hat extracted from the user's client:
    // feed its actual serialized draw data through the production capture,
    // rather than testing build() with an already assembled surface.
    if(argc>1){
        hairMask::Bytes asset;assert(capeMotion::readFile(argv[1],asset));
        const auto u=[&](unsigned p){return hairMask::u32(asset,p);};
        const auto h=[&](unsigned p){return std::uint16_t(hairMask::u16(asset,p));};
        const auto av=u(80),nv=u(68),ni=u(av),nt=u(av+8),ns=u(av+24),nb=u(av+32),nf=u(0x84);
        store(hatHeader+0x44,nv);store(view,ni);store(view+8,nt);store(view+24,ns);store(view+32,nb);store(hatHeader+0x84,nf);
        for(unsigned i=0;i<nv;++i){hairMask::Point p;for(unsigned k=0;k<3;++k)p[k]=hairMask::number(asset,u(72)+48*i+4*k);store(vertices+48*i,p);}
        for(unsigned i=0;i<ni;++i)store(indices+2*i,h(u(av+4)+2*i));
        for(unsigned i=0;i<nt;++i)store(triangles+2*i,h(u(av+12)+2*i));
        for(unsigned i=0;i<ns;++i){store(sections+32*i+8,h(u(av+28)+32*i+8));store(sections+32*i+10,h(u(av+28)+32*i+10));}
        for(unsigned i=0;i<nb;++i){store(batches+24*i+4,h(u(av+36)+24*i+4));store(batches+24*i+10,h(u(av+36)+24*i+10));}
        for(unsigned i=0;i<nf;++i)store(flags+4*i+2,h(u(0x88)+4*i+2));
        store(child+0x1cc,player.model);allowLoad=true;fit.enabled=false;selectedHair=10;
        if(argc>2){fit.enabled=true;fit.values={};fit.values.scale=100;fit.values.pitch=11;fit.values.up=.01f;fit.values.inset=.015f;}
        auto nativeHead=identity(),nativeHat=identity();nativeHat[12]=.052003894f;nativeHat[14]=1.896982789f;
        store(palette+64*3,nativeHead);store(reinterpret_cast<std::uintptr_t>(nativeHat.data()),nativeHat);
        assert(call({0})[0]==1);assert(call({1,3})[0]==0);
        hairMaskRuntime::capture(reinterpret_cast<void*>(child),nativeHat.data());
        assert(hairMaskRuntime::ready&&!hairMaskRuntime::hatSurface.empty());
        auto result=call({1,3});assert(result[0]==1&&result[1]>0);
        if(argc>2){
            hairMask::Bytes base,baked;assert(capeMotion::readFile((folder/"B02.m2").string(),base));assert(capeMotion::readFile(hairMask::activePath,baked));
            const auto vo=hairMask::u32(base,72);
            // Chapeau with the user's saved fit: trim the crown next to a
            // hanging strand while leaving the ponytail and skin joins intact.
            assert(std::memcmp(base.data()+vo+48*1747,baked.data()+vo+48*1747,12)!=0);
            for(unsigned v:{1763u,1768u,1811u,1746u,1754u,1764u,1779u,1808u,1814u,1815u,1816u})
                assert(std::memcmp(base.data()+vo+48*v,baked.data()+vo+48*v,48)==0);
        }
        std::cout<<"PASS: real client hat capture: "<<hairMaskRuntime::hatSurface.size()<<" surface triangles, "<<result[1]<<" fitted hair vertices\n";
    }
    fs::current_path(original);fs::remove_all(temp);
    std::cout<<"PASS: actual native capture/bake dispatch, isolated disk cache, idempotent sync, fit/hat changes, hidden/disabled restore and owner isolation\n";
}
