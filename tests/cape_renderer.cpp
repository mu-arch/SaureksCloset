// Real hook logic with fake native memory and transient draw buffers. This
// checks data flow and isolation, not the client's ABI or live appearance.
#define __fastcall
#define __thiscall
#define SAUREKS_CAPE_TEST
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <map>
#include <memory>
#include <vector>
#include <iostream>
#include "../native/BagCoordinates.h"
#include "../native/CapeCloth.h"
using SIZE_T=std::size_t;
static std::map<std::uintptr_t,std::vector<unsigned char>> memory;
static void* GetCurrentProcess(){return nullptr;}
static bool ReadProcessMemory(void*,const void* address,void* out,SIZE_T size,SIZE_T* copied){
    const auto at=reinterpret_cast<std::uintptr_t>(address);auto it=memory.upper_bound(at);
    while(it!=memory.begin()){
        --it;const auto offset=at-it->first;
        if(offset<=it->second.size()&&size<=it->second.size()-offset){std::memcpy(out,it->second.data()+offset,size);*copied=size;return true;}
    }
    *copied=0;return false;
}
template<class T> static bool read(std::uintptr_t address,T& out){SIZE_T n=0;return ReadProcessMemory(nullptr,reinterpret_cast<void*>(address),&out,sizeof(out),&n);}
template<class T> static void put(std::uintptr_t address,const T& value){auto& b=memory[address];b.resize(sizeof(value));std::memcpy(b.data(),&value,sizeof(value));}
template<class T> static void array(std::uintptr_t field,std::uintptr_t storage,const std::vector<T>& values){
    put(field,static_cast<unsigned>(values.size()));put(field+4,storage);auto& b=memory[storage];b.resize(values.size()*sizeof(T));if(!b.empty())std::memcpy(b.data(),values.data(),b.size());
}
struct Player {std::uintptr_t model=0x10000;std::uint64_t guid=17;};
static Player player;
static bool snapshot(Player& p){p=player;return p.guid!=0;}
static unsigned now=1000;
static unsigned bagClockMilliseconds(){return now;}
static unsigned worldClears=0;
static bool worldAvailable=true;
static void capeClearWorldCollision(){++worldClears;}
static bool capeWorldColliders(const cape::Vec3& low,const cape::Vec3& high,std::vector<cape::ColliderTriangle>&){return worldAvailable&&high.x-low.x<=16&&high.y-low.y<=16&&high.z-low.z<=16;}
struct Buffer {unsigned stride=0,count=0;std::vector<unsigned char> bytes;};
struct Pool {unsigned bytes=0;};
static std::vector<std::unique_ptr<Buffer>> buffers;
static Buffer nativeBuffer;
static Buffer* bound=&nativeBuffer;
static unsigned boundFormat=12,submits=0;
static std::vector<unsigned char> submitted;
static unsigned pools=0,releases=0;
static void* createPool(unsigned kind,unsigned usage,unsigned bytes,unsigned flags,const char*){assert(!kind&&!usage&&!flags);++pools;return new Pool{bytes};}
static void* createBuffer(void* pool,unsigned stride,unsigned count,unsigned flags){assert(!flags&&static_cast<Pool*>(pool)->bytes==stride*count);auto buffer=std::make_unique<Buffer>();buffer->stride=stride;buffer->count=count;buffer->bytes.resize(stride*count);void* out=buffer.get();buffers.push_back(std::move(buffer));return out;}
static void destroyBuffer(void* device,void** value){assert(device==reinterpret_cast<void*>(0xf0000)&&*value!=&nativeBuffer);*value=nullptr;++releases;}
static void destroyPool(void* pool){delete static_cast<Pool*>(pool);--pools;}
static void* mapBuffer(void* buffer){return static_cast<Buffer*>(buffer)->bytes.data();}
static void unmapBuffer(void*,unsigned flags){assert(!flags);}
static void bindBuffer(void* buffer,unsigned format){bound=static_cast<Buffer*>(buffer);boundFormat=format;}
static void submit(const void*,unsigned count){assert(count==1);++submits;submitted=bound->bytes;}
static void cpuSkin(void*,const void*,void*);
template<class T> static T capeFunction(std::uintptr_t address){
    if(address==0x58A160)return reinterpret_cast<T>(&createPool);
    if(address==0x589F80)return reinterpret_cast<T>(&createBuffer);
    if(address==0x594550)return reinterpret_cast<T>(&destroyBuffer);
    if(address==0x58A1A0)return reinterpret_cast<T>(&destroyPool);
    if(address==0x58A080)return reinterpret_cast<T>(&mapBuffer);
    if(address==0x58A0A0)return reinterpret_cast<T>(&unmapBuffer);
    if(address==0x71A460||address==0x71A720||address==0x71A9E0)return reinterpret_cast<T>(&cpuSkin);
    assert(false);return nullptr;
}
#include "../native/CapeRenderer.h"
static void cpuSkin(void*,const void* sectionPointer,void* output){
    CapeSection section;assert(read(reinterpret_cast<std::uintptr_t>(sectionPointer),section));
    const unsigned stride=capeBoundFormat==5?40:32;
    for(unsigned i=0;i<section.count;++i){
        const auto& vertex=capeState.mesh.vertices[section.first+i];BagMatrix matrix;
        assert(capeSkinMatrix(capeState.mesh,section.first+i,matrix));
        const auto p=capeTransform(matrix,vertex.position);
        std::memcpy(static_cast<char*>(output)+i*stride,&p,12);
        std::memcpy(static_cast<char*>(output)+i*stride+12,&vertex.normal,12);
        std::memcpy(static_cast<char*>(output)+i*stride+24,vertex.uv.data(),stride-24);
    }
}
static const BagMatrix identity{{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1}};
static void fixture(bool gpu=true){
    capeReset();memory.clear();buffers.clear();now+=100;worldAvailable=true;
    put(0xC0ED38,std::uintptr_t(0xf0000));
    constexpr std::uintptr_t model=0x10000,resource=0x20000,header=0x30000,view=0x40000;
    player={model,17};
    put(model+0x10,1u);put(model+0x30,resource);put(model+0x94,std::uintptr_t(0x80000));put(model+0x98,std::uintptr_t(0x81000));
    put(model+0x2c,std::uintptr_t(0x82000));put(0x82000+0x9c,identity);put(model+0x1dc,std::uintptr_t(0));
    put(resource+0x130,header);put(resource+0x138,view);put(resource+4,std::uintptr_t(0x83000));put(resource+0x15c,1u);put(0x83000+4,gpu?8u:0u);
    put(header,0x3032444du);put(header+4,256u);put(header+0x34,1u);put(0x80000,identity);
    // Nonidentity lookup verifies the GPU consumes view order, not source order.
    std::vector<CapeSourceVertex> vertices(6);
    const cape::Vec3 points[]={{0,-.3f,2},{0,.3f,2},{0,-.3f,1},{0,.3f,1},{0,-.3f,0},{0,.3f,0}};
    std::vector<std::uint16_t> lookup{5,4,3,2,1,0};if(!gpu)lookup={0,1,2,3,4,5};
    for(unsigned i=0;i<6;++i){auto& v=vertices[lookup[i]];v.position=points[i];v.normal={1,0,0};v.weights[0]=255;v.uv[0]=float(i)/6;v.uv[1]=.5f;}
    array(header+0x44,0x50000,vertices);array(view,0x60000,lookup);
    array(view+8,0x61000,std::vector<std::uint16_t>{0,2,1,1,2,3,2,4,3,3,4,5});
    array(view+16,0x62000,std::vector<std::uint32_t>(6));
    CapeSection section;section.geoset=1502;section.count=6;section.triangleCount=12;section.boneCount=1;
    array(view+24,0x63000,std::vector<CapeSection>{section});
    CapeTextureUnit unit;unit.value[7]=1;
    array(view+32,0x64000,std::vector<CapeTextureUnit>{unit});
    array(header+0x94,0x65000,std::vector<std::uint16_t>{0});array(header+0x5c,0x66000,std::vector<std::array<unsigned,4>>{{2,0,0,0}});
    put(0x81000,1u);
    put(0x90000+0x3310,model);put(0x90000+0x3300,std::uintptr_t(0xa0000));put(0x90000+0x32f0,gpu?8u:0u);
    put(0xa0000+0x30,std::uintptr_t(0x63000));put(0xa0000+0x2c,std::uintptr_t(0x64000));put(0xa0000+0x34,0u);
    put(0xCF04C8,std::uintptr_t(0x71A720));
    capeBindOriginal=&bindBuffer;capeSubmitOriginal=&submit;
    capeSetEnabled(true);capeReset();capeBindHook(&nativeBuffer,gpu?12:3);capeDrawScope=0x90000;
}
static cape::Vec3 outputPoint(unsigned index,unsigned stride){cape::Vec3 p;std::memcpy(&p,submitted.data()+index*stride,12);return p;}
static void drawAndCheck(bool gpu){
    fixture(gpu);const auto pristine=memory;
    assert(capeWriteDraw(0x90000,nullptr,1));assert(capeStatus()==2);
    assert(bound==&nativeBuffer&&boundFormat==(gpu?12u:3u));assert(memory==pristine);
    assert(capeState.pins.size()==2&&capeState.triangles.size()==4);
    const auto first=outputPoint(4,gpu?48:32);now+=17;
    assert(capeWriteDraw(0x90000,nullptr,1));
    assert(outputPoint(4,gpu?48:32).z<first.z); // Genuine integrated gravity.
    assert(std::fabs(outputPoint(0,gpu?48:32).z-2)<1e-5f); // Shoulder seam.
    assert(memory==pristine&&bound==&nativeBuffer);
    BagMatrix moved=identity;moved[12]=50;put(0x80000,moved);now+=17;
    assert(capeWriteDraw(0x90000,nullptr,1));assert(capeStatus()==2);
    assert(std::fabs(capeState.cloth.positions()[0].x-50)<.001f);
    // GPU output is inverse-skinned back to source coordinates; CPU output
    // already includes the translated render-space bone transform.
    assert(std::fabs(outputPoint(0,gpu?48:32).x-(gpu?0:50))<.001f);
    now+=17;worldAvailable=false;
    assert(!capeWriteDraw(0x90000,nullptr,1));assert(capeStatus()==3);
    assert(!capeWriteDraw(0x90000,nullptr,1)); // No stale same-tick success.
    worldAvailable=true;now+=17;
    put(0xa0000+0x34,1u);assert(!capeWriteDraw(0x90000,nullptr,1));assert(capeStatus()==3);
    put(0xa0000+0x34,0u);put(0x66000,8u); // Tauren tail texture.
    const unsigned before=submits;assert(!capeWriteDraw(0x90000,nullptr,1));assert(submits==before);
    capeForgetModel(player.model);assert(!capeState.cloth.ready());
    capeSetEnabled(false);assert(capeStatus()==0);
}
static void invalidRetry(){
    fixture();CapeSection section;assert(read(0x63000,section));const auto valid=section;
    section.count=0;section.triangleCount=0;put(0x63000,section);
    assert(!capeWriteDraw(0x90000,nullptr,1));assert(!capeWriteDraw(0x90000,nullptr,1));
    assert(!capeState.cloth.ready());put(0x63000,valid);
    assert(capeWriteDraw(0x90000,nullptr,1));
    player.model=0xb0000;assert(!capeWriteDraw(0x90000,nullptr,1));
    capeSetEnabled(false);
}
static void optimizedDraw(bool gpu,unsigned cpuFormat=3){
    fixture(gpu);CapeMesh mesh;assert(capeReadMesh(player.model,mesh));
    if(!gpu)capeBindHook(&nativeBuffer,cpuFormat);
    std::vector<CapeSourceVertex> vertices(18);std::vector<std::uint16_t> lookup(18);
    for(unsigned i=0;i<18;++i){lookup[i]=gpu?17-i:i;auto& vertex=vertices[lookup[i]];
        vertex.weights[0]=255;vertex.normal={1,0,0};vertex.position={5,float(i%3),float(i%2)};vertex.uv[0]=float(i)/18;
    }
    for(unsigned start:{3u,12u})for(unsigned i=0;i<6;++i){auto& vertex=vertices[lookup[start+i]];
        vertex=mesh.vertices[capeSource(mesh,i)];vertex.position.x=start==3?0:.1f;vertex.uv[0]=float(start+i)/18;
    }
    std::vector<CapeSection> sections(4);sections[0].geoset=0;sections[0].count=3;sections[0].triangleCount=3;
    sections[1]=mesh.sections[0];sections[1].first=3;sections[1].triangleFirst=3;
    sections[2]=sections[0];sections[2].first=9;sections[2].triangleFirst=15;
    sections[3]=mesh.sections[0];sections[3].first=12;sections[3].triangleFirst=18;
    std::vector<std::uint16_t> triangles{0,1,2};
    for(auto i:mesh.triangles)triangles.push_back(i+3);
    triangles.insert(triangles.end(),{9,10,11});for(auto i:mesh.triangles)triangles.push_back(i+12);
    std::vector<CapeTextureUnit> units(4,mesh.units[0]);for(unsigned i=0;i<4;++i)units[i].value[2]=i;
    array(0x30000+0x44,0x50000,vertices);array(0x40000,0x60000,lookup);
    array(0x40000+8,0x61000,triangles);array(0x40000+16,0x62000,std::vector<std::uint32_t>(18));
    array(0x40000+24,0x63000,sections);array(0x40000+32,0x64000,units);
    const std::array<unsigned,4> visible{{1,1,0,1}};put(0x81000,visible);
    // Reproduce 711230's compact ID rewrite and CPU first=0 rebase. Source
    // range [1,3] crosses a hidden noncape section, which native skips.
    auto copied=sections[1];copied.geoset=1;copied.first=gpu?3:0;copied.count=gpu?15:12;copied.triangleCount=24;
    std::vector<CapeTextureUnit> compactUnits{units[0],units[1]};
    std::vector<CapeSection> compactSections{sections[0],copied};
    array(0x10000+0x3e8,0x87000,compactUnits); // Set exact native pointer/count field order below.
    put(0x10000+0x3ec,std::uintptr_t(0x87000));put(0x10000+0x3f0,2u);
    array(0x10000+0x3f0,0x88000,compactSections);
    put(0x10000+0x3f4,std::uintptr_t(0x88000));put(0x10000+0x3f8,2u);
    put(0x10000+0x3fc,std::uintptr_t(0x89000));put(0x89000+8,std::array<unsigned,2>{{1,3}});
    put(0xa0000+0x2c,std::uintptr_t(0x87000+24));put(0xa0000+0x30,std::uintptr_t(0x88000+32));put(0xa0000+0x34,1u);
    const auto pristine=memory;const unsigned before=submits;
    capeSubmitHook(nullptr,1);assert(capeStatus()==2&&submits==before+1);
    const unsigned stride=gpu?48:cpuFormat==5?40:32;const unsigned a=gpu?3:0,b=gpu?12:6;
    assert(std::fabs(outputPoint(a,stride).z-2)<1e-5f&&std::fabs(outputPoint(b,stride).z-2)<1e-5f);
    for(unsigned j=0;j<12;++j){const unsigned original=(j<6?3:12)+j%6;float uv=0;
        std::memcpy(&uv,submitted.data()+stride*(gpu?original:j)+(gpu?32:24),4);assert(uv==float(original)/18);
    }
    now+=17;capeSubmitHook(nullptr,1);assert(capeStatus()==2);
    assert(outputPoint(a+4,stride).z<0&&outputPoint(b+4,stride).z<0);
    assert(memory==pristine&&bound==&nativeBuffer);
    // A visible noncape member must keep the untouched native batch and
    // explicitly report unsupported; a stale group ID must do the same.
    put(0x81000,std::array<unsigned,4>{{1,1,1,1}});now+=17;const unsigned rejected=submits;
    capeSubmitHook(nullptr,1);assert(capeStatus()==3&&submits==rejected+1&&bound==&nativeBuffer);
    put(0x81000,visible);put(0x89000+8,std::array<unsigned,2>{{1,99}});
    capeSubmitHook(nullptr,1);assert(capeStatus()==3&&bound==&nativeBuffer);
    capeSetEnabled(false);
}
static void sweptEquipment(){
    CapeState state;state.model=1;state.renderToWorld=identity;
    CapeMesh mesh;mesh.model=2;mesh.header=3;mesh.view=4;mesh.lookup={0,1,2};mesh.triangles={0,1,2};mesh.vertices.resize(3);mesh.visible={1};mesh.capeSections={false};mesh.bones={identity};
    CapeSection section;section.count=3;section.triangleCount=3;mesh.sections={section};
    const cape::Vec3 p[]={{0,-1,-1},{0,1,-1},{0,0,1}};
    for(unsigned i=0;i<3;++i){mesh.vertices[i].position=p[i];mesh.vertices[i].weights[0]=255;}
    const cape::Vec3 low{-.1f,-.1f,-.1f},high{.1f,.1f,.1f};
    std::vector<cape::ColliderTriangle> contacts;
    std::unordered_map<std::uint64_t,std::array<cape::Vec3,3>> next;
    mesh.bones[0][12]=-1;
    assert(capeCollectSurface(state,mesh,low,high,contacts,next));assert(contacts.empty()&&next.size()==1);
    state.previousSurfaces=next;next.clear();mesh.bones[0][12]=1;
    assert(capeCollectSurface(state,mesh,low,high,contacts,next));assert(contacts.size()==1);
    assert(contacts[0].previous[0].x==-1&&contacts[0].current[0].x==1);
    contacts.clear();next.clear();++mesh.view;
    assert(capeCollectSurface(state,mesh,low,high,contacts,next));assert(contacts.empty());
}
static void disconnectedCollar(){
    fixture(false);CapeState state;assert(capeReadMesh(player.model,state.mesh));state.sections={0,1};
    auto& mesh=state.mesh;
    for(const cape::Vec3 p:std::array<cape::Vec3,3>{{{.1f,-.1f,1.7f},{.2f,-.1f,1.3f},{.1f,.1f,1.5f}}}){
        CapeSourceVertex v;v.position=p;v.weights[0]=255;mesh.vertices.push_back(v);mesh.lookup.push_back(static_cast<unsigned>(mesh.lookup.size()));
    }
    CapeSection collar;collar.geoset=1502;collar.first=6;collar.count=3;collar.triangleFirst=12;collar.triangleCount=3;
    mesh.sections.push_back(collar);mesh.triangles.insert(mesh.triangles.end(),{6,7,8});
    assert(capeBuildTopology(state));
    for(unsigned i=6;i<9;++i)assert(std::find(state.pins.begin(),state.pins.end(),i)!=state.pins.end());
}
int main(){optimizedDraw(true);optimizedDraw(false);optimizedDraw(false,5);drawAndCheck(true);drawAndCheck(false);invalidRetry();sweptEquipment();disconnectedCollar();capeReset();assert(worldClears>0&&pools==0&&releases>=3);std::cout<<"cape renderer: optimized GPU/CPU groups, UV/rebased indices, private output, binding restore, failure retry, swept equipment, LOD invalidation, collar attachment and tail exclusion passed\n";}
