#include "../native/ArmorInspection.h"
#include <cassert>
#include <cstring>
#include <iostream>
#include <map>
#include <string>

// Distinct component rows and attached meshes reproduce the failure where the
// helper's item field is correct but the already-built armor is different.
struct Memory {
    std::map<std::uint32_t,unsigned char> bytes;
    template<class T> void put(std::uint32_t address,T value){
        unsigned char data[sizeof(T)];std::memcpy(data,&value,sizeof(value));
        for(unsigned i=0;i<sizeof(T);++i)bytes[address+i]=data[i];
    }
    void path(std::uint32_t address,const std::string& value){
        for(unsigned i=0;i<=value.size();++i)put<char>(address+i,i<value.size()?value[i]:0);
    }
    template<class T> bool read(std::uint32_t address,T& value) const {
        unsigned char data[sizeof(T)];
        for(unsigned i=0;i<sizeof(T);++i){auto found=bytes.find(address+i);if(found==bytes.end())return false;data[i]=found->second;}
        std::memcpy(&value,data,sizeof(value));return true;
    }
};
static constexpr std::uint32_t component=0x1000,model=0x2000,row=0x3000;
static Memory fixture(){
    Memory memory;
    memory.put(component+0x38,model);memory.put(model+0x10,1u);
    memory.put(component+0x10,0u);memory.put<unsigned char>(component+0x8,0);
    for(unsigned i=0;i<11;++i)memory.put(component+0x4A8+4*i,0u);
    memory.put(model+0x1DC,0u);memory.put(row,900u);
    memory.put(row+4,0x6000u);memory.put(row+8,0x6200u);
    memory.path(0x6000,"");memory.path(0x6200,"");
    return memory;
}
static void display(Memory& memory,unsigned slot,std::uint32_t address=row){
    memory.put(component+0x4A8+4*static_cast<unsigned>(armorComponentSlot(slot)),address);
}
static void child(Memory& memory,std::uint32_t address,unsigned point,std::uint32_t next=0,
        const std::string& name="Spell\\Holy_DivineSpirit"){
    memory.put(address+0x1CC,model);memory.put(address+0x1D0,point);memory.put(address+0x1E4,next);
    memory.put(address+0x10,1u);memory.put(address+0x30,address+0x40000);
    memory.path(address+0x40020,name);
}
static int inspect(const Memory& memory,unsigned slot,unsigned expected,ArmorInspection* captured=nullptr){
    ArmorInspection local;auto read=[&](std::uint32_t address,auto& value){return memory.read(address,value);};
    return inspectArmor(component,model,slot,expected,read,captured?*captured:local);
}
int main(){
    constexpr unsigned slots[]={1,3,4,5,6,7,8,9,10,19,15};
    for(unsigned i=0;i<11;++i){
        auto memory=fixture();assert(armorComponentSlot(slots[i])==static_cast<int>(i));
        assert(inspect(memory,slots[i],0)==1);
        display(memory,slots[i]);const auto before=memory.bytes;
        ArmorInspection output;
        assert(inspect(memory,slots[i],900,&output)==1&&output.display==900);
        assert(inspect(memory,slots[i],901)==0);
        assert(inspect(memory,slots[i],0)==0);
        assert(memory.bytes==before); // All paths are read-only.
    }
    // Independently authored client layout: never derive these fixtures from
    // the mapping under test. A visible cape and tabard must stay distinct.
    auto split=fixture();
    split.put(component+0x4CC,0x7000u);split.put(0x7000,23140u);
    split.put(component+0x4D0,0x7100u);split.put(0x7100,54321u);
    assert(inspect(split,19,23140)==1&&inspect(split,15,54321)==1);
    assert(inspect(split,19,54321)==0&&inspect(split,15,23140)==0);
    split.put(component+0x4CC,0u);
    assert(inspect(split,19,0)==1&&inspect(split,15,54321)==1);
    split.put(component+0x4CC,0x7000u);split.put(component+0x4D0,0u);
    assert(inspect(split,19,23140)==1&&inspect(split,15,0)==1);
    for(unsigned slot:{0u,2u,11u,12u,13u,14u,16u,17u,18u,20u,999u})
        assert(inspect(fixture(),slot,0)==-2);
    auto memory=fixture();display(memory,3);
    memory.put(component+0x38,0xDEADu);assert(inspect(memory,3,900)==-1);
    memory=fixture();memory.put(model+0x10,0u);assert(inspect(memory,3,0)==-1);
    memory=fixture();memory.put(component+0x10,4u);ArmorInspection output;
    assert(inspect(memory,3,0,&output)==-1&&output.dirty==4);
    memory=fixture();memory.put<unsigned char>(component+0x8,1);assert(inspect(memory,3,0)==-1);
    memory=fixture();display(memory,3,0xDEAD);assert(inspect(memory,3,900)==-1);
    memory=fixture();display(memory,3);memory.put(row,0u);assert(inspect(memory,3,0)==-1);

    // Hidden shoulders/head whose row is already null still detect reappeared
    // equipment children, without treating spell effects as armor.
    for(unsigned point:{5u,6u,11u}){
        const unsigned slot=point==11?1:3;
        memory=fixture();memory.put(model+0x1DC,0x4000u);child(memory,0x4000,point);
        assert(inspect(memory,slot,0)==1);
        child(memory,0x4000,point,0,point==11?
            "ITEM/OBJECTCOMPONENTS/HEAD/Helm_Cloth_HuM":
            "ITEM/OBJECTCOMPONENTS/SHOULDER/LShoulder_Plate");
        assert(inspect(memory,slot,0,&output)==0&&output.attachmentMask);
        memory.put(0x4010,0u);assert(inspect(memory,slot,0)==-1);
    }
    // Hand weapons / decorative storage never invalidate an armor inspection.
    memory=fixture();memory.put(model+0x1DC,0x4000u);child(memory,0x4000,1);
    memory.put(0x4010,0u);assert(inspect(memory,3,0)==1);

    // Shoulder display row and child meshes are independent. A one-sided item
    // is valid; a wrong, extra, duplicate, or missing armor child is drift.
    memory=fixture();display(memory,3);memory.path(0x6000,"LShoulder_Plate_A.MDX");
    memory.put(model+0x1DC,0x4000u);
    child(memory,0x4000,6,0,"Item\\ObjectComponents\\Shoulder\\LShoulder_Plate_A");
    assert(inspect(memory,3,900)==1);
    child(memory,0x4000,6,0,"ITEM/OBJECTCOMPONENTS/SHOULDER/LSHOULDER_PLATE_A.M2");
    assert(inspect(memory,3,900)==1);
    child(memory,0x4000,6,0,"Item\\ObjectComponents\\Shoulder\\LShoulder_Real");
    assert(inspect(memory,3,900)==0);
    child(memory,0x4000,6,0,"Item\\ObjectComponents\\Shoulder\\LShoulder_Plate_A");
    memory.put(0x41E4,0x5000u);child(memory,0x5000,5,0,"Item\\ObjectComponents\\Shoulder\\RShoulder_Plate_A");
    assert(inspect(memory,3,900)==0); // Unselected side should be empty.
    memory.path(0x6200,"RShoulder_Plate_A.mdx");assert(inspect(memory,3,900)==1);
    memory.put(0x41E4,0u);assert(inspect(memory,3,900)==0); // Missing selected side.
    memory.path(0x6200,"");memory.put(0x41E4,0x5000u);
    child(memory,0x5000,6,0,"Item\\ObjectComponents\\Shoulder\\LShoulder_Plate_A");
    assert(inspect(memory,3,900)==0); // Duplicate equipment, even matching mesh.
    child(memory,0x5000,6);assert(inspect(memory,3,900)==1); // Spell can coexist.
    memory.put(model+0x1DC,0u);assert(inspect(memory,3,900)==0);

    // Positive helmets deliberately only compare the compositor row: native
    // Show Helm suppression must not look like a missing attachment failure.
    memory=fixture();display(memory,1);assert(inspect(memory,1,900)==1);

    // Bad reads, corrupt ownership, cycles and excessive lists fail closed.
    memory=fixture();memory.put(model+0x1DC,0x4000u);child(memory,0x4000,5);
    memory.put(0x41CC,0x9999u);assert(inspect(memory,3,0)==-1);
    child(memory,0x4000,5,0x4000);assert(inspect(memory,3,0)==-1);
    child(memory,0x4000,5);memory.bytes.erase(0x44020);assert(inspect(memory,3,0)==-1);
    child(memory,0x4000,5);for(unsigned i=0;i<260;++i)memory.put<char>(0x44020+i,'x');
    assert(inspect(memory,3,0)==-1);
    memory=fixture();memory.put(model+0x1DC,0x10000u);
    for(unsigned i=0;i<64;++i)child(memory,0x10000+i*0x200,1,i<63?0x10200+i*0x200:0);
    assert(inspect(memory,3,0)==1);
    memory.put(0x10000+63*0x200+0x1E4,0x30000u);child(memory,0x30000,1);
    assert(inspect(memory,3,0)==-1);
    std::cout<<"Armor inspection: all 11 slots, hidden and custom shoulders, loading, effects, ownership, bounds and read-only behavior passed\n";
}
