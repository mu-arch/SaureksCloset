#include "../native/CapeNvCloth.h"
#include "../native/CapeFit.h"
#include <cassert>
#include <chrono>
#include <iostream>
#include <limits>
static std::vector<cape::Vec3> pose(){return {{0,-.3f,2},{0,.3f,2},{0,-.3f,1},{0,.3f,1},{0,-.3f,0},{0,.3f,0}};}
static const std::vector<cape::Triangle> faces{{0,2,1},{1,2,3},{2,4,3},{3,4,5}};
static const std::vector<std::uint32_t> pins{0,1};
static void sustainedRunResponse(){std::vector<cape::Vec3> rest;std::vector<cape::Triangle> faces;std::vector<unsigned> pins;
 for(unsigned y=0;y<7;++y)for(unsigned x=0;x<5;++x)rest.push_back({0,(x/4.f-.5f)*.6f,1.8f-y/6.f*1.4f});
 for(unsigned y=0;y<6;++y)for(unsigned x=0;x<4;++x){unsigned a=y*5+x;faces.push_back({a,a+5,a+1});faces.push_back({a+1,a+5,a+6});}
 for(unsigned i=0;i<5;++i)pins.push_back(i);
 cape::Config cfg;cfg.poseLimit=.65f;cfg.damping=9;cfg.fixedStep=1.f/60;cfg.maxSubsteps=3;cfg.maxSpeed=3;
 cape::NvClothSolver cloth;assert(cloth.initialize(rest,faces,pins,cfg));float max=0,sum=0,last=0,step=0,settledStep=0;std::vector<cape::Vec3> previous(rest.size());
 for(unsigned frame=0;frame<600;++frame){float t=std::min(frame/60.f,4.f);auto pose=rest;for(auto& p:pose)p.x+=t*7;
 assert(cloth.step(1.f/60,pose,{}));assert(capeFabricFits(cloth.positions(),pose,cloth.material(),faces,pins));
 float motion=0;for(unsigned i=0;i<pose.size();++i){motion=std::max(motion,cape::length(cloth.positions()[i]-pose[i]));auto relative=cloth.positions()[i]-pose[i];if(frame>0)step=std::max(step,cape::length(relative-previous[i]));if(frame>540)settledStep=std::max(settledStep,cape::length(relative-previous[i]));previous[i]=relative;}
 if(frame>=120&&frame<240){max=std::max(max,motion);sum+=motion/120;}last=motion;
 }
 assert(sum>.15f); // Steady running must visibly move cloth, not only startup.

 assert(settledStep<.005f); // No procedural perpetual sway after stopping.
 assert(step<.2f); // No frame-to-frame launch or safety-reset snap.
 std::cout<<"running mean="<<sum<<" peak="<<max<<" settled="<<last<<"\n";
}

static void gravityAndTuning(){
    auto tilted=pose();for(auto& p:tilted){const float drop=2-p.z;p.x=drop*.6f;p.z=2-drop*.8f;}
    cape::Config config;config.fixedStep=1.f/60;config.maxSubsteps=3;config.damping=9;config.maxSpeed=3;
    cape::NvClothSolver plain,animated;assert(plain.initialize(tilted,faces,pins,config));assert(animated.initialize(tilted,faces,pins,config));
    for(unsigned frame=0;frame<600;++frame){auto native=tilted;
        for(unsigned i=2;i<native.size();++i){native[i].x+=.5f*std::sin(frame*.1f);native[i].z+=.3f*std::cos(frame*.1f);}
        assert(plain.step(1.f/60,tilted,{}));assert(animated.step(1.f/60,native,{}));
        for(unsigned i=0;i<native.size();++i)assert(cape::length(plain.positions()[i]-animated.positions()[i])<1.e-5f);
        assert(capeFabricFits(plain.positions(),tilted,tilted,faces,pins));
    }
    assert(std::fabs(plain.positions()[4].x)<.04f&&plain.positions()[4].z<.04f);
    // The free hem hangs under gravity, independent of native cape animation.
    auto run=[&](float density,float air){cape::NvClothSolver cloth;config.density=density;config.clothAir=air;
        const auto rest=pose();assert(cloth.initialize(rest,faces,pins,config));
        for(unsigned frame=0;frame<600;++frame){auto moving=rest;for(auto& p:moving)p.x+=frame/60.f*7;
            assert(cloth.step(1.f/60,moving,{}));assert(capeFabricFits(cloth.positions(),moving,rest,faces,pins));}
        return std::fabs(cloth.positions()[4].x-599/60.f*7);
    };
    const float light=run(.35f,.0002f),heavy=run(1.05f,.0002f),stillAir=run(.35f,0);
    assert(light>heavy*1.15f&&heavy>.01f&&stillAir<.02f);
    config.clothBending=std::numeric_limits<float>::quiet_NaN();assert(!plain.initialize(tilted,faces,pins,config));
    std::cout<<"gravity hanging, native-pose independence and mass/air response passed (light="<<light<<", heavy="<<heavy<<")\n";
}
static void skinnedMeshContact(){
    const std::array<float,16> identity{{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1}};
    auto mesh=std::make_shared<cape::CollisionMesh>();
    for(cape::Vec3 p:std::vector<cape::Vec3>{{-1,-1,0},{1,-1,0},{1,1,0},{-1,1,0}})mesh->vertices.push_back({p,{{255,0,0,0}},{{0,0,0,0}}});
    mesh->triangles={{0,1,2},{0,2,3}};auto bone=identity;bone[14]=.02f;
    cape::AnimatedCollider collider{mesh,{bone},identity};
    auto contacts=cape::skinCapeContacts({collider},pose());assert(contacts.size()==2);
    assert(std::fabs(contacts[0].current[0].z-.02f)<1.e-5f);
    assert(cape::capeMeshContact({0,0,0},contacts).radius>0);
    assert(cape::capeMeshContact({0,0,-1},contacts).radius==0); // No infinite back-face planes.
    cape::NvClothSolver cloth;auto rest=pose();assert(cloth.initialize(rest,faces,pins));cloth.setColliders({collider});
    for(unsigned frame=0;frame<180;++frame)assert(cloth.step(1.f/60,rest,{}));
    assert(cloth.positions()[4].z>.016f&&cloth.positions()[5].z>.016f);
    assert(cloth.stats().contacts>=2);
    // Remove a collision source as a checkbox does; the cloth can settle again.
    cloth.setColliders({});for(unsigned frame=0;frame<180;++frame)assert(cloth.step(1.f/60,rest,{}));
    assert(cloth.positions()[4].z<.005f&&cloth.stats().contacts==0);
}

