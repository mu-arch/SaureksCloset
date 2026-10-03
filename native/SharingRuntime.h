#pragma once
#include "SharingTransport.h"
static const auto sharingToString=reinterpret_cast<const char* (__fastcall *)(void*,int)>(0x6f3690);
static bool sharingString(void* L,int index,std::string& value,unsigned limit,bool allowEmpty=false){const char* p=sharingToString(L,index);if(!p)return false;value.clear();for(unsigned i=0;i<=limit;++i){if(!p[i])return allowEmpty||!value.empty();if(static_cast<unsigned char>(p[i])<32||p[i]==127)return false;value.push_back(p[i]);}return false;}
static bool sharingInteger(void* L,int index,unsigned max,unsigned& out){if(!isNumber(L,index))return false;const auto n=toNumber(L,index);if(!std::isfinite(n)||n<0||n>max||n!=static_cast<unsigned>(n))return false;out=static_cast<unsigned>(n);return true;}
#include "SharingWorld.h"
static bool __fastcall sharingEnumerate(void* context,void*,std::uint64_t guid){auto& ids=*static_cast<std::vector<std::uint64_t>*>(context);Player p;unsigned loaded=0;
    if(ids.size()<sharing::maxVisible&&sharedPlayer(guid,p)&&p.model&&read(p.model+0x10,loaded)&&loaded)ids.push_back(guid);
    return true;
}
static int __fastcall configureSharing(void* L){
    SharingConfig config;unsigned broadcast=0,receive=0;
    if(!sharingInteger(L,1,1,broadcast)||!sharingInteger(L,2,1,receive))return result(L,-3);
    config.flags=broadcast|receive*2;config.guid=getPlayer();
    if(config.flags){
        if(!sharingString(L,3,config.url,512,true)||!sharingString(L,4,config.token,64,true))return result(L,-3);
        if(!config.url.empty()&&(config.url.size()>512||config.url.compare(0,6,"wss://")!=0))return result(L,-3);
        if(!config.token.empty()&&(config.token.size()!=64||!std::all_of(config.token.begin(),config.token.end(),[](unsigned char c){return (c>='0'&&c<='9')||(c>='a'&&c<='f')||(c>='A'&&c<='F');})))return result(L,-3);
        if(!sharingString(L,5,config.server,128)||!sharingString(L,6,config.realm,128)||!sharingString(L,7,config.name,128)||!sharingString(L,8,config.game,128))return result(L,-3);
    }
    {std::lock_guard<std::mutex> lock(sharingMailbox.mutex);if(!(config==sharingMailbox.config)){
        sharingMailbox.config=config;++sharingMailbox.generation;sharingMailbox.inbox={};sharingMailbox.inbox.self=config.guid;sharingMailbox.inbox.receive=receive!=0;sharingMailbox.debounce={};sharingMailbox.status=config.flags?-1:0;
    }}
    // Always clear on the game thread, before returning from the toggle.
    if(!receive)sharedClearAll();
    if(config.flags&&!sharingStart())return result(L,-2);
    return result(L,1);
}
static int __fastcall updateSharing(void* L){
    sharing::Look look;unsigned v=0;
    for(int i=0;i<8;++i){if(!sharingInteger(L,i+1,255,v))return result(L,-3);look.body[i]=static_cast<unsigned char>(v);}
    if(look.body[0]>1||(look.body[0]&&!sharedBody(look).valid()))return result(L,-3);
    for(int i=0;i<19;++i){if(!isNumber(L,i+9))return result(L,-3);if(toNumber(L,i+9)==-1)look.items[i]=sharing::inherit;else{if(!sharingInteger(L,i+9,1000000,v))return result(L,-3);look.items[i]=v;}}
    if(!sharingInteger(L,28,7,v))return result(L,-3);look.stowed=static_cast<unsigned char>(v);
    sharedCapture(look);
    std::vector<std::uint64_t> ids;
    unsigned flags=0;{std::lock_guard<std::mutex> lock(sharingMailbox.mutex);flags=sharingMailbox.config.flags;}
    if(flags){using Enumerate=bool (__fastcall *)(bool (__fastcall *)(void*,void*,std::uint64_t),void*);reinterpret_cast<Enumerate>(0x468380)(sharingEnumerate,&ids);}
    std::map<std::uint64_t,sharing::Remote> incoming;
    {std::lock_guard<std::mutex> lock(sharingMailbox.mutex);sharingMailbox.debounce.update(look,GetTickCount());sharingMailbox.inbox.subscriptions(std::move(ids));incoming=sharingMailbox.inbox.looks;}
    sharedApply(incoming);return result(L,sharingMailbox.status);
}
