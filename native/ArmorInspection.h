#pragma once
#include <array>
#include <cstdint>
#include "ArmorSlots.h"

// Read-only build-5875 armor composition inspection. These are the compositor's
// ItemDisplayInfo rows, not the player's virtual item fields. No native methods,
// references, model creation, detach operations, or game-memory writes occur.
struct ArmorInspection {
    unsigned display=0,dirty=0,attachmentMask=0;
};

inline char armorPathCharacter(char value){
    if(value=='/')return '\\';
    return value>='A'&&value<='Z'?static_cast<char>(value-'A'+'a'):value;
}
template<class Read> bool armorReadPath(std::uint32_t address,Read read,std::array<char,260>& path){
    if(!address)return false;
    for(unsigned i=0;i<path.size();){
        std::array<char,32> block{};
        const bool batch=path.size()-i>=block.size()&&read(address+i,block);
        const unsigned count=path.size()-i>=block.size()?static_cast<unsigned>(block.size()):static_cast<unsigned>(path.size()-i);
        for(unsigned n=0;n<count;++n,++i){
            char value=block[n];if(!batch&&!read(address+i,value))return false;
            path[i]=armorPathCharacter(value);
            if(!value)return true;
        }
    }
    return false;
}
inline bool armorPathPrefix(const std::array<char,260>& path,const char* prefix){
    for(unsigned i=0;prefix[i];++i)if(i>=path.size()||path[i]!=prefix[i])return false;
    return true;
}
inline unsigned armorMeshLength(const std::array<char,260>& path){
    unsigned size=0;while(size<path.size()&&path[size])++size;
    if(size>=4&&path[size-4]=='.'&&path[size-3]=='m'&&path[size-2]=='d'&&path[size-1]=='x')return size-4;
    if(size>=3&&path[size-3]=='.'&&path[size-2]=='m'&&path[size-1]=='2')return size-3;
    return size;
}
inline bool armorShoulderMeshMatches(const std::array<char,260>& actual,const std::array<char,260>& expected){
    constexpr char prefix[]="item\\objectcomponents\\shoulder\\";
    const unsigned offset=sizeof(prefix)-1;
    if(!armorPathPrefix(actual,prefix))return false;
    const unsigned actualSize=armorMeshLength(actual),expectedSize=armorMeshLength(expected);
    if(actualSize!=offset+expectedSize)return false;
    for(unsigned i=0;i<expectedSize;++i)if(actual[offset+i]!=expected[i])return false;
    return true;
}

// 1=matches, 0=confirmed mismatch, -1=unavailable/loading, -2=invalid armor slot.
// The bridge must first validate that this component/model is the current
// player's ordinary playable body, and never pass a UI-preview component.
template<class Read> int inspectArmor(std::uint32_t component,std::uint32_t model,
        unsigned inventorySlot,unsigned expectedDisplay,Read read,ArmorInspection& out){
    out={};
    const int slot=armorComponentSlot(inventorySlot);if(slot<0)return -2;
    if(!component||!model)return -1;
    std::uint32_t owner=0,row=0,loaded=0;unsigned char initializing=0;
    if(!read(component+0x38,owner)||owner!=model||!read(model+0x10,loaded)||!loaded||
       !read(component+0x10,out.dirty)||!read(component+0x8,initializing)||
       !read(component+0x4A8+4*static_cast<unsigned>(slot),row))return -1;
    // 0x477860 checks both flags before considering composition complete.
    // Never diagnose a partially constructed body as a visual failure.
    if(out.dirty||initializing)return -1;
    if(row&&(!read(row,out.display)||!out.display))return -1;
    if(out.display!=expectedDisplay)return 0;
    if(inventorySlot!=3&&(inventorySlot!=1||expectedDisplay))return 1;

    // 0x479D10 attaches the first shoulder mesh at point 6 and the second at 5.
    // Empty mesh names are valid: many items deliberately use only one side.
    std::array<std::array<char,260>,2> shoulderNames{};
    if(inventorySlot==3&&expectedDisplay){
        for(unsigned i=0;i<2;++i){
            std::uint32_t name=0;
            if(!read(row+4+4*i,name)||!armorReadPath(name,read,shoulderNames[i]))return -1;
        }
    }
    std::array<std::uint32_t,64> seen{};
    std::uint32_t child=0;
    if(!read(model+0x1DC,child))return -1;
    bool mismatch=false;unsigned matched=0;
    for(unsigned i=0;child;++i){
        if(i>=seen.size())return -1;
        for(unsigned j=0;j<i;++j)if(seen[j]==child)return -1;
        seen[i]=child;
        std::uint32_t parent=0,point=0,next=0;
        if(!read(child+0x1CC,parent)||parent!=model||!read(child+0x1D0,point)||
           !read(child+0x1E4,next))return -1;
        const bool relevant=inventorySlot==1?point==11:(point==5||point==6);
        if(relevant){
            std::uint32_t resource=0;std::array<char,260> path{};
            if(!read(child+0x10,loaded)||!loaded||!read(child+0x30,resource)||!resource||
               !armorReadPath(resource+0x20,read,path))return -1;
            const bool armor=armorPathPrefix(path,inventorySlot==1?
                "item\\objectcomponents\\head\\":"item\\objectcomponents\\shoulder\\");
            // Spell effects can use the same attachment points. Their presence
            // is not proof that hidden armor has come back.
            if(armor){
                const unsigned bit=inventorySlot==1?1u:(point==6?2u:4u);
                if(out.attachmentMask&bit)mismatch=true;
                out.attachmentMask|=bit;
                if(!expectedDisplay)mismatch=true;
                else{
                    const unsigned side=point==6?0:1;
                    if(!shoulderNames[side][0]||!armorShoulderMeshMatches(path,shoulderNames[side]))mismatch=true;
                    else matched|=1u<<side;
                }
            }
        }
        child=next;
    }
    if(inventorySlot==3&&expectedDisplay)for(unsigned i=0;i<2;++i)
        if(shoulderNames[i][0]&&!(matched&(1u<<i)))mismatch=true;
    return mismatch?0:1;
}
