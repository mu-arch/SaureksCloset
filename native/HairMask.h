#pragma once
#include "CapeMotion.h"
#include "BagCoordinates.h"
#include <algorithm>

// Bake a private M2. Keep the source and all non-hair geometry untouched.
namespace hairMask {
using capeMotion::Bytes;
using capeMotion::u16;using capeMotion::u32;using capeMotion::put;using capeMotion::range;using capeMotion::append;
using Point=std::array<float,3>;
using Options=std::array<unsigned,4>; // width %, depth %, top %, lower cutoff %
inline Options defaults(){return {{90,90,95,35}};}
inline bool valid(const Options& v){return v[0]>=30&&v[0]<=150&&v[1]>=30&&v[1]<=150&&v[2]>=10&&v[2]<=150&&v[3]<=90&&v[2]>v[3];}
inline void put16(Bytes& b,unsigned at,unsigned v){b[at]=v&255;b[at+1]=(v>>8)&255;}
inline float number(const Bytes& b,unsigned p){float f;std::memcpy(&f,b.data()+p,4);return f;}
inline Point transform(const BagMatrix& m,const Point& p){Point q{};for(unsigned i=0;i<3;++i)q[i]=m[12+i]+m[i]*p[0]+m[4+i]*p[1]+m[8+i]*p[2];return q;}
struct Envelope {BagMatrix modelToHat{};Point center{},radius{};float bottom=0,top=0;};
inline bool envelope(const std::vector<Point>& hat,const BagMatrix& modelToHat,const Options& options,Envelope& out){
    if(hat.size()<3||!valid(options))return false;
    Point lo=hat[0],hi=lo;
    for(const auto& p:hat)for(unsigned a=0;a<3;++a){if(!std::isfinite(p[a]))return false;lo[a]=std::min(lo[a],p[a]);hi[a]=std::max(hi[a],p[a]);}
    const float h=hi[2]-lo[2];if(h<.005f||h>10)return false;
    // Exclude the lower brim when estimating the crown. User controls can
    // tighten irregular/open hats; no claim of an exact mesh Boolean.
    Point crownLo=hi,crownHi=lo;unsigned count=0;
    for(const auto& p:hat)if(p[2]>=lo[2]+h*.35f){++count;for(unsigned a=0;a<2;++a){crownLo[a]=std::min(crownLo[a],p[a]);crownHi[a]=std::max(crownHi[a],p[a]);}}
    if(count<3)return false;
    out={};out.modelToHat=modelToHat;out.bottom=lo[2]+h*options[3]*.01f;out.top=lo[2]+h*options[2]*.01f;
    for(unsigned a=0;a<2;++a){out.center[a]=(crownLo[a]+crownHi[a])*.5f;out.radius[a]=(crownHi[a]-crownLo[a])*.5f*options[a==0?1:0]*.01f;if(out.radius[a]<.005f)return false;}
    return true;
}
struct Vertex {Point p{},normal{};std::array<float,4> uv{};std::array<float,256> weights{};};
inline Vertex mix(const Vertex& a,const Vertex& b,float t){Vertex v;for(unsigned i=0;i<3;++i){v.p[i]=a.p[i]+(b.p[i]-a.p[i])*t;v.normal[i]=a.normal[i]+(b.normal[i]-a.normal[i])*t;}for(unsigned i=0;i<4;++i)v.uv[i]=a.uv[i]+(b.uv[i]-a.uv[i])*t;for(unsigned i=0;i<256;++i)v.weights[i]=a.weights[i]+(b.weights[i]-a.weights[i])*t;return v;}
using Polygon=std::vector<Vertex>;
template<class Distance> Polygon clip(const Polygon& poly,Distance distance){
    Polygon out;if(poly.empty())return out;auto previous=poly.back();float dp=distance(previous.p);
    for(const auto& current:poly){const float dc=distance(current.p);if((dc>=0)!=(dp>=0))out.push_back(mix(previous,current,dp/(dp-dc)));if(dc>=0)out.push_back(current);previous=current;dp=dc;}return out;
}
inline std::array<Polygon,2> cut(const Polygon& triangle,const Envelope& e){
    bool allBelow=true;for(const auto& v:triangle)if(transform(e.modelToHat,v.p)[2]>e.bottom)allBelow=false;
    if(allBelow)return {{triangle,{}}};
    auto below=clip(triangle,[&](const Point& p){return e.bottom-transform(e.modelToHat,p)[2];});
    auto crown=clip(triangle,[&](const Point& p){return transform(e.modelToHat,p)[2]-e.bottom;});
    crown=clip(crown,[&](const Point& p){return e.top-transform(e.modelToHat,p)[2];});
    // A sixteen-sided oval is independent of camera direction.
    for(unsigned i=0;i<16&&!crown.empty();++i){const float a=(i+.5f)*6.28318530718f/16.f,c=std::cos(a),s=std::sin(a);
        crown=clip(crown,[&](const Point& p){const auto q=transform(e.modelToHat,p);return 1.f-c*(q[0]-e.center[0])/e.radius[0]-s*(q[1]-e.center[1])/e.radius[1];});}
    return {{below,crown}};
}
inline bool decode(const Bytes& source,unsigned at,Vertex& v){
    for(unsigned i=0;i<3;++i){v.p[i]=number(source,at+i*4);v.normal[i]=number(source,at+20+i*4);if(!std::isfinite(v.p[i])||!std::isfinite(v.normal[i]))return false;}
    unsigned sum=0;for(unsigned i=0;i<4;++i){v.weights[source[at+16+i]]+=source[at+12+i];sum+=source[at+12+i];v.uv[i]=number(source,at+32+i*4);if(!std::isfinite(v.uv[i]))return false;}return sum==255;
}
inline bool encode(const Vertex& v,const Bytes& base,unsigned lookup,unsigned boneStart,unsigned boneCount,Bytes& vertices,Bytes& properties){
    std::vector<std::pair<float,unsigned>> weights;for(unsigned i=0;i<256;++i)if(v.weights[i]>.00001f)weights.push_back({v.weights[i],i});
    std::sort(weights.begin(),weights.end(),[](const auto& a,const auto& b){return a.first>b.first;});if(weights.empty())return false;if(weights.size()>4)weights.resize(4);
    float total=0;for(const auto& w:weights)total+=w.first;
    const auto at=vertices.size();vertices.resize(at+48,0);const auto pr=properties.size();properties.resize(pr+4,0);
    std::memcpy(vertices.data()+at,v.p.data(),12);auto normal=v.normal;float length=0;for(auto n:normal)length+=n*n;
    if(length>1e-12f){length=std::sqrt(length);for(auto& n:normal)n/=length;}
    std::memcpy(vertices.data()+at+20,normal.data(),12);std::memcpy(vertices.data()+at+32,v.uv.data(),16);
    unsigned remaining=255;for(unsigned i=0;i<weights.size();++i){unsigned local=0;for(;local<boneCount;++local)if(u16(base,lookup+2*(boneStart+local))==weights[i].second)break;if(local>=boneCount||local>255)return false;
        const unsigned amount=i+1==weights.size()?remaining:std::min(remaining,unsigned(std::floor(weights[i].first/total*255+.5f)));remaining-=amount;
        vertices[at+12+i]=amount;vertices[at+16+i]=weights[i].second;properties[pr+i]=local;}
    return true;
}
inline bool build(const Bytes& base,unsigned hairGroup,const Envelope& env,Bytes& result,unsigned& removed){
    removed=0;BagMatrix inverse;if(!bagAffineInverse(env.modelToHat,inverse)||!std::isfinite(env.bottom)||!std::isfinite(env.top)||env.top<=env.bottom)return false;
    for(unsigned a=0;a<2;++a)if(!std::isfinite(env.center[a])||!std::isfinite(env.radius[a])||env.radius[a]<=0)return false;
    if(base.size()<0x144||u32(base,0)!=0x3032444d||u32(base,4)!=256||hairGroup<1||hairGroup>=100)return false;
    const auto nv=u32(base,68),vo=u32(base,72),views=u32(base,76),viewOffset=u32(base,80),nb=u32(base,140),lookup=u32(base,144);
    if(!nv||nv>=65536||!range(base,vo,nv,48)||!views||views>16||!range(base,viewOffset,views,44)||!range(base,lookup,nb,2))return false;
    Bytes out=base,vertices(base.begin()+vo,base.begin()+vo+nv*48);bool found=false;
    for(unsigned view=0;view<views;++view){
        const unsigned a=viewOffset+view*44,ni=u32(base,a),io=u32(base,a+4),nt=u32(base,a+8),to=u32(base,a+12),np=u32(base,a+16),po=u32(base,a+20),ns=u32(base,a+24),so=u32(base,a+28);
        if(ni>65535||nt>65535||ns>512||np!=ni||!range(base,io,ni,2)||!range(base,to,nt,2)||!range(base,po,np,4)||!range(base,so,ns,32))return false;
        Bytes indices(base.begin()+io,base.begin()+io+ni*2),properties(base.begin()+po,base.begin()+po+np*4),triangles,sections(base.begin()+so,base.begin()+so+ns*32);
        for(unsigned section=0;section<ns;++section){const unsigned s=so+32*section,at=32*section,first=u16(base,s+8),count=u16(base,s+10);
            if(first+count>nt||count%3)return false;put16(sections,at+8,triangles.size()/2);
            if(u16(base,s)!=hairGroup){triangles.insert(triangles.end(),base.begin()+to+first*2,base.begin()+to+(first+count)*2);continue;}
            found=true;const unsigned boneCount=u16(base,s+12),boneStart=u16(base,s+14),start=indices.size()/2;if(boneStart+boneCount>nb)return false;
            for(unsigned t=first;t<first+count;t+=3){Polygon original;for(unsigned j=0;j<3;++j){const auto ix=u16(base,to+2*(t+j));if(ix>=ni)return false;const auto vertex=u16(base,io+ix*2);if(vertex>=nv)return false;Vertex v;if(!decode(base,vo+vertex*48,v))return false;original.push_back(v);}
                const auto pieces=cut(original,env);if(view==0){for(const auto& v:original){const auto q=transform(env.modelToHat,v.p);bool keep=q[2]<=env.bottom;if(!keep){keep=q[2]<=env.top;for(unsigned side=0;side<16&&keep;++side){const float angle=(side+.5f)*6.28318530718f/16.f;keep=std::cos(angle)*(q[0]-env.center[0])/env.radius[0]+std::sin(angle)*(q[1]-env.center[1])/env.radius[1]<=1.00001f;}}if(!keep){++removed;break;}}}
                for(const auto& poly:pieces){if(poly.size()<3)continue;const unsigned begin=indices.size()/2;for(const auto& vertex:poly){const auto global=vertices.size()/48;if(global>=65535||indices.size()/2>=65535)return false;indices.push_back(global&255);indices.push_back(global>>8);if(!encode(vertex,base,lookup,boneStart,boneCount,vertices,properties))return false;}
                    for(unsigned j=1;j+1<poly.size();++j)for(auto ix:{begin,begin+j,begin+j+1}){triangles.push_back(ix&255);triangles.push_back(ix>>8);}}
            }
            if(triangles.size()/2>65535)return false;put16(sections,at+4,start);put16(sections,at+6,indices.size()/2-start);put16(sections,at+10,triangles.size()/2-u16(sections,at+8));put16(sections,at+16,4);
        }
        if(triangles.size()/2>65535)return false;
        put(out,a,indices.size()/2);put(out,a+4,append(out,indices.data(),indices.size()));put(out,a+8,triangles.size()/2);put(out,a+12,append(out,triangles.data(),triangles.size()));put(out,a+16,properties.size()/4);put(out,a+20,append(out,properties.data(),properties.size()));put(out,a+28,append(out,sections.data(),sections.size()));
    }
    if(!found)return false;put(out,68,vertices.size()/48);put(out,72,append(out,vertices.data(),vertices.size()));if(out.size()>32*1024*1024)return false;result.swap(out);return true;
}
// Exact allowlist of files generated by this process, never arbitrary paths.
inline std::map<std::string,std::string> generated;
inline bool cachePath(const char* path){if(!path)return false;for(const auto& e:generated)if(capeMotion::pathEqual(e.second.c_str(),path))return true;return false;}
inline const char* save(const Bytes& bytes){
    char key[32];std::snprintf(key,sizeof(key),"%08x_%08x",capeMotion::crc(bytes),unsigned(bytes.size()));auto it=generated.find(key);if(it!=generated.end())return it->second.c_str();
    const char* dir="Interface/AddOns/SaureksCloset/CapeMotion/Cache";
#ifdef _WIN32
    _mkdir(dir);
#else
    mkdir(dir,0755);
#endif
    const std::string path=std::string(dir)+"/H1_"+key+".m2";Bytes existing;
    if(!capeMotion::readFile(path,existing)||existing!=bytes){const auto tmp=path+".tmp";{std::ofstream f(tmp,std::ios::binary|std::ios::trunc);if(!f||!f.write(reinterpret_cast<const char*>(bytes.data()),bytes.size()))return nullptr;}if(std::rename(tmp.c_str(),path.c_str())){std::remove(tmp.c_str());return nullptr;}}
    return generated.emplace(key,path).first->second.c_str();
}
inline void (*capture)(void*,const float*)=nullptr;
inline std::string activePath;
inline std::uint64_t activeOwner=0;
inline unsigned activeBody=0,activeStyle=0;
inline const char* model(const char* original,std::uint64_t owner,unsigned body,unsigned style){return owner&&owner==activeOwner&&body==activeBody&&style==activeStyle&&!activePath.empty()?activePath.c_str():original;}
}
