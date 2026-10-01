#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <limits>
#include <unordered_map>
#include <vector>
#include "CapeCloth.h"

// Build 5875 only. Included after WeaponRenderer.h and CapeWorldCollision.h.
// The source MD20 and its GPU buffer are shared. Only a temporary draw buffer
// is changed; the exact native binding is restored before returning to CM2.
using CapeBatchDraw=void (__thiscall *)(void*);
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
};
struct CapeState {
    std::uintptr_t model=0,header=0,view=0;
    std::uint64_t guid=0;
    std::uint32_t updated=0;
    bool frameValid=false;
    std::vector<unsigned> sections,source,nodeForLookup;
    std::vector<cape::Triangle> triangles;
    std::vector<cape::Vec3> rest,animated,normals;
    std::vector<std::uint32_t> pins;
    std::unordered_map<std::uint64_t,std::array<cape::Vec3,3>> previousSurfaces;
    std::unordered_map<std::uintptr_t,std::pair<std::uintptr_t,std::uintptr_t>> surfaceIdentities;
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
    return true;
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
static bool capeOverlap(const std::array<cape::Vec3,3>& triangle,const cape::Vec3& low,const cape::Vec3& high){
    for(unsigned axis=0;axis<3;++axis){
        const auto get=[axis](const cape::Vec3& p){return axis==0?p.x:axis==1?p.y:p.z;};
        float a=get(triangle[0]),b=a;for(unsigned i=1;i<3;++i){a=std::min(a,get(triangle[i]));b=std::max(b,get(triangle[i]));}
        if(b<get(low)||a>get(high))return false;
    }
    return true;
}
static bool capeSweptOverlap(const std::array<cape::Vec3,3>& before,const std::array<cape::Vec3,3>& after,
                             const cape::Vec3& low,const cape::Vec3& high){
    std::array<cape::Vec3,3> bounds{};bounds[0]=bounds[1]=before[0];
    for(const auto* triangle:{&before,&after})for(const auto& p:*triangle){
        bounds[0].x=std::min(bounds[0].x,p.x);bounds[0].y=std::min(bounds[0].y,p.y);bounds[0].z=std::min(bounds[0].z,p.z);
        bounds[1].x=std::max(bounds[1].x,p.x);bounds[1].y=std::max(bounds[1].y,p.y);bounds[1].z=std::max(bounds[1].z,p.z);
    }
    bounds[2]=bounds[0];return capeOverlap(bounds,low,high);
}
static bool capeCollectSurface(CapeState& s,const CapeMesh& mesh,const cape::Vec3& low,const cape::Vec3& high,
                               std::vector<cape::ColliderTriangle>& colliders,
                               std::unordered_map<std::uint64_t,std::array<cape::Vec3,3>>& next){
    std::vector<cape::Vec3> points(mesh.lookup.size());std::vector<bool> sampled(mesh.lookup.size(),false);
    const auto identity=std::make_pair(mesh.header,mesh.view);
    const auto oldIdentity=s.surfaceIdentities.find(mesh.model);
    const bool sameMesh=oldIdentity!=s.surfaceIdentities.end()&&oldIdentity->second==identity;
    if(oldIdentity==s.surfaceIdentities.end()&&s.surfaceIdentities.size()>=65)return false;
    if(!sameMesh)for(auto it=s.previousSurfaces.begin();it!=s.previousSurfaces.end();){
        if((it->first>>32)==mesh.model)it=s.previousSurfaces.erase(it);else ++it;
    }
    s.surfaceIdentities[mesh.model]=identity;
    for(unsigned sectionIndex=0;sectionIndex<mesh.sections.size();++sectionIndex){
        if(!mesh.visible[sectionIndex]||(mesh.model==s.model&&mesh.capeSections[sectionIndex]))continue;
        const auto& section=mesh.sections[sectionIndex];
        for(unsigned i=section.triangleFirst;i<static_cast<unsigned>(section.triangleFirst)+section.triangleCount;i+=3){
            std::array<cape::Vec3,3> t{};
            for(unsigned k=0;k<3;++k){
                const unsigned at=mesh.triangles[i+k];if(at>=mesh.lookup.size())return false;
                if(!sampled[at]){const unsigned source=capeSource(mesh,at);BagMatrix matrix;
                    if(!capeSkinMatrix(mesh,source,matrix))return false;
                    points[at]=capeTransform(s.renderToWorld,capeTransform(matrix,mesh.vertices[source].position));
                    if(!capeFinite(points[at]))return false;
                    sampled[at]=true;
                }
                t[k]=points[at];
            }
            const std::uint64_t key=(static_cast<std::uint64_t>(mesh.model)<<32)|i;
            const auto previous=s.previousSurfaces.find(key);
            const auto& before=sameMesh&&previous!=s.previousSurfaces.end()?previous->second:t;
            // Retain bounded history even when the final triangle is outside
            // the cape. A fast weapon can cross it and end on the other side.
            if(next.size()>=65536)return false;
            next.emplace(key,t);
            if(!capeSweptOverlap(before,t,low,high))continue;
            if(colliders.size()>=8192)return false;
            cape::ColliderTriangle collider;
            for(unsigned k=0;k<3;++k){collider.current[k]=t[k];collider.previous[k]=before[k];}
            colliders.push_back(collider);
        }
    }
    return true;
}
static bool capeUpdate(CapeState& s,std::uintptr_t model,std::uint64_t guid){
    const auto now=bagClockMilliseconds();
    CapeMesh mesh;if(!capeReadMesh(model,mesh))return false;
    std::vector<unsigned> sections;
    for(unsigned i=0;i<mesh.sections.size();++i)if(mesh.visible[i]&&mesh.capeSections[i])sections.push_back(i);
    if(sections.empty()){s={};capeRuntimeStatus=1;return false;}
    const bool rebuild=!s.cloth.ready()||s.model!=model||s.guid!=guid||s.header!=mesh.header||s.view!=mesh.view||s.sections!=sections||s.mesh.gpu!=mesh.gpu;
    if(rebuild){s={};s.model=model;s.guid=guid;s.header=mesh.header;s.view=mesh.view;s.sections=sections;}
    s.mesh=std::move(mesh);
    std::uintptr_t scene=0;
    if(!read(model+0x2c,scene)||!scene||!read(scene+0x9c,s.worldToRender)||!bagAffineInverse(s.worldToRender,s.renderToWorld))return false;
    if(rebuild&&!capeBuildTopology(s))return false;
    if(!capeAnimate(s))return false;
    if(s.animated.empty())return false;
    if(rebuild&&!s.cloth.initialize(s.animated,s.triangles,s.pins))return false;
    bool resetPose=false;
    if(!rebuild){
        const cape::Config limits;
        resetPose=static_cast<std::uint32_t>(now-s.updated)*.001f>limits.maxFrameTime;
        for(unsigned pin:s.pins)if(pin>=s.cloth.positions().size()||
            cape::length(s.animated[pin]-s.cloth.positions()[pin])>limits.teleportDistance)resetPose=true;
        if(resetPose){s.cloth.reset(s.animated);s.previousSurfaces.clear();s.surfaceIdentities.clear();}
    }
    // The pose/camera are refreshed for every submission. Multiple material
    // passes at the same native clock tick reuse just the simulation result.
    if(!rebuild&&!resetPose&&now==s.updated)return s.frameValid;
    s.frameValid=false;
    cape::Vec3 low=s.animated[0],high=low;
    const auto bounds=[&](const auto& values){for(const auto& p:values){low.x=std::min(low.x,p.x);low.y=std::min(low.y,p.y);low.z=std::min(low.z,p.z);high.x=std::max(high.x,p.x);high.y=std::max(high.y,p.y);high.z=std::max(high.z,p.z);}};
    bounds(s.animated);bounds(s.cloth.positions());
    low.x-=.35f;low.y-=.35f;low.z-=.35f;high.x+=.35f;high.y+=.35f;high.z+=.35f;
    std::vector<cape::ColliderTriangle> colliders;
    std::unordered_map<std::uint64_t,std::array<cape::Vec3,3>> next;
    if(!capeCollectSurface(s,s.mesh,low,high,colliders,next))return false;
    std::uintptr_t child=0;std::array<std::uintptr_t,64> seen{};
    if(!read(model+0x1dc,child))return false;
    for(unsigned i=0;child&&i<seen.size();++i){
        for(unsigned j=0;j<i;++j)if(seen[j]==child)return false;
        seen[i]=child;std::uintptr_t parent=0,following=0;unsigned point=0;
        if(!read(child+0x1cc,parent)||parent!=model||!read(child+0x1e4,following)||!read(child+0x1d0,point))return false;
        // Body/equipment attachment points, including owned bags. Spell emitters
        // and unrelated scene models are not character collision surfaces.
        if(point<=33){
            unsigned loaded=0;if(!read(child+0x10,loaded))return false;
            if(loaded){CapeMesh equipment;
                // Missing geometry on a loaded equipment child is a missing
                // contact surface. Leave the original cape for that frame.
                if(!capeReadMesh(child,equipment)||!capeCollectSurface(s,equipment,low,high,colliders,next))return false;
            }
        }
        child=following;
    }
    if(child||!capeWorldColliders(low,high,colliders)||colliders.size()>8192)return false;
    s.previousSurfaces=std::move(next);
    const float elapsed=rebuild||resetPose?0.f:static_cast<std::uint32_t>(now-s.updated)*.001f;s.updated=now;
    if(!s.cloth.step(elapsed,s.animated,colliders))return false;
    s.normals.assign(s.source.size(),{});const auto& positions=s.cloth.positions();
    for(const auto& t:s.triangles){
        const auto& a=positions[t.a];const auto& b=positions[t.b];const auto& c=positions[t.c];
        const cape::Vec3 ab{b.x-a.x,b.y-a.y,b.z-a.z},ac{c.x-a.x,c.y-a.y,c.z-a.z};
        const cape::Vec3 n{ab.y*ac.z-ab.z*ac.y,ab.z*ac.x-ab.x*ac.z,ab.x*ac.y-ab.y*ac.x};
        for(unsigned i:{t.a,t.b,t.c}){s.normals[i].x+=n.x;s.normals[i].y+=n.y;s.normals[i].z+=n.z;}
    }
    for(auto& normal:s.normals)normal=capeNormal(normal);
    s.frameValid=true;
    return true;
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
    capeState.surfaceIdentities.erase(model);
    for(auto it=capeState.previousSurfaces.begin();it!=capeState.previousSurfaces.end();){
        if((it->first>>32)==model)it=capeState.previousSurfaces.erase(it);else ++it;
    }
    if(capePlayerModel==model){capePlayerModel=0;capePlayerChecked=0;}
}
static void __fastcall capeBindHook(void* buffer,unsigned format){
    capeBindOriginal(buffer,format);capeBoundBuffer=buffer;capeBoundFormat=format;
}
static bool capeWriteDraw(std::uintptr_t renderer,const void* description,unsigned count){
    std::uintptr_t model=0,batch=0,drawSection=0,unit=0;unsigned gpu=0,merged=0;
    if(!read(renderer+0x3310,model)||!read(renderer+0x3300,batch)||!batch||
       !read(batch+0x30,drawSection)||!drawSection||!read(batch+0x2c,unit)||!unit||
       !read(batch+0x34,merged)||!read(renderer+0x32f0,gpu))return false;
    Player player;if(!snapshot(player)||player.model!=model||!player.guid)return false;
    CapeTextureUnit textureUnit;CapeSection section;
    if(!read(unit,textureUnit)||!read(drawSection,section))return false;
    if(section.geoset<1500||section.geoset>=1600||!capeDrawIsCape(model,textureUnit))return false;
    capeRuntimeStatus=3;
    if(count!=1||!capeBoundBuffer||merged)return false;
    if(!capeUpdate(capeState,model,player.guid))return false;
    auto& s=capeState;const unsigned sectionIndex=textureUnit.value[2];
    if(sectionIndex>=s.mesh.sections.size()||!s.mesh.capeSections[sectionIndex]||!s.mesh.visible[sectionIndex])return false;
    const auto& authored=s.mesh.sections[sectionIndex];
    if(section.first!=authored.first||section.count!=authored.count||!section.count||section.count>2048)return false;
    const unsigned stride=gpu?48:capeBoundFormat==5?40:32;
    unsigned vertices=section.count;
    if(gpu){unsigned copies=0;if(capeBoundFormat!=12||!s.mesh.gpu||!read(s.mesh.resource+0x15c,copies)||copies!=1||s.mesh.properties.size()!=s.mesh.lookup.size())return false;vertices=static_cast<unsigned>(s.mesh.lookup.size());}
    else if(s.mesh.gpu||(capeBoundFormat!=3&&capeBoundFormat!=5))return false;
    std::vector<unsigned char> data(static_cast<std::size_t>(vertices)*stride);
    if(gpu){
        for(unsigned i=0;i<vertices;++i){const unsigned source=s.mesh.lookup[i];if(source>=s.mesh.vertices.size())return false;
            auto vertex=s.mesh.vertices[source];std::memcpy(vertex.bones.data(),&s.mesh.properties[i],4);std::memcpy(data.data()+48*i,&vertex,48);}
    }else{
        CapeSkin skin=nullptr;
        if(stride==40)skin=capeFunction<CapeSkin>(0x71A460);
        else {
            std::uintptr_t dispatch=0;
            if(!read(0xCF04C8,dispatch)||(dispatch!=0x71A720&&dispatch!=0x71A9E0))return false;
            skin=capeFunction<CapeSkin>(dispatch);
        }
        skin(reinterpret_cast<void*>(model),reinterpret_cast<void*>(drawSection),data.data());
    }
    for(unsigned i=section.first;i<static_cast<unsigned>(section.first)+section.count;++i){
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
        unsigned char* output=data.data()+stride*(gpu?i:i-section.first);
        std::memcpy(output,&position,12);std::memcpy(output+(gpu?20:12),&normal,12);
    }
    void* buffer=capePrivateBuffer(stride,static_cast<unsigned>(s.mesh.lookup.size()));if(!buffer)return false;
    void* mapped=capeMap(buffer);if(!mapped)return false;
    std::memcpy(mapped,data.data(),data.size());capeUnmap(buffer,0);
    capeBindOriginal(buffer,capeBoundFormat);
    capeSubmitOriginal(description,count);
    capeBindOriginal(capeBoundBuffer,capeBoundFormat);
    capeRuntimeStatus=2;capeLastDraw=bagClockMilliseconds();return true;
}
static void __fastcall capeSubmitHook(const void* description,unsigned count){
    if(capeEnabled&&capeDrawScope){
        if(capeWriteDraw(capeDrawScope,description,count))return;
    }
    capeSubmitOriginal(description,count);
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
