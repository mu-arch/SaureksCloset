#include <cassert>
#include <cmath>
#include <iostream>
#include "../native/BagMotion.h"
#include "../native/BagMounts.h"
static constexpr float scale=.45f;
static constexpr float radians=.01745329252f;
static std::array<float,16> pose(float x=0,float angle=0){
    const float c=std::cos(angle),s=std::sin(angle);
    return {{scale*c,scale*s,0,0,-scale*s,scale*c,0,0,0,0,scale,0,x,0,0,1}};
}
static std::array<float,16> tiltedPose(float angle){
    const float x=1.f/3,y=2.f/3,z=2.f/3,c=std::cos(angle),s=std::sin(angle),v=1-c;
    return {{scale*(c+x*x*v),scale*(x*y*v+z*s),scale*(x*z*v-y*s),0,
             scale*(x*y*v-z*s),scale*(c+y*y*v),scale*(y*z*v+x*s),0,
             scale*(x*z*v+y*s),scale*(y*z*v-x*s),scale*(c+z*z*v),0,0,0,0,1}};
}
static void rigid(const std::array<float,16>& m){
    for(float v:m)assert(std::isfinite(v));
    for(unsigned a=0;a<3;++a)for(unsigned b=0;b<3;++b){
        float dot=0;for(unsigned k=0;k<3;++k)dot+=m[a*4+k]*m[b*4+k];
        assert(std::fabs(dot-(a==b?scale*scale:0))<.00001f);
    }
}
static float angleBetween(const std::array<float,16>& a,const std::array<float,16>& b){
    const auto qa=bagRotation(a,scale),qb=bagRotation(b,scale);
    const BagQuaternion inverse{{-qa[0],-qa[1],-qa[2],qa[3]}};
    const auto difference=bagRotationProduct(qb,inverse);
    const float sine=std::sqrt(difference[0]*difference[0]+difference[1]*difference[1]+difference[2]*difference[2]);
    return 2*std::atan2(sine,std::fabs(difference[3]));
}
static float anchorVertical(const std::array<float,16>& wanted,const std::array<float,16>& actual,float pivot=0,
                            std::array<float,3> up={{0,0,1}},float physicalScale=scale){
    const float length=bagVectorLength(up);for(float& v:up)v/=length;
    std::array<float,3> delta{};float vertical=0;
    for(unsigned axis=0;axis<3;++axis){
        const float expected=wanted[12+axis]+pivot*wanted[8+axis];
        const float emitted=actual[12+axis]+pivot*actual[8+axis];
        delta[axis]=emitted-expected;vertical+=delta[axis]*up[axis];
    }
    for(unsigned axis=0;axis<3;++axis)assert(std::fabs(delta[axis]-vertical*up[axis])<.000001f);
    assert(std::fabs(vertical)<=1.239f*physicalScale*.04001f);
    return vertical;
}
static std::array<float,16> animated(unsigned time){
    const float t=time*.001f,cycle=t*6.283185307f*1.8f;
    auto p=tiltedPose(7*radians*std::sin(cycle));
    p[12]=.1f*std::sin(cycle);p[13]=.06f*std::cos(cycle);p[14]=.04f*std::sin(2*cycle);
    return p;
}
int main(){
    // Sideways and inset movement is never filtered, including fast changes.
    // A stationary orientation cannot acquire a synthetic running oscillation.
    for(bool running:{false,true}){
        BagMotion state;
        for(unsigned t=0;t<=3000;t+=10){
            auto input=pose(.4f*std::sin(t*.015f),.25f);input[13]=.2f*std::cos(t*.02f);input[14]=.1f;
            auto output=input;smoothBagMotion(state,output,scale,t,1,1,running);
            assert(output==input);assert(state.position[0]==input[12]);rigid(output);
        }
    }
    // Rotation is driven by the animation itself, alternates with that motion,
    // stays small and preserves a chosen nonzero strap pivot as well as scale.
    for(float pivot:{0.f,.06195f,.24f}){
        BagMotion state;float minimum=0,maximum=0,largest=0;
        for(unsigned t=0;t<=4000;t+=10){
            const auto input=animated(t);auto output=input;
            smoothBagMotion(state,output,scale,t,1,1,true,pivot);
            anchorVertical(input,output,pivot);rigid(output);
            const float error=angleBetween(input,output);assert(error<=3.0001f*radians);
            if(t>500){minimum=std::fmin(minimum,state.angularOffset[2]);maximum=std::fmax(maximum,state.angularOffset[2]);largest=std::fmax(largest,error);}
            const auto once=output;output=input;
            smoothBagMotion(state,output,scale,t,1,1,true,pivot);assert(output==once);
        }
        assert(minimum<-.2f*radians&&maximum>.2f*radians);
        assert(largest>.5f*radians&&largest<2.5f*radians);
        // Stopping the body quickly removes motion, even if the running flag
        // is still set. No phase/timer continues oscillating an idle bag.
        auto input=animated(4000),output=input;
        for(unsigned t=4010;t<=4400;t+=10){output=input;smoothBagMotion(state,output,scale,t,1,1,true,pivot);}
        assert(angleBetween(input,output)<.06f*radians);
        for(unsigned t=4410;t<=5000;t+=10){output=input;smoothBagMotion(state,output,scale,t,1,1,true,pivot);}
        assert(angleBetween(input,output)<.0001f*radians);
        assert(std::fabs(anchorVertical(input,output,pivot))<.000001f);
    }
    // Matching animation traces have closely matching restrained responses at
    // 1..100ms render intervals; slow frames cannot make the spring explode.
    for(unsigned duration:{200u,400u,800u,1200u,2000u}){
        std::array<float,16> reference{};
        for(unsigned interval:{1u,2u,4u,10u,20u,40u,100u}){
            BagMotion state;auto output=animated(0);smoothBagMotion(state,output,scale,0,1,1,true);
            for(unsigned t=interval;t<=duration;t+=interval){const auto input=animated(t);output=input;smoothBagMotion(state,output,scale,t,1,1,true);anchorVertical(input,output);rigid(output);}
            if(interval==1)reference=output;
            const float tolerance=(interval<=20?.25f:interval<=40?.5f:1.1f)*radians;
            assert(angleBetween(reference,output)<tolerance);
            assert(std::fabs(reference[14]-output[14])<(interval<=20?.003f:interval<=40?.006f:.015f));
        }
    }
    // Severe crouch/turn cuts follow their base pose immediately. A radial
    // three-degree bound applies to the entire emitted rotation at every rate.
    for(unsigned interval:{1u,10u,40u,100u,250u}){
        BagMotion state;float greatest=0;
        for(unsigned t=0;t<=4000;t+=interval){
            auto input=tiltedPose((t/125)%2?1.4f:-1.4f);input[12]=.15f*std::sin(t*.01f);input[14]=t%500<250?-.2f:0;
            auto output=input;smoothBagMotion(state,output,scale,t,77,1,true,.24f);
            anchorVertical(input,output,.24f);rigid(output);
            const float angle=angleBetween(input,output);assert(angle<=3.0001f*radians);greatest=std::fmax(greatest,angle);
        }
        if(interval>=10&&interval<=100)assert(greatest>.5f*radians);
    }
    // Irregular render times and alternating multi-axis impulses stay bounded;
    // a final stationary pose settles without residual oscillation.
    {
        BagMotion irregular;const unsigned frames[]={1,3,7,60,100,16,40};
        unsigned t=0,index=0;auto output=animated(t);smoothBagMotion(irregular,output,scale,t,1,1,true);
        while(t<5000){t+=frames[index++%7];const auto input=animated(t);output=input;smoothBagMotion(irregular,output,scale,t,1,1,true);rigid(output);anchorVertical(input,output);assert(angleBetween(input,output)<=3.0001f*radians);}
        const auto input=animated(t);
        for(unsigned end=t+600;t<end;){t+=10;output=input;smoothBagMotion(irregular,output,scale,t,1,1,true);}
        assert(angleBetween(input,output)<.01f*radians);
        assert(std::fabs(anchorVertical(input,output))<.0001f);
    }
    // A vertical rise leaves the pack slightly behind, then settles back onto
    // the current mount. Running does not create bounce without animation.
    {
        BagMotion state;auto p=pose();smoothBagMotion(state,p,scale,0,1,1,true);
        auto input=pose();input[14]=.03f;p=input;smoothBagMotion(state,p,scale,16,1,1,true);
        assert(anchorVertical(input,p)<-.0001f);
        for(unsigned t=32;t<=616;t+=8){p=input;smoothBagMotion(state,p,scale,t,1,1,true);anchorVertical(input,p);}
        assert(std::fabs(p[14]-input[14])<.0001f);
    }
    // World vertical is independent of the bag's tilt. The same animation
    // displacement gives the same response with its local up pointing sideways.
    for(float tilt:{0.f,1.570796327f,2.5f}){
        BagMotion state;float below=0,above=0;
        for(unsigned t=0;t<=3000;t+=10){
            auto input=tiltedPose(tilt);input[14]=.04f*std::sin(t*.0226f);
            auto output=input;smoothBagMotion(state,output,scale,t,1,1,true);
            const float delta=anchorVertical(input,output);rigid(output);
            for(unsigned i=0;i<12;++i)assert(output[i]==input[i]);
            below=std::fmin(below,delta);above=std::fmax(above,delta);
        }
        assert(below<-.004f&&above>.004f);
    }
    // Up is supplied in the caller's model space. A non-axis-aligned or
    // changing direction still confines all strap displacement to that line.
    for(bool changing:{false,true}){
        BagMotion state;float greatest=0;
        for(unsigned t=0;t<=3000;t+=10){
            const float angle=changing?t*.001f:.7f;
            const std::array<float,3> up{{std::sin(angle),0,std::cos(angle)}};
            auto input=animated(t);auto output=input;
            smoothBagMotion(state,output,scale,t,1,1,true,.24f,up);
            greatest=std::fmax(greatest,std::fabs(anchorVertical(input,output,.24f,up)));rigid(output);
            assert(angleBetween(input,output)<=3.0001f*radians);
        }
        assert(greatest>.003f);
    }
    // Changing the world-up mapping does not itself move a stationary bag;
    // only real attachment animation supplies spring energy.
    {
        BagMotion state;const auto input=tiltedPose(1.3f);
        for(unsigned t=0;t<=3000;t+=10){
            const std::array<float,3> up{{3*std::sin(t*.01f),0,3*std::cos(t*.01f)}};
            auto output=input;smoothBagMotion(state,output,scale,t,1,1,true,.24f,up);
            assert(output==input);
        }
    }
    // Perpendicular motion cannot excite vertical give for an arbitrary up.
    {
        const std::array<float,3> up{{.6f,0,.8f}};BagMotion state;
        for(unsigned t=0;t<=3000;t+=10){
            auto input=tiltedPose(1.7f);const float movement=.05f*std::sin(t*.025f);
            input[12]=.8f*movement;input[14]=-.6f*movement;
            auto output=input;smoothBagMotion(state,output,scale,t,1,1,true,0,up);
            assert(std::fabs(anchorVertical(input,output,0,up))<.000001f);
        }
    }
    // With a nonuniform actor transform, the direction of world up and the
    // measurement of vertical displacement need not be parallel. A horizontal
    // world trace must not create bounce, nor affect a simultaneous vertical one.
    for(bool bouncing:{false,true}){
        const std::array<float,3> up{{.6f,0,.8f}},measure{{1,0,.5f}};
        BagMotion state,reference;float greatest=0;
        for(unsigned t=0;t<=3000;t+=10){
            auto input=tiltedPose(1.7f),pure=input;
            const float side=.05f*std::sin(t*.025f),vertical=bouncing?.03f*std::sin(t*.019f):0;
            pure[12]=.6f*vertical;pure[14]=.8f*vertical;
            input[12]=pure[12]+.5f*side;input[14]=pure[14]-side;
            auto output=input,expected=pure;
            smoothBagMotion(state,output,scale,t,1,1,true,0,up,&measure);
            smoothBagMotion(reference,expected,scale,t,1,1,true,0,up,&measure);
            const float delta=anchorVertical(input,output,0,up);
            assert(std::fabs(delta-anchorVertical(pure,expected,0,up))<.000001f);
            greatest=std::fmax(greatest,std::fabs(delta));
        }
        if(bouncing)assert(greatest>.003f);
        else assert(greatest<.000001f);
    }
    // Size changes scale the travel budget, never the perpendicular position.
    for(float physicalScale:{.1125f,.3825f,.45f,.9f}){
        BagMotion state;float greatest=0;
        for(unsigned t=0;t<=3000;t+=16){
            auto input=pose();for(unsigned i=0;i<12;++i)input[i]*=physicalScale/scale;
            input[14]=(t/80)%2?.2f:-.2f;auto output=input;
            smoothBagMotion(state,output,physicalScale,t,1,1,true);
            greatest=std::fmax(greatest,std::fabs(anchorVertical(input,output,0,{{0,0,1}},physicalScale)));
        }
        assert(greatest>.01f*physicalScale);
    }
    // Downward-momentum lift has its own envelope, independent of gait rotation.
    // A stationary attachment can lift with descent input, then settle on landing.
    // Equal timestamps never advance it and initialization never snaps upward.
    float referenceWeight=0;
    for(unsigned interval:{1u,10u,20u,100u,200u}){
        BagMotion flight;auto p=pose();smoothBagMotion(flight,p,scale,0,1,1,false,0,{{0,0,1}},nullptr,true);
        assert(flight.airborneWeight==0&&p==pose());
        for(unsigned t=interval;t<=600;t+=interval){
            p=pose();smoothBagMotion(flight,p,scale,t,1,1,false,0,{{0,0,1}},nullptr,true);
            assert(flight.airborneWeight>=0&&flight.airborneWeight<=1);
            const auto weight=flight.airborneWeight,velocity=flight.airborneVelocity;
            p=pose();smoothBagMotion(flight,p,scale,t,1,1,false,0,{{0,0,1}},nullptr,true);
            assert(flight.airborneWeight==weight&&flight.airborneVelocity==velocity);
        }
        assert(flight.airborneWeight>.99f);
        if(interval==1)referenceWeight=flight.airborneWeight;
        else assert(std::fabs(flight.airborneWeight-referenceWeight)<.00001f);
        for(unsigned t=600+interval;t<=1200;t+=interval){
            p=pose();smoothBagMotion(flight,p,scale,t,1,1);
            assert(flight.airborneWeight>=0&&flight.airborneWeight<=1);
        }
        assert(flight.airborneWeight<.001f);
        p=pose();smoothBagMotion(flight,p,scale,1501,1,1,false,0,{{0,0,1}},nullptr,true);
        assert(flight.airborneWeight==0&&flight.airborneVelocity==0);
    }
    // Descent strength is continuous: partial falling speed does not switch
    // the bag straight to full lift, while ascent/ground input stays at rest.
    for(float input:{0.f,.25f,.5f,1.f}){
        BagMotion falling;auto p=pose();smoothBagMotion(falling,p,scale,0,1,1);
        for(unsigned t=16;t<=1600;t+=16){
            p=pose();smoothBagMotion(falling,p,scale,t,1,1,false,0,{{0,0,1}},nullptr,input);
            assert(falling.airborneWeight>=0&&falling.airborneWeight<=input+.000001f);
        }
        assert(std::fabs(falling.airborneWeight-input)<.00001f);
    }
    // Strap tension grows near the edge and dissipates outward energy without
    // bouncing off a hard stop. No external driver means no growing oscillation.
    float fraction[2]{};
    for(unsigned i=0;i<2;++i){
        BagMotion state;auto p=pose();smoothBagMotion(state,p,scale,0,1,1);
        const float initial=(i?2.8f:1.f)*radians;state.angularOffset[0]=initial;
        p=pose();smoothBagMotion(state,p,scale,16,1,1);fraction[i]=std::fabs(state.angularOffset[0])/initial;
        float previous=std::fabs(state.angularOffset[0]);
        for(unsigned t=32;t<=500;t+=16){p=pose();smoothBagMotion(state,p,scale,t,1,1);const float current=std::fabs(state.angularOffset[0]);assert(current<=previous+.000001f);previous=current;}
        assert(previous<.001f*radians);
    }
    assert(fraction[1]<fraction[0]-.03f);
    // First load, race/model changes, long gaps and teleports discard old motion.
    BagMotion state;auto p=pose();smoothBagMotion(state,p,scale,1000,1,1);
    p=pose(0,.3f);smoothBagMotion(state,p,scale,1016,1,1);assert(angleBetween(p,pose(0,.3f))>.1f*radians);
    for(unsigned which=0;which<4;++which){
        BagMotion changed=state;auto input=pose(which==3?3.f:.1f,.8f),output=input;
        smoothBagMotion(changed,output,scale,which==2?2000:1032,which==0?2:1,which==1?2:1,true);
        assert(output==input&&bagVectorLength(changed.angularOffset)==0&&changed.runWeight==0&&changed.verticalOffset==0&&changed.verticalVelocity==0);
    }
    // Unsigned wraparound yields the same motion as an ordinary 16ms frame.
    BagMotion wrapped,ordinary;auto a=pose(),b=a;
    smoothBagMotion(wrapped,a,scale,0xfffffff0u,1,1);smoothBagMotion(ordinary,b,scale,1000,1,1);
    a=pose(.008f,.12f);a[14]=.02f;b=a;smoothBagMotion(wrapped,a,scale,0,1,1);smoothBagMotion(ordinary,b,scale,1016,1,1);assert(a==b);
    // A second draw at the same time neither advances nor overwrites the raw
    // animation sample. This also prevents duplicate passes changing velocity.
    const BagMotion history=ordinary;auto anotherPose=pose(.02f,.13f);
    smoothBagMotion(ordinary,anotherPose,scale,1016,1,1);
    assert(ordinary.rotation==history.rotation&&ordinary.position==history.position&&ordinary.angularVelocity==history.angularVelocity&&ordinary.verticalAnchor==history.verticalAnchor&&ordinary.verticalOffset==history.verticalOffset&&ordinary.verticalVelocity==history.verticalVelocity);
    // Shortest arc through +/-180 degrees and equivalent quaternion signs.
    state={};p=pose(0,179*radians);smoothBagMotion(state,p,scale,0,1,1);
    auto wanted=pose(0,-179*radians);p=wanted;smoothBagMotion(state,p,scale,16,1,1);rigid(p);assert(angleBetween(p,wanted)<1*radians);
    BagMotion antipodal=state;for(float& value:antipodal.rotation)value=-value;
    a=pose(0,-178*radians);b=a;smoothBagMotion(state,a,scale,32,1,1);smoothBagMotion(antipodal,b,scale,32,1,1);
    for(unsigned i=0;i<16;++i)assert(std::fabs(a[i]-b[i])<.000001f);
    // Separate bags/preview histories never influence one another.
    BagMotion idle;auto still=pose();smoothBagMotion(idle,still,scale,1000,999,3,true);assert(still==pose());
    for(unsigned t=1010;t<=2000;t+=10){still=pose();smoothBagMotion(idle,still,scale,t,999,3,true);assert(still==pose());}
    for(unsigned sex:{0u,1u})assert(bagMount(1,2,sex).rightDegrees==15&&bagMount(1,2,sex).raisedOrigin==0);
    assert(bagMount(2,2,0).rightDegrees==0&&bagMount(1,1,0).raisedOrigin==0);
    std::cout<<"PASS: world-vertical strap give, exact perpendicular position, animation-driven restrained rotation, rapid settling, stiff edge catch, variable frame times, rigid scale, pivot preservation, independent histories and quaternion seams\n";
}
