#include <cassert>
#include <iostream>
#include <limits>
#include "../native/PlacementTuning.h"
static const BagMatrix identity{{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1}};
static void near(const BagMatrix& a,const BagMatrix& b){
    for(unsigned i=0;i<16;++i)assert(std::fabs(a[i]-b[i])<.0001f);
}
int main(){
    bagTuningUseOwner(1);
    for(unsigned slot=101;slot<=107;++slot)for(unsigned race=1;race<=8;++race)for(unsigned sex=0;sex<2;++sex){
        BagTuningValues values;assert(bagTuningDefaults(slot,race,sex,values)&&values.scale==100);
        BagMatrix out;assert(placementTuning(identity,identity,identity,identity,values,out));near(out,identity);
        values.left=.25f;values.inset=-.1f;values.up=.05f;
        assert(bagTuningSet(slot,race,sex,true,values));
        assert(placementTuning(identity,identity,identity,identity,values,out));
        auto expected=identity;expected[12]=-.1f;expected[13]=.25f;expected[14]=.05f;near(out,expected);
        const auto original=out;
        for(unsigned frame=0;frame<20;++frame){
            assert(placementTuning(identity,identity,identity,identity,values,out));near(out,original);
        }
        auto view=identity;view[0]=0;view[1]=2;view[4]=-2;view[5]=0;view[10]=2;view[12]=20;view[13]=-15;
        assert(placementTuning(view,identity,view,view,values,out));near(out,bagMatrixProduct(view,expected));
        values.pitch=33;values.roll=-50;values.yaw=90;values.scale=150;
        assert(placementTuning(identity,identity,identity,identity,values,out));
        for(unsigned col:{0u,4u,8u}){
            float norm=0;for(unsigned row=0;row<3;++row)norm+=out[col+row]*out[col+row];
            assert(std::fabs(std::sqrt(norm)-1.5f)<.00001f);
        }
        for(unsigned axis=0;axis<3;++axis)assert(std::fabs(out[12+axis]-expected[12+axis])<.00001f);
        auto local=identity;local[0]=.7f;local[5]=1.2f;local[10]=.9f;local[12]=.1f;
        BagMatrix baseline;BagTuningValues defaults;defaults.scale=100;
        assert(placementTuning(view,local,view,view,defaults,baseline));near(baseline,view);
        assert(!bagTuningEntries[(race-1)*2+sex].enabled);
        assert(bagTuningSet(slot,race,sex,false));
    }
    BagTuningValues invalid;invalid.left=std::numeric_limits<float>::quiet_NaN();BagMatrix out;
    assert(!placementTuning(identity,identity,identity,identity,invalid,out));
    BagTuningValues valid;valid.scale=100;assert(bagTuningSet(107,2,0,true,valid));
    bagTuningUseOwner(2);assert(!weaponTuningEntries[6][2].enabled);
    std::cout<<"PASS: all 112 placement identities, default pose, transforms, zoom, no accumulation, validation and owner isolation\n";
}
