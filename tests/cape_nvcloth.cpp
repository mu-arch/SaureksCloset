#include "../native/CapeNvCloth.h"
#include "../native/CapeFit.h"
#include <cassert>
#include <chrono>
#include <iostream>
#include <limits>
static std::vector<cape::Vec3> pose(){return {{0,-.3f,2},{0,.3f,2},{0,-.3f,1},{0,.3f,1},{0,-.3f,0},{0,.3f,0}};}
static const std::vector<cape::Triangle> faces{{0,2,1},{1,2,3},{2,4,3},{3,4,5}};
static const std::vector<std::uint32_t> pins{0,1};
int main(){
    cape::NvClothSolver cloth;cape::Config config;config.fixedStep=1.f/60;config.maxSubsteps=3;config.poseLimit=.12f;config.damping=9;config.maxSpeed=3;
    const auto rest=pose();assert(cloth.initialize(rest,faces,pins,config));
    cape::ColliderBox box;box.currentCenter=box.previousCenter={0,0,1};box.currentHalf=box.previousHalf={.4f,.4f,.9f};box.preferredDirection={-1,0,0};
    float deformation=0;const auto start=std::chrono::steady_clock::now();
    for(unsigned frame=0;frame<360;++frame){
        auto animated=rest;const float t=frame/60.f;
        for(auto& p:animated){p.x+=.06f*std::sin(t*5);p.z+=.04f*std::sin(t*6);}
        // Actual NvCloth, including authored overlap and skipped frames.
        assert(cloth.step(frame%40==0?.08f:1.f/60,animated,{}, {box},animated));
        assert(cloth.limitToFit(animated,[&](const auto& p){return capePoseFits(p,animated,faces,pins);}));
        assert(capePoseFits(cloth.positions(),animated,faces,pins));
        for(auto pin:pins)assert(cape::length(cloth.positions()[pin]-animated[pin])<.00001f);
        for(unsigned i=0;i<animated.size();++i)deformation=std::max(deformation,cape::length(cloth.positions()[i]-animated[i]));
    }
    assert(deformation>.01f&&deformation<=.18f);
    auto warped=rest;for(auto& p:warped){p.x+=10000;p.y-=10000;}
    assert(cloth.step(1.f/60,warped,{}));assert(cloth.stats().reset);
    for(unsigned i=0;i<warped.size();++i)assert(cape::length(cloth.positions()[i]-warped[i])<.001f);
    cloth.reset(rest);assert(cloth.step(0,rest,{}));
    auto bad=rest;bad.back().z=std::numeric_limits<float>::quiet_NaN();assert(!cloth.step(.016f,bad,{}));
    assert(!cloth.initialize(rest,{{0,1,90}},pins,config));
    // Surface contacts execute in NvCloth, not just a fake proxy test.
    assert(cloth.initialize(rest,faces,pins,config));
    cape::ColliderTriangle floor;floor.current[0]={-5,-5,.03f};floor.current[1]={5,-5,.03f};floor.current[2]={0,5,.03f};
    for(unsigned i=0;i<3;++i)floor.previous[i]=floor.current[i];
    for(unsigned frame=0;frame<30;++frame)assert(cloth.step(1.f/60,rest,{floor}));
    assert(cloth.positions()[4].z>=.029f&&cloth.positions()[5].z>=.029f);
    const auto ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
    std::cout<<"NvCloth: real CPU solve, nonzero motion, fixed pins, fit bounds, terrain contact, teleports and invalid input passed ("<<ms<<" ms for sequence)\n";
}
