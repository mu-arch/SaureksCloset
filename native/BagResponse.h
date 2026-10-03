#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include "BagCoordinates.h"
#include "BagJiggle.h"

// A small placement response, not a cloth solver. A shared spatial field drives
// local control bones; the top/rear remain fixed while lower/front fabric gives.
// Shared mesh data and animation tracks are never modified.
struct BagResponseProfile {
    float top=.6195f,height=1.239f;
    float sag=.10f,rate=13.f,inertia=.7f,lateral=.085f,lift=.16f;
    std::array<float,3> low{{-.5f,-.5f,-.6195f}},high{{.5f,.5f,.6195f}};
    bool measuredBounds=false;
    bool operator==(const BagResponseProfile& p) const {
        return top==p.top&&height==p.height&&sag==p.sag&&rate==p.rate&&
            inertia==p.inertia&&lateral==p.lateral&&lift==p.lift&&low==p.low&&high==p.high&&measuredBounds==p.measuredBounds;
    }
};
static BagResponseProfile bagResponseProfile(const char* material,float bottom=-.6195f,float top=.6195f){
    BagResponseProfile profile;
    if(std::isfinite(bottom)&&std::isfinite(top)&&top-bottom>.05f&&top-bottom<10.f){
        profile.top=top;profile.height=top-bottom;
        profile.low[2]=bottom;profile.high[2]=top;
    }
    if(material&&!std::strcmp(material,"leather")){
        profile.sag=.025f;profile.rate=22.f;profile.inertia=.35f;profile.lateral=.035f;profile.lift=.045f;
    }else if(material&&!std::strcmp(material,"canvas")){
        profile.sag=.055f;profile.rate=17.f;profile.inertia=.5f;profile.lateral=.055f;profile.lift=.085f;
    }
    return profile;
}
// The model catalogue normalizes height, but not width or depth. Use the
// authored footprint to distinguish a full backpack from a narrow pouch.
// A broad model fitted as a compact waist bag must retain its light flap.
// Fade only the extra backpack damping in across medium-to-full-size fits;
// the existing size response still controls the base motion and travel.
static float bagModelMass(const BagResponseProfile* profile,float fittedScale=.45f*.85f){
    if(!profile||!profile->measuredBounds)return 1.f;
    const float width=profile->high[1]-profile->low[1];
    const float depth=profile->high[0]-profile->low[0];
    if(!std::isfinite(width)||!std::isfinite(depth)||width<=0||depth<=0)return 1.f;
    if(!std::isfinite(fittedScale)||fittedScale<=0)return 1.f;
    const float sizeBlend=std::fmax(0.f,std::fmin(1.f,(fittedScale-.45f*.45f)/(.45f*.40f)));
    const float weight=sizeBlend*sizeBlend*(3.f-2.f*sizeBlend);
    return 1.f+std::fmax(0.f,std::fmin(1.f,(width*depth-.52f)*3.f))*weight;
}
struct BagResponseStep { float decay=1,dtDecay=0,driverBlend=0; };
struct BagResponse {
    bool tracking=false,ready=false;
    float outputAmplitude=1;
    bool activeMotion=true;
    std::uintptr_t model=0;
    std::uint64_t fit=0;
    std::uint32_t time=0,stableSince=0;
    unsigned builds=0;
    BagResponseProfile profile;
    std::array<BagResponseStep,65> steps{};
    std::array<double,3> pin{};
    std::array<float,3> driver{},offset{},velocity{};
    std::array<std::array<float,3>,60> weights{};
    std::array<float,3> localOffset{};
    std::array<float,3> bobLocal{};
    BagJiggleHistory<4> jiggle;
};
static float bagResponseLength(const std::array<float,3>& value){
    return std::sqrt(value[0]*value[0]+value[1]*value[1]+value[2]*value[2]);
}
static void bagResponseLimit(std::array<float,3>& value,float maximum){
    const float length=bagResponseLength(value);
    if(length>maximum)for(auto& component:value)component*=maximum/length;
}
static std::array<float,3> bagResponseField(const BagResponseProfile& profile,const std::array<float,3>& point){
    const float down=std::fmax(0.f,std::fmin(1.f,(profile.high[2]-point[2])/profile.height));
    const float front=std::fmax(0.f,std::fmin(1.f,(profile.high[0]-point[0])/(profile.high[0]-profile.low[0])));
    const float across=std::fmax(-1.f,std::fmin(1.f,(2*point[1]-profile.high[1]-profile.low[1])/(profile.high[1]-profile.low[1])));
    const float free=front*front*(3-2*front),center=1-.3f*across*across;
    const float vertical=down*std::sqrt(down)*free*center;
    return {{vertical*.45f,vertical*.6f,vertical}};
}
static void bagResponseCompile(BagResponse& state){
    // Precompute the exact critically damped impulse coefficients once the fit
    // has stopped changing. No animation sampling or mesh work runs each frame.
    const float decay=std::exp(-state.profile.rate*.001f),filter=std::exp(-.001f/.06f);
    float d=1,f=1;
    for(unsigned ms=1;ms<state.steps.size();++ms){
        d*=decay;f*=filter;state.steps[ms]={d,ms*.001f*d,1-f};
    }
    unsigned index=0;
    for(unsigned x=0;x<3;++x)for(unsigned y=0;y<4;++y)for(unsigned z=0;z<5;++z){
        const std::array<float,3> point{{state.profile.low[0]+(state.profile.high[0]-state.profile.low[0])*x/2.f,
            state.profile.low[1]+(state.profile.high[1]-state.profile.low[1])*y/3.f,
            state.profile.low[2]+state.profile.height*z/4.f}};
        state.weights[index++]=bagResponseField(state.profile,point);
    }
    state.ready=true;++state.builds;
}
static void bagResponseLocal(BagResponse& state,const BagMatrix& pose){
    BagMatrix inverse;
    if(!bagAffineInverse(pose,inverse)){state.localOffset={};return;}
    for(unsigned axis=0;axis<3;++axis){
        state.localOffset[axis]=inverse[axis]*state.offset[0]+inverse[4+axis]*state.offset[1]+inverse[8+axis]*state.offset[2];
        // Exported culling bounds reserve 22% of rest height in every axis.
        const float limit=std::fmin(.20f*state.profile.height,
            (state.profile.high[axis]-state.profile.low[axis])*(axis==2?.20f:.12f));
        state.localOffset[axis]=std::fmax(-limit,std::fmin(limit,state.localOffset[axis]));
    }
}
static void bagResponseBob(BagResponse& state,const BagMatrix& pose,const std::array<float,3>& up,float amount){
    state.bobLocal={};BagMatrix inverse;
    if(!state.ready||!std::isfinite(amount)||!bagAffineInverse(pose,inverse))return;
    for(unsigned axis=0;axis<3;++axis)
        state.bobLocal[axis]=amount*(inverse[axis]*up[0]+inverse[4+axis]*up[1]+inverse[8+axis]*up[2]);
}
static std::array<BagMatrix,61> bagResponseMatrices(const BagResponse& state,const BagMatrix& modelToRender,bool softBody=true){
    std::array<BagMatrix,61> output;output.fill(modelToRender);
    if(!softBody||!state.ready)return output;
    for(unsigned control=0;control<60;++control){
        auto& matrix=output[control+1];
        // Restore the original vertical spring without moving its body pin.
        // The lower three quarters travel together; only the upper attachment
        // band takes up the give. This does not scale the whole bag. Flap and
        // body share the field, including the rear, so they bob together.
        const float bobWeight=control%5==4?0.f:1.f;
        for(unsigned k=0;k<3;++k){
            const float limit=.20f*state.profile.height;
            const float shift=std::fmax(-limit,std::fmin(limit,
                (state.localOffset[k]*state.weights[control][k]+state.bobLocal[k]*bobWeight)*state.outputAmplitude));
            for(unsigned axis=0;axis<3;++axis)matrix[12+axis]+=modelToRender[k*4+axis]*shift;
        }
    }
    return output;
}
static void updateBagResponse(BagResponse& state,const BagMatrix& pose,float scale,std::uint32_t now,
                             std::uintptr_t model,std::uint64_t fit,const BagResponseProfile& profile,
                             const std::array<float,3>& worldUp,float flight=0,unsigned identity=0,bool activeMotion=true){
    state.bobLocal={};
    const float height=profile.height*scale;
    std::array<double,3> pin{{double(pose[12])+double(pose[8])*profile.top,
        double(pose[13])+double(pose[9])*profile.top,double(pose[14])+double(pose[10])*profile.top}};
    if(!state.tracking||state.model!=model||state.fit!=fit||!(state.profile==profile)){
        const auto builds=state.builds;state={};state.builds=builds;
        state.tracking=true;state.model=model;state.fit=fit;state.profile=profile;state.activeMotion=activeMotion;
        state.time=state.stableSince=now;state.pin=pin;
        return;
    }
    const unsigned elapsed=now-state.time;
    if(!elapsed){bagResponseLocal(state,pose);return;}
    std::array<float,3> movement{};
    for(unsigned axis=0;axis<3;++axis)movement[axis]=pin[axis]-state.pin[axis];
    state.pin=pin;state.time=now;
    // Allow native terminal falling speed (about 60 units/s). A fixed short
    // distance cutoff would repeatedly reset the bag during a genuine fall.
    const float seconds=elapsed*.001f;
    if(elapsed>250||bagResponseLength(movement)>std::fmax(2.5f,90.f*seconds)){
        state.driver={};state.offset={};state.velocity={};state.localOffset={};state.jiggle={};return;
    }
    if(!state.ready){
        if(now-state.stableSince<200)return;
        bagResponseCompile(state);state.driver={};movement={};
    }
    std::array<float,3> measured{};
    for(unsigned axis=0;axis<3;++axis)measured[axis]=movement[axis]/seconds;
    bagResponseLimit(measured,90.f);
    float filter=1;
    for(unsigned remaining=elapsed;remaining;){
        const unsigned ms=remaining>64?64:remaining;filter*=1-state.steps[ms].driverBlend;remaining-=ms;
    }
    std::array<float,3> acceleration{};
    for(unsigned axis=0;axis<3;++axis){
        const float next=state.driver[axis]+(1-filter)*(measured[axis]-state.driver[axis]);
        acceleration[axis]=(next-state.driver[axis])/seconds;state.driver[axis]=next;
    }
    bagResponseLimit(acceleration,35.f*height);
    if(!activeMotion)acceleration={};
    flight=std::isfinite(flight)?std::fmax(-1.f,std::fmin(1.f,flight)):0;
    const auto delayed=delayedBagJiggle(state.jiggle,std::array<float,4>{{acceleration[0],acceleration[1],acceleration[2],flight}},now,identity);
    acceleration={{delayed[0],delayed[1],delayed[2]}};flight=delayed[3];
    if(!activeMotion){
        // Keep static cloth sag and landing lift, but do not replay queued gait
        // impulses or turn idle body animation into fresh fabric wobble.
        acceleration={};state.driver={};
        if(state.activeMotion)state.velocity={};
    }
    state.activeMotion=activeMotion;
    // Scale dynamic loads as well as travel limits. Otherwise the same mount
    // acceleration pushes a tiny bag into its limit on every hip-bone cycle.
    const float strength=bagJiggleSizeGain(scale);
    for(float& value:acceleration)value*=strength;
    const float vertical=height*(-profile.sag+profile.lift*std::fmax(0.f,flight)+profile.sag*.5f*std::fmin(0.f,flight));
    std::array<float,3> target{};
    float verticalInertia=0;
    for(unsigned axis=0;axis<3;++axis)verticalInertia+=acceleration[axis]*worldUp[axis];
    std::array<float,3> lateral{};
    for(unsigned axis=0;axis<3;++axis)
        lateral[axis]=-(acceleration[axis]-verticalInertia*worldUp[axis])*profile.inertia/(profile.rate*profile.rate);
    bagResponseLimit(lateral,height*profile.lateral);
    const float pull=std::fmax(-.22f*height,std::fmin(.10f*height,
        vertical-verticalInertia*profile.inertia/(profile.rate*profile.rate)));
    for(unsigned axis=0;axis<3;++axis)target[axis]=lateral[axis]+worldUp[axis]*pull;
    for(unsigned remaining=elapsed;remaining;){
        const unsigned ms=remaining>64?64:remaining;const auto& step=state.steps[ms];
        for(unsigned axis=0;axis<3;++axis){
            const float error=state.offset[axis]-target[axis],b=state.velocity[axis]+profile.rate*error;
            state.offset[axis]=target[axis]+error*step.decay+b*step.dtDecay;
            state.velocity[axis]=state.velocity[axis]*step.decay-profile.rate*b*step.dtDecay;
        }
        remaining-=ms;
    }
    bagResponseLimit(state.offset,.25f*height);
    bagResponseLocal(state,pose);
}
