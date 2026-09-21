#pragma once
#include "BagCoordinates.h"
#include "BagTuning.h"
// Apply a fresh stored-attachment adjustment. No accumulating matrices or
// changes to the character's bones, held weapons or gameplay equipment.
inline bool placementTuning(const BagMatrix& attachment,const BagMatrix& local,
                            const BagMatrix& torso,const BagMatrix& modelToRender,
                            const BagTuningValues& v,BagMatrix& out){
    if(!bagTuningValid(v))return false;
    BagMatrix inverseRender,inverseLocal;
    if(!bagAffineInverse(modelToRender,inverseRender)||!bagAffineInverse(local,inverseLocal))return false;
    auto base=bagMatrixProduct(inverseRender,bagMatrixProduct(attachment,local));
    auto axes=bagMatrixProduct(inverseRender,torso);
    for(unsigned col:{0u,4u,8u}){
        const float length=std::sqrt(axes[col]*axes[col]+axes[col+1]*axes[col+1]+axes[col+2]*axes[col+2]);
        if(!std::isfinite(length)||length<.000001f)return false;
        for(unsigned row=0;row<3;++row)axes[col+row]/=length;
    }
    const float p=v.pitch*.01745329252f,r=v.roll*.01745329252f,y=v.yaw*.01745329252f;
    const float cp=std::cos(p),sp=std::sin(p),cr=std::cos(r),sr=std::sin(r),cy=std::cos(y),sy=std::sin(y);
    const BagMatrix pitch{{cp,0,-sp,0,0,1,0,0,sp,0,cp,0,0,0,0,1}};
    const BagMatrix roll{{1,0,0,0,0,cr,sr,0,0,-sr,cr,0,0,0,0,1}};
    const BagMatrix yaw{{cy,sy,0,0,-sy,cy,0,0,0,0,1,0,0,0,0,1}};
    auto target=bagMatrixProduct(base,bagMatrixProduct(yaw,bagMatrixProduct(pitch,roll)));
    for(unsigned row=0;row<3;++row){
        for(unsigned col:{0u,4u,8u})target[col+row]*=v.scale/100.f;
        target[12+row]+=axes[row]*v.inset+axes[4+row]*v.left+axes[8+row]*v.up;
    }
    out=bagMatrixProduct(modelToRender,bagMatrixProduct(target,inverseLocal));
    for(float value:out)if(!std::isfinite(value))return false;
    return true;
}
