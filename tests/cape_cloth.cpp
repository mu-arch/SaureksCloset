#include "../native/CapeCloth.h"
#include <cassert>
#include <cstdio>
#include <limits>

using cape::Vec3;
using cape::Triangle;
struct Mesh {std::vector<Vec3> vertices;std::vector<Triangle> triangles;std::vector<std::uint32_t> pins;};
static Mesh grid(unsigned nx=7,unsigned ny=10,float z=1.f){
    Mesh mesh;
    for(unsigned y=0;y<ny;++y)for(unsigned x=0;x<nx;++x)mesh.vertices.push_back({(float(x)/(nx-1)-.5f)*.6f,float(y)/(ny-1)*.9f,z});
    for(unsigned y=0;y+1<ny;++y)for(unsigned x=0;x+1<nx;++x){const unsigned a=y*nx+x,b=a+1,c=a+nx,d=c+1;mesh.triangles.push_back({a,c,b});mesh.triangles.push_back({b,c,d});}
    for(unsigned x=0;x<nx;++x)mesh.pins.push_back(x);
    return mesh;
}
static std::vector<cape::ColliderTriangle> plane(float oldZ,float newZ,float radius=5){
    const Vec3 a{-radius,-radius,oldZ},b{radius,-radius,oldZ},c{radius,radius,oldZ},d{-radius,radius,oldZ};
    const Vec3 aa{-radius,-radius,newZ},bb{radius,-radius,newZ},cc{radius,radius,newZ},dd{-radius,radius,newZ};
    return {{{a,b,c},{aa,bb,cc}},{{a,c,d},{aa,cc,dd}}};
}
static float maxEdgeError(const Mesh& mesh,const cape::Cloth& cloth){
    float error=0;
    for(const auto t:mesh.triangles){const unsigned ids[3]={t.a,t.b,t.c};for(unsigned i=0;i<3;++i){const unsigned a=ids[i],b=ids[(i+1)%3];const float rest=cape::length(mesh.vertices[a]-mesh.vertices[b]);error=std::max(error,std::fabs(cape::length(cloth.positions()[a]-cloth.positions()[b])-rest)/rest);}}
    return error;
}
static void hangingAndTopology(){
    const auto mesh=grid();cape::Cloth cloth;assert(cloth.initialize(mesh.vertices,mesh.triangles,mesh.pins));
    for(unsigned i=0;i<600;++i)assert(cloth.step(1.f/120,mesh.vertices,{}));
    for(auto pin:mesh.pins)assert(cape::length(cloth.positions()[pin]-mesh.vertices[pin])<1.e-6f);
    assert(cloth.positions().size()==mesh.vertices.size());
    assert(cloth.positions().back().z<.3f);
    const float edgeError=maxEdgeError(mesh,cloth);std::printf("hanging maximum edge strain %.5f\n",edgeError);assert(edgeError<.08f);
    for(const auto t:mesh.triangles){const float rest=cape::length(cape::cross(mesh.vertices[t.b]-mesh.vertices[t.a],mesh.vertices[t.c]-mesh.vertices[t.a]));const float area=cape::length(cape::cross(cloth.positions()[t.b]-cloth.positions()[t.a],cloth.positions()[t.c]-cloth.positions()[t.a]));assert(area/rest>.8f&&area/rest<1.2f);}
}
static void movingPinsAndInertia(){
    auto mesh=grid();cape::Cloth cloth;assert(cloth.initialize(mesh.vertices,mesh.triangles,mesh.pins));
    for(unsigned i=0;i<120;++i)assert(cloth.step(1.f/120,mesh.vertices,{}));
    const auto before=cloth.positions();
    for(auto& p:mesh.vertices)p.x+=.06f;
    assert(cloth.step(1.f/120,mesh.vertices,{}));
    for(auto pin:mesh.pins)assert(cape::length(cloth.positions()[pin]-mesh.vertices[pin])<1.e-6f);
    // Moving the mount must leave real inertia in the free tail.
    const float tailTravel=cloth.positions().back().x-before.back().x;
    assert(tailTravel<.045f);
    for(auto& p:mesh.vertices)p.x+=.001f;
    assert(cloth.step(1.f/480,mesh.vertices,{}));
    assert(cloth.stats().substeps==0);
    for(auto pin:mesh.pins)assert(cape::length(cloth.positions()[pin]-mesh.vertices[pin])<1.e-6f);
}
static std::vector<Vec3> simulate(unsigned fps){
    const auto mesh=grid();cape::Config config;config.selfCollision=false;cape::Cloth cloth;assert(cloth.initialize(mesh.vertices,mesh.triangles,mesh.pins,config));
    for(unsigned frame=1;frame<=fps*2;++frame){auto pose=mesh.vertices;const float time=float(frame)/fps;for(auto& p:pose){p.x+=time*.25f;p.z+=time*.05f;}assert(cloth.step(1.f/fps,pose,{}));}
    return cloth.positions();
}
static void frameRates(){
    const auto a=simulate(30),b=simulate(60),c=simulate(120),d=simulate(240);float ab=0,ac=0,ad=0;
    for(unsigned i=0;i<a.size();++i){ab=std::max(ab,cape::length(a[i]-b[i]));ac=std::max(ac,cape::length(a[i]-c[i]));ad=std::max(ad,cape::length(a[i]-d[i]));}
    std::printf("30/60/120/240 fps deviations %.7f %.7f %.7f\n",ab,ac,ad);assert(ab<.003f&&ac<.003f&&ad<.008f);
}
static void staticCollision(){
    const auto mesh=grid(7,10,.6f);cape::Cloth cloth;assert(cloth.initialize(mesh.vertices,mesh.triangles,mesh.pins));const auto floor=plane(0,0);
    unsigned contacts=0;
    for(unsigned frame=0;frame<300;++frame){assert(cloth.step(1.f/120,mesh.vertices,floor));contacts+=cloth.stats().contacts;for(auto p:cloth.positions())assert(p.z>=.014f);}
    assert(contacts>0);
}
static void sweptAndTwoSidedCollision(){
    const std::vector<Vec3> positions={{3,3,.15f},{.05f,.1f,.15f},{-.05f,.1f,.15f}};
    const std::vector<Triangle> triangles={{0,1,2}};const std::vector<std::uint32_t> pins={0};
    cape::Config config;config.fixedStep=1.f/60;config.gravity={0,0,-2000};config.damping=0;config.stretchCompliance=config.areaCompliance=config.bendCompliance=1.e10f;config.selfCollision=false;
    cape::Cloth cloth;assert(cloth.initialize(positions,triangles,pins,config));assert(cloth.step(1.f/60,positions,plane(0,0)));assert(cloth.stats().contacts>0);assert(cloth.positions()[1].z>=config.thickness-.0001f);
    auto below=positions;for(auto& p:below)p.z=-.15f;config.gravity={0,0,2000};assert(cloth.initialize(below,triangles,pins,config));assert(cloth.step(1.f/60,below,plane(0,0)));assert(cloth.positions()[1].z<=-config.thickness+.0001f);
    // A finite triangle is not an infinite collision plane.
    assert(cloth.initialize(positions,triangles,pins,config));config.gravity={0,0,-2000};assert(cloth.initialize(positions,triangles,pins,config));auto away=plane(0,0,.01f);assert(cloth.step(1.f/60,positions,away));assert(cloth.positions()[1].z<0);
}
static void movingCollider(){
    const std::vector<Vec3> positions={{3,3,0},{.05f,.1f,0},{-.05f,.1f,0}};cape::Config config;config.gravity={};config.damping=0;config.stretchCompliance=config.areaCompliance=config.bendCompliance=1.e10f;config.selfCollision=false;
    cape::Cloth cloth;assert(cloth.initialize(positions,{{0,1,2}},{0},config));
    assert(cloth.step(1.f/120,positions,plane(-.5f,.5f)));assert(cloth.stats().contacts>0);assert(cloth.positions()[1].z>=.5f+config.thickness-.0001f);
}
static cape::Config unconstrained(){
    cape::Config config;config.gravity={};config.damping=0;config.stretchCompliance=config.areaCompliance=config.bendCompliance=1.e10f;config.selfCollision=false;return config;
}
static void sparseEdgeCollision(){
    const std::vector<Vec3> positions={{3,3,.15f},{-.2f,0,.15f},{.2f,0,.15f}};
    auto config=unconstrained();config.gravity={0,0,-8000};
    cape::Cloth cloth;assert(cloth.initialize(positions,{{0,1,2}},{0},config));assert(cloth.collisionSampleCount()>positions.size());
    // Neither free vertex is over this narrow obstacle; its actual surface
    // meets the middle of the original cape edge instead.
    assert(cloth.step(1.f/120,positions,plane(0,0,.025f)));
    assert((cloth.positions()[1].z+cloth.positions()[2].z)*.5f>=config.thickness-.0001f);
}
static void sparseFaceCollision(){
    const std::vector<Vec3> positions={{0,-.3f,.15f},{-.2f,.1f,.15f},{.2f,.1f,.15f}};
    auto config=unconstrained();config.gravity={0,0,-8000};
    cape::Cloth cloth;assert(cloth.initialize(positions,{{0,1,2}},{0},config));
    // This obstacle is inside the face, away from every original edge.
    assert(cloth.step(1.f/120,positions,plane(0,0,.012f)));
    const float sampleHeight=cloth.positions()[0].z*.25f+(cloth.positions()[1].z+cloth.positions()[2].z)*.375f;
    assert(sampleHeight>=config.thickness-.0001f);
    config.maxContactSamples=0;assert(cloth.initialize(positions,{{0,1,2}},{0},config));assert(cloth.collisionSampleCount()==positions.size());assert(cloth.step(1.f/120,positions,plane(0,0,.012f)));
    assert(cloth.positions()[1].z<0); // Establish that particles alone miss it.
}
static void wallFloorCorner(){
    const std::vector<Vec3> positions={{3,3,.15f},{.15f,.1f,.15f},{.2f,-.1f,.15f}};
    auto config=unconstrained();config.gravity={-8000,0,-8000};
    cape::Cloth cloth;assert(cloth.initialize(positions,{{0,1,2}},{0},config));
    auto surfaces=plane(0,0);auto wall=surfaces;
    for(auto& triangle:wall)for(unsigned i=0;i<3;++i){std::swap(triangle.previous[i].x,triangle.previous[i].z);std::swap(triangle.current[i].x,triangle.current[i].z);}
    surfaces.insert(surfaces.end(),wall.begin(),wall.end());
    for(unsigned frame=0;frame<20;++frame){assert(cloth.step(1.f/120,positions,surfaces));for(unsigned i=1;i<3;++i){assert(cloth.positions()[i].z>=config.thickness-.0001f);assert(cloth.positions()[i].x>=config.thickness-.0001f);}}
}
static void rotatingTriangle(){
    const std::vector<Vec3> positions={{3,3,.2f},{-.2f,.1f,.2f},{-.2f,-.1f,.2f}};
    cape::Cloth cloth;const auto config=unconstrained();assert(cloth.initialize(positions,{{0,1,2}},{0},config));
    const cape::ColliderTriangle triangle{{{-2,-2,0},{2,-2,0},{0,2,0}},{{0,-2,2},{0,-2,-2},{0,2,0}}};
    assert(cloth.step(1.f/120,positions,{triangle}));assert(cloth.stats().contacts>0);assert(cloth.positions()[1].x>=config.thickness-.0001f);
}
static void substepColliderAndDegeneracy(){
    const std::vector<Vec3> positions={{3,3,0},{.05f,.1f,0},{-.05f,.1f,0}};
    auto config=unconstrained();cape::Cloth cloth;assert(cloth.initialize(positions,{{0,1,2}},{0},config));
    assert(cloth.step(1.f/480,positions,plane(-.5f,.5f)));assert(cloth.stats().substeps==0);assert(cloth.stats().contacts>0);assert(cloth.positions()[1].z>=.5f+config.thickness-.0001f);
    // Collapsed animation triangles are finite edges, never a 0/0 normal.
    assert(cloth.initialize(positions,{{0,1,2}},{0},config));
    const cape::ColliderTriangle line{{{-.1f,.1f,0},{-.1f,.1f,0},{.1f,.1f,0}},{{-.1f,.1f,.1f},{-.1f,.1f,.1f},{.1f,.1f,.1f}}};
    assert(cloth.step(1.f/120,positions,{line}));for(auto p:cloth.positions())assert(cape::finite(p));
}
static void validationAndReset(){
    auto mesh=grid();cape::Cloth cloth;assert(!cloth.initialize(mesh.vertices,{{0,0,2}},mesh.pins));assert(!cloth.initialize(mesh.vertices,mesh.triangles,{9999}));assert(cloth.initialize(mesh.vertices,mesh.triangles,mesh.pins));
    auto invalid=mesh.vertices;invalid[9].x=std::numeric_limits<float>::quiet_NaN();assert(!cloth.step(.01f,invalid,{}));assert(cloth.stats().invalidInput&&cloth.stats().reset);for(auto p:cloth.positions())assert(cape::finite(p));
    for(auto& p:mesh.vertices)p.x+=10;
    assert(cloth.step(.01f,mesh.vertices,{}));assert(cloth.stats().reset&&cloth.stats().substeps==0);for(unsigned i=0;i<mesh.vertices.size();++i)assert(cape::length(cloth.positions()[i]-mesh.vertices[i])<1.e-6f);
    assert(cloth.step(1,mesh.vertices,{}));assert(cloth.stats().reset);
    cape::Config config;config.maxCollisionTests=1;assert(cloth.initialize(mesh.vertices,mesh.triangles,mesh.pins,config));auto surfaces=plane(1,1);for(auto& t:surfaces)for(auto& p:t.previous)p.x+=10;for(auto& t:surfaces)for(auto& p:t.current)p.x+=10;
    assert(!cloth.step(1.f/60,mesh.vertices,surfaces));assert(cloth.stats().budgetExceeded&&cloth.stats().reset);assert(cloth.stats().collisionTests<=1);
    config.maxColliderTriangles=1;assert(cloth.initialize(mesh.vertices,mesh.triangles,mesh.pins,config));assert(!cloth.step(.01f,mesh.vertices,surfaces));assert(cloth.stats().budgetExceeded);
}
int main(){hangingAndTopology();movingPinsAndInertia();frameRates();staticCollision();sweptAndTwoSidedCollision();movingCollider();sparseEdgeCollision();sparseFaceCollision();wallFloorCorner();rotatingTriangle();substepColliderAndDegeneracy();validationAndReset();std::puts("cape cloth tests passed");}
