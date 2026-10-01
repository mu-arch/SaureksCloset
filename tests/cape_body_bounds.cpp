#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <vector>
#include "../native/CapeBodyBounds.h"
using cape::Vec3;
using Matrix=std::array<float,16>;
struct Vertex {Vec3 position;std::array<std::uint8_t,4> bones{},weights{};};
static const Matrix identity{{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1}};
static Vec3 transform(const Matrix& m,Vec3 p){return {m[0]*p.x+m[4]*p.y+m[8]*p.z+m[12],m[1]*p.x+m[5]*p.y+m[9]*p.z+m[13],m[2]*p.x+m[6]*p.y+m[10]*p.z+m[14]};}
static Vec3 skin(const Vertex& vertex,const std::vector<Matrix>& bones,const Matrix& world){Vec3 p{};for(unsigned i=0;i<4;++i)if(vertex.weights[i])p+=transform(bones[vertex.bones[i]],vertex.position)*(vertex.weights[i]/255.f);return transform(world,p);}
static bool contains(const cape::ColliderBox& box,Vec3 p){const Vec3 d=p-box.currentCenter;const float h[]={box.currentHalf.x,box.currentHalf.y,box.currentHalf.z};for(unsigned i=0;i<3;++i)if(std::fabs(cape::dot(d,box.currentAxes[i]))>h[i]+.0002f)return false;return true;}
static void checkTriangles(const std::vector<Vertex>& vertices,const std::vector<cape::Triangle>& triangles,const std::vector<Matrix>& bones,const Matrix& world,const std::vector<cape::ColliderBox>& boxes){
    for(const auto t:triangles){const Vec3 p[]={skin(vertices[t.a],bones,world),skin(vertices[t.b],bones,world),skin(vertices[t.c],bones,world)};bool enclosed=false;
        for(const auto& box:boxes)if(contains(box,p[0])&&contains(box,p[1])&&contains(box,p[2])){enclosed=true;for(unsigned a=0;a<=8;++a)for(unsigned b=0;b<=8-a;++b)assert(contains(box,p[0]*(a/8.f)+p[1]*(b/8.f)+p[2]*((8-a-b)/8.f)));break;}
        assert(enclosed); // whole joint face in a SINGLE convex bound, no gaps
    }
}
int main(){
    std::vector<Vertex> vertices;std::vector<cape::Triangle> triangles;
    for(unsigned i=0;i<12;++i){const float x=(i%3)*.2f,z=(i/3)*.3f;Vertex vertex;vertex.position={x,(i%2)*.15f,z};vertex.bones={static_cast<std::uint8_t>(i/3),static_cast<std::uint8_t>((i/3+1)%4),0,0};vertex.weights={static_cast<std::uint8_t>(150+i*7),static_cast<std::uint8_t>(105-i*7),0,0};vertices.push_back(vertex);}
    for(unsigned i=0;i<10;++i)triangles.push_back({i,i+1,i+2});
    cape::BodyBounds bounds;assert(bounds.fit(vertices,triangles,3));assert(bounds.count()<=3&&bounds.influenceCount()<=12);
    std::vector<Matrix> bones(4,identity);std::vector<cape::ColliderBox> boxes;
    for(unsigned pose=0;pose<180;++pose){
        for(unsigned bone=0;bone<bones.size();++bone){const float angle=(pose*.033f)*(bone+1),s=std::sin(angle),c=std::cos(angle);bones[bone]=identity;bones[bone][0]=c;bones[bone][1]=s;bones[bone][4]=-s;bones[bone][5]=c;bones[bone][8]=.08f*s;bones[bone][10]=1+.1f*c;bones[bone][12]=.4f*s;bones[bone][13]=.2f*c;bones[bone][14]=.1f*bone;}
        Matrix world=identity;world[12]=17;world[13]=-31;world[14]=6;assert(bounds.pose(bones,world,boxes));checkTriangles(vertices,triangles,bones,world,boxes);
        for(const auto& box:boxes){for(unsigned k=0;k<3;++k)assert(std::fabs(cape::length(box.currentAxes[k])-1)<.0001f);assert(std::fabs(cape::dot(box.currentAxes[0],box.currentAxes[1]))<.0001f);}
    }
    const auto before=boxes;assert(bounds.pose(bones,identity,boxes));for(unsigned i=0;i<boxes.size();++i)assert(cape::length(boxes[i].previousCenter-before[i].currentCenter)<.0001f);
    bounds.clearHistory();assert(bounds.pose(bones,identity,boxes));for(const auto& box:boxes)assert(cape::length(box.previousCenter-box.currentCenter)<.0001f);
    // Rigid boxes remain tight, rather than acquiring the large rounded caps
    // of a capsule that would put an artificial hump through the shoulder seam.
    std::vector<Vertex> cube;for(unsigned i=0;i<8;++i){Vertex v;v.position={(i&1)?.2f:-.2f,(i&2)?.3f:-.3f,(i&4)?.5f:-.5f};v.weights[0]=255;cube.push_back(v);}
    const std::vector<cape::Triangle> faces{{0,1,2},{1,2,3},{4,5,6},{5,6,7},{0,2,4},{2,4,6},{1,3,5},{3,5,7}};
    assert(bounds.fit(cube,faces));assert(bounds.pose(std::vector<Matrix>{identity},identity,boxes));assert(boxes.size()==1);
    assert(std::fabs(boxes[0].currentHalf.x-.203f)<.00001f&&std::fabs(boxes[0].currentHalf.y-.303f)<.00001f&&std::fabs(boxes[0].currentHalf.z-.503f)<.00001f);
    checkTriangles(cube,faces,std::vector<Matrix>{identity},identity,boxes);
    // Invalid meshes/poses fail closed; old bounds cannot be reported valid.
    auto bad=cube;bad[0].weights[0]=254;assert(!bounds.fit(bad,faces));assert(!bounds.pose(bones,identity,boxes)&&boxes.empty());assert(!bounds.fit(cube,faces,33));
    assert(bounds.fit(cube,faces));auto broken=identity;broken[0]=NAN;assert(!bounds.pose(std::vector<Matrix>{broken},identity,boxes)&&boxes.empty());
    // Duplicate bone entries still describe one combined influence.
    for(auto& v:cube){v.weights={128,127,0,0};v.bones={0,0,0,0};}
    assert(bounds.fit(cube,faces));assert(bounds.influenceCount()==1);assert(bounds.pose(std::vector<Matrix>{identity},identity,boxes));checkTriangles(cube,faces,std::vector<Matrix>{identity},identity,boxes);
    std::cout<<"cape mesh-derived body bounds passed: weighted joint faces, merged groups, poses, tight rigid bounds, invalid input\n";
}
