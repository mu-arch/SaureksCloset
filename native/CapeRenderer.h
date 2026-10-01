#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <limits>
#include <unordered_map>
#include <vector>
#include "CapeCloth.h"
#include "CapeBodyBounds.h"

// Build 5875 only. Included after WeaponRenderer.h and CapeWorldCollision.h.
// The source MD20 and its GPU buffer are shared. Only a temporary draw buffer
// is changed; the exact native binding is restored before returning to CM2.
using CapeBatchDraw=void (__thiscall *)(void*);
// 58A830 submits one primitive descriptor; EDX is the indexed-draw flag.
// 58A810 passes it as argument 3; D3D 5A1042 chooses indexed/nonindexed.
using CapeSubmit=void (__fastcall *)(const void*,unsigned);
using CapeBind=void (__fastcall *)(void*,unsigned);
using CapeCreatePool=void* (__fastcall *)(unsigned,unsigned,unsigned,unsigned,const char*);
using CapeCreateBuffer=void* (__fastcall *)(void*,unsigned,unsigned,unsigned);
using CapeDestroyBuffer=void (__thiscall *)(void*,void**);
using CapeDestroyPool=void (__fastcall *)(void*);
using CapeMap=void* (__fastcall *)(void*);
using CapeUnmap=void (__fastcall *)(void*,unsigned);
using CapeSkin=void (__fastcall *)(void*,const void*,void*);
static CapeBatchDraw capeDrawOriginal=nullptr;
static CapeSubmit capeSubmitOriginal=nullptr;
static CapeBind capeBindOriginal=nullptr;
#ifndef SAUREKS_CAPE_TEST
template<class T> static T capeFunction(std::uintptr_t address){return reinterpret_cast<T>(address);}
#endif
static const auto capeCreatePool=capeFunction<CapeCreatePool>(0x58A160);
static const auto capeCreateBuffer=capeFunction<CapeCreateBuffer>(0x589F80);
static const auto capeDestroyBuffer=capeFunction<CapeDestroyBuffer>(0x594550);
static const auto capeDestroyPool=capeFunction<CapeDestroyPool>(0x58A1A0);
static const auto capeMap=capeFunction<CapeMap>(0x58A080);
static const auto capeUnmap=capeFunction<CapeUnmap>(0x58A0A0);
static thread_local std::uintptr_t capeDrawScope=0;
static thread_local void* capeBoundBuffer=nullptr;
static thread_local unsigned capeBoundFormat=0;
static bool capeEnabled=false;
static unsigned capeRuntimeStatus=0; // 0 off, 1 waiting/hidden, 2 active, 3 unsupported.
static std::uint32_t capeLastDraw=0;
static std::uintptr_t capePlayerModel=0;
static std::uint32_t capePlayerChecked=0;
struct CapeVertexStorage {void* pool=nullptr;void* buffer=nullptr;unsigned capacity=0;};
static std::array<CapeVertexStorage,3> capeVertexStorage{};
static bool capeReleaseStorage(CapeVertexStorage& storage){
    if(storage.buffer){
        std::uintptr_t device=0;if(!read(0xC0ED38,device)||!device)return false;
        // The public buffer-release wrapper first rewrites the shared
        // transient descriptor. We own this buffer, so call its verified
        // underlying unlink/free method directly (58A040 -> 594550).
        capeDestroyBuffer(reinterpret_cast<void*>(device),&storage.buffer);
    }
    if(storage.pool)capeDestroyPool(storage.pool);
    storage={};return true;
}
static void* capePrivateBuffer(unsigned stride,unsigned count){
    const unsigned index=stride==32?0:stride==40?1:2;
    auto& storage=capeVertexStorage[index];
    if(storage.buffer&&storage.capacity>=count)return storage.buffer;
    if(!capeReleaseStorage(storage))return nullptr;
    // Same pool/sub-buffer construction as CM2 resource 71DC80. Unlike
    // 58A140, these allocate distinct descriptors and leave native ring
    // offsets/strides intact. CGx binding applies its own buffer offset.
    storage.pool=capeCreatePool(0,0,stride*count,0,"SaureksCloset cape vertices");
    if(!storage.pool)return nullptr;
    storage.buffer=capeCreateBuffer(storage.pool,stride,count,0);
    if(!storage.buffer){capeReleaseStorage(storage);return nullptr;}
    storage.capacity=count;return storage.buffer;
}

