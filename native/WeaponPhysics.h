#pragma once
#include "BagCoordinates.h"
#include "BagMotion.h"
#include "BagAirLift.h"
#include "BagAmplitude.h"
struct WeaponPhysicsSettings{
    unsigned bounce=100,rocking=100,jumpLift=100;
};
struct WeaponSlotPhysics{
    unsigned mode=0; // 0 inherits shared defaults, 1 off, 2 on.
    WeaponPhysicsSettings settings;
};
// Run the established rigid bag spring in actor space, then restore the current
// render and child-local transforms. Only the whole weapon matrix is changed.
static bool rigidWeaponPhysics(BagMotion& motion,const BagMatrix& attachment,const BagMatrix& local,
                               const BagMatrix& render,const BagMatrix& worldToRender,BagMatrix& out,
                               std::uintptr_t child,unsigned identity,unsigned point,std::uint32_t now,
                               bool running,bool moving,float airLift,const WeaponPhysicsSettings& settings={}){
    BagMatrix inverseRender,inverseLocal;
    if(!bagAffineInverse(render,inverseRender)||!bagAffineInverse(local,inverseLocal)){motion={};return false;}
    auto target=bagMatrixProduct(inverseRender,bagMatrixProduct(attachment,local));
    BagMatrix validation;if(!bagAffineInverse(target,validation)){motion={};return false;}
    const float scale=std::sqrt(target[0]*target[0]+target[1]*target[1]+target[2]*target[2]);
    if(scale<.00001f||scale>100){motion={};return false;}
    // Quaternion motion requires an orthogonal, uniformly scaled basis. Never
    // normalize an unusual authored transform and accidentally deform a weapon.
    for(unsigned col=0;col<3;++col)for(unsigned other=col;other<3;++other){
        float dot=0;for(unsigned axis=0;axis<3;++axis)dot+=target[col*4+axis]*target[other*4+axis];
        if(std::fabs(dot-(col==other?scale*scale:0))>scale*scale*.001f){motion={};return false;}
    }
    const float determinant=target[0]*(target[5]*target[10]-target[6]*target[9])-target[4]*(target[1]*target[10]-target[2]*target[9])+target[8]*(target[1]*target[6]-target[2]*target[5]);
    if(determinant<=0){motion={};return false;}
    std::array<float,3> up,measure;
    if(!bagWorldUpInModel(inverseRender,worldToRender,up,&measure)){motion={};return false;}
    const auto fitted=target;
    smoothBagMotion(motion,target,scale,now,child,identity*64+point,running,0,up,&measure,airLift,201+identity%5,moving,true,1.f);
    // Seed a reset delay history on the first update, not on a second draw of
    // that frame. Otherwise render-pass count changes the first phase sample.
    if(bagJiggleDelay(201+identity%5)&&!motion.jiggle.count){
        target=fitted;
        smoothBagMotion(motion,target,scale,now,child,identity*64+point,running,0,up,&measure,airLift,201+identity%5,moving,true,1.f);
    }
    const float bob=motion.verticalBob*(1.f+.75f*motion.runWeight);
    for(unsigned axis=0;axis<3;++axis)target[12+axis]+=up[axis]*bob;
    // Independent gains affect only the output, so editing a slider does not
    // restart the spring or change its timing. Keep the existing response at 100%.
    std::array<float,3> displacement;
    for(unsigned axis=0;axis<3;++axis)displacement[axis]=target[12+axis]-fitted[12+axis];
    bagMotionAmplitude(fitted,target,1.5f*settings.rocking*.01f,0);
    for(unsigned axis=0;axis<3;++axis)target[12+axis]=fitted[12+axis]+displacement[axis]*(1.5f*settings.bounce*.01f);
    if(motion.airborneWeight>.00001f&&settings.jumpLift){
        const auto worldToModel=bagMatrixProduct(inverseRender,worldToRender);BagMatrix modelToWorld;
        if(bagAffineInverse(worldToModel,modelToWorld)){
            // Lift away from the body centerline, irrespective of weapon axes.
            std::array<float,3> outward{{fitted[12],fitted[13],0}};if(bagVectorLength(outward)<.01f)outward={{-1,0,0}};
            const std::array<float,3> grip{{0,0,0}};
            liftBagInGravity(target,fitted,modelToWorld,worldToModel,outward,motion.airborneWeight*(.2f*settings.jumpLift*.01f),&grip);
        }
    }
    out=bagMatrixProduct(bagMatrixProduct(render,target),inverseLocal);return true;
}
