#pragma once
#include "CapeCollisionMesh.h"
#include "CapeNvCloth.h"
#include <chrono>
namespace cape {
// Main-thread facade. The worker receives value copies only: no native model,
// graphics buffer, Lua state, or game function can be accessed from its jobs.
class AsyncCloth {
    std::uint64_t generation_=0,received_=0;
    double time_=0;
    bool initialized_=false,hasResult_=false;
    Config config_{};Stats stats_{};
    std::chrono::steady_clock::time_point resultTime_{};
    Rotation frame_=identityRotation(),resultFrame_=identityRotation();
    std::vector<Vec3> positions_,resultOffsets_;
    std::vector<Triangle> faces_;
    std::vector<Vec3> material_;
    std::vector<AnimatedCollider> colliders_;
    std::vector<std::uint32_t> pins_;
public:
    bool initialize(const std::vector<Vec3>&,const std::vector<Triangle>&,const std::vector<std::uint32_t>&,Config={});
    bool step(float,const std::vector<Vec3>&,const std::vector<ColliderTriangle>&,const std::vector<ColliderBox>& = {},const std::vector<Vec3>& = {});
    void reset(const std::vector<Vec3>&);
    void setColliders(const std::vector<AnimatedCollider>& colliders){colliders_=colliders;}
    const std::vector<Vec3>& material()const{return material_;}
    bool ready()const{return initialized_;}
    bool hasResult()const{return hasResult_;}
    void setFrame(const Rotation& frame){frame_=frame;}
    const std::vector<Vec3>& positions()const{return positions_;}
    const Config& config()const{return config_;}
    const Stats& stats()const{return stats_;}
    std::size_t collisionSampleCount()const{return positions_.size();}
    template<class Accept> bool limitToFit(const std::vector<Vec3>& pose,Accept accept){
        if(pose.size()!=positions_.size()||!accept(pose))return false;
        for(auto p:positions_)if(!finite(p))return false;
        if(accept(positions_))return true;
        std::vector<Vec3> candidate(pose.size());float low=0,high=1;
        for(unsigned n=0;n<12;++n){float f=(low+high)*.5f;
            for(unsigned i=0;i<pose.size();++i)candidate[i]=lerp(pose[i],positions_[i],f);
            if(accept(candidate))low=f;else high=f;
        }
        if(low==0)return false;
        for(unsigned i=0;i<pose.size();++i)positions_[i]=lerp(pose[i],positions_[i],low);
        return true;
    }
};
#ifndef _WIN32
// Deterministic synchronization for offline tests ONLY. Never used by a draw.
void waitCapeWorkerForTests();
void delayCapeWorkerForTests(unsigned milliseconds);
#endif
}
