#include "../native/CapeWorker.h"
#include "../native/CapeFit.h"
#include <cassert>
#include <chrono>
#include <iostream>
#include <thread>
int main(){
    cape::Config config;config.fixedStep=1.f/60;config.maxSubsteps=3;config.poseLimit=.12f;config.damping=9;
    const std::vector<cape::Vec3> rest{{0,-.3f,2},{0,.3f,2},{0,-.3f,1},{0,.3f,1},{0,-.3f,0},{0,.3f,0}};
    const std::vector<cape::Triangle> faces{{0,2,1},{1,2,3},{2,4,3},{3,4,5}};const std::vector<std::uint32_t> pins{0,1};
    cape::AsyncCloth cloth;
    assert(cloth.initialize(rest,faces,pins,config));assert(!cloth.hasResult());
    assert(cloth.step(0,rest,{}));cape::waitCapeWorkerForTests();
    float motion=0;
    for(unsigned frame=0;frame<120;++frame){auto animated=rest;for(auto& p:animated)p.x+=.05f*std::sin(frame*.12f);
        assert(cloth.step(1.f/60,animated,{}));cape::waitCapeWorkerForTests();
        assert(cloth.hasResult());assert(capeFabricFits(cloth.positions(),animated,cloth.material(),faces,pins));
        assert(capeFabricFits(cloth.positions(),animated,cloth.material(),faces,pins));
        for(unsigned i=0;i<rest.size();++i)motion=std::max(motion,cape::length(cloth.positions()[i]-animated[i]));
    }
    assert(motion>.01f);
    // A deliberately blocked worker must never block a draw or grow its queue.
    cape::delayCapeWorkerForTests(100);
    const auto begin=std::chrono::steady_clock::now();
    for(unsigned i=0;i<100;++i)assert(cloth.step(.001f,rest,{}));
    const auto duration=std::chrono::steady_clock::now()-begin;
    assert(duration<std::chrono::milliseconds(80));
    auto moved=rest;for(auto& p:moved)p.x+=100;
    cloth.reset(moved);assert(!cloth.hasResult());
    assert(cloth.step(0,moved,{}));
    for(unsigned i=0;i<moved.size();++i)assert(cape::length(cloth.positions()[i]-moved[i])<.00001f);
    cape::waitCapeWorkerForTests();cape::delayCapeWorkerForTests(0);
    assert(cloth.step(.016f,moved,{}));cape::waitCapeWorkerForTests();assert(cloth.hasResult());
    for(auto p:cloth.positions())assert(p.x>99.8f); // No result from before reset can be applied.
    // Expired deformation is not reused while the worker is stalled.
    cape::delayCapeWorkerForTests(300);
    assert(cloth.step(.016f,moved,{}));
    std::this_thread::sleep_for(std::chrono::milliseconds(270));
    assert(cloth.step(.016f,moved,{}));assert(!cloth.hasResult());
    for(unsigned i=0;i<moved.size();++i)assert(cape::length(cloth.positions()[i]-moved[i])<.00001f);
    cape::delayCapeWorkerForTests(0);cape::waitCapeWorkerForTests();
    // Capes with different vertex counts cannot consume each other's result.
    assert(cloth.initialize({{0,0,1},{0,1,1},{0,0,0}},{{0,1,2}},{0,1},config));
    assert(!cloth.hasResult());assert(cloth.step(0,{{0,0,1},{0,1,1},{0,0,0}},{}));
    cape::waitCapeWorkerForTests();assert(cloth.step(.016f,{{0,0,1},{0,1,1},{0,0,0}},{}));assert(cloth.positions().size()==3);
    cape::waitCapeWorkerForTests();
    std::cout<<"cape worker: real asynchronous solve, nonblocking latest-only queue, generation isolation, fixed attachment and model switch passed\n";
}
