#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <iostream>
#include "../native/BagPlacement.h"

static const BagMatrix neutral{{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1}};
static constexpr float pi=3.14159265358979323846f;

static BagResponseProfile measured(float depth,float width){
    auto profile=bagResponseProfile("canvas");
    profile.low={{-depth,-width*.5f,-.6195f}};
    profile.high={{0,width*.5f,.6195f}};
    profile.measuredBounds=true;
    return profile;
}

struct MotionStats {float bob=0,angle=0,flop=0,step=0;};
static MotionStats running(const BagResponseProfile& profile){
    std::array<BagTuningEntry,16> fits{};
    fits[1].enabled=true;fits[1].values.scale=85;
    BagMotion motion;MotionStats stats;
    float previous=0;
    for(unsigned time=0;time<=5200;time+=8){
        const float t=time*.001f;
        const float sway=.10f*std::sin(2*pi*2.8f*t);
        BagMatrix fitted=neutral;
        const float c=std::cos(sway),s=std::sin(sway),scale=.45f*.85f;
        fitted[0]=scale;fitted[5]=c*scale;fitted[6]=s*scale;
        fitted[9]=-s*scale;fitted[10]=c*scale;
        fitted[12]=-.2f;fitted[13]=.18f;
        fitted[14]=1.1f+.03f*std::sin(2*pi*2.8f*t);
        BagMatrix output{};
        assert(bagPlacement(neutral,neutral,neutral,bagFits[1].anchor,output,1,&motion,time,123,true,
            nullptr,nullptr,0,fits.data(),1,201,nullptr,&profile,0,true,&fitted,false));
        if(time<800){previous=motion.verticalBob;continue;}
        stats.bob=std::max(stats.bob,std::fabs(motion.verticalBob));
        stats.angle=std::max(stats.angle,bagVectorLength(motion.angularOffset));
        stats.flop=std::max(stats.flop,std::fabs(motion.flopAngle));
        const float current=motion.verticalBob;
        stats.step=std::max(stats.step,std::fabs(current-previous));
        previous=current;
    }
    return stats;
}

int main(){
    const auto slim=measured(.375f,.971f);
    const auto mageweave=measured(.422f,1.185f);
    const auto ranger=measured(.593f,1.109f);
    const auto broad=measured(.690f,1.376f);
    const auto unknown=bagResponseProfile("canvas");
    assert(bagModelMass(nullptr)==1.f);
    assert(bagModelMass(&unknown)==1.f);
    assert(bagModelMass(&slim)==1.f);
    assert(bagModelMass(&mageweave)<1.01f);
    assert(bagModelMass(&ranger)>1.4f&&bagModelMass(&ranger)<1.6f);
    assert(bagModelMass(&broad)==2.f);
    for(float size:{25.f,35.f,45.f}){
        assert(bagModelMass(&broad,.45f*size/100.f)==1.f);
        assert(bagModelMass(&ranger,.45f*size/100.f)==1.f);
    }
    float previousMass=1.f;
    for(unsigned percent=45;percent<=125;++percent){
        const float mass=bagModelMass(&broad,.45f*percent/100.f);
        assert(mass>=previousMass&&mass-previousMass<.04f&&mass<=2.f);
        previousMass=mass;
    }
    const auto light=running(slim),heavy=running(ranger);
    assert(light.bob>.0001f&&light.angle>.0001f&&light.flop>.0001f);
    assert(heavy.bob<light.bob*.85f&&heavy.bob>light.bob*.25f);
    assert(heavy.angle<light.angle*.85f&&heavy.angle>light.angle*.2f);
    assert(heavy.flop<light.flop*.9f&&heavy.flop>0);
    assert(heavy.step<light.step*.9f);
    std::cout<<"PASS: authored footprint restrains full backpacks; compact fits retain light motion with a smooth mass transition\n";
}
