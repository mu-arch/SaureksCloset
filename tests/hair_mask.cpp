#include "../native/HairMask.h"
#include <cassert>
#include <iostream>
using namespace hairMask;
static BagMatrix identity(){return {{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1}};}
int main(){
    Envelope e;e.modelToHat=identity();e.radius={{.1f,.1f,0}};e.bottom=1.65f;e.top=1.85f;
    assert(valid(defaults())&&!valid({{0,90,95,35}})&&!valid({{90,90,30,35}}));
    Vertex a,b,c;a.p={{-.2f,0,1.6f}};b.p={{.2f,0,1.6f}};c.p={{0,0,2.f}};a.weights[2]=b.weights[2]=c.weights[2]=255;
    const auto pieces=cut({a,b,c},e);assert(pieces[0].size()==4&&!pieces[1].empty());
    for(auto& v:pieces[0])assert(v.p[2]<=e.bottom+.00001f);
    for(auto& v:pieces[1])assert(v.p[2]>=e.bottom-.00001f&&v.p[2]<=e.top+.00001f);
    a.p[2]=b.p[2]=c.p[2]=e.bottom;const auto plane=cut({a,b,c},e);assert(plane[0].size()==3&&plane[1].empty());
    // Rotation/translation of the hat doesn't change clipping in hat space.
    e.modelToHat={{0,1,0,0,-1,0,0,0,0,0,1,0,.2f,-.3f,0,1}};
    auto bad=e;bad.radius[0]=0;Bytes none;unsigned count;assert(!build(Bytes(500),2,bad,none,count));
    unsigned groups=0;
    for(unsigned body=1;body<=16;++body){char path[100];std::snprintf(path,sizeof(path),"addon/SaureksCloset/CapeMotion/B%02u.m2",body);Bytes base;assert(capeMotion::readFile(path,base));
        const unsigned view=u32(base,80),ns=u32(base,view+24),so=u32(base,view+28),vo=u32(base,72);
        std::vector<unsigned> tested;
        for(unsigned s=0;s<ns;++s){const auto group=u16(base,so+s*32);if(group<1||group>=100||std::find(tested.begin(),tested.end(),group)!=tested.end())continue;tested.push_back(group);
            const unsigned io=u32(base,view+4),start=u16(base,so+s*32+4),n=u16(base,so+s*32+6);Point lo{{100,100,100}},hi{{-100,-100,-100}};
            for(unsigned i=start;i<start+n;++i){const auto at=vo+48*u16(base,io+2*i);for(unsigned k=0;k<3;++k){const auto v=number(base,at+4*k);lo[k]=std::min(lo[k],v);hi[k]=std::max(hi[k],v);}}
            e.modelToHat=identity();for(unsigned k=0;k<2;++k){e.center[k]=(lo[k]+hi[k])*.5f;e.radius[k]=std::max(.005f,(hi[k]-lo[k])*.3f);}e.bottom=lo[2]+(hi[2]-lo[2])*.4f;e.top=std::max(e.bottom+.001f,hi[2]-.005f);
            Bytes out;unsigned removed=0;assert(build(base,group,e,out,removed));++groups;
            assert(u32(out,52)==u32(base,52)&&u32(out,56)==u32(base,56)); // Skeleton untouched.
            const auto views=u32(out,76),viewsAt=u32(out,80),newVO=u32(out,72),nv=u32(out,68),boneLookup=u32(out,144);
            for(unsigned v=0;v<views;++v){const auto x=viewsAt+44*v,newIO=u32(out,x+4),oldIO=u32(base,x+4),newTO=u32(out,x+12),oldTO=u32(base,x+12),newSO=u32(out,x+28),oldSO=u32(base,x+28),props=u32(out,x+20);
                for(unsigned s2=0;s2<u32(out,x+24);++s2){const auto nsec=newSO+32*s2,osec=oldSO+32*s2,nt=u16(out,nsec+10),first=u16(out,nsec+8),bones=u16(out,nsec+14);
                    if(u16(out,nsec)!=group){assert(nt==u16(base,osec+10));for(unsigned t=0;t<nt;++t)assert(u16(out,newTO+2*(first+t))==u16(base,oldTO+2*(u16(base,osec+8)+t)));continue;}
                    assert(nt%3==0);
                    for(unsigned t=0;t<nt;t+=3){bool below=true,above=true;
                        for(unsigned j=0;j<3;++j){const auto ix=u16(out,newTO+2*(first+t+j));assert(ix<u32(out,x));const auto global=u16(out,newIO+ix*2);assert(global<nv);const auto at=newVO+48*global;const float z=number(out,at+8);below=below&&z<=e.bottom+1e-5f;above=above&&z>=e.bottom-1e-5f&&z<=e.top+1e-5f;
                            unsigned sum=0;for(unsigned w=0;w<4;++w){const auto weight=out[at+12+w];sum+=weight;if(weight)assert(u16(out,boneLookup+2*(bones+out[props+ix*4+w]))==out[at+16+w]);}assert(sum==255);
                            if(z>e.bottom+1e-5f)for(unsigned side=0;side<16;++side){const float angle=(side+.5f)*6.28318530718f/16.f;assert(std::cos(angle)*(number(out,at)-e.center[0])/e.radius[0]+std::sin(angle)*(number(out,at+4)-e.center[1])/e.radius[1]<1.0001f);}
                        }assert(below||above);
                    }
                }
                (void)oldIO;
            }
            // Deterministic bake and malformed-file rejection.
            Bytes again;assert(build(base,group,e,again,count)&&again==out);auto corrupt=base;put(corrupt,72,0xfffffff0);assert(!build(corrupt,group,e,again,count));
        }
    }
    assert(groups>100);assert(!cachePath("../../bad.m2"));std::cout<<"PASS: "<<groups<<" hairstyle meshes across 16 bodies, every LOD, true clipping, protected lower hair, bone weights/palettes, untouched body geometry, deterministic bake and invalid inputs\n";
}
