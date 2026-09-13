// Exercise production voice routing against simulated build-5875 records.
#define __fastcall
#define __thiscall
#include <cassert>
#include <cstdint>
#include <map>
#include <iostream>
#include "../native/Appearance.h"
struct Player { std::uintptr_t unit=0x1000;std::uint64_t guid=42;unsigned identity=1,display=49,native=49; };
static Player player;
static bool ready=true;
static struct { Appearance body;bool enabled=false;std::uint64_t guid=42; } state;
static bool snapshot(Player& p){p=player;return ready;}
static bool applies(const Player& p){return state.enabled&&state.guid==p.guid&&p.display==p.native;}
static std::map<std::uintptr_t,std::uintptr_t> memory;
template<typename T> static bool read(std::uintptr_t address,T& value){
    auto found=memory.find(address);if(found==memory.end())return false;
    value=static_cast<T>(found->second);return true;
}
#include "../native/VoiceRenderer.h"
static unsigned originalRace(void*){return player.identity&255;}
static unsigned originalSex(void*){return (player.identity>>16)&255;}
static void* originalData(void*){return reinterpret_cast<void*>(0xDEAD);}
static unsigned cacheRace=0,cacheSex=2,cacheCalls=0,playedSex=2,playedEvent=0,playCalls=0;
static void cache(unsigned race,unsigned sex){cacheRace=race;cacheSex=sex;++cacheCalls;}
static void play(unsigned sex,unsigned event){playedSex=sex;playedEvent=event;++playCalls;}
static void* unit(){return reinterpret_cast<void*>(player.unit);}
static void record(unsigned id,std::uintptr_t map,std::uintptr_t location){memory[map+id*4]=location;memory[location]=id;}
int main(){
    voiceRaceOriginal=&originalRace;voiceSexOriginal=&originalSex;voiceSoundDataOriginal=&originalData;
    vocalCacheOriginal=&cache;vocalPlayOriginal=&play;
    // Hook entry points must leave non-audio callers alone, too.
    assert(voiceRaceHook(unit(),nullptr)==1&&voiceSexHook(unit(),nullptr)==0);
    assert(voiceSoundDataHook(unit(),nullptr)==originalData(unit()));
    assert(!selectedVoice(player));
    state.enabled=true;
    memory[0xC0DE90]=0x10000;memory[0xC0DE94]=2000;
    memory[0xC0DE68]=0x20000;memory[0xC0DE6C]=500;
    memory[0xC0DE54]=0x30000;memory[0xC0DE58]=500;
    unsigned index=0;
    for(const auto& model:raceModels){
        // IDs from this client's CreatureDisplayInfo / CreatureModelData records.
        unsigned modelId=model.display,sound=model.display;
        if(model.display==1563){modelId=182;sound=294;}
        if(model.display==1564){modelId=183;sound=295;}
        if(model.display==1478){modelId=185;sound=296;}
        if(model.display==1479){modelId=186;sound=297;}
        const auto displayRecord=0x40000+index*0x100,modelRecord=0x50000+index*0x100,soundRecord=0x60000+index*0x100;
        record(model.display,0x10000,displayRecord);record(modelId,0x20000,modelRecord);record(sound,0x30000,soundRecord);
        memory[displayRecord+4]=modelId;memory[displayRecord+8]=index<12?0:sound;
        memory[modelRecord+0x34]=sound;
        state.body.race=model.race;state.body.sex=model.sex;player.identity=model.race==1?2:1;
        auto before=memory;
        const auto identity=player.identity,display=player.display;
        assert(voiceRaceAt(unit(),0x623CDF)==model.race);
        assert(voiceSexAt(unit(),0x623CD7)==model.sex);
        assert(voiceRaceAt(unit(),0x5EF6E4)==originalRace(unit()));
        assert(voiceSexAt(unit(),0x5EF6DC)==originalSex(unit()));
        for(auto caller:{0x6234DB,0x62F8B5})assert(voiceSoundDataAt(unit(),caller)==reinterpret_cast<void*>(soundRecord));
        assert(voiceSoundDataAt(unit(),0x6233E6)==originalData(unit())); // Footsteps.
        assert(voiceSoundDataAt(reinterpret_cast<void*>(0x2000),0x62F8B5)==originalData(unit()));
        assert(voiceRaceAt(reinterpret_cast<void*>(0x2000),0x623CDF)==originalRace(unit()));
        assert(before==memory); // All client records remain untouched.
        vocalCacheHook(originalRace(unit()),0);
        assert(cacheRace==model.race&&cacheSex==model.sex);
        unsigned calls=cacheCalls;
        vocalPlayHook(0,7);vocalPlayHook(0,7);
        assert(cacheCalls==calls&&playedSex==model.sex&&playedEvent==7);
        assert(player.identity==identity&&player.display==display&&before==memory);
        // Gender-only morphs also select the corresponding voice automatically.
        player.identity=model.race|((1-model.sex)<<16);
        const auto sameRaceIdentity=player.identity;
        assert(selectedVoice(player)&&voiceRaceAt(unit(),0x623CDF)==model.race);
        assert(voiceSexAt(unit(),0x623CD7)==model.sex);
        assert(voiceSoundDataAt(unit(),0x62F8B5)==reinterpret_cast<void*>(soundRecord));
        vocalPlayHook(1-model.sex,3);
        assert(cacheRace==model.race&&cacheSex==model.sex&&playedSex==model.sex);
        assert(player.identity==sameRaceIdentity&&player.display==display&&before==memory);
        // Choosing the real body restores all native voice lookup paths and cache.
        state.enabled=false;
        assert(!selectedVoice(player)&&voiceSexAt(unit(),0x623CD7)==1-model.sex);
        assert(voiceSoundDataAt(unit(),0x62F8B5)==originalData(unit()));
        vocalPlayHook(model.sex,4);
        assert(cacheRace==model.race&&cacheSex==1-model.sex&&playedSex==1-model.sex);
        state.enabled=true;
        ++index;
    }
    state.body.race=4;state.body.sex=1;player.identity=1;
    vocalPlayHook(0,7);assert(cacheRace==4&&playedSex==1);
    state.enabled=false;
    vocalPlayHook(0,8);assert(cacheRace==1&&playedSex==0&&playedEvent==8);
    assert(!selectedVoice(player));vocalPlayHook(0,9);assert(cacheRace==1&&playedSex==0);
    state.enabled=true;player.display=1234;
    assert(!selectedVoice(player)&&voiceSoundDataAt(unit(),0x62F8B5)==originalData(unit()));
    player.display=player.native;player.guid=43;
    assert(!selectedVoice(player));vocalPlayHook(0,10);assert(cacheRace==1);
    player.guid=42;memory.erase(0xC0DE90);
    assert(voiceSoundDataAt(unit(),0x62F8B5)==originalData(unit()));
    ready=false;unsigned calls=cacheCalls;vocalPlayHook(0,11);
    assert(cacheCalls==calls&&playedSex==0&&playedEvent==11&&playCalls>32);
    std::cout<<"PASS: automatic voice routing for all 16 race/gender models, gender-only morphs, native fallback, cache restoration, and no identity/DBC writes\n";
}
