#pragma once
#include "BagCoordinates.h"

// Gravity-relative jump/fall pitch is separate from the small animation sway.
// These bounds describe the Runecloth Bag's back panel, whose origin is its
// center. Select the upper part of that panel using gravity, including when
// the user has fitted the bag sideways.
static bool bagAirAffine(const BagMatrix& matrix){
    for(float value:matrix)if(!std::isfinite(value))return false;
    if(std::fabs(matrix[3])>.00001f||std::fabs(matrix[7])>.00001f||std::fabs(matrix[11])>.00001f||std::fabs(matrix[15]-1)>.001f)return false;
    const float determinant=matrix[0]*(matrix[5]*matrix[10]-matrix[9]*matrix[6])
        -matrix[4]*(matrix[1]*matrix[10]-matrix[9]*matrix[2])
        +matrix[8]*(matrix[1]*matrix[6]-matrix[5]*matrix[2]);
    return std::isfinite(determinant)&&std::fabs(determinant)>.000000001f;
}
static float bagAirFade(float amount){
    const float value=std::fmax(0.f,std::fmin(1.f,amount));
    return value*value*(3-2*value);
}
static bool liftBagInGravity(BagMatrix& pose,const BagMatrix& fitted,const BagMatrix& modelToWorld,
                             const BagMatrix& worldToModel,std::array<float,3> outwardModel,float weight){
    if(!std::isfinite(weight)||weight<=0||!bagAirAffine(pose)||!bagAirAffine(fitted)
        ||!bagAirAffine(modelToWorld)||!bagAirAffine(worldToModel))return false;
    for(float value:outwardModel)if(!std::isfinite(value))return false;
    // Work around the actor's origin. Scene/world positions can be very large;
    // they have no role in this rotation and would lose small bag offsets.
    auto worldBasis=modelToWorld,modelBasis=worldToModel;
    for(unsigned row=0;row<3;++row){worldBasis[12+row]=0;modelBasis[12+row]=0;}
    const auto raw=bagMatrixProduct(worldBasis,fitted);
    const auto current=bagMatrixProduct(worldBasis,pose);
    if(!bagAirAffine(raw)||!bagAirAffine(current))return false;

    std::array<float,3> outward{};
    for(unsigned row=0;row<3;++row)for(unsigned column=0;column<3;++column)
        outward[row]+=modelToWorld[column*4+row]*outwardModel[column];
    const float outwardLength=std::sqrt(outward[0]*outward[0]+outward[1]*outward[1]+outward[2]*outward[2]);
    const float horizontalLength=std::sqrt(outward[0]*outward[0]+outward[1]*outward[1]);
    if(!std::isfinite(outwardLength)||!std::isfinite(horizontalLength)||outwardLength<.000001f
        ||horizontalLength<outwardLength*.00001f)return false;
    const float outwardFade=bagAirFade(horizontalLength/(outwardLength*.15f));
    outward={{outward[0]/horizontalLength,outward[1]/horizontalLength,0}};

    // The support point of an inset ellipse stays on the back panel and moves
    // smoothly from its authored top to its side as the fitted bag is rolled.
    // Use complete world-space columns: their unequal scale matters here.
    constexpr float halfWidth=.441f,halfHeight=.6195f,inset=.75f;
    const float widthUp=halfWidth*raw[6],heightUp=halfHeight*raw[10];
    const float support=std::sqrt(widthUp*widthUp+heightUp*heightUp);
    float panelSizeSquared=0;
    for(unsigned row=0;row<3;++row)panelSizeSquared+=halfWidth*halfWidth*raw[4+row]*raw[4+row]
        +halfHeight*halfHeight*raw[8+row]*raw[8+row];
    const float panelSize=std::sqrt(panelSizeSquared);
    if(!std::isfinite(support)||!std::isfinite(panelSize)||panelSize<.000001f
        ||support<panelSize*.00001f)return false;
    const float panelFade=bagAirFade(support/(panelSize*.15f));
    const std::array<float,3> pivot{{0,inset*halfWidth*widthUp/support,inset*halfHeight*heightUp/support}};
    std::array<float,3> restingPivot{},currentPivot{};
    for(unsigned row=0;row<3;++row)for(unsigned column=0;column<3;++column){
        restingPivot[row]+=raw[column*4+row]*pivot[column];
        currentPivot[row]+=current[column*4+row]*pivot[column];
    }
    // Negative resting angles lean the bottom into the body. Compute the
    // original fitted swing, then double its entire angle at every frame,
    // including the inward-lean compensation and the eased landing response.
    constexpr float radians=.01745329252f;
    const float restingAngle=std::atan2(-restingPivot[0]*outward[0]-restingPivot[1]*outward[1],restingPivot[2]);
    const float travel=std::fmax(0.f,std::fmin(45*radians,
        std::fmin(20*radians+std::fmax(0.f,-restingAngle),75*radians-restingAngle)));
    const float angle=2.f*travel*std::fmin(1.f,weight)*outwardFade*panelFade;
    if(!std::isfinite(angle)||angle<.0000001f)return false;
    // O cross world-up is always a horizontal hinge, regardless of bag tilt,
    // root tilt, camera orientation, or the actor's nonuniform scale.
    const std::array<float,3> hinge{{outward[1],-outward[0],0}};
    const float cosine=std::cos(angle),sine=std::sin(angle);
    const auto rotate=[&](const std::array<float,3>& value){
        const float dot=hinge[0]*value[0]+hinge[1]*value[1];
        return std::array<float,3>{{
            value[0]*cosine+hinge[1]*value[2]*sine+hinge[0]*dot*(1-cosine),
            value[1]*cosine-hinge[0]*value[2]*sine+hinge[1]*dot*(1-cosine),
            value[2]*cosine+(hinge[0]*value[1]-hinge[1]*value[0])*sine}};
    };
    auto lifted=current;
    for(unsigned column=0;column<3;++column){
        const auto value=rotate({{current[column*4],current[column*4+1],current[column*4+2]}});
        for(unsigned row=0;row<3;++row)lifted[column*4+row]=value[row];
    }
    const auto rotatedPivot=rotate(currentPivot);
    for(unsigned row=0;row<3;++row)lifted[12+row]+=currentPivot[row]-rotatedPivot[row];
    const auto output=bagMatrixProduct(modelBasis,lifted);
    if(!bagAirAffine(output))return false;
    pose=output;
    return true;
}
