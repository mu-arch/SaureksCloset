#pragma once
#include "CapeMotionCatalog.h"
#include "RaceModels.h"
#include <array>
#include <cstdint>
#include <vector>
#include <map>
#include <string>
#include <fstream>
#include <cstring>
#include <cmath>
#include <cstdio>
#include <algorithm>
#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif
// Adjust only private lower-cape rotation tracks. Native timing, the upper
// attachment, translations, scales, body bones and all other clips stay intact.
namespace capeMotion {
using Bytes=std::vector<unsigned char>;
using Amounts=std::array<unsigned char,4>; // walk, run, idle, airborne
inline Amounts defaults(){return {{100,100,100,100}};}
using Advanced=std::array<unsigned char,6>; // forward, side, twist, lean+30, smoothing, hem
inline Advanced advancedDefaults(){return {{100,100,100,30,0,100}};}
inline bool valid(const Advanced& a){return a[0]<=200&&a[1]<=200&&a[2]<=200&&a[3]<=60&&a[4]<=100&&a[5]<=200;}
inline Advanced advanced=advancedDefaults();
inline std::uint64_t owner=0;
inline bool enabled=false;
inline Amounts amounts=defaults();
inline bool valid(const Amounts& a){for(auto v:a)if(v>200)return false;return true;}
inline unsigned u16(const Bytes& b,unsigned p){return b[p]|unsigned(b[p+1])<<8;}
inline unsigned u32(const Bytes& b,unsigned p){return u16(b,p)|u16(b,p+2)<<16;}
inline void put(Bytes& b,unsigned p,unsigned value){for(unsigned i=0;i<4;++i)b[p+i]=static_cast<unsigned char>(value>>(8*i));}
inline bool range(const Bytes& b,unsigned p,unsigned count,unsigned stride){return p<=b.size()&&count<=(b.size()-p)/stride;}
inline unsigned append(Bytes& b,const void* data,unsigned size){while(b.size()%16)b.push_back(0);auto p=static_cast<unsigned>(b.size());const auto* a=static_cast<const unsigned char*>(data);b.insert(b.end(),a,a+size);return p;}
inline int category(unsigned id){return id==4||id==13?0:id==5?1:id==0?2:id==37||id==38||id==39||id==40||id==187?3:-1;}
using Q=std::array<float,4>;
inline bool normalize(Q& q){float n=0;for(auto v:q){if(!std::isfinite(v))return false;n+=v*v;}if(n<.00001f)return false;n=std::sqrt(n);for(auto& v:q)v/=n;return true;}
inline float dot(const Q& a,const Q& b){float d=0;for(unsigned i=0;i<4;++i)d+=a[i]*b[i];return d;}
inline Q scaleRotation(Q q,const Q& center,float gain){
    float d=dot(q,center);if(d<0){for(auto& v:q)v=-v;d=-d;}
    const float angle=std::acos(std::fmin(1.f,std::fmax(-1.f,d))),s=std::sin(angle);
    const float a=s>.00001f?std::sin((1-gain)*angle)/s:1-gain,b=s>.00001f?std::sin(gain*angle)/s:gain;
    for(unsigned i=0;i<4;++i)q[i]=center[i]*a+q[i]*b;
    normalize(q);return q;
}
inline Q multiply(const Q& a,const Q& b){return {{a[3]*b[0]+a[0]*b[3]+a[1]*b[2]-a[2]*b[1],a[3]*b[1]-a[0]*b[2]+a[1]*b[3]+a[2]*b[0],a[3]*b[2]+a[0]*b[1]-a[1]*b[0]+a[2]*b[3],a[3]*b[3]-a[0]*b[0]-a[1]*b[1]-a[2]*b[2]}};}
using Vector=std::array<float,3>;
inline Vector rotationVector(Q q,const Q& center){
    if(dot(q,center)<0)for(auto& v:q)v=-v;
    auto d=multiply(q,{{-center[0],-center[1],-center[2],center[3]}});normalize(d);
    const float n=std::sqrt(d[0]*d[0]+d[1]*d[1]+d[2]*d[2]);
    const float gain=n>.000001f?2*std::atan2(n,d[3])/n:2;
    return {{d[0]*gain,d[1]*gain,d[2]*gain}};
}
inline Q fromRotationVector(const Vector& v){
    const float n=std::sqrt(v[0]*v[0]+v[1]*v[1]+v[2]*v[2]);const float gain=n>.000001f?std::sin(n*.5f)/n:.5f;
    return {{v[0]*gain,v[1]*gain,v[2]*gain,std::cos(n*.5f)}};
}
inline void smooth(std::vector<Vector>& values,const std::vector<unsigned>& times,unsigned start,unsigned amount,bool loop){
    if(!amount||values.size()<2)return;
    // Two-sided exponential filtering rounds abrupt reversals without delaying
    // the whole clip. Periodic warm-up avoids introducing a seam in run cycles.
    const float tau=40.f+200.f*amount*.01f;
    auto pass=[&](bool reverse){
        Vector previous=values[reverse?values.size()-1:0];
        for(unsigned cycle=0;cycle<(loop?3u:1u);++cycle)for(unsigned k=0;k<values.size();++k){
            const unsigned i=reverse?values.size()-1-k:k,old=reverse?std::min(i+1,unsigned(values.size()-1)):(i?i-1:0);
            const float dt=std::fmax(1.f,std::fabs(float(times[start+i])-float(times[start+old])));
            const float alpha=1-std::exp(-dt/tau);
            for(unsigned axis=0;axis<3;++axis)previous[axis]+=alpha*(values[i][axis]-previous[axis]);
            if(cycle==(loop?2u:0u))values[i]=previous;
        }
    };
    pass(false);pass(true);
    if(loop)for(unsigned axis=0;axis<3;++axis)values.front()[axis]=values.back()[axis]=(values.front()[axis]+values.back()[axis])*.5f;
}
inline bool build(const Bytes& base,unsigned body,const Amounts& amounts,Bytes& result,const Advanced& options=advancedDefaults()){
    if(body<1||body>16||!valid(amounts)||!valid(options)||base.size()<0x150||u32(base,0)!=0x3032444d||u32(base,4)!=256)return false;
    const unsigned n=u32(base,52),bo=u32(base,56),ns=u32(base,28),so=u32(base,32),first=capeOriginalBones[body-1];
    if(n<=first+1||n>255||!range(base,bo,n,108)||!ns||ns>1024||!range(base,so,ns,68))return false;
    Bytes out=base;
    for(unsigned bone=first+1;bone<n;++bone){ // The top cape bone remains wholly native.
        const unsigned track=bo+bone*108+40;
        const unsigned nr=u32(base,track+4),ro=u32(base,track+8),nt=u32(base,track+12),to=u32(base,track+16),nk=u32(base,track+20),ko=u32(base,track+24);
        if(!nk)continue;
        if(u16(base,track)>1||u16(base,track+2)!=65535||nk!=nt||nr<ns||!range(base,ro,nr,8)||!range(base,to,nt,4)||!range(base,ko,nk,16))return false;
        std::vector<unsigned> ranges,times;std::vector<Q> keys;
        bool changed=false;
        for(unsigned seq=0;seq<nr;++seq){
            const unsigned lo=u32(base,ro+seq*8),hi=u32(base,ro+seq*8+4);
            if(lo>hi||hi>=nk)return false;
            const int group=seq<ns?category(u16(base,so+seq*68)):-1;
            const unsigned gain=group>=0?amounts[group]:100;
            const auto start=keys.size();Q center{},reference{};
            for(unsigned k=lo;k<=hi;++k){
                Q q;std::memcpy(q.data(),base.data()+ko+k*16,16);
                if(!normalize(q))return false;
                if(k==lo)reference=q;
                const float sign=dot(q,reference)<0?-1.f:1.f;
                for(unsigned i=0;i<4;++i)center[i]+=sign*q[i];
                // Preserve bytes exactly for unaffected clips, including their signs.
                std::memcpy(q.data(),base.data()+ko+k*16,16);keys.push_back(q);times.push_back(u32(base,to+k*4));
            }
            if(gain!=100||(group>=0&&options!=advancedDefaults())){
                if(!normalize(center))return false;
                if(options==advancedDefaults()){
                    for(unsigned k=start;k<keys.size();++k){normalize(keys[k]);keys[k]=scaleRotation(keys[k],center,gain*.01f);}
                }else{
                    std::vector<Vector> vectors;
                    for(unsigned k=start;k<keys.size();++k){normalize(keys[k]);vectors.push_back(rotationVector(keys[k],center));}
                    smooth(vectors,times,static_cast<unsigned>(start),options[4],group>=0&&group<3);
                    const float hem=bone==n-1?options[5]*.01f:1.f;
                    const float lean=bone==first+1?(int(options[3])-30)*.01745329252f:0.f;
                    const Q leanRotation{{0,std::sin(lean*.5f),0,std::cos(lean*.5f)}};
                    for(unsigned k=0;k<vectors.size();++k){
                        auto v=vectors[k];v[0]*=gain*.01f*options[1]*.01f*hem;v[1]*=gain*.01f*options[0]*.01f*hem;v[2]*=gain*.01f*options[2]*.01f*hem;
                        keys[start+k]=multiply(leanRotation,multiply(fromRotationVector(v),center));normalize(keys[start+k]);
                    }
                }
                changed=true;
            }
            ranges.push_back(static_cast<unsigned>(start));ranges.push_back(static_cast<unsigned>(keys.size()-1));
        }
        if(changed){
            put(out,track+8,append(out,ranges.data(),ranges.size()*4));
            put(out,track+12,times.size());put(out,track+16,append(out,times.data(),times.size()*4));
            put(out,track+20,keys.size());put(out,track+24,append(out,keys.data(),keys.size()*16));
        }
    }
    if(out.size()>32*1024*1024)return false;
    result.swap(out);return true;
}
inline bool readFile(const std::string& path,Bytes& bytes){std::ifstream f(path,std::ios::binary|std::ios::ate);if(!f)return false;auto n=f.tellg();if(n<0||n>32*1024*1024)return false;bytes.resize(static_cast<unsigned>(n));f.seekg(0);return bool(f.read(reinterpret_cast<char*>(bytes.data()),bytes.size()));}
inline unsigned crc(const Bytes& bytes){unsigned c=~0u;for(auto v:bytes){c^=v;for(unsigned i=0;i<8;++i)c=(c>>1)^(0xedb88320u&-(c&1));}return ~c;}
inline bool cachePath(const char* path){
    if(!path)return false;std::string s(path);for(auto& c:s){if(c=='\\')c='/';if(c>='A'&&c<='Z')c+=32;}
    unsigned body,a,b,c,d,f,side,t,l,smooth,hem,hash;int n=0;
    if(std::sscanf(s.c_str(),"interface/addons/saurekscloset/capemotion/cache/n%u_%u_%u_%u_%u_%u_%u_%u_%u_%u_%u_%8x.m2%n",&body,&a,&b,&c,&d,&f,&side,&t,&l,&smooth,&hem,&hash,&n)!=12||unsigned(n)!=s.size()||body<1||body>16||a>200||b>200||c>200||d>200||f>200||side>200||t>200||l>60||smooth>100||hem>200)return false;
    char expected[256];std::snprintf(expected,sizeof(expected),"interface/addons/saurekscloset/capemotion/cache/n%02u_%03u_%03u_%03u_%03u_%03u_%03u_%03u_%03u_%03u_%03u_%08x.m2",body,a,b,c,d,f,side,t,l,smooth,hem,hash);return s==expected;
}
inline const char* prepare(unsigned body,const Amounts& amounts,const Advanced& options=advancedDefaults()){
    if(body<1||body>16||!valid(amounts)||!valid(options))return nullptr;
    std::array<unsigned,11> key{{body,amounts[0],amounts[1],amounts[2],amounts[3],options[0],options[1],options[2],options[3],options[4],options[5]}};
    static std::map<std::array<unsigned,11>,std::string> ready;
    auto found=ready.find(key);if(found!=ready.end())return found->second.c_str();
    char source[120];std::snprintf(source,sizeof(source),"Interface/AddOns/SaureksCloset/CapeMotion/B%02u.m2",body);
    Bytes base,out;if(!readFile(source,base)||!build(base,body,amounts,out,options))return nullptr;
    const char* directory="Interface/AddOns/SaureksCloset/CapeMotion/Cache";
#ifdef _WIN32
    _mkdir(directory);
#else
    mkdir(directory,0755);
#endif
    char path[256];std::snprintf(path,sizeof(path),"%s/N%02u_%03u_%03u_%03u_%03u_%03u_%03u_%03u_%03u_%03u_%03u_%08x.m2",directory,body,amounts[0],amounts[1],amounts[2],amounts[3],options[0],options[1],options[2],options[3],options[4],options[5],crc(out));
    Bytes existing;if(!readFile(path,existing)||existing!=out){
        const auto temp=std::string(path)+".tmp";
        {std::ofstream f(temp,std::ios::binary|std::ios::trunc);if(!f||!f.write(reinterpret_cast<const char*>(out.data()),out.size()))return nullptr;}
        if(std::rename(temp.c_str(),path)){std::remove(temp.c_str());return nullptr;}
    }
    return ready.emplace(key,path).first->second.c_str();
}
inline bool pathEqual(const char* a,const char* b){if(!a||!b)return false;for(;*a&&*b;++a,++b){auto x=*a,y=*b;if(x>='A'&&x<='Z')x+=32;if(y>='A'&&y<='Z')y+=32;if(x=='/')x='\\';if(y=='/')y='\\';if(x!=y)return false;}return !*a&&!*b;}
inline const char* model(const char* original,bool enabled,const Amounts& amounts,const Advanced& options=advancedDefaults()){
    if(!enabled||(amounts==defaults()&&options==advancedDefaults()))return original;
    for(unsigned i=0;i<16;++i){std::string m2=raceModels[i].filename;m2.replace(m2.size()-3,3,"m2");
        if(pathEqual(original,raceModels[i].filename)||pathEqual(original,m2.c_str())){const auto* path=prepare(i+1,amounts,options);return path?path:original;}}
    return original;
}
}
