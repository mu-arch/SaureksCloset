#include <cassert>
#include <cmath>
#include <iostream>
#include "../native/BagPlacement.h"
static const BagMatrix identity{{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1}};
static void near(float a,float b,float e=.000004f){assert(std::isfinite(a)&&std::fabs(a-b)<e);}
static float exercise(float percent,float yaw,unsigned dt,float amplitude,bool running=true,bool broadModel=false){
    const float scale=.45f*percent/100,c=std::cos(yaw),s=std::sin(yaw);
    auto profile=bagResponseProfile("leather");
    if(broadModel){
        // Shipped Amber Traveler bounds, also used as a 35% waist bag.
        profile.low={{-.6902362f,-.688162f,-.6195f}};
        profile.high={{0,.688162f,.6195f}};profile.measuredBounds=true;
    }
    std::array<BagTuningEntry,16> fits{};fits[1].enabled=true;fits[1].values.scale=percent;
    std::array<BagMotion,3> motions{};std::array<BagResponse,3> responses{};
    float peak=0,last=0,largestStep=0;
    for(unsigned time=0;time<=6800;time+=dt){
        const bool active=time<4400;const float t=std::fmin(time,4400u)*.001f;
        const BagMatrix fitted{{scale*c,scale*s,0,0,-scale*s,scale*c,0,0,0,0,scale,0,
            -.13f,.22f,1.1f+amplitude*std::sin(t*(running?17.6f:12.5663706f)),1}};
        BagMatrix reference;
        for(unsigned preset=0;preset<3;++preset){
            BagMatrix output;
            assert(bagPlacement(identity,identity,identity,bagFits[1].anchor,output,1,&motions[preset],time,7,active&&running,
                nullptr,nullptr,0,fits.data(),preset,202,&responses[preset],&profile,0,active,&fitted,false));
            if(!preset)reference=output;
            else for(unsigned i=0;i<16;++i)near(output[i],reference[i]); // Preset never selects the response.
            const float bob=motions[preset].verticalBob*(1+.75f*motions[preset].runWeight);
            for(unsigned axis=0;axis<3;++axis)
                near(output[12+axis]+profile.top*output[8+axis],
                    fitted[12+axis]+profile.top*fitted[8+axis]+(axis==2?bob:0));
            for(const auto& matrix:bagResponseMatrices(responses[preset],output,false))assert(matrix==output);
            // The hinge follows the fitted bag's outward direction, including
            // a bag moved to either side after starting from the Back preset.
            const float outward=(-output[8]*c-output[9]*s)/scale;
            near((-output[8]*s+output[9]*c)/scale,0);
            assert(outward<=.0000001f); // Bottom (negative Z) always lifts outward.
            // Relative to the translated support, the bottom rises as it opens.
            assert(output[10]<=scale+.0000001f);
            near(output[8]*output[8]+output[9]*output[9]+output[10]*output[10],scale*scale);
            if(!preset){
                const float angle=std::atan2(outward,output[10]/scale);
                if(time>1000&&active){peak=std::fmax(peak,std::fabs(angle));largestStep=std::fmax(largestStep,std::fabs(angle-last));}
                last=angle;
            }
            if(time>6500)for(unsigned i=0;i<16;++i)near(output[i],fitted[i],.00001f);
            auto duplicate=output;
            assert(bagPlacement(identity,identity,identity,bagFits[1].anchor,duplicate,1,&motions[preset],time,7,active&&running,
                nullptr,nullptr,0,fits.data(),preset,202,&responses[preset],&profile,0,active,&fitted,false));
            for(unsigned i=0;i<16;++i)near(duplicate[i],output[i]);
        }
    }
    assert(largestStep<.10f);
    assert(peak<10.001f*.01745329252f);
    if(amplitude>.02f)assert(peak>.25f*.01745329252f);
    return peak;
}
int main(){
    for(float size:{25.f,35.f,56.f,85.f})for(float yaw:{0.f,-1.57f,1.57f}){
        const float moving=exercise(size,yaw,8,.03f),quiet=exercise(size,yaw,8,.003f);
        assert(moving>quiet*2);
        for(unsigned dt:{16u,32u}){const float peak=exercise(size,yaw,dt,.03f);assert(peak>moving*.8f&&peak<moving*1.2f);}
    }
    // Walking has no running gain. Inspect the emitted lower edge, not just
    // the spring state: a small hip bag must visibly swing out at walking pace.
    // Human-female Walk has ~0.0674 model-unit hip travel over its one-second loop.
    for(unsigned dt:{8u,16u,32u}){
        const float small=exercise(35,1.57f,dt,.0337f,false);
        const float large=exercise(85,1.57f,dt,.0337f,false);
        assert(small>4.f*.01745329252f&&small>large*1.15f);
        for(bool running:{false,true})for(float size:{25.f,35.f,45.f}){
            const float previous=exercise(size,-.925f,dt,.0337f,running);
            const float compact=exercise(size,-.925f,dt,.0337f,running,true);
            near(compact,previous); // Authored breadth must not suppress a compact fit.
            assert(compact>4.f*.01745329252f);
        }
        const float fullBackpack=exercise(85,1.57f,dt,.0337f,false,true);
        assert(fullBackpack>0&&fullBackpack<large*.6f);
    }
    near(exercise(35,1.57f,8,0,false),0);
    near(exercise(35,1.57f,8,0),0);
    std::cout<<"PASS: size-compensated outward walking flop follows actual mount motion/orientation, is independent of Back/hip presets, remains smooth on small bags, preserves rigid shape and settles at idle\n";
}
