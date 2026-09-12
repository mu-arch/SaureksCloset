#include "../native/WeaponryProbe.h"
#include <map>
#include <cassert>
#include <iostream>
int main(){
    std::map<std::uint32_t,std::uint32_t> memory;
    auto read=[&](std::uint32_t address,std::uint32_t& value){auto i=memory.find(address);if(i==memory.end())return false;value=i->second;return true;};
    WeaponryChild out;
    assert(inspectWeaponryChild(0,0,read,out)==-2);
    assert(inspectWeaponryChild(0x1000,64,read,out)==-2);
    assert(inspectWeaponryChild(0x1000,0,read,out)==-1);
    memory[0x11dc]=0;assert(inspectWeaponryChild(0x1000,0,read,out)==0);
    auto child=[&](unsigned address,unsigned next,unsigned point){
        memory[address+0x1cc]=0x1000;memory[address+0x1d0]=point;memory[address+0x1d4]=point+1;
        memory[address+0x10]=1;memory[address]=2;memory[address+0x1e4]=next;
    };
    memory[0x11dc]=0x2000;child(0x2000,0x3000,26);child(0x3000,0,32);
    assert(inspectWeaponryChild(0x1000,0,read,out)==1&&out.model==0x2000&&out.attachment==26);
    assert(inspectWeaponryChild(0x1000,1,read,out)==1&&out.model==0x3000&&out.attachment==32);
    assert(inspectWeaponryChild(0x1000,2,read,out)==0);
    memory[0x31e4]=0x2000;assert(inspectWeaponryChild(0x1000,2,read,out)==-1);
    memory[0x31cc]=0x9000;assert(inspectWeaponryChild(0x1000,1,read,out)==-1);
    memory.erase(0x21d4);assert(inspectWeaponryChild(0x1000,0,read,out)==-1);
    std::cout<<"Weapon attachment reader: bounds, missing data, parent ownership, traversal and cycles passed\n";
}
