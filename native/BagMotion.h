#pragma once
#include <array>
#include <cmath>
#include <cstdint>
// Restrained secondary motion in character model space. The strap can give a
// little along world up; its other position axes and base orientation follow
// immediately. Each rendered bag owns its world/preview history.
using BagQuaternion=std::array<float,4>; // x,y,z,w
struct BagMotion {
    bool ready=false;
    std::uint32_t time=0;
    std::uintptr_t model=0;
    unsigned fit=0;
    float runWeight=0;
    // Latest unmodified attachment pose, never a delayed position/orientation.
    std::array<float,3> position{};
    BagQuaternion rotation{{0,0,0,1}};
    std::array<float,3> angularOffset{},angularVelocity{},bodyVelocity{};
    std::array<float,3> verticalAnchor{};
    float verticalOffset=0,verticalVelocity=0,bodyVerticalVelocity=0;
    float airborneWeight=0,airborneVelocity=0;
};
static BagQuaternion bagRotation(const std::array<float,16>& m,float scale){
    const float a=m[0]/scale,b=m[5]/scale,c=m[10]/scale;
    BagQuaternion q{};float s;
    if(a+b+c>0){s=std::sqrt(1+a+b+c)*2;q={{(m[6]-m[9])/scale/s,(m[8]-m[2])/scale/s,(m[1]-m[4])/scale/s,s*.25f}};}
    else if(a>b&&a>c){s=std::sqrt(1+a-b-c)*2;q={{s*.25f,(m[4]+m[1])/scale/s,(m[8]+m[2])/scale/s,(m[6]-m[9])/scale/s}};}
    else if(b>c){s=std::sqrt(1+b-a-c)*2;q={{(m[4]+m[1])/scale/s,s*.25f,(m[9]+m[6])/scale/s,(m[8]-m[2])/scale/s}};}
    else {s=std::sqrt(1+c-a-b)*2;q={{(m[8]+m[2])/scale/s,(m[9]+m[6])/scale/s,s*.25f,(m[1]-m[4])/scale/s}};}
    float length=0;for(float v:q)length+=v*v;length=std::sqrt(length);
    for(float& v:q)v/=length;
    return q;
}
static float bagRotationDot(const BagQuaternion& a,const BagQuaternion& b){
    float dot=0;for(unsigned i=0;i<4;++i)dot+=a[i]*b[i];return dot;
}
static BagQuaternion bagRotationProduct(const BagQuaternion& a,const BagQuaternion& b){
    return {{a[3]*b[0]+a[0]*b[3]+a[1]*b[2]-a[2]*b[1],
             a[3]*b[1]-a[0]*b[2]+a[1]*b[3]+a[2]*b[0],
             a[3]*b[2]+a[0]*b[1]-a[1]*b[0]+a[2]*b[3],
             a[3]*b[3]-a[0]*b[0]-a[1]*b[1]-a[2]*b[2]}};
}
static float bagVectorLength(const std::array<float,3>& v){
    return std::sqrt(v[0]*v[0]+v[1]*v[1]+v[2]*v[2]);
}
static void bagLimitVector(std::array<float,3>& v,float limit){
    const float length=bagVectorLength(v);
    if(length>limit)for(float& value:v)value*=limit/length;
}
// Three degrees is a radial limit on the complete rotation, not per axis.
static constexpr float bagMotionLimit=3.f*.01745329252f;
static constexpr float bagMotionKnee=1.5f*.01745329252f;
// Runecloth Bag authored height; travel is four percent of its physical size.
static constexpr float bagMotionHeight=1.239f;
static constexpr float bagMotionVerticalFraction=.04f;
static void smoothBagMotion(BagMotion& state,std::array<float,16>& pose,float scale,
                            std::uint32_t now,std::uintptr_t model,unsigned fit,bool running=false,float pivotHeight=0,
                            std::array<float,3> worldUp={{0,0,1}},const std::array<float,3>* verticalMeasure=nullptr,float airLiftTarget=0){
    const auto wanted=bagRotation(pose,scale);
    const std::array<float,3> position{{pose[12],pose[13],pose[14]}};
    // The caller removes the actor's root/render transform before this step.
    // Up is world vertical expressed in that same space, never a bag-local axis.
    const float upLength=bagVectorLength(worldUp);
    if(std::isfinite(upLength)&&upLength>.000001f){for(float& value:worldUp)value/=upLength;}
    else worldUp={{0,0,1}};
    // Nonuniform actor scaling makes displacement measurement different from
    // the output direction. The caller supplies a covector with dot(up)=1.
    const auto& measure=verticalMeasure?*verticalMeasure:worldUp;
    std::array<float,3> anchor=position;
    for(unsigned i=0;i<3;++i)anchor[i]+=pivotHeight*pose[8+i];
    const float height=bagMotionHeight*scale,verticalLimit=height*bagMotionVerticalFraction;
    float distance=0;for(unsigned i=0;i<3;++i){const float d=position[i]-state.position[i];distance+=d*d;}
    const std::uint32_t elapsed=now-state.time; // Also handles timer wraparound.
    if(!state.ready||state.model!=model||state.fit!=fit||elapsed>250||distance>.75f*.75f){
        state={};state.ready=true;state.position=position;state.rotation=wanted;state.verticalAnchor=anchor;
        state.time=now;state.model=model;state.fit=fit;
        return;
    }
    if(elapsed){
        const float seconds=elapsed*.001f;
        // Separate from small gait rotation: follow downward momentum only,
        // then return with a critically damped strap response after landing.
        // An exact spring step remains stable through slow/irregular frames.
        const float airTarget=std::isfinite(airLiftTarget)?std::fmax(0.f,std::fmin(1.f,airLiftTarget)):0;
        const float airRate=airTarget>0?14.f:17.f;
        const float airError=state.airborneWeight-airTarget;
        const float airStep=(state.airborneVelocity+airRate*airError)*seconds;
        const float airDecay=std::exp(-airRate*seconds);
        state.airborneWeight=airTarget+(airError+airStep)*airDecay;
        state.airborneVelocity=(state.airborneVelocity-airRate*airStep)*airDecay;
        if(state.airborneWeight<0){state.airborneWeight=0;state.airborneVelocity=std::fmax(0.f,state.airborneVelocity);}
        if(state.airborneWeight>1){state.airborneWeight=1;state.airborneVelocity=std::fmin(0.f,state.airborneVelocity);}
        state.runWeight+=(1-std::exp(-seconds/(running?.18f:.25f)))*((running?1.f:0.f)-state.runWeight);
        if(state.runWeight<.0001f)state.runWeight=0;
        // Shortest-arc angular velocity from actual animation, expressed in
        // character space. No independent running cycle or random damping.
        const BagQuaternion inverse{{-state.rotation[0],-state.rotation[1],-state.rotation[2],state.rotation[3]}};
        auto delta=bagRotationProduct(wanted,inverse);
        if(bagRotationDot(wanted,state.rotation)<0)for(float& v:delta)v=-v;
        const float sine=std::sqrt(delta[0]*delta[0]+delta[1]*delta[1]+delta[2]*delta[2]);
        const float factor=sine>.000001f?2*std::atan2(sine,std::fmax(0.f,delta[3]))/(sine*seconds):2/seconds;
        std::array<float,3> velocity{{delta[0]*factor,delta[1]*factor,delta[2]*factor}};
        bagLimitVector(velocity,5.f); // Animation cuts cannot inject huge impulses.
        float verticalDriver=0;
        for(unsigned i=0;i<3;++i)verticalDriver+=(anchor[i]-state.verticalAnchor[i])*measure[i]/seconds;
        verticalDriver=std::fmax(-4*height,std::fmin(4*height,verticalDriver));

        // A short driver filter bounds angular acceleration. Only its change
        // excites the spring; a stationary body has no procedural motion.
        // Small fixed substeps keep the stiff strap stable through slow frames.
        const unsigned steps=static_cast<unsigned>(std::ceil(seconds*480.f));
        const float dt=seconds/steps,driverBlend=1-std::exp(-dt/.035f);
        const float inertia=.65f+.15f*state.runWeight;
        for(unsigned step=0;step<steps;++step){
            std::array<float,3> acceleration{};
            for(unsigned i=0;i<3;++i){
                const float next=state.bodyVelocity[i]+driverBlend*(velocity[i]-state.bodyVelocity[i]);
                acceleration[i]=(next-state.bodyVelocity[i])/dt;
                state.bodyVelocity[i]=next;
            }
            bagLimitVector(acceleration,80.f);
            // Only changing vertical animation velocity excites the spring.
            // There is no sideways/inset lag, and no independent bounce cycle.
            const float nextVertical=state.bodyVerticalVelocity+driverBlend*(verticalDriver-state.bodyVerticalVelocity);
            const float verticalAcceleration=std::fmax(-30*height,std::fmin(30*height,(nextVertical-state.bodyVerticalVelocity)/dt));
            state.bodyVerticalVelocity=nextVertical;
            const float verticalStretch=std::fmax(0.f,(std::fabs(state.verticalOffset)-verticalLimit*.5f)/(verticalLimit*.5f));
            const float verticalStiffness=16.f*16.f*(1+10*verticalStretch*verticalStretch);
            state.verticalVelocity+=dt*(-verticalStiffness*state.verticalOffset-.9f*verticalAcceleration);
            state.verticalVelocity*=std::exp(-2*std::sqrt(verticalStiffness)*dt);
            state.verticalOffset+=dt*state.verticalVelocity;
            if(std::fabs(state.verticalOffset)>verticalLimit){
                state.verticalOffset=std::copysign(verticalLimit,state.verticalOffset);
                if(state.verticalVelocity*state.verticalOffset>0)state.verticalVelocity=0;
            }
            const float length=bagVectorLength(state.angularOffset);
            const float stretch=std::fmax(0.f,(length-bagMotionKnee)/(bagMotionLimit-bagMotionKnee));
            const float stiffness=22.f*22.f*(1+8*stretch*stretch);
            const float damping=2*std::sqrt(stiffness);
            for(unsigned i=0;i<3;++i){
                state.angularVelocity[i]+=dt*(-stiffness*state.angularOffset[i]-inertia*acceleration[i]);
                state.angularVelocity[i]*=std::exp(-damping*dt);
                state.angularOffset[i]+=dt*state.angularVelocity[i];
            }
            const float angle=bagVectorLength(state.angularOffset);
            if(angle>bagMotionLimit){
                // Safety stop for a discontinuous animation: keep tangential
                // velocity, discard outward energy instead of bouncing away.
                float outward=0;
                for(unsigned i=0;i<3;++i){state.angularOffset[i]*=bagMotionLimit/angle;outward+=state.angularVelocity[i]*state.angularOffset[i]/bagMotionLimit;}
                if(outward>0)for(unsigned i=0;i<3;++i)state.angularVelocity[i]-=outward*state.angularOffset[i]/bagMotionLimit;
            }
        }
        state.position=position;state.rotation=wanted;state.verticalAnchor=anchor;state.time=now;
    }
    // Repeated renders at the same timestamp use the current fitted pose but
    // never advance the spring or overwrite its animation sample history.
    const float angle=bagVectorLength(state.angularOffset);
    if(angle>=.0000001f){
        const float sine=std::sin(angle*.5f)/angle;
        const BagQuaternion offset{{state.angularOffset[0]*sine,state.angularOffset[1]*sine,state.angularOffset[2]*sine,std::cos(angle*.5f)}};
        auto rotation=bagRotationProduct(offset,wanted);
        const float norm=std::sqrt(bagRotationDot(rotation,rotation));
        for(float& v:rotation)v/=norm;
        const auto original=pose;
        const float x=rotation[0],y=rotation[1],z=rotation[2],w=rotation[3];
        pose={{(1-2*(y*y+z*z))*scale,2*(x*y+z*w)*scale,2*(x*z-y*w)*scale,0,
               2*(x*y-z*w)*scale,(1-2*(x*x+z*z))*scale,2*(y*z+x*w)*scale,0,
               2*(x*z+y*w)*scale,2*(y*z-x*w)*scale,(1-2*(x*x+y*y))*scale,0,
               original[12],original[13],original[14],1}};
        // Rotate about the selected strap point before adding vertical give.
        if(pivotHeight!=0)for(unsigned row=0;row<3;++row)
            pose[12+row]+=pivotHeight*(original[8+row]-pose[8+row]);
    }
    for(unsigned axis=0;axis<3;++axis)pose[12+axis]+=worldUp[axis]*state.verticalOffset;
}
