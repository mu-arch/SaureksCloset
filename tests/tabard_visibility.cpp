#include "../native/TabardVisibility.h"
#include <cassert>
#include <iostream>
#include <map>

struct Client {
    static constexpr std::uintptr_t player=0x1000,component=0x2000,record=0x3000,row=0x4000;
    std::map<std::uintptr_t,unsigned> memory{{record+8,0},{component+0x4D0,row},{row,23140},{row+0x24,1}};
    std::map<unsigned,int> displays{{5976,23140},{100,900}};
    bool recordAvailable=true;
    unsigned lookups=0,resolves=0,rendered=0,emblemWrites=0;
    auto lookup(){return [this](void* unit,int slot)->void*{
        assert(reinterpret_cast<std::uintptr_t>(unit)==player&&slot==18);
        ++lookups;return recordAvailable?reinterpret_cast<void*>(record):nullptr;
    };}
    auto read(){return [this](std::uintptr_t address,unsigned& value){
        auto it=memory.find(address);if(it==memory.end())return false;value=it->second;return true;
    };}
    auto resolve(){return [this](void* input){
        assert(reinterpret_cast<std::uintptr_t>(input)==record);++resolves;
        return displays[memory[record+8]];
    };}
    unsigned update(unsigned incoming=23140,std::uintptr_t unit=player,std::uintptr_t owner=player){
        const unsigned display=tabardDisplayForUpdate(reinterpret_cast<void*>(unit),incoming,owner,lookup(),read(),resolve());
        // Native 5E0720 resolves a positive display, then writes the tabard row.
        if(display){rendered=display;memory[row]=display;}
        return display;
    }
    bool emblem(std::uintptr_t target=component,std::uintptr_t owner=player,std::uintptr_t liveComponent=component){
        bool allow=allowGuildTabardTextures(reinterpret_cast<void*>(target),liveComponent,owner,lookup(),read(),resolve());
        if(allow)++emblemWrites;
        return allow;
    }
};
int main(){
    Client c;
    // Real equipped guild tabard refreshes cannot restore a hidden tabard.
    // Cover repeated rebuilds, inventory refreshes, and guild data arrivals.
    const auto fields=c.memory;
    for(int i=0;i<50;++i){assert(c.update()==0);assert(!c.emblem());}
    assert(c.rendered==0&&c.emblemWrites==0&&c.resolves==0&&c.memory==fields);

    // Showing it again (including disabling the addon) uses current visible
    // fields immediately, without a sticky hidden-state cache.
    c.memory[Client::record+8]=5976;
    assert(c.update()==23140&&c.emblem());
    c.memory[Client::record+8]=0;c.rendered=0;
    assert(c.update()==0&&!c.emblem()); // delayed work queued before hiding
    assert(c.rendered==0);

    // Choosing a different tabard replaces stale real-item displays. A pending
    // guild response must not paint that non-guild tabard (or an old row).
    c.memory[Client::record+8]=100;
    assert(!c.emblem()); // new selection while old guild row still exists
    assert(c.update()==900&&c.rendered==900);
    c.memory[Client::row+0x24]=0;assert(!c.emblem());
    c.memory[Client::record+8]=5976;assert(!c.emblem());
    c.update();c.memory[Client::row+0x24]=1;assert(c.emblem());

    // While selected item data is loading, do not fall back to the real tabard.
    c.memory[Client::record+8]=999;c.rendered=0;
    assert(c.update()==0&&!c.emblem()&&c.rendered==0);
    c.displays[999]=777;c.memory[Client::row]=777;c.memory[Client::row+0x24]=1;
    assert(c.update()==777&&c.emblem());
    c.memory[Client::component+0x4D0]=0;assert(!c.emblem());

    // Other players, previews, unavailable owners and transformed players
    // (the bridge supplies no owner for transformations) retain native data.
    const auto calls=c.lookups;
    assert(c.update(23140,0x9999)==23140);
    assert(c.update(23140,Client::player,0)==23140);
    assert(c.emblem(0x9999)&&c.emblem(Client::component,0)&&c.emblem(Client::component,Client::player,0));
    assert(c.lookups==calls);

    // Failed ownership/record reads must not invent a hidden selection.
    c.recordAvailable=false;assert(c.update()==23140&&c.emblem());
    c.recordAvailable=true;c.memory.erase(Client::record+8);
    assert(c.update()==23140&&c.emblem());
    c.memory[Client::record+8]=5976;c.memory.erase(Client::component+0x4D0);
    assert(c.emblem());
    c.memory[Client::component+0x4D0]=Client::row;c.memory.erase(Client::row+0x24);
    assert(c.emblem());
    std::cout<<"Tabard visibility: hidden/visible/replaced/loading selections, repeated real-item refreshes, delayed guild textures, owner isolation and read failures passed\n";
}
