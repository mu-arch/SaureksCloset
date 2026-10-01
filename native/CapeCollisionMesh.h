#pragma once
#include "CapeCloth.h"
#include <array>
#include <memory>
namespace cape {
struct CollisionVertex {Vec3 position;std::array<std::uint8_t,4> weights,bones;};
struct CollisionMesh {
    std::vector<CollisionVertex> vertices;std::vector<Triangle> triangles;
    bool body=false;
    std::vector<std::vector<unsigned>> tailGroups;
};
struct AnimatedCollider {
    std::shared_ptr<const CollisionMesh> mesh;
    std::vector<std::array<float,16>> bones;
    std::array<float,16> renderToWorld{};
};
inline Vec3 collisionTransform(const std::array<float,16>& m,Vec3 p){return {m[0]*p.x+m[4]*p.y+m[8]*p.z+m[12],m[1]*p.x+m[5]*p.y+m[9]*p.z+m[13],m[2]*p.x+m[6]*p.y+m[10]*p.z+m[14]};}
inline std::array<float,16> capeCompose(const std::array<float,16>& a,const std::array<float,16>& b){
    std::array<float,16> out{};for(unsigned c=0;c<4;++c)for(unsigned r=0;r<4;++r)for(unsigned k=0;k<4;++k)out[c*4+r]+=a[k*4+r]*b[c*4+k];return out;
}
inline std::vector<AnimatedCollider> interpolateCapeColliders(const std::vector<AnimatedCollider>& prior,const std::vector<AnimatedCollider>& current,float alpha){
    auto out=current;
    for(auto& entry:out){const AnimatedCollider* old=nullptr;
        for(const auto& candidate:prior)if(candidate.mesh==entry.mesh&&candidate.bones.size()==entry.bones.size()){old=&candidate;break;}
        for(unsigned i=0;i<entry.bones.size();++i){auto world=capeCompose(entry.renderToWorld,entry.bones[i]);
            if(old){const auto before=capeCompose(old->renderToWorld,old->bones[i]);for(unsigned k=0;k<16;++k)world[k]=before[k]+alpha*(world[k]-before[k]);}
            entry.bones[i]=world;
        }
        entry.renderToWorld={{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1}};
    }
    return out;
}
inline Vec3 capeClosestPoint(Vec3 p,Vec3 a,Vec3 b,Vec3 c){
    const Vec3 ab=b-a,ac=c-a,ap=p-a;
    const float d1=dot(ab,ap),d2=dot(ac,ap);if(d1<=0&&d2<=0)return a;
    const Vec3 bp=p-b;const float d3=dot(ab,bp),d4=dot(ac,bp);if(d3>=0&&d4<=d3)return b;
    const float vc=d1*d4-d3*d2;if(vc<=0&&d1>=0&&d3<=0)return a+ab*(d1/(d1-d3));
    const Vec3 cp=p-c;const float d5=dot(ab,cp),d6=dot(ac,cp);if(d6>=0&&d5<=d6)return c;
    const float vb=d5*d2-d1*d6;if(vb<=0&&d2>=0&&d6<=0)return a+ac*(d2/(d2-d6));
    const float va=d3*d6-d5*d4;if(va<=0&&d4-d3>=0&&d5-d6>=0)return b+(c-b)*((d4-d3)/(d4-d3+d5-d6));
    return a+ab*(vb/(va+vb+vc))+ac*(vc/(va+vb+vc));
}
struct CapeContact {Vec3 center{};float radius=0;};
// A finite local backstop for each cloth particle. Raw NvCloth triangle
// colliders extrapolate the nearest triangle's plane outside its edges; open
// character meshes then launch cloth away from unrelated body faces.
inline CapeContact capeMeshContact(Vec3 p,const std::vector<ColliderTriangle>& triangles){
    float nearest=.15f*.15f;Vec3 point{},normal{};bool found=false;
    for(const auto& triangle:triangles){const auto* t=triangle.current;
        Vec3 q=capeClosestPoint(p,t[0],t[1],t[2]);float d=dot(p-q,p-q);
        if(d<nearest){nearest=d;point=q;normal=normalized(cross(t[1]-t[0],t[2]-t[0]),{0,0,1});found=true;}
    }
    if(!found)return {};
    const float side=dot(p-point,normal);
    if(side>.06f)return {}; // No distant/open-surface plane extrusion.
    // A nearby collision corrects penetration gradually on acquisition. Subsequent
    // solves follow the actual surface, with no native-animation target.
    if(side<-.02f)point+=normal*(side+.02f);
    return {point-normal*.20f,.208f};
}
struct CapeCapsule {Vec3 a,b;float radius=0;};
inline std::vector<CapeCapsule> capeTailCapsules(const std::vector<AnimatedCollider>& meshes){
    std::vector<CapeCapsule> out;
    for(const auto& entry:meshes){if(!entry.mesh)continue;
        for(const auto& group:entry.mesh->tailGroups){std::vector<Vec3> points;
            for(auto id:group){if(id>=entry.mesh->vertices.size())continue;const auto& v=entry.mesh->vertices[id];Vec3 p{};bool okay=true;
                for(unsigned k=0;k<4;++k)if(v.weights[k]){if(v.bones[k]>=entry.bones.size()){okay=false;break;}p+=collisionTransform(entry.bones[v.bones[k]],v.position)*(v.weights[k]/255.f);}
                p=collisionTransform(entry.renderToWorld,p);if(okay&&finite(p))points.push_back(p);
            }
            if(points.size()<3||group.empty()||group[0]>=entry.mesh->vertices.size())continue;
            Vec3 center{};for(auto p:points)center+=p;center=center/float(points.size());
            // Principal axis of this small, skinned tail segment. A covariance
            // fit follows its bend without snapping between a box's axes.
            Vec3 lo{1.e30f,1.e30f,1.e30f},hi{-1.e30f,-1.e30f,-1.e30f};
            for(auto id:group)if(id<entry.mesh->vertices.size()){auto p=entry.mesh->vertices[id].position;
                lo={std::min(lo.x,p.x),std::min(lo.y,p.y),std::min(lo.z,p.z)};hi={std::max(hi.x,p.x),std::max(hi.y,p.y),std::max(hi.z,p.z)};}
            Vec3 span=hi-lo;const Vec3 authored=span.x>=span.y&&span.x>=span.z?Vec3{1,0,0}:span.y>=span.z?Vec3{0,1,0}:Vec3{0,0,1};
            const auto& first=entry.mesh->vertices[group[0]];unsigned influence=0;for(unsigned k=1;k<4;++k)if(first.weights[k]>first.weights[influence])influence=k;
            if(first.bones[influence]>=entry.bones.size())continue;
            const auto matrix=capeCompose(entry.renderToWorld,entry.bones[first.bones[influence]]);
            const Vec3 guide=normalized(collisionTransform(matrix,authored)-collisionTransform(matrix,{}),authored);
            Vec3 axis=guide;
            for(unsigned n=0;n<8;++n){Vec3 next{};for(auto p:points){const auto d=p-center;next+=d*dot(d,axis);}axis=normalized(next,axis);}
            if(dot(axis,guide)<0)axis=-axis; // Stable endpoint identity for swept contact.
            float low=0,high=0,radius=0;for(auto p:points){auto d=p-center;float along=dot(d,axis);low=std::min(low,along);high=std::max(high,along);radius=std::max(radius,length(d-axis*along));}
            if(high-low<.001f){low-=.0005f;high+=.0005f;}
            out.push_back({center+axis*low,center+axis*high,radius+.012f});if(out.size()==16)return out;
        }
    }
    return out;
}
// Runs on the worker from immutable geometry and copied bone matrices.
inline std::vector<ColliderTriangle> skinCapeContacts(const std::vector<AnimatedCollider>& meshes,const std::vector<Vec3>& cloth){
    std::vector<ColliderTriangle> result;if(cloth.empty())return result;
    Vec3 low=cloth[0],high=low;
    for(auto p:cloth){low={std::min(low.x,p.x),std::min(low.y,p.y),std::min(low.z,p.z)};high={std::max(high.x,p.x),std::max(high.y,p.y),std::max(high.z,p.z)};}
    low-=Vec3{.25f,.25f,.25f};high+=Vec3{.25f,.25f,.25f};
    for(const auto& entry:meshes){
        if(!entry.mesh||entry.mesh->vertices.size()>16384)continue;
        std::vector<Vec3> vertices;vertices.reserve(entry.mesh->vertices.size());bool okay=true;
        for(const auto& vertex:entry.mesh->vertices){Vec3 p{};unsigned weight=0;
            for(unsigned k=0;k<4;++k)if(vertex.weights[k]){if(vertex.bones[k]>=entry.bones.size()){okay=false;break;}
                p+=collisionTransform(entry.bones[vertex.bones[k]],vertex.position)*(vertex.weights[k]/255.f);weight+=vertex.weights[k];}
            p=collisionTransform(entry.renderToWorld,p);if(weight!=255||!finite(p)){okay=false;break;}vertices.push_back(p);
        }
        if(!okay)continue;
        for(auto face:entry.mesh->triangles){if(face.a>=vertices.size()||face.b>=vertices.size()||face.c>=vertices.size())continue;
            Vec3 a=vertices[face.a],b=vertices[face.b],c=vertices[face.c];
            if(std::max({a.x,b.x,c.x})<low.x||std::min({a.x,b.x,c.x})>high.x||std::max({a.y,b.y,c.y})<low.y||std::min({a.y,b.y,c.y})>high.y||std::max({a.z,b.z,c.z})<low.z||std::min({a.z,b.z,c.z})>high.z)continue;
            if(length(cross(b-a,c-a))<1.e-7f)continue;
            result.push_back({{a,b,c},{a,b,c}});
        }
    }
    if(result.size()>1024){const Vec3 center=(low+high)*.5f;const auto distance=[&](const ColliderTriangle& t){Vec3 d=(t.current[0]+t.current[1]+t.current[2])/3.f-center;return dot(d,d);};
        std::nth_element(result.begin(),result.begin()+1024,result.end(),[&](const auto& a,const auto& b){return distance(a)<distance(b);});result.resize(1024);}
    return result;
}
}
