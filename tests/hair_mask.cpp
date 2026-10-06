#include "../native/HairMask.h"
#include <cassert>
#include <iostream>
using namespace hairMask;
static BagMatrix identity(){return {{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1}};}
static std::vector<Triangle> mesh(const Bytes& b,unsigned group,bool skinOnly=false){
    std::vector<Triangle> out;auto a=u32(b,80),so=u32(b,a+28),io=u32(b,a+4),to=u32(b,a+12),vo=u32(b,72);
    for(unsigned s=0;s<u32(b,a+24);++s){auto at=so+32*s;if(u16(b,at)!=group||(skinOnly&&hairSection(b,a,s)))continue;
        for(unsigned t=u16(b,at+8);t<u16(b,at+8)+u16(b,at+10);t+=3){Triangle tri;
            for(unsigned j=0;j<3;++j){auto v=u16(b,io+2*u16(b,to+2*(t+j)));for(unsigned k=0;k<3;++k)tri[j][k]=number(b,vo+48*v+4*k);}out.push_back(tri);}}
    return out;
}
int main(){
    Triangle plane{{{{-1,-1,1}},{{1,-1,1}},{{0,1,1}}}};
    assert(std::fabs(hit({{0,0,0}},{{0,0,2}},plane)-.5f)<1e-6f);
    assert(hit({{2,0,0}},{{0,0,2}},plane)<0); // uncovered side
    // A fringe protruding .0005 model units must not be pulled another .01 inward.
    // The same maximum remains available for a deeply penetrating crown.
    const float shallow=fitMargin(.1f,.995f,1e10f,.01f);
    assert(shallow*.1f<=.000201f&&shallow>0);
    assert((1.f-(.995f-shallow))*.1f<.00071f);
    const float deep=fitMargin(.2f,.5f,1e10f,.01f);
    assert(std::fabs(deep*.2f-.01f)<1e-6f);
    const float closeSkin=fitMargin(.1f,.995f,.994f,.01f);
    assert(.995f-closeSkin>.994f&&closeSkin<shallow);
    unsigned groups=0,modified=0,skinSections=0;
    for(unsigned body=1;body<=16;++body){char path[100];std::snprintf(path,sizeof(path),"addon/SaureksCloset/CapeMotion/B%02u.m2",body);Bytes base;assert(capeMotion::readFile(path,base));
        auto scalp=mesh(base,1);Point lo=scalp.empty()?Point{}:scalp[0][0],hi=lo;
        for(auto t:scalp)for(auto p:t)for(unsigned k=0;k<3;++k){lo[k]=std::min(lo[k],p[k]);hi[k]=std::max(hi[k],p[k]);}
        Point center;for(unsigned k=0;k<3;++k)center[k]=(lo[k]+hi[k])*.5f;
        auto hat=scalp;for(auto& t:hat)for(auto& p:t)for(unsigned k=0;k<3;++k)p[k]=center[k]+(p[k]-center[k])*1.10f;
        auto a=u32(base,80),so=u32(base,a+28),vo=u32(base,72),nv=u32(base,68);std::vector<unsigned> tested;
        for(unsigned s=0;s<u32(base,a+24);++s){auto group=u16(base,so+32*s);if(group<1||group>=100||std::find(tested.begin(),tested.end(),group)!=tested.end())continue;tested.push_back(group);++groups;
            auto visibleSkin=mesh(base,0,true);auto patches=mesh(base,group,true);visibleSkin.insert(visibleSkin.end(),patches.begin(),patches.end());
            Bytes out;unsigned changed=0;assert(build(base,group,hat,identity(),out,changed));modified+=changed;
            // NO topology changes, no erased triangles or new cut boundaries.
            assert(out.size()==base.size());for(unsigned byte=0;byte<base.size();++byte)if(byte<vo||byte>=vo+nv*48||(byte-vo)%48>=12)assert(out[byte]==base[byte]);
            // Explicitly protect skin batches even when they share the selected
            // hair geoset (Human Female 10 includes such a scalp patch).
            for(unsigned v=0;v<u32(base,76);++v){auto view=u32(base,80)+44*v,sections=u32(base,view+28),io=u32(base,view+4),to=u32(base,view+12);
                for(unsigned sec=0;sec<u32(base,view+24);++sec){auto at=sections+32*sec;if(u16(base,at)==group&&hairSection(base,view,sec))continue;
                    if(u16(base,at)==group)++skinSections;
                    for(unsigned t=u16(base,at+8);t<u16(base,at+8)+u16(base,at+10);++t){auto vertex=u16(base,io+2*u16(base,to+2*t));assert(std::memcmp(base.data()+vo+48*vertex,out.data()+vo+48*vertex,48)==0);}
                }
            }
            for(unsigned i=0;i<nv;++i){Point original,fit;for(unsigned k=0;k<3;++k){original[k]=number(base,vo+48*i+4*k);fit[k]=number(out,vo+48*i+4*k);}
                if(original[2]<lo[2]-(hi[2]-lo[2])*.4f)assert(original==fit); // hanging hair
                if(original!=fit){auto ray=sub(fit,center);assert(nearest(center,ray,hat)>1.f);const auto skinDistance=nearest(center,ray,visibleSkin);assert(skinDistance>=1e9f||skinDistance<1.f);} // strictly BETWEEN skin and hat
            }
            Bytes again;unsigned n=0;assert(build(base,group,hat,identity(),again,n)&&again==out&&n==changed);
            assert(build(base,group,{},identity(),again,n)&&again==base&&!n);
            auto unsafe=hat;for(auto& tri:unsafe)for(auto& p:tri)for(unsigned k=0;k<3;++k)p[k]=center[k]+(p[k]-center[k])*.1f;
            assert(build(base,group,unsafe,identity(),again,n)&&again==base&&!n); // deeply inside head: preserve
            auto moved=identity();moved[12]=2;moved[13]=-3;moved[14]=4;auto localHat=hat;for(auto& t:localHat)for(auto& p:t)p=transform(moved,p);
            assert(build(base,group,localHat,moved,again,n)&&n==changed); // coordinates follow actual fitted hat
            auto corrupt=base;capeMotion::put(corrupt,72,0xfffffff0);assert(!build(corrupt,group,hat,identity(),again,n));
        }
    }
    assert(groups>100&&modified>0&&skinSections>0);
    std::cout<<"PASS: "<<groups<<" hairstyles; protected scalp/materials, closed topology, skin/hat clearance, lower hair, all LODs, unsafe/empty coverage and deterministic fits ("<<modified<<" fitted vertices)\n";
}
