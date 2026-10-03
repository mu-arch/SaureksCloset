#pragma once
#include "../native/Appearance.h"
static std::map<std::uint64_t,std::uintptr_t> sharedObjects;
static void* objectPtr(unsigned,const char*,std::uint64_t guid,int){return pointer(sharedObjects[guid]);}
template<std::size_t N>static bool read(std::uintptr_t a,std::array<std::uint32_t,N>& out){for(unsigned i=0;i<N;++i)if(!read(a+4*i,out[i]))return false;return true;}
#include "../native/SharingAppearance.h"
