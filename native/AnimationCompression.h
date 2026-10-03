#pragma once
#include <vector>
#include <cstdint>
namespace animationCompression {
using Bytes=std::vector<unsigned char>;
inline bool unpack(const Bytes& in,Bytes& out){
    if(in.size()<8||in[0]!='S'||in[1]!='L'||in[2]!='Z'||in[3]!='1')return false;
    unsigned size=0;for(unsigned i=0;i<4;++i)size|=unsigned(in[4+i])<<(8*i);
    if(size>32*1024*1024)return false;
    Bytes data;data.reserve(size);unsigned p=8;
    while(p<in.size()){
        if(in.size()-p<6)return false;
        const unsigned literal=in[p]|unsigned(in[p+1])<<8,match=in[p+2]|unsigned(in[p+3])<<8,distance=in[p+4]|unsigned(in[p+5])<<8;p+=6;
        if(literal>in.size()-p||literal>size-data.size())return false;
        data.insert(data.end(),in.begin()+p,in.begin()+p+literal);p+=literal;
        if(match){if(match<8||!distance||distance>data.size()||match>size-data.size())return false;for(unsigned i=0;i<match;++i)data.push_back(data[data.size()-distance]);}
        else if(distance)return false;
    }
    if(data.size()!=size)return false;out.swap(data);return true;
}
}
