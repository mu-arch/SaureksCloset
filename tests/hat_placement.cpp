#include "../native/HatPlacement.h"
#include <cassert>
#include <iostream>
#include <limits>
static void near(const BagMatrix& a,const BagMatrix& b){for(unsigned i=0;i<16;++i)assert(std::fabs(a[i]-b[i])<.0001f);}
int main(){
    const BagMatrix identity{{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1}};
    BagTuningValues v;assert(bagTuningDefaults(111,1,0,v)&&v.scale==100);
    BagMatrix out;assert(hatPlacement(identity,v,out));near(out,identity);
    v.up=.1f;v.left=.03f;v.inset=-.02f;
    assert(hatPlacement(identity,v,out)&&out[12]==v.inset&&out[13]==v.left&&out[14]==v.up);
    v.pitch=17;v.roll=-8;v.yaw=12;v.scale=115;
    BagMatrix delta;assert(hatPlacement(identity,v,delta));
    for(unsigned frame=0;frame<300;++frame){
        float angle=frame*.04f;
        BagMatrix head{{std::cos(angle),0,-std::sin(angle),0,0,1,0,0,std::sin(angle),0,std::cos(angle),0,1,2,float(frame)*.01f,1}};
        const auto original=head;
        assert(hatPlacement(head,v,out));near(out,bagMatrixProduct(head,delta));
        BagMatrix repeated;assert(hatPlacement(head,v,repeated));near(repeated,out);assert(head==original);
        BagMatrix view{{0,2,0,0,-2,0,0,0,0,0,2,0,40,-20,10,1}};
        BagMatrix camera;assert(hatPlacement(bagMatrixProduct(view,head),v,camera));near(camera,bagMatrixProduct(view,out));
    }
    for(float bad:{0.f,201.f,std::numeric_limits<float>::quiet_NaN()}){v.scale=bad;assert(!hatPlacement(identity,v,out));}
    v.scale=100;auto broken=identity;broken[0]=0;assert(!hatPlacement(broken,v,out));
    std::cout<<"PASS: neutral hat fit, all axes, head-relative animation, camera invariance, no accumulated drift and invalid fit rejection\n";
}
