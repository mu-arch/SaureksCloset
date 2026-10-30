#pragma once
#include <array>
#include <cstdint>
#include <cstring>

// Short-lived read cache for immutable mesh data during one capture. Failed
// batches fall back to the exact original scalar read, including at allocation
// and page boundaries. Never retain game addresses between render callbacks.
template<class T,unsigned Stride=sizeof(T),unsigned Count=32> struct ReadBatch {
    static_assert(Stride>=sizeof(T)&&Count>0,"invalid read layout");
    std::uintptr_t base;
    unsigned size,block=~0u;
    bool loaded=false;
    std::array<unsigned char,Stride*Count> bytes{};
    template<class Read> bool get(unsigned index,T& value,Read read){
        if(index>=size)return false;
        const unsigned first=index-index%Count;
        if(block!=first){
            block=first;
            loaded=size-first>=Count&&read(base+std::uintptr_t(first)*Stride,bytes);
        }
        if(loaded){std::memcpy(&value,bytes.data()+(index-first)*Stride,sizeof(T));return true;}
        return read(base+std::uintptr_t(index)*Stride,value);
    }
};
