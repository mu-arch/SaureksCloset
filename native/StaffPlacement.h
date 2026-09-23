#pragma once
#include "BagCoordinates.h"
// Move a selected stored staff toward the torso without changing its native
// orientation, scale, attachment ownership or held pose. All fitting is done
// in actor space so preview zoom and world/camera transforms stay independent.
static bool staffContactPlacement(const BagMatrix& attachment,const BagMatrix& local,
                                  const BagMatrix& torso,const BagMatrix& modelToRender,
                                  float surfaceDepth,const std::array<float,4>& shaftBounds,BagMatrix& out){
    if(!std::isfinite(surfaceDepth)||surfaceDepth<=0)return false;
    for(float value:shaftBounds)if(!std::isfinite(value))return false;
    if(shaftBounds[0]>shaftBounds[1]||shaftBounds[2]>shaftBounds[3])return false;
    BagMatrix inverseRender,validation;
    if(!bagAffineInverse(modelToRender,inverseRender)||!bagAffineInverse(attachment,validation)||
       !bagAffineInverse(local,validation)||!bagAffineInverse(torso,validation))return false;
    const auto actorAttachment=bagMatrixProduct(inverseRender,attachment);
    const auto actorStaff=bagMatrixProduct(actorAttachment,local);
    const auto actorTorso=bagMatrixProduct(inverseRender,torso);
    std::array<float,3> inward{{actorTorso[0],actorTorso[1],actorTorso[2]}};
    const float length=std::sqrt(inward[0]*inward[0]+inward[1]*inward[1]+inward[2]*inward[2]);
    if(!std::isfinite(length)||length<.000001f)return false;
    for(float& value:inward)value/=length;
    float yProjection=0,zProjection=0,originOffset=0;
    for(unsigned axis=0;axis<3;++axis){
        yProjection+=inward[axis]*actorStaff[4+axis];
        zProjection+=inward[axis]*actorStaff[8+axis];
        originOffset+=inward[axis]*(actorStaff[12+axis]-actorAttachment[12+axis]);
    }
    const float support=originOffset+yProjection*shaftBounds[yProjection>=0?1:0]+
        zProjection*shaftBounds[zProjection>=0?3:2];
    float distance=surfaceDepth-support-.012f;
    if(!std::isfinite(distance)||distance<=.000001f)return false;
    if(distance>.12f)distance=.12f;
    out=attachment;
    for(unsigned axis=0;axis<3;++axis){
        out[12+axis]+=distance*(modelToRender[axis]*inward[0]+
            modelToRender[4+axis]*inward[1]+modelToRender[8+axis]*inward[2]);
        if(!std::isfinite(out[12+axis]))return false;
    }
    return true;
}