struct CapeSourceVertex {
    cape::Vec3 position;
    std::array<std::uint8_t,4> weights,bones;
    cape::Vec3 normal;
    std::array<float,4> uv;
};
static_assert(sizeof(CapeSourceVertex)==48,"5875 MD20 vertex");
struct CapeSection {
    std::uint32_t geoset=0;
    std::uint16_t first=0,count=0,triangleFirst=0,triangleCount=0,boneCount=0,boneFirst=0,boneInfluences=0,centerBone=0;
    std::array<float,3> center{};
};
static_assert(sizeof(CapeSection)==32,"5875 MD20 section");
struct CapeTextureUnit { std::array<std::uint16_t,12> value{}; };
struct CapeMesh {
    std::uintptr_t model=0,resource=0,header=0,view=0;
    bool gpu=false;
    std::vector<CapeSourceVertex> vertices;
    std::vector<std::uint16_t> lookup,triangles;
    std::vector<std::uint32_t> properties,visible;
    std::vector<CapeSection> sections;
    std::vector<CapeTextureUnit> units;
    std::vector<bool> capeSections;
    std::vector<BagMatrix> bones;
    std::array<std::uintptr_t,6> storage{};
};
struct CapeEquipmentBounds {
    CapeMesh mesh;
    cape::BodyBounds fitted;
    std::vector<std::uint32_t> visibility;
};
struct CapeState {
    std::uintptr_t model=0,header=0,view=0;
    std::uint64_t guid=0;
    std::uint32_t updated=0;
    bool frameValid=false;
    std::vector<unsigned> sections,source,nodeForLookup;
    std::vector<cape::Triangle> triangles;
    std::vector<cape::Vec3> rest,animated,normals,reference;
    std::vector<std::uint32_t> pins;
    cape::BodyBounds bodyBounds;
    std::vector<std::uint32_t> boundsVisibility;
    std::unordered_map<std::uintptr_t,CapeEquipmentBounds> equipmentBounds;
    std::vector<cape::ColliderBox> bounds;
    std::vector<cape::ColliderTriangle> worldSurfaces;
    cape::Vec3 worldLow{},worldHigh{};
    cape::Vec3 backDirection{-1,0,0};
    std::uint32_t worldUpdated=0;
    bool worldValid=false;
    std::vector<unsigned char> gpuVertices,drawVertices;
    BagMatrix worldToRender{},renderToWorld{};
    CapeMesh mesh;
    cape::Cloth cloth;
};
static CapeState capeState;
static bool capeFinite(const cape::Vec3& p){return std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z);}
static cape::Vec3 capeTransform(const BagMatrix& m,const cape::Vec3& p,bool direction=false){
    return {m[0]*p.x+m[4]*p.y+m[8]*p.z+(direction?0:m[12]),
            m[1]*p.x+m[5]*p.y+m[9]*p.z+(direction?0:m[13]),
            m[2]*p.x+m[6]*p.y+m[10]*p.z+(direction?0:m[14])};
}
static cape::Vec3 capeNormal(cape::Vec3 p){
    const float l=std::sqrt(p.x*p.x+p.y*p.y+p.z*p.z);
    return std::isfinite(l)&&l>1e-8f?cape::Vec3{p.x/l,p.y/l,p.z/l}:cape::Vec3{0,0,1};
}
template<class T> static bool capeReadArray(std::uintptr_t address,unsigned count,unsigned limit,std::vector<T>& out){
    if(count>limit||(!address&&count))return false;
    out.resize(count);if(!count)return true;
    SIZE_T copied=0;
    return ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<const void*>(address),out.data(),count*sizeof(T),&copied)&&copied==count*sizeof(T);
}
template<class T> static bool capeArray(std::uintptr_t at,unsigned limit,std::vector<T>& out){
    unsigned count=0;std::uintptr_t ptr=0;
    return read(at,count)&&read(at+4,ptr)&&capeReadArray(ptr,count,limit,out);
}
static bool capeReadMesh(std::uintptr_t model,CapeMesh& m){
    m={};m.model=model;unsigned loaded=0,magic=0,version=0,flags=0,bones=0;std::uintptr_t config=0,palette=0,visibility=0;
    if(!read(model+0x10,loaded)||!loaded||!read(model+0x30,m.resource)||!m.resource||
       !read(m.resource+0x130,m.header)||!m.header||!read(m.resource+0x138,m.view)||!m.view||
       !read(m.resource+4,config)||!config||!read(config+4,flags)||
       !read(m.header,magic)||magic!=0x3032444d||!read(m.header+4,version)||version!=256||
       !capeArray(m.header+0x44,65536,m.vertices)||!capeArray(m.view,65536,m.lookup)||
       !capeArray(m.view+8,196608,m.triangles)||!capeArray(m.view+16,65536,m.properties)||
       !capeArray(m.view+24,512,m.sections)||!capeArray(m.view+32,2048,m.units)||
       !read(model+0x98,visibility)||!capeReadArray(visibility,static_cast<unsigned>(m.sections.size()),512,m.visible)||
       !read(m.header+0x34,bones)||!read(model+0x94,palette)||!capeReadArray(palette,bones,2048,m.bones))return false;
    m.gpu=(flags&8)!=0;
    if(m.vertices.empty()||m.lookup.empty()||m.sections.empty()||m.bones.empty())return false;
    std::vector<std::uint16_t> textureLookup;std::vector<std::array<unsigned,4>> textures;
    if(!capeArray(m.header+0x94,1024,textureLookup)||!capeArray(m.header+0x5c,1024,textures))return false;
    m.capeSections.assign(m.sections.size(),false);
    for(const auto& unit:m.units){
        const auto index=unit.value[2],first=unit.value[8],count=unit.value[7];
        if(index>=m.sections.size()||static_cast<unsigned>(first)+count>textureLookup.size())return false;
        const auto& section=m.sections[index];
        if(section.geoset<1500||section.geoset>=1600)continue;
        // Tauren tails also use the cloak geoset family. Only type 2 is the
        // cape texture; type 1 skin/type 8 fur must keep native animation.
        for(unsigned i=0;i<count;++i){
            const auto tex=textureLookup[first+i];if(tex>=textures.size())return false;
            if(textures[tex][0]==2)m.capeSections[index]=true;
        }
    }
    for(const auto& s:m.sections)if(static_cast<unsigned>(s.first)+s.count>m.lookup.size()||
       static_cast<unsigned>(s.triangleFirst)+s.triangleCount>m.triangles.size()||s.triangleCount%3)return false;
    if(!read(m.header+0x48,m.storage[0]))return false;
    for(unsigned i=0;i<5;++i)if(!read(m.view+4+8*i,m.storage[i+1]))return false;
    return true;
}
// Original geometry is immutable for a loaded resource/view. Only the bone
// palette and geoset visibility change with ordinary animation. Do not copy
// the entire character (and every equipped mesh) for each cape material pass.
static bool capeRefreshMesh(std::uintptr_t model,CapeMesh& mesh,bool& changed){
    std::uintptr_t resource=0,header=0,view=0,config=0,palette=0,visibility=0;
    unsigned loaded=0,flags=0;std::array<std::uintptr_t,6> storage{};
    if(!read(model+0x10,loaded)||!loaded||!read(model+0x30,resource)||!resource||
       !read(resource+0x130,header)||!header||!read(resource+0x138,view)||!view||
       !read(resource+4,config)||!config||!read(config+4,flags)||!read(header+0x48,storage[0]))return false;
    for(unsigned i=0;i<5;++i)if(!read(view+4+8*i,storage[i+1]))return false;
    changed=mesh.model!=model||mesh.resource!=resource||mesh.header!=header||mesh.view!=view||
        mesh.gpu!=((flags&8)!=0)||mesh.storage!=storage||mesh.vertices.empty();
    if(changed)return capeReadMesh(model,mesh);
    return read(model+0x98,visibility)&&capeReadArray(visibility,static_cast<unsigned>(mesh.sections.size()),512,mesh.visible)&&
        read(model+0x94,palette)&&capeReadArray(palette,static_cast<unsigned>(mesh.bones.size()),2048,mesh.bones);
}
static bool capeSkinMatrix(const CapeMesh& m,unsigned source,BagMatrix& matrix){
    if(source>=m.vertices.size())return false;
    const auto& v=m.vertices[source];unsigned total=0;matrix={};
    for(unsigned i=0;i<4;++i)if(v.weights[i]){
        if(v.bones[i]>=m.bones.size())return false;
        total+=v.weights[i];const float w=v.weights[i]/255.f;
        for(unsigned k=0;k<16;++k)matrix[k]+=m.bones[v.bones[i]][k]*w;
    }
    if(total!=255)return false;
    for(float value:matrix)if(!std::isfinite(value))return false;
    return true;
}
static unsigned capeSource(const CapeMesh& m,unsigned index){return m.gpu?m.lookup[index]:index;}
static bool capeDrawIsCape(std::uintptr_t model,const CapeTextureUnit& unit){
    std::uintptr_t resource=0,header=0,lookup=0,textures=0;unsigned lookupCount=0,textureCount=0;
    if(!read(model+0x30,resource)||!read(resource+0x130,header)||
       !read(header+0x94,lookupCount)||!read(header+0x98,lookup)||
       !read(header+0x5c,textureCount)||!read(header+0x60,textures)||
       static_cast<unsigned>(unit.value[8])+unit.value[7]>lookupCount)return false;
    for(unsigned i=0;i<unit.value[7];++i){std::uint16_t index=0;unsigned type=0;
        if(!read(lookup+2*(unit.value[8]+i),index)||index>=textureCount||!read(textures+16*index,type))return false;
        if(type==2)return true;
    }
    return false;
}
static bool capeBuildTopology(CapeState& s){
    s.source.clear();s.rest.clear();s.triangles.clear();s.pins.clear();
    s.nodeForLookup.assign(s.mesh.lookup.size(),std::numeric_limits<unsigned>::max());
    for(const auto sectionIndex:s.sections){
        const auto& section=s.mesh.sections[sectionIndex];
        for(unsigned i=section.first;i<static_cast<unsigned>(section.first)+section.count;++i){
            const unsigned source=capeSource(s.mesh,i);if(source>=s.mesh.vertices.size())return false;
            const auto& vertex=s.mesh.vertices[source];if(!capeFinite(vertex.position))return false;
            unsigned node=0;
            for(;node<s.rest.size();++node){
                const auto& other=s.mesh.vertices[s.source[node]];
                const float x=other.position.x-vertex.position.x,y=other.position.y-vertex.position.y,z=other.position.z-vertex.position.z;
                if(x*x+y*y+z*z<1e-10f&&other.bones==vertex.bones&&other.weights==vertex.weights)break;
            }
            if(node==s.rest.size()){s.rest.push_back(vertex.position);s.source.push_back(source);}
            s.nodeForLookup[i]=node;
        }
        for(unsigned i=section.triangleFirst;i<static_cast<unsigned>(section.triangleFirst)+section.triangleCount;i+=3){
            std::array<unsigned,3> n{};
            for(unsigned k=0;k<3;++k){
                const auto index=s.mesh.triangles[i+k];
                if(index<section.first||index>=static_cast<unsigned>(section.first)+section.count)return false;
                n[k]=s.nodeForLookup[index];
            }
            if(n[0]!=n[1]&&n[1]!=n[2]&&n[0]!=n[2])s.triangles.push_back({n[0],n[1],n[2]});
        }
    }
    if(s.rest.size()<4||s.rest.size()>2048||s.triangles.empty())return false;
    float low=s.rest[0].z,high=low;
    for(const auto& p:s.rest){low=std::min(low,p.z);high=std::max(high,p.z);}
    if(high-low<.05f)return false;
    // Fix the authored shoulder seam. No body/neck bones are changed.
    const float seam=high-std::min(.16f,(high-low)*.15f);
    std::vector<bool> pinned(s.rest.size(),false);
    for(unsigned i=0;i<s.rest.size();++i)if(s.rest[i].z>=seam)pinned[i]=true;
    for(const auto sectionIndex:s.sections){
        const auto& section=s.mesh.sections[sectionIndex];float a=high,b=low;
        for(unsigned i=section.first;i<static_cast<unsigned>(section.first)+section.count;++i){const float z=s.rest[s.nodeForLookup[i]].z;a=std::min(a,z);b=std::max(b,z);}
        // Separate small collar panels stay attached to the neckline.
        if(b-a<(high-low)*.12f)for(unsigned i=section.first;i<static_cast<unsigned>(section.first)+section.count;++i)pinned[s.nodeForLookup[i]]=true;
    }
    // Some authored collars have disconnected left/right panels below the
    // cape's highest point (notably female Tauren's shortest cloak). Every
    // connected piece needs its own attachment; otherwise it falls away.
    std::vector<unsigned> parent(s.rest.size());
    for(unsigned i=0;i<parent.size();++i)parent[i]=i;
    const auto root=[&](unsigned i){while(parent[i]!=i)i=parent[i];return i;};
    for(const auto& t:s.triangles){parent[root(t.a)]=root(t.b);parent[root(t.b)]=root(t.c);}
    for(unsigned component=0;component<parent.size();++component)if(root(component)==component){
        unsigned count=0;bool attached=false;float top=-std::numeric_limits<float>::max(),bottom=std::numeric_limits<float>::max();
        for(unsigned i=0;i<parent.size();++i)if(root(i)==component){++count;attached=attached||pinned[i];top=std::max(top,s.rest[i].z);bottom=std::min(bottom,s.rest[i].z);}
        if(attached)continue;
        const float edge=top-std::min(.16f,(top-bottom)*.15f);
        for(unsigned i=0;i<parent.size();++i)if(root(i)==component&&(count<=12||s.rest[i].z>=edge))pinned[i]=true;
    }
    for(unsigned i=0;i<pinned.size();++i)if(pinned[i])s.pins.push_back(i);
    return s.pins.size()>=2&&s.pins.size()<s.rest.size();
}
static bool capeAnimate(CapeState& s){
    s.animated.resize(s.source.size());
    for(unsigned i=0;i<s.source.size();++i){BagMatrix matrix;
        if(!capeSkinMatrix(s.mesh,s.source[i],matrix))return false;
        s.animated[i]=capeTransform(s.renderToWorld,capeTransform(matrix,s.mesh.vertices[s.source[i]].position));
        if(!capeFinite(s.animated[i]))return false;
    }
    return true;
}
static bool capeFitBounds(const CapeMesh& mesh,cape::BodyBounds& fitted,bool body){
    std::vector<cape::Triangle> triangles;
    for(unsigned index=0;index<mesh.sections.size();++index){
        if(!mesh.visible[index]||(body&&mesh.capeSections[index]))continue;
        const auto& section=mesh.sections[index];
        for(unsigned i=section.triangleFirst;i<static_cast<unsigned>(section.triangleFirst)+section.triangleCount;i+=3){
            cape::Triangle triangle;unsigned* ids[]={&triangle.a,&triangle.b,&triangle.c};
            for(unsigned k=0;k<3;++k){const unsigned lookup=mesh.triangles[i+k];if(lookup>=mesh.lookup.size())return false;*ids[k]=capeSource(mesh,lookup);}
            triangles.push_back(triangle);
        }
    }
    return fitted.fit(mesh.vertices,triangles,body?32:4);
}
static bool capeBoundsOverlap(const cape::ColliderBox& box,cape::Vec3 low,cape::Vec3 high){
    // A swept AABB only culls clearly unrelated volumes. Hard contact uses the
    // oriented box and whole cape triangles, not this broad-phase box.
    cape::Vec3 a{1.e30f,1.e30f,1.e30f},b{-1.e30f,-1.e30f,-1.e30f};
    for(unsigned old=0;old<2;++old){
        const auto center=old?box.previousCenter:box.currentCenter;
        const auto half=old?box.previousHalf:box.currentHalf;
        const auto* axes=old?box.previousAxes:box.currentAxes;
        cape::Vec3 extent{};
        for(unsigned k=0;k<3;++k){const float h=k==0?half.x:k==1?half.y:half.z;
            extent.x+=std::fabs(axes[k].x)*h;extent.y+=std::fabs(axes[k].y)*h;extent.z+=std::fabs(axes[k].z)*h;}
        const auto lo=center-extent,hi=center+extent;
        a={std::min(a.x,lo.x),std::min(a.y,lo.y),std::min(a.z,lo.z)};
        b={std::max(b.x,hi.x),std::max(b.y,hi.y),std::max(b.z,hi.z)};
    }
    return a.x<=high.x&&b.x>=low.x&&a.y<=high.y&&b.y>=low.y&&a.z<=high.z&&b.z>=low.z;
}
static bool capePoseBounds(CapeState& s,cape::BodyBounds& fitted,const CapeMesh& mesh,
                           cape::Vec3 low,cape::Vec3 high,bool reset,bool body){
    std::vector<cape::ColliderBox> boxes;
    if(!fitted.pose(mesh.bones,s.renderToWorld,boxes,reset))return false;
    for(unsigned index=0;index<boxes.size();++index){auto& box=boxes[index];
        // All contacts share a continuous rear direction; opposing local
        // box faces must not pump the cape between incompatible corrections.
        box.preferredDirection=s.backDirection;
        if(!body&&!capeBoundsOverlap(box,low,high))continue;
        if(s.bounds.size()>=64)return false;
        s.bounds.push_back(box);
    }
    return true;
}
static bool capeRefreshWorld(CapeState& s,cape::Vec3 low,cape::Vec3 high,std::uint32_t now){
    const bool inside=low.x>=s.worldLow.x&&low.y>=s.worldLow.y&&low.z>=s.worldLow.z&&
        high.x<=s.worldHigh.x&&high.y<=s.worldHigh.y&&high.z<=s.worldHigh.z;
    if(s.worldUpdated&&inside&&static_cast<std::uint32_t>(now-s.worldUpdated)<100)return s.worldValid;
    const cape::Vec3 margin{.5f,.5f,.5f};
    s.worldLow=low-margin;s.worldHigh=high+margin;s.worldUpdated=now;s.worldValid=false;
    s.worldSurfaces.clear();
    if(!capeWorldColliders(s.worldLow,s.worldHigh,s.worldSurfaces))return false;
    // Keep detailed world contact a small, independent budget. Body and gear
    // exclusion bounds are never dropped to satisfy this scenery budget.
    if(s.worldSurfaces.size()>128){
        const auto center=(low+high)*.5f;
        const auto distance=[&](const cape::ColliderTriangle& t){auto p=(t.current[0]+t.current[1]+t.current[2])/3.f-center;return cape::dot(p,p);};
        std::nth_element(s.worldSurfaces.begin(),s.worldSurfaces.begin()+128,s.worldSurfaces.end(),[&](const auto& a,const auto& b){return distance(a)<distance(b);});
        s.worldSurfaces.resize(128);
    }
    s.worldValid=true;return true;
}
static bool capeUpdate(CapeState& s,std::uintptr_t model,std::uint64_t guid){
    const auto now=bagClockMilliseconds();bool geometryChanged=true;
    if(!s.cloth.ready()){if(!capeReadMesh(model,s.mesh))return false;}
    else if(!capeRefreshMesh(model,s.mesh,geometryChanged))return false;
    std::vector<unsigned> sections;
    for(unsigned i=0;i<s.mesh.sections.size();++i)if(s.mesh.visible[i]&&s.mesh.capeSections[i])sections.push_back(i);
    if(sections.empty()){s={};capeRuntimeStatus=1;return false;}
    const bool rebuild=!s.cloth.ready()||geometryChanged||s.model!=model||s.guid!=guid||s.sections!=sections;
    if(rebuild){auto mesh=std::move(s.mesh);s={};s.mesh=std::move(mesh);s.model=model;s.guid=guid;s.header=s.mesh.header;s.view=s.mesh.view;s.sections=sections;}
    std::uintptr_t scene=0;BagMatrix modelToRender;
    if(!read(model+0x2c,scene)||!scene||!read(scene+0x9c,s.worldToRender)||!bagAffineInverse(s.worldToRender,s.renderToWorld)||!read(model+0xFC,modelToRender))return false;
    s.backDirection=capeNormal(capeTransform(s.renderToWorld,capeTransform(modelToRender,{-1,0,0},true),true));
    if(rebuild&&!capeBuildTopology(s))return false;
    if(!capeAnimate(s)||s.animated.empty())return false;
    s.reference.resize(s.source.size());
    for(unsigned i=0;i<s.source.size();++i)s.reference[i]=capeTransform(s.renderToWorld,capeTransform(modelToRender,s.mesh.vertices[s.source[i]].position));
    cape::Config config;
    config.fixedStep=1.f/60.f;config.maxSubsteps=3;config.iterations=4;
    config.maxContactSamples=0;config.selfCollision=false;config.maxColliderTriangles=128;
    config.damping=9.f;config.maxSpeed=3.f;config.stableBounds=true;config.poseLimit=.30f;
    config.maxCollisionTests=6000;config.maxColliderBoxes=64;config.maxBoundTests=100000;
    if(rebuild&&!s.cloth.initialize(s.animated,s.triangles,s.pins,config))return false;
    bool resetPose=false;
    if(!rebuild){
        resetPose=static_cast<std::uint32_t>(now-s.updated)*.001f>config.maxFrameTime;
        for(unsigned pin:s.pins)if(pin>=s.cloth.positions().size()||
            cape::length(s.animated[pin]-s.cloth.positions()[pin])>config.teleportDistance)resetPose=true;
        if(resetPose){s.cloth.reset(s.animated);s.worldValid=false;}
    }
    // Camera/palette refresh is still needed by each GPU material submission.
    // Reuse simulation and collision results for duplicate submissions.
    if(!rebuild&&!resetPose&&now==s.updated)return s.frameValid;
    s.frameValid=false;
    cape::Vec3 low=s.animated[0],high=low;
    const auto grow=[&](const auto& values){for(const auto& p:values){low.x=std::min(low.x,p.x);low.y=std::min(low.y,p.y);low.z=std::min(low.z,p.z);high.x=std::max(high.x,p.x);high.y=std::max(high.y,p.y);high.z=std::max(high.z,p.z);}};
    grow(s.animated);grow(s.cloth.positions());
    low-=cape::Vec3{.15f,.15f,.15f};high+=cape::Vec3{.15f,.15f,.15f};
    s.bounds.clear();
    if(!s.bodyBounds.ready()||s.boundsVisibility!=s.mesh.visible){
        if(!capeFitBounds(s.mesh,s.bodyBounds,true))return false;
        s.boundsVisibility=s.mesh.visible;
    }
    if(!capePoseBounds(s,s.bodyBounds,s.mesh,low,high,rebuild||resetPose,true))return false;
    std::uintptr_t child=0;std::array<std::uintptr_t,64> seen{};unsigned count=0;
    if(!read(model+0x1dc,child))return false;
    for(;child&&count<seen.size();++count){
        for(unsigned j=0;j<count;++j)if(seen[j]==child)return false;
        seen[count]=child;std::uintptr_t parent=0,following=0;unsigned point=0;
        if(!read(child+0x1cc,parent)||parent!=model||!read(child+0x1e4,following)||!read(child+0x1d0,point))return false;
        if(point<=33){
            unsigned loaded=0;if(!read(child+0x10,loaded))return false;
            if(loaded){auto& entry=s.equipmentBounds[child];bool changed=false;
                if(!capeRefreshMesh(child,entry.mesh,changed))return false;
                if(changed||!entry.fitted.ready()||entry.visibility!=entry.mesh.visible){
                    if(!capeFitBounds(entry.mesh,entry.fitted,false))return false;
                    entry.visibility=entry.mesh.visible;
                }
                if(!capePoseBounds(s,entry.fitted,entry.mesh,low,high,changed||resetPose,false))return false;
            }
        }
        child=following;
    }
    if(child)return false;
    for(auto it=s.equipmentBounds.begin();it!=s.equipmentBounds.end();){
        if(std::find(seen.begin(),seen.begin()+count,it->first)==seen.begin()+count)it=s.equipmentBounds.erase(it);else ++it;
    }
    // Scenery queries can be unavailable while cells load. Solid body/gear
    // limits stay active independently of that optional detailed contact.
    if(!capeRefreshWorld(s,low,high,now))s.worldSurfaces.clear();
    const float elapsed=rebuild||resetPose?0.f:static_cast<std::uint32_t>(now-s.updated)*.001f;s.updated=now;
    if(!s.cloth.step(elapsed,s.animated,s.worldSurfaces,s.bounds,s.reference)){
        s.cloth.reset(s.animated);
        if(!s.cloth.step(0,s.animated,{},s.bounds,s.reference))return false;
    }
    s.normals.assign(s.source.size(),{});const auto& positions=s.cloth.positions();
    for(const auto& t:s.triangles){
        const auto n=cape::cross(positions[t.b]-positions[t.a],positions[t.c]-positions[t.a]);
        for(unsigned i:{t.a,t.b,t.c})s.normals[i]+=n;
    }
    for(auto& normal:s.normals)normal=capeNormal(normal);
    s.frameValid=true;return true;
}
static void capeReset(){
    for(auto& storage:capeVertexStorage)capeReleaseStorage(storage);
    capeState={};capeClearWorldCollision();capeLastDraw=0;capePlayerModel=0;capePlayerChecked=0;capeRuntimeStatus=capeEnabled?1:0;
}
static void capeSetEnabled(bool enabled){if(capeEnabled!=enabled){capeEnabled=enabled;capeReset();}}
static unsigned capeStatus(){
    if(capeRuntimeStatus==2&&static_cast<std::uint32_t>(bagClockMilliseconds()-capeLastDraw)>250)capeRuntimeStatus=1;
    return capeRuntimeStatus;
}
static void capeForgetModel(std::uintptr_t model){
    if(capeState.model==model){capeReset();return;}
    capeState.equipmentBounds.erase(model);
    if(capePlayerModel==model){capePlayerModel=0;capePlayerChecked=0;}
}
static void __fastcall capeBindHook(void* buffer,unsigned format){
    capeBindOriginal(buffer,format);capeBoundBuffer=buffer;capeBoundFormat=format;
}
struct CapeDrawSpan {unsigned section=0,output=0;};
static bool capeDrawSpans(const CapeMesh& mesh,const CapeTextureUnit& unit,const CapeSection& section,
                          std::uintptr_t unitAddress,std::uintptr_t sectionAddress,unsigned optimized,
                          std::vector<CapeDrawSpan>& spans){
    spans.clear();unsigned firstUnit=0,lastUnit=0;
    if(optimized){
        // 707A1E/707C8C: batch+34 is model+404 != 0. 711230 replaces
        // section.geoset and unit.section with a compact group index. The
        // inclusive range at model+3FC refers to ORIGINAL texture units.
        std::uintptr_t units=0,sections=0,ranges=0;unsigned unitCount=0,sectionCount=0;
        const unsigned group=unit.value[2];
        if(optimized!=1||section.geoset!=group||!read(mesh.model+0x3ec,units)||!units||
           !read(mesh.model+0x3f0,unitCount)||unitCount>2048||group>=unitCount||
           !read(mesh.model+0x3f4,sections)||!sections||!read(mesh.model+0x3f8,sectionCount)||
           sectionCount>2048||group>=sectionCount||unitAddress!=units+24*group||sectionAddress!=sections+32*group||
           !read(mesh.model+0x3fc,ranges)||!ranges||!read(ranges+8*group,firstUnit)||
           !read(ranges+8*group+4,lastUnit)||firstUnit>lastUnit||lastUnit>=mesh.units.size())return false;
    }else{
        const unsigned index=unit.value[2];
        if(index>=mesh.sections.size()||!mesh.visible[index]||!mesh.capeSections[index])return false;
        const auto& source=mesh.sections[index];
        if(section.geoset!=source.geoset||section.first!=source.first||section.count!=source.count||
           section.triangleCount!=source.triangleCount)return false;
        spans.push_back({index,0});return source.count!=0;
    }
    unsigned vertices=0,triangles=0,low=65536,high=0;
    for(unsigned i=firstUnit;i<=lastUnit;++i){
        const auto& sourceUnit=mesh.units[i];const unsigned index=sourceUnit.value[2];
        if(index>=mesh.sections.size())return false;
        if(!mesh.visible[index])continue;
        const auto& source=mesh.sections[index];
        // A group containing other visible geometry cannot be replaced by a
        // cape-only buffer. Hidden units inside the range are skipped exactly
        // as 719930 (indices) and 719B20 (CPU vertices) skip them.
        if(!mesh.capeSections[index]||!capeDrawIsCape(mesh.model,sourceUnit)||!source.count)return false;
        spans.push_back({index,vertices});vertices+=source.count;triangles+=source.triangleCount;
        low=std::min(low,static_cast<unsigned>(source.first));
        high=std::max(high,static_cast<unsigned>(source.first)+source.count);
    }
    return !spans.empty()&&triangles==section.triangleCount&&
           (mesh.gpu?(section.first==low&&section.count==high-low):(section.first==0&&section.count==vertices));
}
static bool capeWriteDraw(std::uintptr_t renderer,const void* description,unsigned indexed){
    std::uintptr_t model=0,batch=0,drawSection=0,unit=0;unsigned gpu=0,merged=0;
    if(!read(renderer+0x3310,model)||!read(renderer+0x3300,batch)||!batch||
       !read(batch+0x30,drawSection)||!drawSection||!read(batch+0x2c,unit)||!unit||
       !read(batch+0x34,merged)||!read(renderer+0x32f0,gpu))return false;
    CapeTextureUnit textureUnit;CapeSection section;
    if(!read(unit,textureUnit)||!read(drawSection,section))return false;
    if((!merged&&(section.geoset<1500||section.geoset>=1600))||!capeDrawIsCape(model,textureUnit))return false;
    Player player;if(!snapshot(player)||player.model!=model||!player.guid)return false;
    capeRuntimeStatus=3;
    if(indexed!=1||!capeBoundBuffer)return false;
    if(!capeUpdate(capeState,model,player.guid)){
        // Never replace a rejected solid-bound frame with the unbounded
        // native pose. Keep other draws intact and retry on the next frame.
        if(capeState.cloth.stats().boundsRejected)return true;
        return false;
    }
    auto& s=capeState;std::vector<CapeDrawSpan> spans;
    if(!capeDrawSpans(s.mesh,textureUnit,section,unit,drawSection,merged,spans)||!section.count||section.count>2048)return false;
    const unsigned stride=gpu?48:capeBoundFormat==5?40:32;
    unsigned vertices=section.count;
    if(gpu){unsigned copies=0;if(capeBoundFormat!=12||!s.mesh.gpu||!read(s.mesh.resource+0x15c,copies)||copies!=1||s.mesh.properties.size()!=s.mesh.lookup.size())return false;vertices=static_cast<unsigned>(s.mesh.lookup.size());}
    else if(s.mesh.gpu||(capeBoundFormat!=3&&capeBoundFormat!=5))return false;
    auto& data=gpu?s.gpuVertices:s.drawVertices;
    const bool prepare=data.size()!=static_cast<std::size_t>(vertices)*stride;
    data.resize(static_cast<std::size_t>(vertices)*stride);
    if(gpu&&prepare){
        for(unsigned i=0;i<vertices;++i){const unsigned source=s.mesh.lookup[i];if(source>=s.mesh.vertices.size())return false;
            auto vertex=s.mesh.vertices[source];std::memcpy(vertex.bones.data(),&s.mesh.properties[i],4);std::memcpy(data.data()+48*i,&vertex,48);}
    }else if(!gpu){
        CapeSkin skin=nullptr;
        if(stride==40)skin=capeFunction<CapeSkin>(0x71A460);
        else {
            std::uintptr_t dispatch=0;
            if(!read(0xCF04C8,dispatch)||(dispatch!=0x71A720&&dispatch!=0x71A9E0))return false;
            skin=capeFunction<CapeSkin>(dispatch);
        }
        std::uintptr_t sourceSections=0;if(!read(s.mesh.view+28,sourceSections)||!sourceSections)return false;
        // The optimized CPU index buffer concatenates each original section
        // after subtracting its authored first vertex. Skin the same ordered
        // spans so UVs and every node use that exact rebased index mapping.
        for(const auto& span:spans)skin(reinterpret_cast<void*>(model),reinterpret_cast<void*>(sourceSections+32*span.section),data.data()+stride*span.output);
    }
    for(const auto& span:spans){const auto& authored=s.mesh.sections[span.section];
      for(unsigned offset=0;offset<authored.count;++offset){const unsigned i=authored.first+offset;
        if(i>=s.nodeForLookup.size())return false;
        const unsigned node=s.nodeForLookup[i];
        if(node>=s.cloth.positions().size())return false;
        cape::Vec3 position=capeTransform(s.worldToRender,s.cloth.positions()[node]);
        cape::Vec3 normal=capeNormal(capeTransform(s.worldToRender,s.normals[node],true));
        if(gpu){BagMatrix skin,inverse;
            if(!capeSkinMatrix(s.mesh,s.mesh.lookup[i],skin)||!bagAffineInverse(skin,inverse))return false;
            position=capeTransform(inverse,position);normal=capeNormal(capeTransform(inverse,normal,true));
        }
        if(!capeFinite(position)||!capeFinite(normal))return false;
        unsigned char* output=data.data()+stride*(gpu?i:span.output+offset);
        std::memcpy(output,&position,12);std::memcpy(output+(gpu?20:12),&normal,12);
      }
    }
    void* buffer=capePrivateBuffer(stride,std::max(vertices,static_cast<unsigned>(s.mesh.lookup.size())));if(!buffer)return false;
    void* mapped=capeMap(buffer);if(!mapped)return false;
    std::memcpy(mapped,data.data(),data.size());capeUnmap(buffer,0);
    capeBindOriginal(buffer,capeBoundFormat);
    capeSubmitOriginal(description,indexed);
    capeBindOriginal(capeBoundBuffer,capeBoundFormat);
    capeRuntimeStatus=2;capeLastDraw=bagClockMilliseconds();return true;
}
static void __fastcall capeSubmitHook(const void* description,unsigned indexed){
    if(capeEnabled&&capeDrawScope){
        if(capeWriteDraw(capeDrawScope,description,indexed))return;
    }
    capeSubmitOriginal(description,indexed);
}
static void __fastcall capeDrawHook(void* renderer,void*){
    const auto previous=capeDrawScope;capeDrawScope=0;
    if(capeEnabled){
        std::uintptr_t model=0;const auto now=bagClockMilliseconds();
        // Crowds submit many material batches. Resolve the local player only
        // once per short interval; actual cape draws revalidate the full ID.
        if(!capePlayerChecked||static_cast<std::uint32_t>(now-capePlayerChecked)>=16){
            Player player;capePlayerModel=snapshot(player)?player.model:0;capePlayerChecked=now;
        }
        if(read(reinterpret_cast<std::uintptr_t>(renderer)+0x3310,model)&&model&&model==capePlayerModel){
            capeDrawScope=reinterpret_cast<std::uintptr_t>(renderer);
        }
    }
    capeDrawOriginal(renderer);capeDrawScope=previous;
}
