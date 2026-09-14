#pragma once
// Hand-tuned fits are separate from the generated character contact anchors.
// Bag 1 is Runecloth Bag (DarkSchoolbag), height 1.239 model units. Sex: 0/1.
// Back offsets use fixed character units along normalized torso axes.
struct BagMount { unsigned bag,race,sex; float raisedOrigin,inwardDegrees,rightDegrees,backInward,backUp; };
static constexpr BagMount bagMounts[]={
    {1,2,0,0,15.f,15.f,.0375f,.0375f}, // Orc male: higher, closer, more inward tilt
    {1,2,1,0,10.f,15.f,0,0}, // Orc female: centered back-panel pivot
};
static BagMount bagMount(unsigned bag,unsigned race,unsigned sex){
    for(const auto& mount:bagMounts)if(mount.bag==bag&&mount.race==race&&mount.sex==sex)return mount;
    return {bag,race,sex,0,0,0,0,0};
}
