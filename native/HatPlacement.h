#pragma once
#include "PlacementTuning.h"

// Offsets/rotations live in the animated head attachment's coordinate frame.
// Always start with the fresh native transform, including lazy model updates.
// No position accumulation, skeleton edits or dynamics are involved.
inline bool hatPlacement(const BagMatrix& attachment,const BagTuningValues& v,BagMatrix& out){
    if(!bagTuningValid(v))return false;
    BagMatrix inverse;
    if(!bagAffineInverse(attachment,inverse))return false;
    const float p=v.pitch*.01745329252f,r=v.roll*.01745329252f,y=v.yaw*.01745329252f;
    const float cp=std::cos(p),sp=std::sin(p),cr=std::cos(r),sr=std::sin(r),cy=std::cos(y),sy=std::sin(y);
    const BagMatrix pitch{{cp,0,-sp,0,0,1,0,0,sp,0,cp,0,0,0,0,1}};
    const BagMatrix roll{{1,0,0,0,0,cr,sr,0,0,-sr,cr,0,0,0,0,1}};
    const BagMatrix yaw{{cy,sy,0,0,-sy,cy,0,0,0,0,1,0,0,0,0,1}};
    auto delta=bagMatrixProduct(yaw,bagMatrixProduct(pitch,roll));
    for(unsigned col:{0u,4u,8u})for(unsigned row=0;row<3;++row)delta[col+row]*=v.scale/100.f;
    delta[12]=v.inset;delta[13]=v.left;delta[14]=v.up;
    out=bagMatrixProduct(attachment,delta);
    for(float value:out)if(!std::isfinite(value))return false;
    return true;
}
