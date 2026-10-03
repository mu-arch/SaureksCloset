#include <cassert>
#include <cmath>
#include <iostream>
#include "../native/BagPlacement.h"
static const BagMatrix identity{{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1}};
static void near(float a,float b){assert(std::fabs(a-b)<.000003f);}
int main(){
    for(bool soft:{false,true})for(float amount:{0.f,.5f,1.f,2.f}){
        std::array<BagTuningEntry,16> fits{};fits[1].enabled=true;fits[1].values.amplitude=amount;
        auto defaults=fits;defaults[1].values.amplitude=1;
        BagMotion motion,reference;BagResponse response,original;
        auto profile=bagResponseProfile(soft?"cloth":"leather");float peak=0;
        for(unsigned tick=0;tick<5000;tick+=16){
            auto fitted=identity;for(unsigned i:{0u,5u,10u})fitted[i]=.3825f;
            fitted[12]=.2f;fitted[14]=1.1f+.025f*std::sin(tick*.0176f);
            BagMatrix actual,base;
            assert(bagPlacement(identity,identity,identity,bagFits[1].anchor,actual,1,&motion,tick,3,true,nullptr,nullptr,0,fits.data(),0,201,&response,&profile,0,true,&fitted,soft));
            assert(bagPlacement(identity,identity,identity,bagFits[1].anchor,base,1,&reference,tick,3,true,nullptr,nullptr,0,defaults.data(),0,201,&original,&profile,0,true,&fitted,soft));
            if(amount==0)for(unsigned i=0;i<16;++i)near(actual[i],fitted[i]);
            if(amount==1)for(unsigned i=0;i<16;++i)near(actual[i],base[i]);
            for(unsigned col=0;col<3;++col){float length=0;for(unsigned k=0;k<3;++k)length+=actual[4*col+k]*actual[4*col+k];near(length,.3825f*.3825f);}
            if(response.ready){near(response.outputAmplitude,amount);const auto rig=bagResponseMatrices(response,actual,soft);if(!soft)for(const auto& m:rig)assert(m==actual);}
            peak=std::fmax(peak,std::fabs(actual[14]-fitted[14]));
        }
        if(amount==0)assert(peak==0);else assert(peak>.0001f);
        fits[1].values.motion=false;fits[1].values.amplitude=2;auto fitted=identity;BagMatrix stopped;
        assert(bagPlacement(identity,identity,identity,bagFits[1].anchor,stopped,1,&motion,6000,3,true,nullptr,nullptr,1,fits.data(),0,201,&response,&profile,1,true,&fitted,soft));
        assert(stopped==fitted&&!response.ready&&!motion.ready);
    }
    auto invalid=BagTuningValues{};invalid.amplitude=std::nanf("");assert(!bagTuningValid(invalid,true));
    std::cout<<"PASS: exact default/zero motion, rigid geometry at all amplitudes, dynamic off follows attachment\n";
}