static void fixedClockAndTail(){
    auto motion=[](unsigned fps){cape::Config config;config.fixedStep=1.f/60;config.maxSubsteps=3;config.damping=9;config.maxSpeed=3;
        cape::NvClothSolver cloth;auto rest=pose();assert(cloth.initialize(rest,faces,pins,config));
        for(unsigned frame=0;frame<=fps*3;++frame){auto input=rest;const float time=frame/float(fps);const float x=time<.5f?7*time*time:7*(time-.25f);
            for(auto& p:input)p.x+=x;assert(cloth.step(frame?1.f/fps:0,input,{}));}
        return cloth.positions();
    };
    const auto reference=motion(60);
    for(unsigned fps:{30u,90u,120u,144u,240u}){auto result=motion(fps);float error=0;
        for(unsigned i=0;i<result.size();++i)error=std::max(error,cape::length(result[i]-reference[i]));
        std::cout<<"cape timing "<<fps<<" fps error="<<error<<"\n";assert(error<.035f);
    }
    const std::array<float,16> identity{{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1}};
    auto mesh=std::make_shared<cape::CollisionMesh>();mesh->body=true;mesh->tailGroups.resize(1);
    for(unsigned i=0;i<8;++i){mesh->vertices.push_back({{i&1?.2f:-.2f,i&2?.04f:-.04f,.5f+(i&4?.04f:-.04f)},{{255,0,0,0}},{{0,0,0,0}}});mesh->tailGroups[0].push_back(i);}
    cape::AnimatedCollider collider{mesh,{identity},identity};auto capsules=cape::capeTailCapsules({collider});assert(capsules.size()==1);
    assert(capsules[0].a.x<capsules[0].b.x&&capsules[0].radius>.05f);
    cape::Config config;config.fixedStep=1.f/60;config.maxSubsteps=3;config.damping=9;config.maxSpeed=3;
    cape::NvClothSolver cloth;auto rest=pose();assert(cloth.initialize(rest,faces,pins,config));cloth.setColliders({collider});
    for(unsigned frame=0;frame<240;++frame){assert(cloth.step(1.f/60,rest,{}));assert(capeFabricFits(cloth.positions(),rest,rest,faces,pins));}
    const auto mid=(cloth.positions()[2]+cloth.positions()[5])*.5f;
    const float distance=std::sqrt(mid.y*mid.y+(mid.z-.5f)*(mid.z-.5f));
    std::cout<<"tail virtual contact distance="<<distance<<" radius="<<capsules[0].radius<<"\n";
    assert(distance>=capsules[0].radius-.012f); // A tail between real vertices must move the face.
    for(auto pin:pins)assert(cape::length(cloth.positions()[pin]-rest[pin])<1.e-5f);
}

int main(){
    fixedClockAndTail();gravityAndTuning();skinnedMeshContact();sustainedRunResponse();
    cape::NvClothSolver cloth;cape::Config config;config.fixedStep=1.f/60;config.maxSubsteps=3;config.poseLimit=.12f;config.damping=9;config.maxSpeed=3;
    const auto rest=pose();assert(cloth.initialize(rest,faces,pins,config));
    cape::ColliderBox box;box.currentCenter=box.previousCenter={0,0,1};box.currentHalf=box.previousHalf={.4f,.4f,.9f};box.preferredDirection={-1,0,0};
    float deformation=0;const auto start=std::chrono::steady_clock::now();
    for(unsigned frame=0;frame<360;++frame){
        auto animated=rest;const float t=frame/60.f;
        for(auto& p:animated){p.x+=.06f*std::sin(t*5);p.z+=.04f*std::sin(t*6);}
        // Actual NvCloth, including authored overlap and skipped frames.
        assert(cloth.step(frame%40==0?.08f:1.f/60,animated,{}, {box},animated));
        assert(capeFabricFits(cloth.positions(),animated,cloth.material(),faces,pins));
        for(auto pin:pins)assert(cape::length(cloth.positions()[pin]-animated[pin])<.00001f);
        for(unsigned i=0;i<animated.size();++i)deformation=std::max(deformation,cape::length(cloth.positions()[i]-animated[i]));
    }
    assert(deformation>.01f&&deformation<1.f);
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
