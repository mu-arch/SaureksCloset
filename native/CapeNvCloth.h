#pragma once
#include "CapeCloth.h"
#include <array>
#include <memory>

namespace cape {
using Rotation=std::array<Vec3,3>;
inline Rotation identityRotation(){return {{{1,0,0},{0,1,0},{0,0,1}}};}
// Actual NVIDIA CPU solver. Owned and called exclusively by the cape worker.
class NvClothSolver {
    struct Impl;
    std::unique_ptr<Impl> impl_;
    Config config_{};
    Stats stats_{};
    std::vector<Vec3> positions_;
    void acceptFit(const std::vector<Vec3>& pose,float fraction);
public:
    NvClothSolver();
    ~NvClothSolver();
    NvClothSolver(NvClothSolver&&) noexcept;
    NvClothSolver& operator=(NvClothSolver&&) noexcept;
    bool initialize(const std::vector<Vec3>& pose,const std::vector<Triangle>& faces,const std::vector<std::uint32_t>& pins,Config config={});
    bool step(float elapsed,const std::vector<Vec3>& pose,const std::vector<ColliderTriangle>& surfaces,const std::vector<ColliderBox>& boxes={},const std::vector<Vec3>& reference={});
    void reset(const std::vector<Vec3>& pose);
    bool ready()const{return bool(impl_);}
    bool hasResult()const{return ready();}
    void setFrame(const Rotation&){}
    const std::vector<Vec3>& positions()const{return positions_;}
    const Config& config()const{return config_;}
    const Stats& stats()const{return stats_;}
    std::size_t collisionSampleCount()const{return positions_.size();}
    template<class Accept> bool limitToFit(const std::vector<Vec3>& pose,Accept accept){
        if(pose.size()!=positions_.size()||!accept(pose))return false;
        for(auto p:positions_)if(!finite(p))return false;
        if(accept(positions_))return true;
        std::vector<Vec3> candidate(pose.size());float low=0,high=1;
        for(unsigned n=0;n<12;++n){const float f=(low+high)*.5f;
            for(unsigned i=0;i<pose.size();++i)candidate[i]=lerp(pose[i],positions_[i],f);
            if(accept(candidate))low=f;else high=f;
        }
        if(low==0)return false;
        acceptFit(pose,low);return true;
    }
};
}
