#pragma once
#include "CapeCloth.h"
// Rendering a collision solution is conditional on preserving the actual
// current garment fit. These checks apply AFTER every contact and fallback.
inline bool capePoseFits(const std::vector<cape::Vec3>& pose,const std::vector<cape::Vec3>& animated,
                         const std::vector<cape::Triangle>& triangles,const std::vector<std::uint32_t>& pins){
    if(pose.size()!=animated.size()||pose.empty())return false;
    for(unsigned i=0;i<pose.size();++i)if(!cape::finite(pose[i])||cape::length(pose[i]-animated[i])>.18f)return false;
    for(unsigned pin:pins)if(pin>=pose.size()||cape::length(pose[pin]-animated[pin])>.00001f)return false;
    for(const auto& t:triangles){const unsigned ids[]={t.a,t.b,t.c};
        for(unsigned i:ids)if(i>=pose.size())return false;
        for(unsigned j=0;j<3;++j){const unsigned a=ids[j],b=ids[(j+1)%3];
            const float original=cape::length(animated[a]-animated[b]),actual=cape::length(pose[a]-pose[b]);
            if(std::fabs(actual-original)>std::max(.001f,original*.05f))return false;
        }
        const auto before=cape::cross(animated[t.b]-animated[t.a],animated[t.c]-animated[t.a]);
        const auto after=cape::cross(pose[t.b]-pose[t.a],pose[t.c]-pose[t.a]);
        const float area=cape::length(before),changed=cape::length(after);
        if(area>1.e-7f&&(changed<area*.85f||changed>area*1.15f||cape::dot(before,after)<=0))return false;
    }
    return true;
}
