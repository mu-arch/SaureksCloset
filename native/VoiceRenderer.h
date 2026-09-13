// Build 5875 voice routing. Changes sound lookup results, never unit identity fields.
#pragma once
using VoiceUnitValue=unsigned (__thiscall *)(void*);
using VoiceSoundData=void* (__thiscall *)(void*);
using VocalCache=void (__fastcall *)(unsigned,unsigned);
using VocalPlay=void (__fastcall *)(unsigned,unsigned);
static VoiceUnitValue voiceRaceOriginal=nullptr,voiceSexOriginal=nullptr;
static VoiceSoundData voiceSoundDataOriginal=nullptr;
static VocalCache vocalCacheOriginal=nullptr;
static VocalPlay vocalPlayOriginal=nullptr;
static unsigned cachedVoiceRace=0,cachedVoiceSex=2;
static bool selectedVoice(const Player& p){
    return applies(p);
}
static bool voiceUnit(void* unit,Player& p){
    return snapshot(p)&&p.unit==reinterpret_cast<std::uintptr_t>(unit)&&selectedVoice(p);
}
static unsigned voiceRaceAt(void* unit,std::uintptr_t caller){
    Player p;
    // Only the audible emote path. Race queries elsewhere retain client behavior.
    if(caller==0x623CDF&&voiceUnit(unit,p))return state.body.race;
    return voiceRaceOriginal(unit);
}
static unsigned __fastcall voiceRaceHook(void* unit,void*){
    return voiceRaceAt(unit,reinterpret_cast<std::uintptr_t>(__builtin_return_address(0)));
}
static unsigned voiceSexAt(void* unit,std::uintptr_t caller){
    Player p;
    if(caller==0x623CD7&&voiceUnit(unit,p))return state.body.sex;
    return voiceSexOriginal(unit);
}
static unsigned __fastcall voiceSexHook(void* unit,void*){
    return voiceSexAt(unit,reinterpret_cast<std::uintptr_t>(__builtin_return_address(0)));
}
static std::uintptr_t voiceRecord(unsigned id,std::uintptr_t table,std::uintptr_t maximum){
    unsigned limit=0;std::uintptr_t rows=0,record=0;unsigned found=0;
    if(!id||id>1000000||!read(maximum,limit)||id>limit||!read(table,rows)||!rows||
       !read(rows+id*4,record)||!record||!read(record,found)||found!=id)return 0;
    return record;
}
static std::uintptr_t selectedVoiceSoundData(){
    // Same CreatureDisplayInfo -> CreatureSoundData / CreatureModelData fallback
    // used by the client's sound initialization at 0x60B039..0x60B0C5.
    auto display=voiceRecord(state.body.model()->display,0xC0DE90,0xC0DE94);
    unsigned sound=0,model=0;
    if(!display||!read(display+8,sound))return 0;
    if(auto record=voiceRecord(sound,0xC0DE54,0xC0DE58))return record;
    if(!read(display+4,model))return 0;
    auto modelRecord=voiceRecord(model,0xC0DE68,0xC0DE6C);
    if(!modelRecord||!read(modelRecord+0x34,sound))return 0;
    return voiceRecord(sound,0xC0DE54,0xC0DE58);
}
static void* voiceSoundDataAt(void* unit,std::uintptr_t caller){
    Player p;
    // Unit and player combat vocal events; footsteps and breathing stay native.
    if((caller==0x6234DB||caller==0x62F8B5)&&voiceUnit(unit,p))
        if(auto data=selectedVoiceSoundData())return reinterpret_cast<void*>(data);
    return voiceSoundDataOriginal(unit);
}
static void* __fastcall voiceSoundDataHook(void* unit,void*){
    return voiceSoundDataAt(unit,reinterpret_cast<std::uintptr_t>(__builtin_return_address(0)));
}
static void __fastcall vocalCacheHook(unsigned race,unsigned sex){
    // This is the client's local UI-error sound cache, not character data.
    Player p;
    if(snapshot(p)&&selectedVoice(p)){race=state.body.race;sex=state.body.sex;}
    vocalCacheOriginal(race,sex);cachedVoiceRace=race;cachedVoiceSex=sex;
}
static void __fastcall vocalPlayHook(unsigned sex,unsigned event){
    Player p;
    if(snapshot(p)){
        unsigned race=p.identity&255;unsigned desiredSex=(p.identity>>16)&255;
        if(selectedVoice(p)){race=state.body.race;desiredSex=state.body.sex;}
        if(race>=1&&race<=8&&desiredSex<=1){
            // Refresh only when needed, preserving stock repetition/rate controls.
            if(race!=cachedVoiceRace||desiredSex!=cachedVoiceSex){
                vocalCacheOriginal(race,desiredSex);cachedVoiceRace=race;cachedVoiceSex=desiredSex;
            }
            sex=desiredSex;
        }
    }
    vocalPlayOriginal(sex,event);
}
