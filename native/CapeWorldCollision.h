#pragma once
#include "CapeCloth.h"
#include <cstddef>

namespace cape {
// Build 5875's collision query returns a plane followed by three WORLD-space
// vertices, not vertex pointers. Keep the exact layout separate from physics.
struct WorldFace { Vec3 normal; float distance; Vec3 vertices[3]; };
static_assert(sizeof(WorldFace)==0x34,"client collision face layout");
static_assert(offsetof(WorldFace,vertices)==0x10,"client triangle offset");
inline bool validWorldBounds(Vec3 minimum,Vec3 maximum){
    if(!finite(minimum)||!finite(maximum))return false;
    const Vec3 extent=maximum-minimum;
    return extent.x>=0&&extent.y>=0&&extent.z>=0&&
        extent.x<=16&&extent.y<=16&&extent.z<=16&&
        std::fabs(minimum.x)<20000&&std::fabs(minimum.y)<20000&&std::fabs(minimum.z)<20000&&
        std::fabs(maximum.x)<20000&&std::fabs(maximum.y)<20000&&std::fabs(maximum.z)<20000;
}
inline bool appendWorldFaces(const WorldFace* faces,std::size_t count,
                             std::vector<ColliderTriangle>& output,std::size_t limit=8192){
    if((count&&!faces)||output.size()>limit||count>limit-output.size())return false;
    const auto before=output.size();
    for(std::size_t i=0;i<count;++i){
        const auto& p=faces[i].vertices;
        if(!finite(p[0])||!finite(p[1])||!finite(p[2])){output.resize(before);return false;}
        // Degenerate collision polygons cannot constrain a cloth surface.
        const Vec3 n=cross(p[1]-p[0],p[2]-p[0]);
        if(dot(n,n)<1.e-14f)continue;
        ColliderTriangle t;
        for(unsigned j=0;j<3;++j)t.previous[j]=t.current[j]=p[j];
        output.push_back(t);
    }
    return true;
}
}

#ifdef _WIN32
namespace {
struct CapeWorldArray {std::uint32_t capacity=0,count=0;std::uintptr_t data=0;std::uint32_t chunk=0;};
struct CapeWorldQuery {CapeWorldArray faces,owners;};
static_assert(sizeof(CapeWorldArray)==16,"32-bit client collision array");
static CapeWorldQuery capeWorldQuery;
static bool capeWorldQueryBusy=false;
static void capeClearWorldCollision(){
    // These allocations belong to the client's heap, never the DLL's CRT.
    using Free=void(__stdcall*)(void*,const char*,int,unsigned);
    auto release=reinterpret_cast<Free>(0x646430);
    if(capeWorldQuery.faces.data)release(reinterpret_cast<void*>(capeWorldQuery.faces.data),"SaureksCloset cape",-2,0);
    if(capeWorldQuery.owners.data)release(reinterpret_cast<void*>(capeWorldQuery.owners.data),"SaureksCloset cape",-2,0);
    capeWorldQuery={};
}
static bool capeWorldColliders(const cape::Vec3& minimum,const cape::Vec3& maximum,
                              std::vector<cape::ColliderTriangle>& output){
    if(capeWorldQueryBusy||!cape::validWorldBounds(minimum,maximum))return false;
    struct Bounds {cape::Vec3 minimum,maximum;} bounds{minimum,maximum};
    using Query=bool(__fastcall*)(const Bounds*,CapeWorldQuery*,unsigned,void*);
    // The ordinary walking query mask from 0x6315F0: terrain, WMO and M2.
    // Do not set 0x4000 (walkable-normal filter), nor the liquid flags.
    capeWorldQueryBusy=true;
    const bool available=reinterpret_cast<Query>(0x6721B0)(&bounds,&capeWorldQuery,0x100111,nullptr);
    capeWorldQueryBusy=false;
    const auto& faces=capeWorldQuery.faces;
    if(!available||faces.count>faces.capacity||faces.count>8192||
       (faces.count&&!faces.data))return false;
    return cape::appendWorldFaces(reinterpret_cast<const cape::WorldFace*>(faces.data),faces.count,output);
}
}
#endif
