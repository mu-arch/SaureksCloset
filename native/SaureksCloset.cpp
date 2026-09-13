// Saurek's Closet 3 renderer bridge, GPL-3.0-or-later. Exact client: 1.12.1 / 5875.
// Overrides model filename + a COPY of the character compositor input.
// Never writes player update fields, identity, display IDs, scale or movement state.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdint>
#include <cstring>
#include <cmath>
#include "MinHook.h"
#include "BuildSignatures.h"
#include "Appearance.h"
#include "PreviewState.h"
#include "WeaponryProbe.h"
using Register=void (__fastcall *)(const char*,std::uintptr_t);
using GetPlayer=std::uint64_t (__fastcall *)();
using ObjectPtr=void* (__fastcall *)(std::uint32_t,const char*,std::uint64_t,int);
using GetName=const char* (__thiscall *)(void*);
using InitComponent=bool (__thiscall *)(void*,const std::uint32_t*);
using Changed=int (__thiscall *)(void*);
using Refresh=void (__thiscall *)(void*);
using Transform=void (__thiscall *)(void*,const float*,float,const float*,const float*,const float*);
using SetMatrix=void (__thiscall *)(void*,const float*);
using IsNumber=bool (__fastcall *)(void*,int);
using ToNumber=double (__fastcall *)(void*,int);
using PushNumber=void (__fastcall *)(void*,double);
static Register registerOriginal=nullptr;
static GetName nameOriginal=nullptr;
static InitComponent initOriginal=nullptr;
static Changed changedOriginal=nullptr;
static Transform transformOriginal=nullptr;
static SetMatrix matrixOriginal=nullptr;
static const auto getPlayer=reinterpret_cast<GetPlayer>(0x468550);
static const auto objectPtr=reinterpret_cast<ObjectPtr>(0x468460);
static const auto updateDisplay=reinterpret_cast<Refresh>(0x60ABE0);
static const auto rebuildComponent=reinterpret_cast<Refresh>(0x5FB200);
static const auto isNumber=reinterpret_cast<IsNumber>(0x6F34D0);
static const auto toNumber=reinterpret_cast<ToNumber>(0x6F3620);
static const auto pushNumber=reinterpret_cast<PushNumber>(0x6F3810);
struct State {
    Appearance body;
    std::uint64_t guid=0;
    std::uintptr_t unit=0,model=0;
    bool enabled=false,busy=false;
    float ratio=1;
    unsigned revision=0,composed=0,reloads=0,detailUpdates=0;
};
using CloneModel=void* (__thiscall *)(void*,void*,unsigned);
using CreateModel=void* (__thiscall *)(void*,const char*,unsigned);
using CloneComponent=bool (__thiscall *)(void*,void*,void*);
using UpdateComponent=bool (__thiscall *)(void*,int);
using DestroyModel=void (__thiscall *)(void*);
static CloneModel cloneModelOriginal=nullptr;
static CloneComponent cloneComponentOriginal=nullptr;
static UpdateComponent updateComponentOriginal=nullptr;
static DestroyModel destroyModelOriginal=nullptr;
static const auto createModel=reinterpret_cast<CreateModel>(0x707350);
static const auto releaseModel=reinterpret_cast<DestroyModel>(0x7103A0);
static PreviewRegistry previews;
static Appearance requestedPreview;
static bool previewArmed=false;
static DWORD previewThread=0;
static unsigned previewToken=0;
static State state;
static void* forceRefresh=nullptr;
template<typename T> static bool read(std::uintptr_t address,T& result){
    SIZE_T count=0;
    return address&&ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<const void*>(address),&result,sizeof(result),&count)&&count==sizeof(result);
}
struct Player {
    std::uintptr_t unit=0,fields=0,model=0,component=0;
    std::uint64_t guid=0;
    unsigned display=0,native=0,identity=0,body=0,facial=0;
};
static bool snapshot(Player& p){
    p.guid=getPlayer();if(!p.guid)return false;
    p.unit=reinterpret_cast<std::uintptr_t>(objectPtr(0x10,nullptr,p.guid,0));
    // Build 5875: PLAYER_BYTES = OBJECT_END (6) + UNIT_END (0xB6) + 5.
    // 0xB5/0xB6 are spell-cost multipliers, not player appearance fields.
    unsigned type=0;std::uint64_t guid=0;
    return p.unit&&read(p.unit+0x14,type)&&type==4&&read(p.unit+8,p.fields)&&p.fields&&
        read(p.fields,guid)&&guid==p.guid&&read(p.fields+0x83*4,p.display)&&read(p.fields+0x84*4,p.native)&&
        read(p.fields+0x24*4,p.identity)&&read(p.fields+0xC1*4,p.body)&&read(p.fields+0xC2*4,p.facial)&&
        read(p.unit+0xD8,p.model)&&read(p.unit+0xD30,p.component);
}
static bool applies(const Player& p){
    return state.enabled&&state.guid==p.guid&&p.display==p.native&&nativeModel(p.native);
}
static const char* __fastcall nameHook(void* unit,void*){
    Player p;
    if(state.enabled&&snapshot(p)&&p.unit==reinterpret_cast<std::uintptr_t>(unit)&&applies(p))
        return state.body.model()->filename;
    return nameOriginal(unit);
}
static bool __fastcall initHook(void* component,void*,const std::uint32_t* input){
    Player p;std::array<std::uint32_t,91> copy;
    if(state.enabled&&snapshot(p)&&applies(p)&&p.component==reinterpret_cast<std::uintptr_t>(component)&&
       read(reinterpret_cast<std::uintptr_t>(input),copy)&&copy[8]==p.model&&p.model){
        state.body.compose(copy);
        const bool result=initOriginal(component,copy.data());
        if(result){state.unit=p.unit;state.model=p.model;state.ratio=state.body.model()->scale/nativeModel(p.native)->scale;state.composed=state.revision;}
        return result;
    }
    return initOriginal(component,input);
}
static int __fastcall changedHook(void* unit,void*){
    if(unit==forceRefresh){forceRefresh=nullptr;return 1;}
    return changedOriginal(unit);
}
static void __fastcall transformHook(void* model,void*,const float* position,float angle,const float* axis,const float* scale,const float* smoothPosition){
    // 0x7106C0: ret 20, position/angle/axis/scale/smoothed-position verified at 0x614DED.
    // Only change a local scale vector. Translation, rotation and animation remain native.
    if(state.enabled&&reinterpret_cast<std::uintptr_t>(model)==state.model&&state.guid==getPlayer()){
        std::uintptr_t live=0,fields=0;std::uint64_t guid=0;unsigned display=0,native=0;
        if(read(state.unit+0xD8,live)&&live==state.model&&read(state.unit+8,fields)&&read(fields,guid)&&guid==state.guid&&
           read(fields+0x83*4,display)&&read(fields+0x84*4,native)&&display==native){
            const float adjusted[]={scale[0]*state.ratio,scale[1]*state.ratio,scale[2]*state.ratio};
            transformOriginal(model,position,angle,axis,adjusted,smoothPosition);return;
        }
    }
    transformOriginal(model,position,angle,axis,scale,smoothPosition);
}
static void __fastcall matrixHook(void* model,void*,const float* matrix){
    // Mounted rider matrix is built from scratch at this exact call site.
    // Never rescale copied matrices from other paths (which could compound scale).
    if(state.enabled&&reinterpret_cast<std::uintptr_t>(__builtin_return_address(0))==0x607BC8){
        Player p;std::array<float,16> copy;
        if(snapshot(p)&&applies(p)&&p.model==reinterpret_cast<std::uintptr_t>(model)&&read(reinterpret_cast<std::uintptr_t>(matrix),copy)){
            scaleBasis(copy,state.body.model()->scale/nativeModel(p.native)->scale);
            matrixOriginal(model,copy.data());return;
        }
    }
    matrixOriginal(model,matrix);
}
static void discardInheritedPreviewWeapons(std::uintptr_t model);
// Only the explicit, bracketed addon SetUnit call can substitute a preview model.
static void* __fastcall cloneModelHook(void* scene,void*,void* source,unsigned flags){
    if(previewArmed&&previewThread==GetCurrentThreadId()&&
       reinterpret_cast<std::uintptr_t>(__builtin_return_address(0))==0x5059DA){
        previewArmed=false;
        Player p;
        if(!snapshot(p)||p.model!=reinterpret_cast<std::uintptr_t>(source)||!p.component||!previews.freeEntry())return nullptr;
        std::array<std::uint32_t,91> descriptor;
        unsigned loaded=0,dirty=1;
        const bool copyAppearance=read(p.model+0x10,loaded)&&loaded&&
            read(p.component+0x10,dirty)&&!dirty&&read(p.component+0x18,descriptor)&&descriptor[8]==p.model&&
            previewBodyMatches(descriptor,requestedPreview)&&(!applies(p)||state.composed==state.revision);
        // Preserve the stock model/texture clone when the requested body is already visible.
        // A different saved body still needs its own model and fresh compositor.
        void* model=copyAppearance?cloneModelOriginal(scene,source,flags):createModel(scene,requestedPreview.model()->filename,flags);
        if(model){
            previewToken=previews.bind(reinterpret_cast<std::uintptr_t>(model),p.guid,requestedPreview);
            if(auto* entry=previews.find(reinterpret_cast<std::uintptr_t>(model)))entry->copiedAppearance=copyAppearance;
        }
        return model;
    }
    return cloneModelOriginal(scene,source,flags);
}
static bool __fastcall cloneComponentHook(void* component,void*,void* model,void* source){
    auto* entry=previews.find(reinterpret_cast<std::uintptr_t>(model));
    if(entry&&reinterpret_cast<std::uintptr_t>(__builtin_return_address(0))==0x5043DA){
        std::array<std::uint32_t,91> descriptor;
        Player p;
        if(!snapshot(p)||p.guid!=entry->guid||component==source||p.component!=reinterpret_cast<std::uintptr_t>(source)||
           !read(reinterpret_cast<std::uintptr_t>(source)+0x18,descriptor)){
            entry->status=-1;releaseModel(model);return false;
        }
        if(entry->copiedAppearance&&!previewBodyMatches(descriptor,entry->body)){
            entry->status=-1;releaseModel(model);return false;
        }
        const auto copy=previewDescriptor(descriptor,entry->body,static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(model)));
        // Caller already retained the model for this component, just as for the stock clone.
        // Initialize a new compositor; never copy source texture caches for a different race.
        const bool ok=entry->copiedAppearance?cloneComponentOriginal(component,model,source):initOriginal(component,copy.data());
        // The stock clone can include world weapons at our custom sheath points.
        // Undress only knows stock points, so strip those inherited children
        // before the preview is dressed with its independently owned weapons.
        if(ok&&entry->copiedAppearance)discardInheritedPreviewWeapons(entry->model);
        entry->component=ok?reinterpret_cast<std::uintptr_t>(component):0;
        entry->status=ok?0:-1;return ok;
    }
    return cloneComponentOriginal(component,model,source);
}
static bool __fastcall updateComponentHook(void* component,void*,int wait){
    const bool ready=updateComponentOriginal(component,wait);
    // Initialize only queues skin/hair assets. The stock update returns true once
    // textures and geosets have been composed; publish that result to the UI.
    for(auto& entry:previews.entries)if(entry.model&&entry.component==reinterpret_cast<std::uintptr_t>(component)){
        std::uintptr_t model=0;
        if(read(entry.component+0x38,model)&&model==entry.model)entry.status=ready?1:0;
        else entry.status=-1;
    }
    return ready;
}
static void forgetWeapons(std::uintptr_t model);
static void __fastcall destroyModelHook(void* model,void*){
    forgetWeapons(reinterpret_cast<std::uintptr_t>(model));
    previews.forget(reinterpret_cast<std::uintptr_t>(model));destroyModelOriginal(model);
}
static int result(void* L,int status){pushNumber(L,status);return 1;}
static bool refresh(const Player& p,bool modelChanged){
    if(p.display!=p.native)return true; // Shapeshifts resume through normal client rebuilds.
    if(modelChanged){
        state.model=0;forceRefresh=reinterpret_cast<void*>(p.unit);
        ++state.reloads;updateDisplay(forceRefresh);forceRefresh=nullptr;
    }else if(p.component&&p.model){
        ++state.detailUpdates;rebuildComponent(reinterpret_cast<void*>(p.unit));
    }
    // If the M2 is loading, its normal component initialization uses the newest selection.
    return true;
}
static int __fastcall setAppearance(void* L){
    unsigned values[7];
    for(int i=0;i<7;++i){
        if(!isNumber(L,i+1))return result(L,-2);
        const double v=toNumber(L,i+1);
        if(!std::isfinite(v)||v<0||v>255||v!=static_cast<unsigned>(v))return result(L,-2);
        values[i]=static_cast<unsigned>(v);
    }
    Appearance next{values[0],values[1],values[2],values[3],values[4],values[5],values[6]};
    if(!next.valid())return result(L,-2);
    Player p;if(!snapshot(p))return result(L,-1);
    if(!nativeModel(p.native))return result(L,-3);
    if(state.busy)return result(L,-4);
    if(state.enabled&&state.guid==p.guid&&state.body==next)return result(L,1);
    const bool wasActive=state.enabled&&state.guid==p.guid;
    const auto* previous=wasActive?state.body.model():nativeModel(p.native);
    const bool modelChanged=previous->race!=next.race||previous->sex!=next.sex;
    state.busy=true;state.guid=p.guid;state.unit=p.unit;state.body=next;state.enabled=true;++state.revision;
    refresh(p,modelChanged);state.busy=false;return result(L,1);
}
static int __fastcall clearAppearance(void* L){
    if(state.busy)return result(L,-4);
    if(!state.enabled)return result(L,1);
    Player p;const bool current=snapshot(p)&&p.guid==state.guid;
    state.enabled=false;state.model=0;++state.revision;
    if(current){state.busy=true;refresh(p,true);state.busy=false;}
    return result(L,1);
}
static int __fastcall bodyInfo(void* L){
    Player p;if(!snapshot(p))return result(L,-1);
    const double values[]={double(p.identity&255),double((p.identity>>16)&255),double(p.body&255),double((p.body>>8)&255),
        double((p.body>>16)&255),double((p.body>>24)&255),double(p.facial&255)};
    for(auto v:values)pushNumber(L,v);
    return 7;
}
static int __fastcall inspect(void* L){
    Player p;if(!snapshot(p))return result(L,0);
    std::array<unsigned,9> a{};if(p.component)read(p.component+0x18,a);
    float scale=0;read(p.fields+16,scale);
    const double values[]={3,state.enabled?1.0:0.0,applies(p)?1.0:0.0,double(state.revision),double(state.composed),
        double(state.reloads),double(state.detailUpdates),double(p.display),double(p.native),scale,
        double(a[0]),double(a[1]),double(a[2]),double(a[3]),double(a[5]),double(a[6]),double(a[7]),state.ratio};
    for(auto v:values)pushNumber(L,v);
    return sizeof(values)/sizeof(values[0]);
}
static int __fastcall beginPreview(void* L){
    if(previewArmed)return result(L,-4);
    unsigned values[7];
    for(int i=0;i<7;++i){
        if(!isNumber(L,i+1))return result(L,-2);
        const double v=toNumber(L,i+1);
        if(!std::isfinite(v)||v<0||v>255||v!=static_cast<unsigned>(v))return result(L,-2);
        values[i]=static_cast<unsigned>(v);
    }
    Appearance body{values[0],values[1],values[2],values[3],values[4],values[5],values[6]};
    Player p;if(!body.valid())return result(L,-2);
    if(!snapshot(p)||!p.model||!p.component||!previews.freeEntry())return result(L,-1);
    requestedPreview=body;previewThread=GetCurrentThreadId();previewToken=0;previewArmed=true;
    return result(L,1);
}
static int __fastcall endPreview(void* L){
    previewArmed=false;const unsigned token=previewToken;previewToken=0;
    return result(L,token?static_cast<int>(token):-1);
}
static int __fastcall previewStatus(void* L){
    if(!isNumber(L,1))return result(L,-1);
    const double token=toNumber(L,1);
    if(!std::isfinite(token)||token<1||token>2147483647||token!=static_cast<unsigned>(token))return result(L,-1);
    return result(L,previews.query(static_cast<unsigned>(token),getPlayer()));
}
static int __fastcall inspectPreview(void* L){
    if(!isNumber(L,1))return result(L,-1);
    const double token=toNumber(L,1);
    for(const auto& entry:previews.entries)if(entry.model&&entry.token==token&&entry.guid==getPlayer()){
        std::array<unsigned,9> body{};unsigned dirty=0;std::uintptr_t model=0;
        if(!entry.component||!read(entry.component+0x38,model)||model!=entry.model||
           !read(entry.component+0x18,body)||!read(entry.component+0x10,dirty))return result(L,-1);
        const double values[]={double(entry.status),entry.copiedAppearance?1.0:0.0,
            double(body[0]),double(body[1]),double(body[3]),double(body[5]),double(body[7]),double(body[2]),double(body[6]),double(dirty)};
        for(auto value:values)pushNumber(L,value);
        return sizeof(values)/sizeof(values[0]);
    }
    return result(L,-1);
}
// An explicitly requested, bounded read-only capture. Selector -1 returns the
// player weapon summary; 0..63 returns one attached child. Stay under Lua 5.0's
// 20 guaranteed result stack slots. The capture itself does not modify attachments.
static int __fastcall weaponryProbe(void* L){
    if(!isNumber(L,1))return result(L,-2);
    const double selector=toNumber(L,1);
    if(!std::isfinite(selector)||selector< -1||selector>63||selector!=static_cast<int>(selector))return result(L,-2);
    Player p;if(!snapshot(p)||!p.model)return result(L,-1);
    if(selector== -1){
        unsigned loaded=0,mode=0;std::array<unsigned,3> displays{};
        std::array<unsigned,6> info{};
        if(!read(p.model+0x10,loaded)||!read(p.unit+0xD40,mode)||
           !read(p.fields+0x25*4,displays)||!read(p.fields+0x28*4,info))return result(L,-1);
        const double values[]={1,double(p.model),double(loaded),double(mode),double(p.display),double(p.native),
            double(displays[0]),double(displays[1]),double(displays[2]),
            double(info[0]),double(info[1]),double(info[2]),double(info[3]),double(info[4]),double(info[5])};
        for(auto value:values)pushNumber(L,value);
        return sizeof(values)/sizeof(values[0]);
    }
    WeaponryChild child;
    const auto reader=[](std::uint32_t address,std::uint32_t& value){return read(address,value);};
    const int status=inspectWeaponryChild(static_cast<std::uint32_t>(p.model),static_cast<unsigned>(selector),reader,child);
    if(status!=1)return result(L,status);
    const double values[]={1,double(child.model),double(child.parent),double(child.attachment),double(child.bone),
        double(child.loaded),double(child.references),double(child.next)};
    for(auto value:values)pushNumber(L,value);
    return sizeof(values)/sizeof(values[0]);
}
#include "WeaponRenderer.h"
#include "UpdateChecker.h"
#include "VoiceRenderer.h"
static int __fastcall version(void* L){return result(L,30500);}
static void __fastcall registerHook(const char* name,std::uintptr_t function){
    registerOriginal(name,function);
    if(name&&std::strcmp(name,"SetUnitVisibleItemID")==0){
        registerOriginal("SaureksClosetSetAppearance",reinterpret_cast<std::uintptr_t>(&setAppearance));
        registerOriginal("SaureksClosetClearAppearance",reinterpret_cast<std::uintptr_t>(&clearAppearance));
        registerOriginal("SaureksClosetRealBody",reinterpret_cast<std::uintptr_t>(&bodyInfo));
        registerOriginal("SaureksClosetRendererVersion",reinterpret_cast<std::uintptr_t>(&version));
        registerOriginal("SaureksClosetBeginPreview",reinterpret_cast<std::uintptr_t>(&beginPreview));
        registerOriginal("SaureksClosetEndPreview",reinterpret_cast<std::uintptr_t>(&endPreview));
        registerOriginal("SaureksClosetPreviewStatus",reinterpret_cast<std::uintptr_t>(&previewStatus));
        registerOriginal("SaureksClosetInspectPreview",reinterpret_cast<std::uintptr_t>(&inspectPreview));
        registerOriginal("SaureksClosetInspect",reinterpret_cast<std::uintptr_t>(&inspect));
        registerOriginal("SaureksClosetSetWeapons",reinterpret_cast<std::uintptr_t>(&setWeapons));
        registerOriginal("SaureksClosetWeaponryProbe",reinterpret_cast<std::uintptr_t>(&weaponryProbe));
        registerOriginal("SaureksClosetSetUpdateChecks",reinterpret_cast<std::uintptr_t>(&setUpdateChecks));
        registerOriginal("SaureksClosetStartUpdateCheck",reinterpret_cast<std::uintptr_t>(&startUpdateCheck));
        registerOriginal("SaureksClosetPollUpdateCheck",reinterpret_cast<std::uintptr_t>(&pollUpdateCheck));
        registerOriginal("SaureksClosetOpenWebsite",reinterpret_cast<std::uintptr_t>(&openWebsite));
    }
}
static bool compatible(){
    if(reinterpret_cast<std::uintptr_t>(GetModuleHandleA(nullptr))!=0x400000)return false;
    for(const auto& s:signatures){unsigned char bytes[12]{};if(!read(s.address,bytes)||std::memcmp(bytes,s.bytes,12))return false;}
    return true;
}
BOOL WINAPI DllMain(HINSTANCE module,DWORD reason,LPVOID){
    if(reason!=DLL_PROCESS_ATTACH)return TRUE;
    DisableThreadLibraryCalls(module);
    if(!compatible()||MH_Initialize()!=MH_OK)return TRUE;
    struct Hook {std::uintptr_t address;void* replacement;void** original;};
    Hook hooks[]={
        {0x611770,reinterpret_cast<void*>(&sheathTransitionHook),reinterpret_cast<void**>(&sheathTransitionOriginal)},
        {0x60C480,reinterpret_cast<void*>(&voiceSoundDataHook),reinterpret_cast<void**>(&voiceSoundDataOriginal)},
        {0x60C6A0,reinterpret_cast<void*>(&voiceRaceHook),reinterpret_cast<void**>(&voiceRaceOriginal)},
        {0x60C6C0,reinterpret_cast<void*>(&voiceSexHook),reinterpret_cast<void**>(&voiceSexOriginal)},
        {0x4580F0,reinterpret_cast<void*>(&vocalCacheHook),reinterpret_cast<void**>(&vocalCacheOriginal)},
        {0x458250,reinterpret_cast<void*>(&vocalPlayHook),reinterpret_cast<void**>(&vocalPlayOriginal)},
        {0x714260,reinterpret_cast<void*>(&updateAttachedHook),reinterpret_cast<void**>(&updateAttachedOriginal)},
        {0x611FF0,reinterpret_cast<void*>(&bowStringDrawHook),reinterpret_cast<void**>(&bowStringDrawOriginal)},
        {0x47A0C0,reinterpret_cast<void*>(&weaponComposeHook),reinterpret_cast<void**>(&weaponComposeOriginal)},
        {0x47A070,reinterpret_cast<void*>(&sheathPointHook),reinterpret_cast<void**>(&sheathPointOriginal)},
        {0x60B590,reinterpret_cast<void*>(&moveWeaponHook),reinterpret_cast<void**>(&moveWeaponOriginal)},
        {0x60B770,reinterpret_cast<void*>(&rebuildWeaponHook),reinterpret_cast<void**>(&rebuildWeaponOriginal)},
        {0x712F00,reinterpret_cast<void*>(&findChildHook),reinterpret_cast<void**>(&findChildOriginal)},
        {0x7130A0,reinterpret_cast<void*>(&clearChildrenHook),reinterpret_cast<void**>(&clearChildrenOriginal)},
        {0x707400,reinterpret_cast<void*>(&cloneModelHook),reinterpret_cast<void**>(&cloneModelOriginal)},
        {0x476CB0,reinterpret_cast<void*>(&cloneComponentHook),reinterpret_cast<void**>(&cloneComponentOriginal)},
        {0x477860,reinterpret_cast<void*>(&updateComponentHook),reinterpret_cast<void**>(&updateComponentOriginal)},
        {0x70E170,reinterpret_cast<void*>(&destroyModelHook),reinterpret_cast<void**>(&destroyModelOriginal)},
        {0x600320,reinterpret_cast<void*>(&nameHook),reinterpret_cast<void**>(&nameOriginal)},
        {0x476B90,reinterpret_cast<void*>(&initHook),reinterpret_cast<void**>(&initOriginal)},
        {0x60AE10,reinterpret_cast<void*>(&changedHook),reinterpret_cast<void**>(&changedOriginal)},
        {0x710620,reinterpret_cast<void*>(&matrixHook),reinterpret_cast<void**>(&matrixOriginal)},
        {0x7106C0,reinterpret_cast<void*>(&transformHook),reinterpret_cast<void**>(&transformOriginal)},
        {0x704120,reinterpret_cast<void*>(&registerHook),reinterpret_cast<void**>(&registerOriginal)}};
    for(auto& h:hooks)if(MH_CreateHook(reinterpret_cast<void*>(h.address),h.replacement,h.original)!=MH_OK){MH_Uninitialize();return TRUE;}
    // All hooks exist before any are activated; failure leaves the API unavailable.
    if(MH_EnableHook(MH_ALL_HOOKS)!=MH_OK)MH_Uninitialize();
    return TRUE;
}
