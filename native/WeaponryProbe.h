#pragma once
#include <array>
#include <cstdint>
// Read-only inspection of build 5875's CM2 child list. No native model methods,
// retained references, allocation, detach, or writes to the game are involved.
struct WeaponryChild {
    std::uint32_t model=0,parent=0,attachment=0,bone=0,loaded=0,references=0,next=0;
};
template<class Read> int inspectWeaponryChild(std::uint32_t parent,unsigned ordinal,Read read,WeaponryChild& out){
    if(!parent||ordinal>=64)return -2;
    std::array<std::uint32_t,64> seen{};
    std::uint32_t child=0;
    if(!read(parent+0x1dc,child))return -1;
    for(unsigned i=0;i<=ordinal;++i){
        if(!child)return 0;
        for(unsigned j=0;j<i;++j)if(seen[j]==child)return -1;
        seen[i]=child;
        WeaponryChild current;current.model=child;
        if(!read(child+0x1cc,current.parent)||current.parent!=parent||
           !read(child+0x1d0,current.attachment)||!read(child+0x1d4,current.bone)||
           !read(child+0x10,current.loaded)||!read(child,current.references)||
           !read(child+0x1e4,current.next))return -1;
        if(i==ordinal){out=current;return 1;}
        child=current.next;
    }
    return -1;
}
