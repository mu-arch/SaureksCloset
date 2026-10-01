#pragma once
#include "CapeCloth.h"
#include <array>
#include <memory>
namespace cape {
struct CollisionVertex {Vec3 position;std::array<std::uint8_t,4> weights,bones;};
struct CollisionMesh {std::vector<CollisionVertex> vertices;std::vector<Triangle> triangles;};
struct AnimatedCollider {
    std::shared_ptr<const CollisionMesh> mesh;
    std::vector<std::array<float,16>> bones;
    std::array<float,16> renderToWorld{};
};
inline Vec3 collisionTransform(const std::array<float,16>& m,Vec3 p){return {m[0]*p.x+m[4]*p.y+m[8]*p.z+m[12],m[1]*p.x+m[5]*p.y+m[9]*p.z+m[13],m[2]*p.x+m[6]*p.y+m[10]*p.z+m[14]};}
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
    if(side>.06f||side<-.06f)return {}; // No distant/open-surface plane extrusion.
    // A nearby collision may correct at most 1 cm on acquisition. Subsequent
    // solves follow the actual surface, with no native-animation target.
    if(side<-.007f)point+=normal*(side+.007f);
    return {point-normal*.12f,.123f};
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
    if(result.size()>512){const Vec3 center=(low+high)*.5f;const auto distance=[&](const ColliderTriangle& t){Vec3 d=(t.current[0]+t.current[1]+t.current[2])/3.f-center;return dot(d,d);};
        std::nth_element(result.begin(),result.begin()+512,result.end(),[&](const auto& a,const auto& b){return distance(a)<distance(b);});result.resize(512);}
    return result;
}
}
