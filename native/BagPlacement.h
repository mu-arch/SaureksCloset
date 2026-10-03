#pragma once
#include <array>
#include "BagFits.h"
#include "BagMounts.h"
#include "BagMotion.h"
#include "BagCoordinates.h"
#include "BagAirLift.h"
#include "BagMountFlop.h"
#include "BagTuning.h"
#include "BagResponse.h"
#include "BagAmplitude.h"
// Dark Schoolbag is authored upright (+Z), outward (-X), with the origin at
// the center of its back panel. Point 28 supplies the back surface; its
// parent bone supplies the torso orientation, without a shield's sheath tilt.
static bool bagPlacement(const std::array<float,16>& renderedBack,const std::array<float,16>& renderedTorso,
                         const std::array<float,16>& local,const std::array<float,3>& anchor,std::array<float,16>& out,
                         unsigned bag=1,BagMotion* motion=nullptr,std::uint32_t now=0,std::uintptr_t model=0,bool running=false,
                         const BagMatrix* modelToRender=nullptr,const BagMatrix* worldToRender=nullptr,float airLiftTarget=0,
                         const BagTuningEntry* instanceFits=nullptr,unsigned baseMount=0,unsigned identity=0,
                         BagResponse* response=nullptr,const BagResponseProfile* responseProfile=nullptr,float flight=0,bool activeMotion=true,
                         const BagMatrix* fittedOverride=nullptr,bool softBody=true){
    BagMatrix back=renderedBack,torso=renderedTorso,renderToModel{};
    if(modelToRender){
        if(!bagAffineInverse(*modelToRender,renderToModel))return false;
        // Build 5875 initializes root bones from CM2Model+0xFC at 0x714945.
        // Therefore +0x94 bone matrices already carry model/view transforms.
        // Remove those BEFORE fitting, quaternion conversion or motion history.
        back=bagMatrixProduct(renderToModel,renderedBack);
        torso=bagMatrixProduct(renderToModel,renderedTorso);
    }
    for(const auto* m:{static_cast<const BagMatrix*>(&back),static_cast<const BagMatrix*>(&torso),&local}){
        for(float v:*m)if(!std::isfinite(v))return false;
        if(std::fabs((*m)[15]-1.f)>.001f)return false;
    }
    const BagFit* fit=nullptr;
    for(const auto& candidate:bagFits){
        bool matches=true;for(unsigned i=0;i<3;++i)if(!std::isfinite(anchor[i])||std::fabs(candidate.anchor[i]-anchor[i])>.00001f)matches=false;
        if(matches){fit=&candidate;break;}
    }
    if(!fit)return false;
    const auto fitIndex=static_cast<unsigned>(fit-bagFits);
    const auto mount=baseMount?BagMount{}:bagMount(bag,fitIndex/2+1,fitIndex%2);
    BagTuningValues tuning;
    if(instanceFits){if(!bagInstanceTuningDefaults(baseMount,fitIndex/2+1,fitIndex%2,tuning))return false;}
    else if(!bagTuningDefaults(bag,fitIndex/2+1,fitIndex%2,tuning))return false;
    const auto& override=instanceFits?instanceFits[fitIndex]:bagTuningEntries[fitIndex];
    if(override.enabled)tuning=override.values;
    // Size changes leave the fitted center of the back panel in place.
    const float size=bagModelScale*(tuning.scale/100.f);
    // Keep the pack's physical size independent of race height and bone scale.
    // Retain the animated torso directions and fitted contact depth.
    std::array<float,16> orientation=torso;
    for(unsigned column:{0u,4u,8u}){
        const float length=std::sqrt(torso[column]*torso[column]+torso[column+1]*torso[column+1]+torso[column+2]*torso[column+2]);
        if(!std::isfinite(length)||length<.000001f)return false;
        for(unsigned axis=0;axis<3;++axis)orientation[column+axis]/=length;
    }
    const float pitch=tuning.pitch*.01745329252f,roll=tuning.roll*.01745329252f;
    const float cp=std::cos(pitch),sp=std::sin(pitch),cr=std::cos(roll),sr=std::sin(roll);
    // +X points into the player, -Y is their right. Rotate about the straps:
    // the negative-Z bottom moves inward and toward the opposite back side.
    std::array<float,9> rotation{{cp,sr*sp,cr*sp,0,cr,-sr,-sp,sr*cp,cr*cp}};
    // Twist around the fitted bag's upright axis, preserving the existing
    // pitch/roll calculation exactly when no yaw override is requested.
    if(tuning.yaw!=0){
        const float yaw=tuning.yaw*.01745329252f,c=std::cos(yaw),s=std::sin(yaw);
        const auto original=rotation;
        for(unsigned row=0;row<3;++row){
            rotation[row]=original[row]*c+original[3+row]*s;
            rotation[3+row]=-original[row]*s+original[3+row]*c;
        }
    }
    std::array<float,16> target{};target[15]=1;
    for(unsigned axis=0;axis<3;++axis){
        for(unsigned col=0;col<3;++col)for(unsigned k=0;k<3;++k)
            target[col*4+axis]+=orientation[k*4+axis]*rotation[col*3+k]*size;
        target[12+axis]=back[12+axis]+(baseMount?0:fit->depth)*torso[axis]+tuning.inset*orientation[axis]
            +tuning.left*orientation[4+axis]+tuning.up*orientation[8+axis]
            -mount.raisedOrigin*target[8+axis];
    }
    // Cancel the child factory's local pose: the final product must be target,
    // even if its local matrix contains scale/rotation or an origin offset.
    BagMatrix inverseLocal;
    if(!bagAffineInverse(local,inverseLocal))return false;
    // A body binding supplies the exact live skin contact in model space.
    // Saved numeric fits still describe the same neutral placement.
    if(fittedOverride){
        BagMatrix validation;
        if(!bagAffineInverse(*fittedOverride,validation))return false;
        target=*fittedOverride;
    }
    const auto fittedTarget=target;
    if(motion){
        std::array<float,3> worldUp{{0,0,1}};
        std::array<float,3> verticalMeasure=worldUp;
        const bool directionReady=!worldToRender || (modelToRender&&bagWorldUpInModel(renderToModel,*worldToRender,worldUp,&verticalMeasure));
        // Revision changes reset once for the matching bag/race/sex. Repeated
        // identical Lua setters leave the motion history untouched.
        if(response&&responseProfile){
            if(softBody&&tuning.motion&&tuning.amplitude>0&&directionReady){
                const std::uint64_t fitKey=(std::uint64_t(identity)<<48)|(std::uint64_t(baseMount)<<40)|
                    (std::uint64_t(fitIndex)<<32)|override.revision;
                BagMatrix sample=target,renderToWorld;
                auto gravity=worldUp;
                bool sampleReady=true;
                if(modelToRender&&worldToRender){
                    sampleReady=bagAffineInverse(*worldToRender,renderToWorld);
                    if(sampleReady){
                        sample=bagMatrixProduct(bagMatrixProduct(renderToWorld,*modelToRender),target);
                        gravity={{0,0,1}};
                    }
                }
                // Include root travel/jumps and the offset from the mounting
                // bone. Camera motion cancels before sample history is stored.
                const float sampleScale=std::sqrt(sample[8]*sample[8]+sample[9]*sample[9]+sample[10]*sample[10]);
                if(sampleReady)updateBagResponse(*response,sample,sampleScale,now,model,fitKey,*responseProfile,gravity,flight,instanceFits?identity:0,activeMotion);
                else *response={};
            }else *response={};
        }
        // Keep the original mounting-point momentum and upward/outward fall
        // lift. Local fabric controls are sampled from the unmodified mount
        // above, so this rigid secondary movement cannot feed back into itself.
        if(tuning.motion&&tuning.amplitude>0&&directionReady){
            const float mass=bagModelMass(responseProfile,size);
            const auto fitted=target;
            const float contactTop=responseProfile?responseProfile->top:.6195f;
            const float pivot=(fittedOverride||baseMount)?contactTop:mount.raisedOrigin;
            smoothBagMotion(*motion,target,size,now,model,
                (identity?identity:bag)*32+fitIndex+(override.revision<<6),running,pivot,worldUp,&verticalMeasure,airLiftTarget,instanceFits?identity:0,activeMotion,fittedOverride!=nullptr,mass);
            const float bobGain=1.f+.75f*motion->runWeight;
            if(fittedOverride)flopBagAtContact(target,fitted,worldUp,contactTop,
                (responseProfile?responseProfile->height:bagMotionHeight)*size,motion->verticalBob*bobGain,size,*motion,now,mass);
            if(motion->airborneWeight>.000001f){
                BagMatrix worldToModel{{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1}},modelToWorld;
                if(modelToRender&&worldToRender)worldToModel=bagMatrixProduct(renderToModel,*worldToRender);
                const std::array<float,3> contact{{0,0,contactTop}};
                if(bagAffineInverse(worldToModel,modelToWorld))
                    liftBagInGravity(target,fitted,modelToWorld,worldToModel,
                        {{-fitted[0],-fitted[1],-fitted[2]}},motion->airborneWeight,fittedOverride?&contact:nullptr);
            }
            // Make the running bob visible without speeding up its cycle or
            // moving the body pin. Ease amplitude in/out with locomotion.
            if(softBody){
                if(fittedOverride&&response)bagResponseBob(*response,target,worldUp,motion->verticalBob*bobGain);
            }else{
                // Rigid bags translate as one object. No control bone may
                // stretch the top, body, straps or flap to hold a fixed pin.
                const float bob=motion->verticalBob*(fittedOverride?bobGain:bobGain-1.f);
                for(unsigned axis=0;axis<3;++axis)target[12+axis]+=worldUp[axis]*bob;
            }
        }
        else if(motion->ready)*motion={};
    }
    if(motion){
        bagMotionAmplitude(fittedTarget,target,tuning.motion?tuning.amplitude:0,responseProfile?responseProfile->top:.6195f);
        if(response)response->outputAmplitude=tuning.motion?tuning.amplitude:0;
    }
    // Restore THIS frame's model/view transform, without filtering camera or
    // character travel. Keep handedness/scale outside quaternion conversion.
    if(modelToRender)target=bagMatrixProduct(*modelToRender,target);
    out=bagMatrixProduct(target,inverseLocal);
    for(float v:out)if(!std::isfinite(v))return false;
    return true;
}
