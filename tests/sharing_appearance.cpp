#include <array>
#include <map>
#include <cstring>
#include <cassert>
#include <iostream>
#include "../native/Appearance.h"
std::map<std::uintptr_t,unsigned char> memory;
template<class T>void put(std::uintptr_t a,T value){const auto* bytes=reinterpret_cast<const unsigned char*>(&value);for(unsigned i=0;i<sizeof(T);++i)memory[a+i]=bytes[i];}
template<class T>bool read(std::uintptr_t a,T& value){auto* bytes=reinterpret_cast<unsigned char*>(&value);for(unsigned i=0;i<sizeof(T);++i){auto p=memory.find(a+i);if(p==memory.end())return false;bytes[i]=p->second;}return true;}
std::uint64_t getPlayer(){return 1;}
void* objectPtr(unsigned,const char*,std::uint64_t guid,int){return guid==2?reinterpret_cast<void*>(0x1000):nullptr;}
struct Player{std::uintptr_t unit=0,fields=0,model=0,component=0;std::uint64_t guid=0;unsigned display=0,native=0,identity=0,body=0,facial=0;};
#include "../native/SharingAppearance.h"
int main(){const std::uintptr_t unit=0x1000,fields=0x2000,model=0x3000,component=0x4000;
put(unit+0x14,4u);put(unit+8,fields);put(fields,std::uint64_t(2));put(fields+0x83*4,49u);put(fields+0x84*4,49u);put(fields+0x24*4,0u);put(fields+0xc1*4,0u);put(fields+0xc2*4,0u);put(unit+0xd8,model);put(unit+0xd30,component);
SharedAppearance entry;entry.snapshot.look.body={1,1,1,0,0,0,0,0};assert(sharedBody(entry.snapshot.look).valid());entry.snapshot.look.items[0]=1234;entry.model=model;sharedAppearances[2]=entry;
auto* native=reinterpret_cast<void*>(0x5555);auto* unitPtr=reinterpret_cast<void*>(unit);
for(auto caller:{0x4c8428u,0x533319u,0x5189f1u,0x52d909u,0x5ec412u,0u})assert(sharedVisibleItem(unitPtr,0,caller,native)==native);
auto* copy=static_cast<std::uint32_t*>(sharedVisibleItem(unitPtr,0,0x5fb551,native));assert(copy!=native&&copy[2]==1234);
sharedAppearances[2].snapshot.look.items[0]=0;assert(static_cast<std::uint32_t*>(sharedVisibleItem(unitPtr,0,0x5fb551,native))[2]==0);
sharedAppearances[2].snapshot.look.items[0]=sharing::inherit;assert(sharedVisibleItem(unitPtr,0,0x5fb551,native)==native);
assert(sharedVisibleItem(unitPtr,-1,0x5fb551,native)==native&&sharedVisibleItem(unitPtr,19,0x5fb551,native)==native);
std::array<std::uint32_t,91> descriptor;for(unsigned i=0;i<91;++i)descriptor[i]=0x10000+i;descriptor[8]=model;auto original=descriptor;assert(sharedCompose(component,descriptor));for(unsigned i=0;i<91;++i)if(i!=0&&i!=1&&i!=2&&i!=3&&i!=5&&i!=6&&i!=7)assert(descriptor[i]==original[i]);assert(sharedName(unit));assert(sharedWorldIdentity(2,unit,model));assert(!sharedWorldIdentity(2,unit,model+1));
put(fields+0x83*4,999u);assert(!sharedName(unit));assert(sharedScale(model)==1);assert(!sharedWorldIdentity(2,unit,model));put(fields+0x83*4,49u);put(fields,std::uint64_t(3));assert(!sharedName(unit));assert(sharedVisibleItem(unitPtr,0,0x5fb551,native)==native);
std::cout<<"PASS: remote body and armor isolation, real inventory/tooltip reads, hide/inherit, shape changes and recycled object identity\n";}
