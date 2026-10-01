#include "../native/CapeCloth.h"
#include <cassert>
#include <cstdio>
#include <limits>
#include <chrono>

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
static cape::ColliderBox solidBox(Vec3 center={},Vec3 half={.1f,.1f,.1f}){
    cape::ColliderBox box;box.previousCenter=box.currentCenter=center;box.previousHalf=box.currentHalf=half;return box;
}
static void assertFacesClear(const cape::Cloth& cloth,const std::vector<Triangle>& faces,const cape::ColliderBox& box){
    // Independent dense barycentric check also inspects edges/interiors, not
    // just particles. Solver production validation uses exact triangle SAT.
    const float thickness=cloth.config().thickness-.00001f;
    for(const auto face:faces)for(unsigned u=0;u<=32;++u)for(unsigned v=0;u+v<=32;++v){const float a=float(u)/32,b=float(v)/32;const Vec3 point=cloth.positions()[face.a]*a+cloth.positions()[face.b]*b+cloth.positions()[face.c]*(1-a-b)-box.currentCenter;const bool inside=std::fabs(cape::dot(point,box.currentAxes[0]))<box.currentHalf.x+thickness&&std::fabs(cape::dot(point,box.currentAxes[1]))<box.currentHalf.y+thickness&&std::fabs(cape::dot(point,box.currentAxes[2]))<box.currentHalf.z+thickness;assert(!inside);}
}
static void wholeFaceBoundsAndPinnedSeam(){
    const std::vector<Vec3> pose={{-1,-1,0},{1,-1,0},{0,1,0}};const std::vector<Triangle> faces={{0,1,2}};
    auto config=unconstrained();config.maxContactSamples=0;cape::Cloth cloth;assert(cloth.initialize(pose,faces,{0},config));
    auto box=solidBox();box.preferredDirection={0,0,1};assert(cloth.step(0,pose,{}, {box}));
    assert(cloth.stats().substeps==0&&cloth.stats().contacts>0);assertFacesClear(cloth,faces,box);
    for(const auto p:cloth.positions())assert(p.z>=.115f);
    // Even an authored seam inside the bound cannot override final clearance.
    assert(cloth.positions()[0].z>pose[0].z);
    for(unsigned i=0;i<20;++i){assert(cloth.step(1.f/60,pose,{}, {box}));assertFacesClear(cloth,faces,box);}
}
static void movingAndRotatingSolidBounds(){
    const std::vector<Vec3> pose={{-.5f,-.5f,0},{.5f,-.5f,0},{0,.5f,0}};const std::vector<Triangle> faces={{0,1,2}};
    auto config=unconstrained();config.maxContactSamples=0;cape::Cloth cloth;assert(cloth.initialize(pose,faces,{0},config));
    auto box=solidBox({0,0,.5f});box.previousCenter.z=-.5f;
    // The small bound passes through the triangle interior, away from every
    // original vertex. A sub-tick display frame still retains the entry side.
    assert(cloth.step(1.f/480,pose,{}, {box}));assert(cloth.stats().substeps==0);assertFacesClear(cloth,faces,box);
    for(const auto p:cloth.positions())assert(p.z>=.615f);
    assert(cloth.initialize(pose,faces,{0},config));box=solidBox({}, {.5f,.2f,.03f});box.currentAxes[0]={0,0,1};box.currentAxes[2]={-1,0,0};box.preferredDirection={1,0,0};
    assert(cloth.step(1.f/60,pose,{}, {box}));assertFacesClear(cloth,faces,box);
}
static void solidBoundsResetValidationAndBudget(){
    const std::vector<Vec3> pose={{-.5f,-.5f,0},{.5f,-.5f,0},{0,.5f,0}};const std::vector<Triangle> faces={{0,1,2}};auto box=solidBox();box.preferredDirection={0,0,1};
    auto config=unconstrained();config.fixedStep=1.f/60;config.maxSubsteps=4;cape::Cloth cloth;assert(cloth.initialize(pose,faces,{0},config));
    assert(cloth.step(3,pose,{}, {box}));assert(cloth.stats().reset);assertFacesClear(cloth,faces,box);
    assert(cloth.step(.1f,pose,{}, {box}));assert(cloth.stats().reset&&cloth.stats().budgetExceeded);assertFacesClear(cloth,faces,box);
    auto overlapping=box;overlapping.currentCenter.x=overlapping.previousCenter.x=.1f;assert(cloth.step(0,pose,{}, {box,overlapping}));assertFacesClear(cloth,faces,box);assertFacesClear(cloth,faces,overlapping);
    auto invalid=box;invalid.currentAxes[1]=invalid.currentAxes[0];assert(!cloth.step(0,pose,{}, {invalid}));assert(cloth.stats().invalidInput);
    config.maxCollisionTests=0;assert(cloth.initialize(pose,faces,{0},config));assert(cloth.step(1.f/60,pose,plane(0,0),{box}));assert(cloth.stats().reset&&cloth.stats().budgetExceeded&&!cloth.stats().boundsRejected);assertFacesClear(cloth,faces,box);
    config.maxColliderTriangles=0;assert(cloth.initialize(pose,faces,{0},config));assert(cloth.step(1.f/60,pose,plane(0,0),{box}));assert(cloth.stats().reset&&cloth.stats().budgetExceeded);assertFacesClear(cloth,faces,box);
    config.maxColliderTriangles=8192;config.maxCollisionTests=400000;assert(cloth.initialize(pose,faces,{0},config));auto conflict=solidBox({0,0,.1f},{.2f,.2f,.2f});conflict.preferredDirection={0,0,-1};auto deep=solidBox({},{.2f,.2f,.2f});deep.preferredDirection={0,0,1};assert(!cloth.step(0,pose,{}, {deep,conflict}));assert(cloth.stats().boundsRejected);
    config.maxBoundTests=1;assert(cloth.initialize(pose,faces,{0},config));assert(!cloth.step(0,pose,{}, {box}));assert(cloth.stats().budgetExceeded&&cloth.stats().boundsRejected&&cloth.stats().boundTests==1);
}
static void contactCorrectionDoesNotLaunch(){
    const std::vector<Vec3> pose={{-.5f,-.5f,0},{.5f,-.5f,0},{0,.5f,0}};
    const std::vector<Triangle> faces={{0,1,2}};auto config=unconstrained();config.fixedStep=1.f/60;
    cape::Cloth cloth;assert(cloth.initialize(pose,faces,{0},config));auto box=solidBox();box.preferredDirection={0,0,1};
    assert(cloth.step(0,pose,{}, {box}));const auto cleared=cloth.positions();
    for(unsigned frame=0;frame<60;++frame)assert(cloth.step(1.f/60,pose,{}));
    // A positional clearance correction is not a launch impulse once the
    // obstruction disappears. This failed with the old correction/h velocity.
    for(unsigned i=1;i<pose.size();++i)assert(cape::length(cloth.positions()[i]-cleared[i])<.001f);
}
static void stableOverlapsAndMotion(){
    auto mesh=grid(5,7,0);auto config=unconstrained();config.fixedStep=1.f/60;config.stableBounds=true;
    config.damping=9;config.maxSpeed=3;config.poseLimit=.3f;config.maxSubsteps=3;
    cape::Cloth cloth;assert(cloth.initialize(mesh.vertices,mesh.triangles,mesh.pins,config));
    auto a=solidBox({0,.3f,0},{.3f,.5f,.1f}),b=solidBox({.1f,.3f,.05f},{.3f,.5f,.1f});
    a.preferredDirection=b.preferredDirection={0,0,1};
    for(unsigned frame=0;frame<240;++frame){auto pose=mesh.vertices;
        const float t=frame/60.f;for(auto& p:pose){p.x+=std::sin(t*3)*.12f;p.z+=std::max(0.f,std::sin(t*2))*.15f;}
        assert(cloth.step(frame%23==0?.08f:1.f/60,pose,{}, {a,b},pose));
        assert(!cloth.stats().reset); // Slow frames drop catch-up time, not the current pose.
        assertFacesClear(cloth,mesh.triangles,a);assertFacesClear(cloth,mesh.triangles,b);
        for(unsigned i=0;i<pose.size();++i)assert(cape::length(cloth.positions()[i]-pose[i])<.5f);
    }
    config.maxBoundTests=0;assert(cloth.initialize(mesh.vertices,mesh.triangles,mesh.pins,config));
    assert(cloth.step(0,mesh.vertices,{}, {a,b},mesh.vertices));
    assert(!cloth.stats().boundsRejected);assertFacesClear(cloth,mesh.triangles,a);assertFacesClear(cloth,mesh.triangles,b);
}
static void solidBoundsCost(){
    const auto mesh=grid(9,13,.03f);cape::Config bounded;bounded.fixedStep=1.f/60;bounded.maxSubsteps=4;bounded.iterations=4;bounded.selfCollision=false;bounded.maxContactSamples=0;
    cape::Cloth fast;assert(fast.initialize(mesh.vertices,mesh.triangles,mesh.pins,bounded));const auto box=solidBox({0,0,-.5f},{4,4,.5f});
    const auto begin=std::chrono::steady_clock::now();unsigned tests=0;
    for(unsigned frame=0;frame<90;++frame){assert(fast.step(1.f/60,mesh.vertices,{}, {box}));tests+=fast.stats().boundTests;assert(fast.stats().boundTests<=(bounded.boundIterations+1)*mesh.triangles.size()+bounded.boundIterations*mesh.pins.size());assert(fast.stats().collisionTests==0&&fast.stats().selfPairs==0);}
    const auto fastEnd=std::chrono::steady_clock::now();
    std::vector<cape::ColliderTriangle> surface;const unsigned side=32;
    for(unsigned x=0;x<side;++x)for(unsigned y=0;y<side;++y){const float a=-4+8.f*x/side,b=-4+8.f*y/side,c=-4+8.f*(x+1)/side,d=-4+8.f*(y+1)/side;surface.push_back({{{a,b,0},{c,b,0},{a,d,0}},{{a,b,0},{c,b,0},{a,d,0}}});surface.push_back({{{c,b,0},{c,d,0},{a,d,0}},{{c,b,0},{c,d,0},{a,d,0}}});}
    cape::Cloth legacy;assert(legacy.initialize(mesh.vertices,mesh.triangles,mesh.pins));const auto oldBegin=std::chrono::steady_clock::now();
    for(unsigned frame=0;frame<90;++frame)assert(legacy.step(1.f/60,mesh.vertices,surface));
    const auto end=std::chrono::steady_clock::now();
    std::printf("solid bound benchmark: %.3f ms/frame, %u exact bound tests/frame; legacy 2048-surface sample path %.3f ms/frame\n",std::chrono::duration<double,std::milli>(fastEnd-begin).count()/90,tests/90,std::chrono::duration<double,std::milli>(end-oldBegin).count()/90);
}
int main(){hangingAndTopology();movingPinsAndInertia();frameRates();staticCollision();sweptAndTwoSidedCollision();movingCollider();sparseEdgeCollision();sparseFaceCollision();wallFloorCorner();rotatingTriangle();substepColliderAndDegeneracy();validationAndReset();wholeFaceBoundsAndPinnedSeam();movingAndRotatingSolidBounds();solidBoundsResetValidationAndBudget();contactCorrectionDoesNotLaunch();stableOverlapsAndMotion();solidBoundsCost();std::puts("cape cloth tests passed");}
