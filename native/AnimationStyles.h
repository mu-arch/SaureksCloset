#pragma once
#include "AnimationCompression.h"
#include <vector>
#include <algorithm>
#include <string>
#include <fstream>
#include <map>
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <cmath>
#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif
namespace animationStyles {
using Bytes=std::vector<unsigned char>;
inline unsigned u16(const Bytes& b,unsigned p){return b[p]|unsigned(b[p+1])<<8;}
inline unsigned u32(const Bytes& b,unsigned p){return u16(b,p)|u16(b,p+2)<<16;}
inline void put(Bytes& b,unsigned p,unsigned n){for(unsigned i=0;i<4;++i)b[p+i]=static_cast<unsigned char>(n>>(8*i));}
inline unsigned crc(const Bytes& b){unsigned c=~0u;for(auto v:b){c^=v;for(unsigned i=0;i<8;++i)c=(c>>1)^(0xedb88320u&-(c&1));}return ~c;}
inline bool valid(unsigned body,unsigned cape){return body<=16&&cape<=16;}
inline bool apply(Bytes& model,const Bytes& patch,unsigned baseSize,unsigned checksum,bool cape){
    if(model.size()<60||patch.size()<16||std::memcmp(patch.data(),"SCA1",4)||u32(patch,4)!=baseSize||u32(patch,8)!=checksum)return false;
    const unsigned bones=u32(model,52),offset=u32(model,56),count=u32(patch,12);
    if(bones>255||count>128||offset>model.size()||bones*108>model.size()-offset)return false;
    unsigned p=16;Bytes out=model;std::vector<unsigned> seen;
    for(unsigned i=0;i<count;++i){
        if(p+8>patch.size())return false;
        const unsigned bone=u16(patch,p),track=u16(patch,p+2),length=u32(patch,p+4);p+=8;
        if(bone>=bones||(track!=12&&track!=40)||length<28||length>patch.size()-p||u16(patch,p)>1||u16(patch,p+2)!=65535)return false;
        const auto key=bone*108+track;if(std::find(seen.begin(),seen.end(),key)!=seen.end())return false;seen.push_back(key);
        // Original named/body bones and private cape bones are disjoint.
        const auto originalKey=u32(model,offset+bone*108);
        (void)originalKey;
        if(cape&&track!=40)return false;
        unsigned sizes[]={8,4,track==40?16u:12u};
        for(unsigned j=0;j<3;++j){const auto n=u32(patch,p+4+j*8),at=u32(patch,p+8+j*8);if(n>1000000||at<28||at>length||n>(length-at)/sizes[j])return false;}
        const unsigned nr=u32(patch,p+4),ro=u32(patch,p+8),nt=u32(patch,p+12),to=u32(patch,p+16),nk=u32(patch,p+20),ko=u32(patch,p+24);
        if(!nr||!nk||nt!=nk)return false;
        for(unsigned j=0;j<nr;++j){const auto lo=u32(patch,p+ro+j*8),hi=u32(patch,p+ro+j*8+4);if(lo>hi||hi>=nk)return false;
            for(unsigned k=lo+1;k<=hi;++k)if(u32(patch,p+to+k*4)<u32(patch,p+to+(k-1)*4))return false;}
        for(unsigned j=0;j<nk;++j){float total=0;for(unsigned k=0;k<(track==40?4u:3u);++k){float value;std::memcpy(&value,patch.data()+p+ko+j*sizes[2]+k*4,4);if(!std::isfinite(value))return false;total+=value*value;}
            if(track==40&&(total<.98f||total>1.02f))return false;}
        while(out.size()%16)out.push_back(0);const unsigned start=static_cast<unsigned>(out.size());
        out.insert(out.end(),patch.begin()+p,patch.begin()+p+length);
        for(unsigned j=0;j<3;++j)put(out,start+8+j*8,start+u32(out,start+8+j*8));
        std::memcpy(out.data()+offset+bone*108+track,out.data()+start,28);p+=length;
    }
    if(p!=patch.size()||out.size()>32*1024*1024)return false;
    model.swap(out);return true;
}
inline bool readFile(const std::string& path,Bytes& out){std::ifstream f(path,std::ios::binary|std::ios::ate);if(!f)return false;const auto n=f.tellg();if(n<0||n>32*1024*1024)return false;out.resize(static_cast<unsigned>(n));f.seekg(0);if(!f.read(reinterpret_cast<char*>(out.data()),out.size()))return false;if(out.size()>=4&&!std::memcmp(out.data(),"SLZ1",4)){Bytes decoded;if(!animationCompression::unpack(out,decoded))return false;out.swap(decoded);}return true;}
inline std::string asset(unsigned recipient,unsigned donor,char kind){char b[120];if(kind=='B')std::snprintf(b,sizeof(b),"Interface/AddOns/SaureksCloset/Animations/Styles/B%02u.m2",recipient);else std::snprintf(b,sizeof(b),"Interface/AddOns/SaureksCloset/Animations/Styles/%c%02u_%02u.sca",kind,recipient,donor);return b;}
inline bool cachePath(const char* path){
    if(!path)return false;std::string s(path);for(auto& c:s){if(c=='\\')c='/';if(c>='A'&&c<='Z')c+=32;}
    unsigned r,b,c,g,hash;int n=0;
    if(std::sscanf(s.c_str(),"interface/addons/saurekscloset/animations/cache/r%u_b%u_c%u_g%u_%8x.m2%n",&r,&b,&c,&g,&hash,&n)!=5||unsigned(n)!=s.size()||r<1||r>16||!valid(b,c)||g>1)return false;
    char expected[150];std::snprintf(expected,sizeof(expected),"interface/addons/saurekscloset/animations/cache/r%02u_b%02u_c%02u_g%u_%08x.m2",r,b,c,g,hash);return s==expected;
}
inline const char* prepare(unsigned recipient,unsigned body,unsigned cape,bool calm){
    if(recipient<1||recipient>16||!valid(body,cape))return nullptr;
    if(body==recipient)body=0;if(cape==recipient)cape=0;
    static std::map<unsigned,std::string> ready;const unsigned key=recipient|(body<<5)|(cape<<10)|(unsigned(calm)<<15);
    auto it=ready.find(key);if(it!=ready.end())return it->second.c_str();
    Bytes base,patch;if(!readFile(asset(recipient,0,'B'),base))return nullptr;
    const auto baseSize=static_cast<unsigned>(base.size()),checksum=crc(base);
    if(body&&(!readFile(asset(recipient,body,'A'),patch)||!apply(base,patch,baseSize,checksum,false)))return nullptr;
    if(cape&&(!readFile(asset(recipient,cape,'C'),patch)||!apply(base,patch,baseSize,checksum,true)))return nullptr;
    // Gentler baked HF option is composed as a cape-only patch too.
    if(calm&&recipient==2&&!cape&&(!readFile(asset(2,17,'C'),patch)||!apply(base,patch,baseSize,checksum,true)))return nullptr;
    const char* directory="Interface/AddOns/SaureksCloset/Animations/Cache";
#ifdef _WIN32
    _mkdir(directory);
#else
    mkdir(directory,0755);
#endif
    char name[150];std::snprintf(name,sizeof(name),"%s/R%02u_B%02u_C%02u_G%u_%08x.m2",directory,recipient,body,cape,calm?1:0,crc(base));
    const std::string temp=std::string(name)+".tmp";{std::ofstream f(temp,std::ios::binary|std::ios::trunc);if(!f||!f.write(reinterpret_cast<const char*>(base.data()),base.size()))return nullptr;}
    if(std::rename(temp.c_str(),name)!=0){Bytes existing;if(!readFile(name,existing)||existing!=base){std::remove(temp.c_str());return nullptr;}std::remove(temp.c_str());}
    auto inserted=ready.emplace(key,name);return inserted.first->second.c_str();
}
}
