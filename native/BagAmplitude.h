#pragma once
#include "BagMotion.h"
// Scale only secondary movement about its contact, preserving rigid lengths.
// Never blend matrix bases: that would squash rigid bags during rotation.
static void bagMotionAmplitude(const BagMatrix& fitted,BagMatrix& moving,float amount,float contact){
    if(amount==1)return;
    if(amount<=0){moving=fitted;return;}
    float scale=std::sqrt(fitted[0]*fitted[0]+fitted[1]*fitted[1]+fitted[2]*fitted[2]);
    auto a=bagRotation(fitted,scale),b=bagRotation(moving,scale);
    float dot=bagRotationDot(a,b);if(dot<0){for(auto& v:b)v=-v;dot=-dot;}
    dot=std::fmax(-1.f,std::fmin(1.f,dot));
    const float angle=std::acos(dot),s=std::sin(angle);
    const float wa=s>.00001f?std::sin((1-amount)*angle)/s:1-amount,wb=s>.00001f?std::sin(amount*angle)/s:amount;
    BagQuaternion q;float norm=0;for(unsigned i=0;i<4;++i){q[i]=wa*a[i]+wb*b[i];norm+=q[i]*q[i];}
    norm=std::sqrt(norm);for(auto& v:q)v/=norm;
    const float x=q[0],y=q[1],z=q[2],w=q[3];
    BagMatrix result{{1-2*(y*y+z*z),2*(x*y+z*w),2*(x*z-y*w),0,
        2*(x*y-z*w),1-2*(x*x+z*z),2*(y*z+x*w),0,
        2*(x*z+y*w),2*(y*z-x*w),1-2*(x*x+y*y),0,0,0,0,1}};
    for(unsigned col=0;col<3;++col)for(unsigned axis=0;axis<3;++axis)result[4*col+axis]*=scale;
    for(unsigned axis=0;axis<3;++axis){
        const float anchor=fitted[12+axis]+contact*fitted[8+axis];
        const float changed=moving[12+axis]+contact*moving[8+axis];
        result[12+axis]=anchor+(changed-anchor)*amount-contact*result[8+axis];
    }
    moving=result;
}
