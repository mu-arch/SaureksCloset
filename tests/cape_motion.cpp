#include "../native/CapeMotion.h"
#include <cassert>
#include <iostream>
#include <filesystem>
#include <chrono>
using namespace capeMotion;
int main(){
    for(unsigned body=1;body<=16;++body){
        char path[100];std::snprintf(path,sizeof(path),"addon/SaureksCloset/CapeMotion/B%02u.m2",body);Bytes base;assert(readFile(path,base));
        Bytes unchanged;assert(build(base,body,defaults(),unchanged)&&unchanged==base);
        auto invalid=base;put(invalid,56,0xfffffff0);assert(!build(invalid,body,defaults(),unchanged));
        const unsigned bo=u32(base,56),n=u32(base,52),first=capeOriginalBones[body-1],ns=u32(base,28),so=u32(base,32);
        for(unsigned group=0;group<4;++group)for(unsigned amount:{0u,50u,200u}){
            Amounts gains=defaults();gains[group]=amount;Bytes out;assert(build(base,body,gains,out));
            // Body, tail, tabard and the upper cape anchor are byte-for-byte native.
            assert(std::memcmp(base.data()+bo,out.data()+bo,(first+1)*108)==0);
            bool changed=false;
            for(unsigned bone=first+1;bone<n;++bone){
                const auto record=bo+bone*108,track=record+40;
                assert(std::memcmp(base.data()+record,out.data()+record,40)==0);
                assert(std::memcmp(base.data()+record+68,out.data()+record+68,40)==0);
                if(!u32(base,track+20))continue;
                for(unsigned seq=0;seq<ns;++seq){
                    const auto ro=u32(base,track+8),newro=u32(out,track+8),ko=u32(base,track+24),newko=u32(out,track+24);
                    unsigned lo=u32(base,ro+seq*8),hi=u32(base,ro+seq*8+4),newlo=u32(out,newro+seq*8);
                    const auto oldto=u32(base,track+16),newto=u32(out,track+16);
                    Q firstValue{};
                    for(unsigned k=lo;k<=hi;++k){
                        const auto at=newlo+k-lo;assert(u32(base,oldto+k*4)==u32(out,newto+at*4));
                        Q oldq,q;std::memcpy(oldq.data(),base.data()+ko+k*16,16);std::memcpy(q.data(),out.data()+newko+at*16,16);
                        if(category(u16(base,so+seq*68))!=int(group))assert(q==oldq);
                        else{
                            assert(std::fabs(dot(q,q)-1)<.0001f);
                            if(k==lo)firstValue=q;
                            if(amount==0)assert(std::fabs(std::fabs(dot(firstValue,q))-1)<.0001f);
                            if(q!=oldq)changed=true;
                        }
                    }
                }
            }
            assert(changed);
        }
    }
    // Filtering softens reversals and closes the cycle without adding oscillation.
    std::vector<Vector> jagged{{{0,0,0}},{{0,1,0}},{{0,-1,0}},{{0,1,0}},{{0,-1,0}},{{0,0,0}}};
    std::vector<unsigned> ticks{0,50,100,150,200,250};auto filtered=jagged;smooth(filtered,ticks,0,65,true);
    float beforeEnergy=0,afterEnergy=0;
    for(unsigned i=1;i<jagged.size();++i){beforeEnergy+=std::fabs(jagged[i][1]-jagged[i-1][1]);afterEnergy+=std::fabs(filtered[i][1]-filtered[i-1][1]);}
    assert(afterEnergy<beforeEnergy*.5f&&filtered.front()==filtered.back());
    for(unsigned body=1;body<=16;++body){
        char path[100];std::snprintf(path,sizeof(path),"addon/SaureksCloset/CapeMotion/B%02u.m2",body);Bytes base;assert(readFile(path,base));
        const unsigned bo=u32(base,56),first=capeOriginalBones[body-1],n=u32(base,52),ns=u32(base,28),so=u32(base,32);
        const Advanced soft{{55,85,60,35,65,60}};Bytes softened;assert(build(base,body,defaults(),softened,soft)&&softened!=base);
        assert(std::memcmp(base.data()+bo,softened.data()+bo,(first+1)*108)==0);
        for(unsigned option=0;option<3;++option){
            auto options=advancedDefaults();options[option]=0;Bytes out;assert(build(base,body,defaults(),out,options));
            const unsigned axis=option==0?1:option==1?0:2;
            for(unsigned bone=first+1;bone<n;++bone){
                const auto track=bo+bone*108+40;if(!u32(base,track+20))continue;
                for(unsigned seq=0;seq<ns;++seq){
                    if(category(u16(base,so+seq*68))<0)continue;
                    const auto ro=u32(base,track+8),ko=u32(base,track+24),newro=u32(out,track+8),newko=u32(out,track+24);
                    const unsigned lo=u32(base,ro+seq*8),hi=u32(base,ro+seq*8+4),newlo=u32(out,newro+seq*8);
                    Q center{},reference{};
                    for(unsigned k=lo;k<=hi;++k){Q q;std::memcpy(q.data(),base.data()+ko+k*16,16);normalize(q);if(k==lo)reference=q;const float sign=dot(q,reference)<0?-1.f:1.f;for(unsigned i=0;i<4;++i)center[i]+=q[i]*sign;}
                    assert(normalize(center));
                    for(unsigned k=lo;k<=hi;++k){Q oldq,q;std::memcpy(oldq.data(),base.data()+ko+k*16,16);std::memcpy(q.data(),out.data()+newko+(newlo+k-lo)*16,16);normalize(oldq);
                        const auto original=rotationVector(oldq,center),adjusted=rotationVector(q,center);
                        assert(std::fabs(adjusted[axis])<.0001f);
                        for(unsigned i=0;i<3;++i)if(i!=axis)assert(std::fabs(adjusted[i]-original[i])<.0001f);
                    }
                }
            }
        }
        for(unsigned bone=first+1;bone<n;++bone){const auto record=bo+bone*108;assert(std::memcmp(base.data()+record,softened.data()+record,40)==0);assert(std::memcmp(base.data()+record+68,softened.data()+record+68,40)==0);}
    }
    assert(!cachePath("Interface/AddOns/SaureksCloset/CapeMotion/Cache/../../WoW.exe"));
    assert(!cachePath("Interface/AddOns/SaureksCloset/CapeMotion/Cache/N17_100_100_100_100_100_100_100_030_000_100_12345678.m2"));
    assert(cachePath("Interface/AddOns/SaureksCloset/CapeMotion/Cache/N02_050_100_100_100_100_100_100_030_000_100_12345678.m2"));
    // Exercise the actual cache used by model-name resolution, in a disposable root.
    namespace fs=std::filesystem;const auto cwd=fs::current_path();
    const auto temporary=fs::temp_directory_path()/("closet-cape-cache-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    fs::create_directories(temporary/"Interface/AddOns/SaureksCloset/CapeMotion");
    fs::copy_file(cwd/"addon/SaureksCloset/CapeMotion/B02.m2",temporary/"Interface/AddOns/SaureksCloset/CapeMotion/B02.m2");
    fs::current_path(temporary);const Amounts custom{{50,25,100,75}};
    const char* cached=prepare(2,custom);assert(cached&&cachePath(cached));Bytes saved,source,expected;
    assert(readFile(cached,saved)&&readFile("Interface/AddOns/SaureksCloset/CapeMotion/B02.m2",source)&&build(source,2,custom,expected)&&saved==expected);
    assert(prepare(2,custom)==cached&&model(raceModels[1].filename,true,custom)==cached);
    fs::current_path(cwd);fs::remove_all(temporary);
    const char* human=raceModels[1].filename;
    assert(model(human,false,{{0,0,0,0}})==human&&model(human,true,defaults())==human);
    std::cout<<"PASS: all 16 cape bases, native upper mounts/body/tails, independent clip gains, fixed timing, normalized rotations, reset and strict asset paths\n";
}
