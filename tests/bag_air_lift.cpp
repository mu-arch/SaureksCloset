#include <cassert>
#include <iostream>
#include <limits>
#include "../native/BagAirLift.h"
static constexpr float radians=.01745329252f;
static constexpr float scale=.3825f;
static const BagMatrix identity{{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1}};
static BagMatrix rotate(unsigned axis,float angle){
    auto result=identity;const float c=std::cos(angle),s=std::sin(angle);
    const unsigned a=(axis+1)%3,b=(axis+2)%3;
    result[a*4+a]=c;result[b*4+b]=c;result[a*4+b]=s;result[b*4+a]=-s;
    return result;
}
static BagMatrix fitted(float inward=0,float sideways=0){
    auto result=bagMatrixProduct(rotate(1,-inward*radians),rotate(0,sideways*radians));
    for(unsigned col=0;col<3;++col)for(unsigned row=0;row<3;++row)result[col*4+row]*=scale;
    result[12]=.2f;result[13]=-.1f;result[14]=.3f;
    return result;
}
static void near(float actual,float expected,float tolerance=.00002f){assert(std::fabs(actual-expected)<tolerance);}
static void near(const BagMatrix& actual,const BagMatrix& expected,float tolerance=.00002f){
    for(unsigned i=0;i<16;++i)near(actual[i],expected[i],tolerance);
}
static std::array<float,3> point(const BagMatrix& matrix,const std::array<float,3>& local){
    std::array<float,3> output{};
    for(unsigned row=0;row<3;++row){output[row]=matrix[12+row];for(unsigned col=0;col<3;++col)output[row]+=matrix[col*4+row]*local[col];}
    return output;
}
static void sameContact(const BagMatrix& before,const BagMatrix& after,const std::array<float,3>& pivot){
    const auto a=point(before,pivot),b=point(after,pivot);for(unsigned i=0;i<3;++i)near(a[i],b[i]);
}
static void sameShape(const BagMatrix& before,const BagMatrix& after){
    for(unsigned a=0;a<3;++a)for(unsigned b=0;b<3;++b){
        float oldDot=0,newDot=0;
        for(unsigned row=0;row<3;++row){oldDot+=before[a*4+row]*before[b*4+row];newDot+=after[a*4+row]*after[b*4+row];}
        near(oldDot,newDot);
    }
}
int main(){
    const std::array<float,3> outward{{-1,0,0}},top{{0,0,.6195f*.75f}};
    // Double the previous full travel: upright 20 -> 40 degrees; the Orc's
    // inward-compensated 35-degree swing becomes 70 degrees around its strap.
    for(float inward:{0.f,15.f}){
        const auto raw=fitted(inward);auto output=raw;
        assert(liftBagInGravity(output,raw,identity,identity,outward,1));
        auto expected=fitted(inward-2*(20+inward));
        for(unsigned i=0;i<12;++i)near(output[i],expected[i]);
        sameContact(raw,output,top);sameShape(raw,output);
        assert(output[12]<raw[12]&&output[14]>raw[14]);
        const auto oldBottom=point(raw,{{0,0,-.6195f}}),newBottom=point(output,{{0,0,-.6195f}});
        assert(newBottom[0]<oldBottom[0]&&newBottom[2]>oldBottom[2]);
        auto half=raw;assert(liftBagInGravity(half,raw,identity,identity,outward,.5f));
        const auto expectedHalf=fitted(inward-2*(20+inward)*.5f);
        for(unsigned i=0;i<12;++i)near(half[i],expectedHalf[i]);
    }
    // A sideways pack chooses its gravity-high side as contact. It still
    // rotates around world horizontal, rather than its authored lateral axis.
    for(float side:{90.f,-90.f}){
        const auto raw=fitted(0,side);auto output=raw;
        assert(liftBagInGravity(output,raw,identity,identity,outward,1));
        sameContact(raw,output,{{0,std::copysign(.441f*.75f,side),0}});
        assert(output[12]<raw[12]&&output[14]>raw[14]);sameShape(raw,output);
        const auto expected=bagMatrixProduct(rotate(1,40*radians),raw);
        for(unsigned i=0;i<12;++i)near(output[i],expected[i]);
    }
    // Preserve the current shifted/rotated strap, not the raw fitted center.
    // Repeated draw inputs produce exactly one deterministic air-lift, without
    // accumulating earlier draws or replaying their small animation motion.
    {
        const auto raw=fitted(15);auto current=bagMatrixProduct(rotate(0,2*radians),raw);
        current[12]=raw[12]+.007f;current[13]=raw[13]-.006f;current[14]=raw[14]+.012f;
        auto first=current;assert(liftBagInGravity(first,raw,identity,identity,outward,.8f));
        sameContact(current,first,top);sameShape(current,first);
        for(unsigned render=0;render<20;++render){auto output=current;assert(liftBagInGravity(output,raw,identity,identity,outward,.8f));assert(output==first);}
    }
    // A tilted, stretched, reflected root must still get a rigid world-space
    // rotation: world lengths and angles remain unchanged and gravity stays Z.
    for(float reflection:{1.f,-1.f}){
        auto root=bagMatrixProduct(rotate(2,.8f),rotate(0,.4f));
        for(unsigned row=0;row<3;++row){root[row]*=.6f*reflection;root[4+row]*=1.2f;root[8+row]*=1.5f;}
        root[12]=5;root[13]=-3;root[14]=2;BagMatrix inverse;assert(bagAffineInverse(root,inverse));
        std::array<float,3> gravity;assert(bagWorldUpInModel(inverse,identity,gravity));
        near(root[0]*gravity[0]+root[4]*gravity[1]+root[8]*gravity[2],0);
        near(root[1]*gravity[0]+root[5]*gravity[1]+root[9]*gravity[2],0);
        const auto raw=fitted(15,40);auto output=raw;
        assert(liftBagInGravity(output,raw,root,inverse,outward,1));
        const auto before=bagMatrixProduct(root,raw),after=bagMatrixProduct(root,output);
        sameShape(before,after);
        const float uy=.441f*before[6],uz=.6195f*before[10],denom=std::sqrt(uy*uy+uz*uz);
        const std::array<float,3> pivot{{0,.75f*.441f*uy/denom,.75f*.6195f*uz/denom}};
        sameContact(before,after,pivot);
        // Moving the whole root cannot affect orientation or relative motion.
        root[12]+=12;root[13]-=7;root[14]+=9;assert(bagAffineInverse(root,inverse));
        auto moved=raw;assert(liftBagInGravity(moved,raw,root,inverse,outward,1));near(output,moved);
        root[12]=100000000.f;root[13]=-75000000.f;root[14]=60000000.f;
        assert(bagAffineInverse(root,inverse));moved=raw;
        assert(liftBagInGravity(moved,raw,root,inverse,outward,1));assert(output==moved);
    }
    // Outward direction follows torso heading; every hinge stays horizontal.
    for(float heading:{-.8f,.5f,2.f}){
        const auto turn=rotate(2,heading);const auto raw=bagMatrixProduct(turn,fitted(15));auto output=raw;
        const std::array<float,3> direction{{-turn[0],-turn[1],0}};
        assert(liftBagInGravity(output,raw,identity,identity,direction,1));
        auto reference=fitted(15);const auto original=reference;
        assert(liftBagInGravity(reference,original,identity,identity,outward,1));
        near(output,bagMatrixProduct(turn,reference));
    }
    // Double even swings shortened by the original fitted-travel limit.
    // Already-flat fits that originally had no added swing still stay put.
    for(float away:{30.f,70.f,80.f}){
        const auto raw=fitted(-away);auto output=raw;
        const bool changed=liftBagInGravity(output,raw,identity,identity,outward,1);
        if(away>=75){assert(!changed&&output==raw);continue;}
        assert(changed);const auto expected=fitted(-(away+2*std::fmin(20.f,75.f-away)));
        for(unsigned i=0;i<12;++i)near(output[i],expected[i]);
    }
    const auto raw=fitted();
    for(float weight:{0.f,-1.f,std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::infinity()}){
        auto output=raw;assert(!liftBagInGravity(output,raw,identity,identity,outward,weight));assert(output==raw);
    }
    for(const auto& direction:{std::array<float,3>{{0,0,0}},std::array<float,3>{{0,0,1}},std::array<float,3>{{0,0,std::numeric_limits<float>::infinity()}}}){
        auto output=raw;assert(!liftBagInGravity(output,raw,identity,identity,direction,1));assert(output==raw);
    }
    auto flat=fitted(90);const auto beforeFlat=flat;assert(!liftBagInGravity(flat,beforeFlat,identity,identity,outward,1));assert(flat==beforeFlat);
    auto broken=identity;broken[0]=0;auto output=raw;assert(!liftBagInGravity(output,raw,broken,identity,outward,1));assert(output==raw);
    broken=identity;broken[12]=std::numeric_limits<float>::infinity();assert(!liftBagInGravity(output,raw,broken,identity,outward,1));assert(output==raw);
    // Near-degenerate geometry fades smoothly instead of choosing a new hinge.
    auto nearlyFlat=fitted(89.9f),almostVertical=raw;
    assert(liftBagInGravity(nearlyFlat,fitted(89.9f),identity,identity,outward,1));
    near(nearlyFlat,fitted(89.9f),.0004f);
    assert(liftBagInGravity(almostVertical,raw,identity,identity,{{-.001f,0,1}},1));near(almostVertical,raw,.0004f);
    std::cout<<"PASS: gravity-horizontal jump pitch, upper-back contact, Orc inward compensation, sideways fits, exact strap preservation, rigid world shape, nonuniform roots, repeated draws, limits and safe degeneracies\n";
}
