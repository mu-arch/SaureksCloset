#pragma once
#include <atomic>
#include "UpdateManifest.h"
// One worker owns HTTP handles and writes the payload. Lua only polls from the
// game thread. Generation IDs discard requests cancelled by the preference.
struct UpdateMailbox {
    std::atomic<bool> enabled{false},running{false};
    std::atomic<unsigned> generation{0},published{0};
    std::atomic<int> state{0}; // 0 idle, 1 checking, 2 ready, -1 failed
    UpdateRelease release{};unsigned error=0;
    void enable(bool value){
        if(enabled.exchange(value)!=value){++generation;state=0;}
    }
    bool begin(unsigned& ticket){
        if(!enabled||running.exchange(true))return false;
        ticket=generation;state=1;return true;
    }
    bool cancelled(unsigned ticket)const{return !enabled||generation!=ticket;}
    void finish(unsigned ticket,const UpdateRelease& value,unsigned failure){
        release=value;error=failure;published=ticket;state=failure?-1:2;running=false;
    }
    int status()const{
        if(!enabled)return -2;
        const int current=state;
        if(current==1)return 1;
        return published==generation?current:0;
    }
};
