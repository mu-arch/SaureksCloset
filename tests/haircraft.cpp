#include "../native/Haircraft.h"
#include <array>
#include <cassert>
#include <iostream>
int main(){
    using haircraft::owns;
    assert(owns(7,7,true,10,20,10,20,true,0));
    assert(owns(7,7,true,30,40,10,20,true,7));
    assert(!owns(7,7,false,10,20,10,20,true,0));
    assert(!owns(7,8,true,10,20,10,20,true,0));
    assert(!owns(0,0,true,10,20,10,20,true,0));
    assert(!owns(7,7,true,10,0,10,0,true,0));
    assert(!owns(7,7,true,10,21,10,20,true,0));
    assert(!owns(7,7,true,10,20,10,20,false,0));
    assert(!owns(7,7,true,30,40,10,20,true,8));
    assert(!owns(7,7,true,30,40,10,20,true,0));
    // Preserve native hat attachment and ears/facial masks. Only restore hair.
    std::array<unsigned,400> memory{};auto* component=memory.data();
    const auto address=reinterpret_cast<std::uintptr_t>(component);
    unsigned hatAttachments=0,resolves=0,chosenGroup=12;
    auto original=[&](void* p){assert(p==component);++hatAttachments;
        memory[0x144/4]=1;memory[0x148/4]=101;memory[0x160/4]=701;};
    auto resolve=[&](void* p){assert(reinterpret_cast<std::uintptr_t>(p)==address+0x18);++resolves;return chosenGroup;};
    auto store=[&](std::uintptr_t at,unsigned value){assert(at==address+0x144);memory[0x144/4]=value;};
    for(unsigned race=1;race<=8;++race)for(unsigned sex=0;sex<2;++sex){
        memory[0x18/4]=race;memory[0x1c/4]=sex;memory[0x34/4]=6;
        for(unsigned style:{1u,2u,12u,25u}){
            chosenGroup=style;
            for(unsigned rebuild=0;rebuild<5;++rebuild){
                auto before=memory;original(component);auto native=memory;memory=before;
                haircraft::compose(component,true,original,resolve,store);
                native[0x144/4]=style;assert(memory==native);
            }
        }
    }
    const auto before=resolves;
    haircraft::compose(component,false,original,resolve,store);
    assert(memory[0x144/4]==1&&resolves==before);
    chosenGroup=8;haircraft::compose(component,true,original,resolve,store);
    assert(memory[0x144/4]==8&&hatAttachments>0);
    std::cout<<"Haircraft: hat composition, selected/bald hair, repeated rebuilds, default restoration and ownership isolation passed\n";
}
