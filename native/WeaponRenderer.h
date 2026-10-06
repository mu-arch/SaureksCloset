#pragma once
#include "HairMask.h"
#include "WeaponState.h"
#include <map>
#include "BowPlacement.h"
#include "BagPlacement.h"
#include "BagBodyBinding.h"
#include "BagCatalog.h"
#include "BagBehavior.h"
#include "WeaponPhysics.h"
#include "PlacementTuning.h"
#include "HatPlacement.h"
#include "ArmorInspection.h"
#include "StaffPlacement.h"
#include "StaffFits.h"
#include "StaffShafts.h"
// Included after the bridge's player reader, registry and Lua result helpers.
using WeaponCompose=int (__fastcall *)(void*,void*,unsigned,unsigned,unsigned,unsigned,unsigned);
using SheathPoint=int (__fastcall *)(unsigned,unsigned);
using MoveWeapon=void (__thiscall *)(void*,unsigned,unsigned);
using RebuildWeapon=void (__thiscall *)(void*,unsigned);
using SheathTransition=void (__thiscall *)(void*);
using FindChild=void* (__thiscall *)(void*,unsigned);
using ClearChildren=void (__thiscall *)(void*,unsigned);
using AttachChild=void (__thiscall *)(void*,void*,unsigned);
using HasPoint=bool (__thiscall *)(void*,unsigned);
using LoadChild=void (__fastcall *)(void*,unsigned,const char*,const char*,unsigned);
using UpdateAttachedModel=void (__thiscall *)(void*,const float*,const float*,const float*,float);
using BowStringDraw=void (__fastcall *)(void*,void*,void*);
using ModelSequenceTime=void (__thiscall *)(void*,int,unsigned,int,unsigned,float,unsigned,unsigned);
using ModelSequenceOffset=void (__thiscall *)(void*,int,unsigned);
static WeaponCompose weaponComposeOriginal=nullptr;
static SheathPoint sheathPointOriginal=nullptr;
static MoveWeapon moveWeaponOriginal=nullptr;
static RebuildWeapon rebuildWeaponOriginal=nullptr;
static SheathTransition sheathTransitionOriginal=nullptr;
static FindChild findChildOriginal=nullptr;
static ClearChildren clearChildrenOriginal=nullptr;
static UpdateAttachedModel updateAttachedOriginal=nullptr;
static BowStringDraw bowStringDrawOriginal=nullptr;
#ifndef SAUREKS_WEAPON_TEST
template<typename T> static T weaponFunction(std::uintptr_t address){return reinterpret_cast<T>(address);}
static long long bagCounterFrequency=0;
static std::uint32_t bagClockMilliseconds(){
    // Called with the renderer's model state on its single update thread.
    if(!bagCounterFrequency){LARGE_INTEGER value{};bagCounterFrequency=QueryPerformanceFrequency(&value)?value.QuadPart:-1;}
    const auto frequency=bagCounterFrequency;
    LARGE_INTEGER counter{};
    if(frequency>0&&QueryPerformanceCounter(&counter))
        return static_cast<std::uint32_t>((counter.QuadPart/frequency)*1000+(counter.QuadPart%frequency)*1000/frequency);
    return GetTickCount();
}
static bool writeBagResponseMatrices(std::uintptr_t address,const std::array<BagMatrix,61>& matrices){
    MEMORY_BASIC_INFORMATION region{};
    if(!address||!VirtualQuery(reinterpret_cast<const void*>(address),&region,sizeof(region))||region.State!=MEM_COMMIT||
       (region.Protect&(PAGE_GUARD|PAGE_NOACCESS))||
       !(region.Protect&(PAGE_READWRITE|PAGE_WRITECOPY|PAGE_EXECUTE_READWRITE|PAGE_EXECUTE_WRITECOPY)))return false;
    const auto start=reinterpret_cast<std::uintptr_t>(region.BaseAddress);
    if(address<start||address-start>region.RegionSize||sizeof(matrices)>region.RegionSize-(address-start))return false;
    std::array<BagMatrix,61> original;SIZE_T count=0;
    if(!ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<const void*>(address),original.data(),sizeof(original),&count)||count!=sizeof(original))return false;
    if(WriteProcessMemory(GetCurrentProcess(),reinterpret_cast<void*>(address),matrices.data(),sizeof(matrices),&count)&&count==sizeof(matrices))return true;
    // No partial palette remains if a write unexpectedly fails. The renderer
    // owns this heap allocation on its update thread for this entire call.
    WriteProcessMemory(GetCurrentProcess(),reinterpret_cast<void*>(address),original.data(),sizeof(original),&count);
    return false;
}
#endif
static const auto retainChild=weaponFunction<DestroyModel>(0x710390);
static const auto detachChild=weaponFunction<DestroyModel>(0x713020);
static const auto attachChild=weaponFunction<AttachChild>(0x712F70);
static const auto hasPoint=weaponFunction<HasPoint>(0x712CB0);
static const auto loadChild=weaponFunction<LoadChild>(0x4798C0);
static const auto refreshMelee=weaponFunction<void (__thiscall *)(void*,unsigned)>(0x605DA0);
static const auto refreshRanged=weaponFunction<void (__thiscall *)(void*,unsigned)>(0x611E10);
// The same native setter used by Model:SetSequenceTime at 0x76CF80.
static const auto modelSequenceTime=weaponFunction<ModelSequenceTime>(0x7121A0);
static const auto modelSequenceOffset=weaponFunction<ModelSequenceOffset>(0x7127F0);
struct BagInstance {
    unsigned model=0,mount=0;
    void* child=nullptr;
    BagMotion motion;
    BagResponse response;
    BagBodyBinding bodyBinding;
    bool bodyBound=false;
    std::array<BagTuningEntry,16> fits{};
};
static unsigned bagAttachment(unsigned mount){return mount==1?32:mount==2?33:28;}
struct WeaponRigidMotion {std::uintptr_t child=0;unsigned point=0;BagMotion motion;};
static bool weaponPhysicsEnabled=false;
static WeaponPhysicsSettings weaponPhysicsSettings;
static std::array<WeaponSlotPhysics,10> weaponSlotPhysics{};
static std::uint64_t weaponPhysicsOwner=0;
struct WeaponContext {
    std::uintptr_t parent=0,unit=0;std::uint64_t guid=0;unsigned token=0;
    WeaponSelection selection;
    bool quiverHorizontal=false,hideRangedWhenStored=false,hideMeleeWhenStored=false;
    unsigned actualQuiver=0;
    void* passthroughQuiver=nullptr;
    unsigned backBag=0;
    void* backpack=nullptr;
    BagMotion bagMotion;
    std::array<BagInstance,8> bags{};
    unsigned bagGeneration=0;
    std::array<int,3> routes{{-1,-1,-1}};
    std::array<void*,10> extra{};
    // Observed native composition results, never owned or retained by Closet.
    // The model destruction hook clears these before an address can be reused.
    std::array<void*,3> nativeChildren{};
    unsigned previewMode=0;
    bool sharedWeaponPhysics=false;
    WeaponPhysicsSettings sharedWeaponPhysicsSettings;
    std::array<WeaponSlotPhysics,10> sharedWeaponSlotPhysics{};
    std::array<WeaponRigidMotion,13> rigidWeapons{};
    std::array<BagTuningEntry,10> sharedFits{};
    std::array<unsigned char,8> rangedInfo{};
};
static std::array<WeaponContext,9> weaponContexts{};
#if !defined(SAUREKS_WEAPON_TEST) || defined(SAUREKS_SHARING_TEST)
static std::map<std::uint64_t,WeaponContext> sharedWeaponContexts;
#endif
static bool weaponContextActive(const WeaponContext& c){
    if(c.guid==getPlayer())return true;
#if !defined(SAUREKS_WEAPON_TEST) || defined(SAUREKS_SHARING_TEST)
    return !c.token&&sharedWorldIdentity(c.guid,c.unit,c.parent);
#else
    return false;
#endif
}
static bool weaponPlayer(void* unit,Player& p){
    if(snapshot(p)&&p.unit==reinterpret_cast<std::uintptr_t>(unit))return true;
#if !defined(SAUREKS_WEAPON_TEST) || defined(SAUREKS_SHARING_TEST)
    std::uintptr_t fields=0;std::uint64_t guid=0;
    return read(reinterpret_cast<std::uintptr_t>(unit)+8,fields)&&read(fields,guid)&&
        sharedAppearances.count(guid)&&sharedPlayer(guid,p)&&p.unit==reinterpret_cast<std::uintptr_t>(unit);
#else
    return false;
#endif
}
static std::uint32_t bagGenerationCounter=0;
static unsigned nextBagGeneration(){
    if(++bagGenerationCounter==0)++bagGenerationCounter;
    return bagGenerationCounter;
}
static int scopedSheathPoint=-1;
static bool scopedProjectileAppearance=false;
static std::uintptr_t loadingExtraParent=0;
static WeaponContext* weaponContext(std::uintptr_t model){
    for(auto& c:weaponContexts)if(model&&c.parent==model)return &c;
#if !defined(SAUREKS_WEAPON_TEST) || defined(SAUREKS_SHARING_TEST)
    const auto owner=sharedModelOwners.find(model);
    if(owner!=sharedModelOwners.end()){auto i=sharedWeaponContexts.find(owner->second);if(i!=sharedWeaponContexts.end()&&i->second.parent==model)return &i->second;}
#endif
    return nullptr;
}
static bool hasBagInstances(const WeaponContext& context){
    for(const auto& bag:context.bags)if(bag.model)return true;
    return false;
}
static BagInstance* ownedBagInstance(WeaponContext& context,void* child){
    for(auto& bag:context.bags)if(child&&bag.child==child)return &bag;
    return nullptr;
}
// Stock CharacterModelFrame/DressUpModel clones do not use addon preview tokens.
// Remember only copies originating from our player's bag-bearing model. These
// are weak identities: the client owns every cloned model and child reference.
struct ClonedBagPreview { std::uintptr_t model=0;std::uint64_t guid=0; };
static std::array<ClonedBagPreview,32> clonedBagPreviews{};
struct ClonedBagChild {
    std::uintptr_t child=0;std::uint64_t guid=0;unsigned identity=0;
    BagInstance bag;
};
static std::array<ClonedBagChild,256> clonedBagChildren{};
static ClonedBagChild* clonedBagChild(std::uintptr_t model){
    for(auto& entry:clonedBagChildren)if(model&&entry.child==model)return &entry;
    return nullptr;
}
static std::uint64_t clonedBagOwner(std::uintptr_t model){
    for(const auto& entry:clonedBagPreviews)if(model&&entry.model==model)return entry.guid;
    return 0;
}
static void rememberClonedBagPreview(std::uintptr_t source,std::uintptr_t model){
    if(!source||!model||source==model)return;
    // 70EB85 recursively calls the hooked clone factory for each attachment.
    // Preserve the exact source instance: repeated meshes can have different fits.
    ClonedBagChild copy;
    for(const auto& context:weaponContexts)if(context.guid==getPlayer())
        for(unsigned i=0;i<context.bags.size();++i)if(source==reinterpret_cast<std::uintptr_t>(context.bags[i].child)){
            copy.child=model;copy.guid=context.guid;copy.identity=201+i;copy.bag=context.bags[i];
        }
    if(!copy.child)if(const auto* previous=clonedBagChild(source)){copy=*previous;copy.child=model;}
    if(copy.child&&copy.guid==getPlayer()){
        copy.bag.child=reinterpret_cast<void*>(model);copy.bag.motion={};copy.bag.response={};
        copy.bag.bodyBinding={};copy.bag.bodyBound=false;
        for(auto& entry:clonedBagChildren)if(entry.child==model){entry=copy;return;}
        for(auto& entry:clonedBagChildren)if(!entry.child){entry=copy;return;}
        return;
    }
    const auto* context=weaponContext(source);
    const auto guid=context&&(context->backBag==1||hasBagInstances(*context))?context->guid:clonedBagOwner(source);
    if(!guid||guid!=getPlayer())return;
    for(auto& entry:clonedBagPreviews)if(entry.model==model){entry.guid=guid;return;}
    for(auto& entry:clonedBagPreviews)if(!entry.model){entry={model,guid};return;}
}
static bool ownedExtra(const WeaponContext& c,void* child){
    if(child&&c.backpack==child)return true;
    for(const auto& bag:c.bags)if(child&&bag.child==child)return true;
    if(child&&c.passthroughQuiver==child)return true;
    for(auto p:c.extra)if(p&&p==child)return true;
    return false;
}
static bool weaponModelMatches(void* child,const char* filename){
    std::uintptr_t resource=0;unsigned loaded=0;
    const auto model=reinterpret_cast<std::uintptr_t>(child);
    std::array<char,260> name{};
    if(!filename||!read(model+0x10,loaded)||!loaded||!read(model+0x30,resource)||!resource||
       !read(resource+0x20,name))return false;
    unsigned length=0;while(length<name.size()&&filename[length])++length;
    if(length>=name.size())return false;
    // Build 5875's resource cache stores a case-insensitive, extensionless path.
    const auto lower=[](char c){if(c=='/')return '\\';return c>='A'&&c<='Z'?static_cast<char>(c-'A'+'a'):c;};
    if(length>=4&&filename[length-4]=='.'&&lower(filename[length-3])=='m'&&
       lower(filename[length-2])=='d'&&lower(filename[length-1])=='x')length-=4;
    else if(length>=3&&filename[length-3]=='.'&&lower(filename[length-2])=='m'&&filename[length-1]=='2')length-=3;
    if(name[length])return false;
    for(unsigned i=0;i<length;++i)if(lower(name[i])!=lower(filename[i]))return false;
    return true;
}
static bool nativeQuiverModel(void* child){
    return weaponModelMatches(child,"Item\\ObjectComponents\\Quiver\\Quiver_A.mdx");
}
static bool bagIsRunning(const WeaponContext& context){
    // Build 5875 reads this movement word at 0x602D9B (XY motion) and
    // 0x5FD83C (swimming). Preview models have their own stationary pose.
    unsigned flags=0;
    constexpr unsigned notRunning=0x100|0x400|0x800|0x2000|0x4000|0x200000|0x800000|0x1000000|0x8000000;
    return !context.token&&context.unit&&read(context.unit+0x9E8,flags)&&(flags&0xF)&&!(flags&notRunning);
}
#include "ClothAnimation.h"
static bool bagIsAirborne(const WeaponContext& context){
    // Build 5875 starts jump/drop motion with 0x2000 at 7C620B; extended
    // falling adds 0x4000 at 633240. Landing clears both at 7C629B.
    // 7C61D6 excludes nonground modes from natural fall start. Do not reject
    // walking, transport or feather-fall: these can still be real jumps.
    unsigned flags=0;
    constexpr unsigned nonBallistic=0x400|0x800|0x200000|0x800000|0x1000000|0x8000000;
    return !context.token&&context.unit&&read(context.unit+0x9E8,flags)&&(flags&0x6000)&&!(flags&nonBallistic);
}
static bool bagMotionActive(const WeaponContext& context){
    // Walking/swimming still move the mount even though they are not running.
    // Turning in place and idle animation do not supply locomotion impulses.
    unsigned flags=0;
    return bagIsAirborne(context)||(!context.token&&context.unit&&read(context.unit+0x9E8,flags)&&(flags&0xF)&&!(flags&0x800));
}
static float bagAirLiftTarget(const WeaponContext& context){
    if(!bagIsAirborne(context))return 0;
    unsigned elapsed=0,flags=0;float initialDown=0;
    if(!read(context.unit+0xA20,elapsed)||!read(context.unit+0xA48,initialDown)
        ||!std::isfinite(initialDown)||!read(context.unit+0x9E8,flags)||!(flags&0x2000))return 0;
    // 7C5D70/7C5D20 use native fall milliseconds and initial DOWNWARD
    // velocity. A normal jump starts negative; do not lift during ascent.
    // Native elapsed can be corrected by collision, so use it as-is.
    const float downSpeed=std::fmin((flags&0x20000000)?7.f:60.148f,
        initialDown+19.29110527f*(elapsed*.001f));
    // Build toward full lift over the first eight units/second of descent;
    // crossing the apex starts at zero rather than switching to full tilt.
    return std::fmax(0.f,std::fmin(1.f,downSpeed/8.f));
}
static float bagResponseFlight(const WeaponContext& context){
    if(!bagIsAirborne(context))return 0;
    unsigned elapsed=0,flags=0;float initialDown=0;
    if(!read(context.unit+0xA20,elapsed)||elapsed>600000||!read(context.unit+0xA48,initialDown)||
       !std::isfinite(initialDown)||initialDown< -100.f||initialDown>100.f||!read(context.unit+0x9E8,flags))return 0;
    const float speed=std::fmin((flags&0x20000000)?7.f:60.148f,initialDown+19.29110527f*(elapsed*.001f));
    return std::fmax(-1.f,std::fmin(1.f,speed/8.f));
}
static bool bagResponseResource(std::uintptr_t model,const char* material,BagResponseProfile& profile){
    std::uintptr_t data=0,header=0,name=0,definitions=0;unsigned size=0,bones=0;
    static constexpr char marker[]="ClosetBagV3";
    if(!read(model+0x30,data)||!data||!read(data+0x130,header)||!header||
       !read(header+8,size)||size!=sizeof(marker)||!read(header+12,name)||!name||
       !read(header+0x34,bones)||bones!=61||!read(header+0x38,definitions)||!definitions)return false;
    for(unsigned i=0;i<sizeof(marker);++i){unsigned char value=0;if(!read(name+i,value)||value!=marker[i])return false;}
    // The first/last lattice pivots are the exact REST bounds. Header bounds
    // include deformation room for culling and must not define the response.
    std::array<float,3> low{},high{};
    if(!read(definitions+108+96,low)||!read(definitions+108*60+96,high))return false;
    for(unsigned axis=0;axis<3;++axis)
        if(!std::isfinite(low[axis])||!std::isfinite(high[axis])||high[axis]-low[axis]<.005f||high[axis]-low[axis]>10.f)return false;
    profile=bagResponseProfile(material,low[2],high[2]);profile.low=low;profile.high=high;profile.measuredBounds=true;
    return true;
}
static bool bagTuningLuaKey(void* L,unsigned& bag,unsigned& race,unsigned& sex){
    unsigned values[3]{};
    for(int i=0;i<3;++i){
        if(!isNumber(L,i+1))return false;
        const double value=toNumber(L,i+1);
        if(!std::isfinite(value)||value<0||value>208||value!=static_cast<unsigned>(value))return false;
        values[i]=static_cast<unsigned>(value);
    }
    bag=values[0];race=values[1];sex=values[2];
    return bagTuningKey(bag,race,sex);
}
static int __fastcall getBagFitDefaults(void* L){
    unsigned bag=0,race=0,sex=0;BagTuningValues values;
    if(!bagTuningLuaKey(L,bag,race,sex)||!bagTuningDefaults(bag,race,sex,values))return result(L,-2);
    if(bag>=201&&isNumber(L,4)){
        const double mount=toNumber(L,4);
        if(mount!=0&&mount!=1&&mount!=2)return result(L,-2);
        if(!bagInstanceTuningDefaults(static_cast<unsigned>(mount),race,sex,values))return result(L,-2);
    }
    pushNumber(L,1);
    for(float value:{values.left,values.inset,values.up,values.pitch,values.roll,values.yaw,values.scale})pushNumber(L,value);
    return 8;
}
static int __fastcall setBagFit(void* L){
    unsigned bag=0,race=0,sex=0;
    if(!bagTuningLuaKey(L,bag,race,sex)||bag>=201||!isNumber(L,4))return result(L,-2);
    const double enabled=toNumber(L,4);
    if(enabled!=0&&enabled!=1)return result(L,-2);
    BagTuningValues values;
    if(enabled==1){
        float* fields[]={&values.left,&values.inset,&values.up,&values.pitch,&values.roll,&values.yaw,&values.scale};
        for(int i=0;i<7;++i){
            if(!isNumber(L,i+5))return result(L,-2);
            const double value=toNumber(L,i+5);
            const double minimum=i==2&&bag==1?-3:(i<3?-1:(i<6?-180:25)),maximum=i<3?1:(i<6?180:200);
            // Validate as a double before narrowing; just-outside values must
            // not round back into the permitted float range.
            if(!std::isfinite(value)||value<minimum||value>maximum)return result(L,-2);
            *fields[i]=static_cast<float>(value);
        }
        if(!isNumber(L,12))return result(L,-2);
        const double motion=toNumber(L,12);
        if(motion!=0&&motion!=1)return result(L,-2);
        values.motion=motion==1;
    }
    const auto guid=getPlayer();
    // A successful draft apply must belong to an actual player. Returning a
    // retryable failure during login prevents Lua caching a pre-login success
    // that would be discarded when the first character model arrives.
    if(enabled==1&&!guid)return result(L,-1);
    bagTuningUseOwner(guid);
    return result(L,bagTuningSet(bag,race,sex,enabled==1,values)?1:-2);
}
static bool positionBackpack(void* child,std::array<float,16>& adjusted,bool smooth=false){
    const auto model=reinterpret_cast<std::uintptr_t>(child);
    std::uintptr_t parent=0,data=0,header=0,attachments=0,lookup=0;
    unsigned point=0;std::uint16_t index=0;
    if(!read(model+0x1CC,parent)||!read(model+0x1D0,point))return false;
    auto* context=weaponContext(parent);
    BagInstance* instance=context?ownedBagInstance(*context,child):nullptr;
    unsigned identity=instance?201+static_cast<unsigned>(instance-context->bags.data()):1;
    const auto owner=context?context->guid:clonedBagOwner(parent);
    bool legacy=context&&context->backpack==child&&context->backBag==1;
    if(!context&&owner){
        if(auto* copy=clonedBagChild(model)){
            if(copy->guid!=owner)return false;
            instance=&copy->bag;identity=copy->identity;
        }else legacy=weaponModelMatches(child,"Interface\\AddOns\\SaureksCloset\\Models\\DarkSchoolbag.mdx");
        smooth=false;
    }
    const unsigned mount=instance?instance->mount:0;
    if(!owner||(owner!=getPlayer()&&(!context||!weaponContextActive(*context)))||point!=bagAttachment(mount)||(!instance&&!legacy))return false;
    if(instance){const auto* asset=bagAsset(instance->model);if(!asset||!weaponModelMatches(child,asset->model))return false;}
    if(owner==getPlayer())bagTuningUseOwner(owner);
    BagMotion previewMotion;
    auto& motion=instance?instance->motion:context?context->bagMotion:previewMotion;
    std::array<float,16> back,torso,local,modelToRender;std::array<float,3> position,mountRest;
    // The back anchor identifies the race/sex fit even for a hip-mounted bag.
    // Position and animation come from the chosen attachment's own parent bone.
    if(!animatedAttachmentMatrix(parent,point,back,&torso,&mountRest)||!read(model+0xBC,local)||!read(parent+0xFC,modelToRender)||
       !read(parent+0x30,data)||!read(data+0x130,header)||!read(header+0x110,lookup)||
       !read(lookup+56,index)||!read(header+0x108,attachments)||!read(attachments+48*index+8,position)){
        motion={};if(instance)instance->response={};return false;
    }
    BagMatrix worldToRender;std::uintptr_t scene=0;
    if(smooth&&(!read(parent+0x2C,scene)||!scene||!read(scene+0x9C,worldToRender))){
        motion={};if(instance)instance->response={};smooth=false;
    }
    ClothAnimationResource cloth;
    const bool clothReady=instance&&clothAnimationResource(model,instance->model,cloth);
    BagResponseProfile responseProfile;bool responseReady=false;
    if(instance){
        const auto* asset=bagAsset(instance->model);
        responseReady=bagResponseResource(model,asset->material,responseProfile);
        if(!responseReady)instance->response={};
    }
    const auto now=bagClockMilliseconds();
    BagMatrix bodyFit;
    bool bodyBound=false;
    if(instance){
        const BagMatrix neutral{{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1}};
        auto restAttachment=neutral;BagMatrix restBag;
        for(unsigned axis=0;axis<3;++axis)restAttachment[12+axis]=mountRest[axis];
        const unsigned previousBuilds=instance->bodyBinding.builds;
        // Reconstruct the saved neutral fit before attaching its upper rear
        // contact to the nearby body's skin. Dragging down to a foot therefore
        // follows the foot, regardless of the original back/hip menu preset.
        if(bagPlacement(restAttachment,neutral,neutral,position,restBag,1,nullptr,0,header,false,
            nullptr,nullptr,0,instance->fits.data(),mount,identity))
            bodyBound=bagBindBodyPoint(instance->bodyBinding,parent,restBag,
                responseReady?responseProfile.top:.6195f,modelToRender,now,bodyFit);
        if(bodyBound!=instance->bodyBound||previousBuilds!=instance->bodyBinding.builds){
            motion={};instance->response={};
        }
        instance->bodyBound=bodyBound;
    }
    const bool valid=bagPlacement(back,torso,local,position,adjusted,1,
        smooth?&motion:nullptr,now,header,context&&bagIsRunning(*context),&modelToRender,
        smooth?&worldToRender:nullptr,context?bagAirLiftTarget(*context):0,instance?instance->fits.data():nullptr,mount,identity,
        instance&&smooth&&responseReady?&instance->response:nullptr,responseReady?&responseProfile:nullptr,context?bagResponseFlight(*context):0,
        context&&bagMotionActive(*context),bodyBound?&bodyFit:nullptr,instance?bagSoftBody(instance->model):true);
    if(valid&&clothReady){
        // Compatibility with an older cloth asset still installed on disk:
        // clear its blend and hold the undeformed Stand pose. Native character
        // Run/Jump/Fall transitions no longer select bag animation clips.
        clothSetPlayback(child,cloth,0,0,false);
    }
    if(!valid){motion={};if(instance)instance->response={};}
    return valid;
}
static bool applyBagResponseBones(void* child){
    const auto model=reinterpret_cast<std::uintptr_t>(child);
    std::uintptr_t parent=0,bones=0;if(!read(model+0x1CC,parent))return false;
    auto* context=weaponContext(parent);
    BagInstance* instance=context&&weaponContextActive(*context)?ownedBagInstance(*context,child):nullptr;
    if(!instance){
        auto* cloned=clonedBagChild(model);
        if(cloned&&cloned->guid==getPlayer()&&clonedBagOwner(parent)==cloned->guid)instance=&cloned->bag;
    }
    if(!instance)return false;
    const auto* asset=bagAsset(instance->model);BagResponseProfile profile;BagMatrix modelToRender;std::uintptr_t parentBones=0;
    if(!asset||!weaponModelMatches(child,asset->model)||!bagResponseResource(model,asset->material,profile)||
       !read(model+0x94,bones)||!bones||!read(model+0xFC,modelToRender))return false;
    if(read(parent+0x94,parentBones)&&parentBones==bones)return false;
    for(auto value:modelToRender)if(!std::isfinite(value))return false;
    if(std::fabs(modelToRender[15]-1.f)>.001f)return false;
    // 0x714260 has now evaluated this child's bones. Rebuild from its current
    // model/view transform, never last frame's deformed matrices. Root and
    // attachment fields remain unchanged; CPU/GPU skinning consumes this
    // per-instance matrix palette after the update returns.
    return writeBagResponseMatrices(bones,bagResponseMatrices(instance->response,modelToRender,bagSoftBody(instance->model)));
}
// Hand-role attachments have different identities from decorative back slots.
// Recognize only the exact current native/preview child; sharing a bone or mesh
// with a weapon must not turn an unrelated decoration into a hand weapon.
static const WeaponAsset* modernHandWeaponAsset(const WeaponContext& c,void* child,unsigned role){
    if(c.selection.stowedMask<0||role>=3)return nullptr;
    const int route=c.routes[role];
    const auto* asset=weaponAsset(route>=0?c.selection.items[route]:c.selection.equipped[role]);
    if(!asset)return nullptr;
    if(c.token){
        if(route!=static_cast<int>(7+role)||c.extra[7+role]!=child)return nullptr;
    }else if(c.nativeChildren[role]!=child||ownedExtra(c,child))return nullptr;
    return weaponModelMatches(child,asset->model)?asset:nullptr;
}
static bool positionStoredBow(void* child,const float* attachment,std::array<float,16>& adjusted){
    std::uintptr_t parent=0;unsigned point=0;
    const auto model=reinterpret_cast<std::uintptr_t>(child);
    if(!read(model+0x1CC,parent)||!read(model+0x1D0,point)||point!=weaponPoints[5])return false;
    const auto* c=weaponContext(parent);
    if(!c||!weaponContextActive(*c))return false;
    const auto* asset=modernHandWeaponAsset(*c,child,2);
    if(!asset){
        asset=weaponAsset(c->selection.items[5]);
        if(c->extra[5]!=child&&(c->token||c->routes[2]!=5||findChildOriginal(reinterpret_cast<void*>(parent),point)!=child))return false;
    }
    if(!asset||asset->kind!=4||asset->subclass!=2)return false;
    std::array<float,3> back;
    if(!animatedBackPosition(parent,back)||!read(reinterpret_cast<std::uintptr_t>(attachment),adjusted))return false;
    // Keep the ranged sheath's animated orientation, but center the bow's grip
    // on this model's authored back anchor. Logical point 27 stays independent
    // of shields at 28, preserving ownership and native draw/sheath callbacks.
    for(unsigned axis=0;axis<3;++axis)adjusted[12+axis]=back[axis];
    return true;
}
static bool positionStoredStaff(void* child,std::array<float,16>& adjusted){
    std::uintptr_t parent=0;
    unsigned point=0;
    const auto model=reinterpret_cast<std::uintptr_t>(child);
    if(!read(model+0x1CC,parent)||!read(model+0x1D0,point)||(point!=30&&point!=31))return false;
    const auto* context=weaponContext(parent);
    if(!context||!weaponContextActive(*context))return false;
    const WeaponAsset* asset=nullptr;
    // Only an explicitly selected appearance gets a closer fit. Native
    // passthrough weapons and every hand attachment retain their stock pose.
    for(unsigned i=0;i<context->extra.size();++i)
        if(context->extra[i]==child)asset=weaponAsset(context->selection.items[i]);
    if(!asset&&!context->token&&!ownedExtra(*context,child)){
        for(unsigned role=0;role<3;++role){
            const int route=context->routes[role];
            if(route<0||route>=static_cast<int>(context->selection.items.size()))continue;
            const auto* candidate=weaponAsset(context->selection.items[route]);
            if(candidate&&weaponModelMatches(child,candidate->model)&&
               (context->nativeChildren[role]==child||findChildOriginal(reinterpret_cast<void*>(parent),point)==child))asset=candidate;
        }
    }
    if(!asset||asset->kind!=2||asset->subclass!=10||asset->sheath!=2)return false;
    const auto* shaft=staffShaftFor(asset->model);
    if(!shaft)return false;
    BagMatrix attachment,torso,local,render;
    std::array<float,3> anchor;
    if(!animatedAttachmentMatrix(parent,point,attachment,&torso,&anchor)||
       !read(model+0xBC,local)||!read(parent+0xFC,render))return false;
    for(const auto& fit:staffFits){
        if(fit.point!=point)continue;
        bool matches=true;
        for(unsigned axis=0;axis<3;++axis)
            if(std::fabs(anchor[axis]-fit.anchor[axis])>.00001f)matches=false;
        if(matches)return staffContactPlacement(attachment,local,torso,render,fit.inward,
            {{shaft->minY,shaft->maxY,shaft->minZ,shaft->maxZ}},adjusted);
    }
    return false;
}
static bool positionStoredBackWeapon(void* child,std::array<float,16>& adjusted){
    std::uintptr_t parent=0;unsigned point=0;
    const auto model=reinterpret_cast<std::uintptr_t>(child);
    if(!read(model+0x1CC,parent)||!read(model+0x1D0,point)||(point!=30&&point!=31))return false;
    const auto* c=weaponContext(parent);
    if(!c||!weaponContextActive(*c))return false;
    const auto* hand=modernHandWeaponAsset(*c,child,point==30?0:1);
    if(hand){
        if((hand->kind!=1&&hand->kind!=2)||hand->sheath!=1)return false;
    }else{
        const unsigned position=point==30?2:3;
        const auto* asset=weaponAsset(c->selection.items[position]);
        if(!asset||asset->kind!=2||asset->sheath!=1)return false;
        if(c->extra[position]!=child){
            if(c->token)return false;
            bool routed=false;for(auto route:c->routes)if(route==static_cast<int>(position))routed=true;
            if(!routed||findChildOriginal(reinterpret_cast<void*>(parent),point)!=child)return false;
        }
    }
    // Points 30/31 have a staff-style pose. Keep those independent logical
    // homes, but draw sheath-type-1 weapons using the model's sword pose at 26/27.
    // The full animated transform preserves both the grip position and angle.
    return animatedAttachmentMatrix(parent,point==30?26:27,adjusted);
}
// Center of the sole allowlisted Quiver_A mesh, measured from its render bounds.
// Verified against the installed build-5875 model by tools/audit_quiver_model.py.
static constexpr std::array<float,3> quiverCenter{{.351867050f,.012878005f,.014971241f}};
static constexpr float quiverRightOffset=.105362409f; // 10% of Quiver_A's mesh length.
static bool positionStoredQuiver(void* child,const float* attachment,std::array<float,16>& adjusted){
    std::uintptr_t parent=0;unsigned point=0;
    const auto model=reinterpret_cast<std::uintptr_t>(child);
    if(!read(model+0x1CC,parent)||!read(model+0x1D0,point)||point!=weaponPoints[6])return false;
    const auto* c=weaponContext(parent);
    if(!c||!weaponContextActive(*c))return false;
    if(c->extra[6]==child){
        const auto* asset=weaponAsset(c->selection.items[6]);
        if(!asset||asset->kind!=5)return false;
    }else{
        // A passthrough quiver keeps the client's ordinary pose while unchecked.
        if(!c->quiverHorizontal||c->selection.items[6]||
           (c->passthroughQuiver!=child&&!nativeQuiverModel(child)))return false;
    }
    if(!read(reinterpret_cast<std::uintptr_t>(attachment),adjusted))return false;
    std::array<float,16> local;
    std::array<float,3> back,center;
    if(!read(model+0xBC,local)||!animatedBackPosition(parent,back,quiverRightOffset))return false;
    for(const auto* matrix:{&adjusted,&local}){
        for(auto value:*matrix)if(!std::isfinite(value))return false;
        if(std::fabs((*matrix)[15]-1.f)>.001f)return false;
    }
    // 0x714389 composes the child's local transform with this attachment.
    // Include its scale, orientation and offset before moving the mesh center.
    for(unsigned i=0;i<3;++i){
        center[i]=local[12+i]+quiverCenter[0]*local[i]+
            quiverCenter[1]*local[4+i]+quiverCenter[2]*local[8+i];
        if(!std::isfinite(center[i]))return false;
    }
    // Reverse the previous rotation: +45 degrees about animated local +Y.
    // Rotate a fresh copy each frame, preserving the basis lengths and angles.
    std::array<float,3> axis{{adjusted[4],adjusted[5],adjusted[6]}};
    const float length=std::sqrt(axis[0]*axis[0]+axis[1]*axis[1]+axis[2]*axis[2]);
    if(!std::isfinite(length)||length<.000001f)return false;
    for(auto& value:axis)value/=length;
    constexpr float cosine=.7071067811865475f,sine=.7071067811865475f;
    if(c->quiverHorizontal)for(unsigned column:{0u,8u}){
        const std::array<float,3> value{{adjusted[column],adjusted[column+1],adjusted[column+2]}};
        const float dot=axis[0]*value[0]+axis[1]*value[1]+axis[2]*value[2];
        for(unsigned i=0;i<3;++i){
            const unsigned j=(i+1)%3,k=(i+2)%3;
            adjusted[column+i]=cosine*value[i]+sine*(axis[j]*value[k]-axis[k]*value[j])+
                (1.f-cosine)*dot*axis[i];
            if(!std::isfinite(adjusted[column+i]))return false;
        }
    }
    // Point 26 is a shoulder-side origin, and the mesh origin is near one end.
    // Use the active body's center-back anchor in both modes. Compensate for
    // the transformed mesh center so rotation happens in place, not around
    // the shoulder: T = back - rotatedBasis * localMeshCenter.
    for(unsigned i=0;i<3;++i){
        adjusted[12+i]=back[i]-center[0]*adjusted[i]-
            center[1]*adjusted[4+i]-center[2]*adjusted[8+i];
        if(!std::isfinite(adjusted[12+i]))return false;
    }
    return true;
}
static bool hideNativeQuiver(void* child){
    const auto model=reinterpret_cast<std::uintptr_t>(child);
    std::uintptr_t parent=0,resource=0;unsigned point=0,loaded=0;
    if(!read(model+0x1CC,parent)||!read(model+0x1D0,point)||point!=weaponPoints[6])return false;
    const auto* c=weaponContext(parent);
    if(!c||!weaponContextActive(*c)||ownedExtra(*c,child))return false;
    if(c->selection.carriedMode==1)return nativeQuiverModel(child);
    if(c->selection.carriedMode==0)return false;
    const auto replacement=c->extra[6]?c->extra[6]:c->passthroughQuiver;
    if(!replacement)return false;
    const auto* asset=weaponAsset(c->extra[6]?c->selection.items[6]:c->actualQuiver);
    if(!asset||asset->kind!=5)return false;
    const auto custom=reinterpret_cast<std::uintptr_t>(replacement);
    std::uintptr_t customParent=0,customResource=0;unsigned customPoint=0,customLoaded=0;
    // Resources are cached by mesh filename; different quiver skins share one.
    // Compare the loaded mesh, not just point 26 (which swords also use).
    return read(model+0x10,loaded)&&loaded&&read(model+0x30,resource)&&resource&&
        read(custom+0x1CC,customParent)&&customParent==parent&&
        read(custom+0x1D0,customPoint)&&customPoint==point&&
        read(custom+0x10,customLoaded)&&customLoaded&&
        read(custom+0x30,customResource)&&customResource==resource;
}
static bool replacesStoredWeapon(const WeaponContext& c,const WeaponAsset& asset,unsigned point){
    if(!c.selection.independent)return false;
    int position=-1;
    if(asset.kind==4)position=5;
    else if(asset.kind==3&&point==28)position=4;
    else if(asset.kind==1||asset.kind==2){
        // Native swords use 26/27, while our independent back slots keep
        // ownership at 30/31 and borrow those sword poses when necessary.
        if(point==26||point==30)position=2;
        else if(point==27||point==31)position=3;
        else if(point==32)position=0;
        else if(point==33)position=1;
    }
    if(position<0||!c.selection.items[position]||!c.extra[position])return false;
    const auto replacement=reinterpret_cast<std::uintptr_t>(c.extra[position]);
    std::uintptr_t parent=0;unsigned home=0,loaded=0;
    // Keep the original visible until its replacement is actually attached
    // and loaded. Clearing/detaching a carried choice restores it immediately.
    return read(replacement+0x1CC,parent)&&parent==c.parent&&
        read(replacement+0x1D0,home)&&home==weaponPoints[position]&&
        read(replacement+0x10,loaded)&&loaded;
}
static int selectedWeaponHome(const WeaponSelection& selection,unsigned role,int route){
    if(selection.stowedMask>=0&&role<3){
        const auto* asset=weaponAsset(route>=0?selection.items[route]:selection.equipped[role]);
        if(!asset)return -1;
        // All ranged appearances, including wands, have a stowed back home.
        // The per-slot visibility bit decides whether that model is shown.
        if(asset->kind==4)return 27;
        const int home=sheathPointOriginal(asset->sheath,role==0);
        // Keep melee away from the native quiver and ranged attachment points.
        return home==26?30:home==27?31:home;
    }
    if(route<0||route>=static_cast<int>(weaponPoints.size()))return -1;
    if(selection.carriedMode!=0||route<7)return static_cast<int>(weaponPoints[route]);
    const auto* asset=weaponAsset(selection.items[route]);
    if(!asset)return -1;
    const unsigned side=role==0||(role==2&&(asset->inventory==25||asset->inventory==26));
    return sheathPointOriginal(asset->sheath,side);
}
static bool hideStoredWeapon(void* child){
    const auto model=reinterpret_cast<std::uintptr_t>(child);
    std::uintptr_t parent=0;unsigned point=0;
    if(!read(model+0x1CC,parent)||!read(model+0x1D0,point)||point<26||point>33)return false;
    const auto* c=weaponContext(parent);
    if(!c||!weaponContextActive(*c))return false;
    if(c->selection.stowedMask>=0){
        // Decoration is independent; only the actual hand-role child is hidden.
        for(unsigned i=0;i<7;++i)if(c->extra[i]==child)return false;
        if(c->passthroughQuiver==child)return false;
        for(unsigned role=0;role<3;++role){
            const bool hidden=(c->selection.stowedMask&(1<<role))==0;
            if(c->nativeChildren[role]==child||c->extra[7+role]==child)return hidden;
            const int route=c->routes[role];
            const int home=selectedWeaponHome(c->selection,role,route);
            for(unsigned item:{route>=0?c->selection.items[route]:0,c->selection.equipped[role]}){
                const auto* asset=weaponAsset(item);if(!asset)continue;
                const unsigned side=role==0||(role==2&&(asset->inventory==25||asset->inventory==26));
                if((home==static_cast<int>(point)||sheathPointOriginal(asset->sheath,side)==static_cast<int>(point))&&
                    weaponModelMatches(child,asset->model))return hidden;
            }
        }
        return false;
    }
    if(c->selection.carriedMode==0)return false;
    if(c->selection.carriedMode==1){
        if(ownedExtra(*c,child))return false;
        for(auto native:c->nativeChildren)if(native==child)return true;
        // TryOn-created preview children can use the appearance's native home,
        // rather than the world's routed home. Match both actual and selected
        // meshes at their audited sheath points, never every child on a bone.
        for(unsigned role=0;role<3;++role){
            const int route=c->routes[role];
            const unsigned selected=route>=0?c->selection.items[route]:0;
            for(unsigned item:{selected,c->selection.equipped[role]}){
                const auto* asset=weaponAsset(item);if(!asset)continue;
                const unsigned side=role==0||(role==2&&(asset->inventory==25||asset->inventory==26));
                if((sheathPointOriginal(asset->sheath,side)==static_cast<int>(point)||
                    (item==selected&&selectedWeaponHome(c->selection,role,route)==static_cast<int>(point)))&&
                    weaponModelMatches(child,asset->model))return true;
            }
        }
        return false;
    }
    const auto hidden=[c](const WeaponAsset* asset){
        return asset&&((asset->kind==4&&c->hideRangedWhenStored)||
            ((asset->kind==1||asset->kind==2)&&c->hideMeleeWhenStored));
    };
    for(unsigned i=0;i<c->extra.size();++i)if(c->extra[i]==child)
        return point==weaponPoints[i]&&hidden(weaponAsset(c->selection.items[i]));
    if(c->passthroughQuiver==child)return false;
    // A native child's point alone is ambiguous. Match its selected/equipped
    // model as well, leaving hands, quivers and unrelated props untouched.
    // Carried choices also replace real equipment when no hand override exists.
    for(unsigned role=0;role<3;++role){
        const int route=c->routes[role];
        const auto* asset=weaponAsset(route>=0?c->selection.items[route]:c->selection.equipped[role]);
        if(!asset||(route<7&&!hidden(asset)&&!replacesStoredWeapon(*c,*asset,point)))continue;
        const unsigned side=role==0||(role==2&&(asset->inventory==25||asset->inventory==26));
        const int home=route>=0?selectedWeaponHome(c->selection,role,route):sheathPointOriginal(asset->sheath,side);
        if(home==static_cast<int>(point)&&weaponModelMatches(child,asset->model))return true;
    }
    return false;
}
static void __fastcall bowStringDrawHook(void* model,void* renderState,void* unit){
    // Build 5875's 0x611FF0 callback draws a separate line through $WTT/$WTB.
    // Its color alpha is fixed at 255 (0x61211A), ignoring the mesh's alpha.
    // Skip only that draw submission; normal model/animation updates continue,
    // and the original callback runs again immediately when the bow is drawn.
    if(!hideStoredWeapon(model))bowStringDrawOriginal(model,renderState,unit);
}
static void* __fastcall findChildHook(void* parent,void*,unsigned point);
static bool tuneStoredPlacement(void* child,const BagMatrix& base, std::array<float,16>& out){
    const auto model=reinterpret_cast<std::uintptr_t>(child);
    std::uintptr_t parent=0,resource=0,header=0,lookup=0,records=0;
    unsigned point=0;std::uint16_t index=0;
    if(!read(model+0x1CC,parent)||!read(model+0x1D0,point))return false;
    const auto* c=weaponContext(parent);
    if(!c||!weaponContextActive(*c))return false;
    int slot=-1;
    for(unsigned i=0;i<7;++i)if(point==weaponPoints[i]&&c->selection.items[i]){
        if(c->extra[i]==child)slot=i;
        else if(!c->token&&findChildOriginal(reinterpret_cast<void*>(parent),point)==child)
            for(auto route:c->routes)if(route==static_cast<int>(i))slot=i;
    }
    // Equipped-slot fits apply only while stored. Drawing always uses the
    // untouched native hand transform, including native rotation and size.
    if(point<=2)return false;
    if(c->selection.stowedMask>=0){
        for(unsigned role=0;role<3;++role){
            const int route=c->routes[role];
            const auto* asset=weaponAsset(route>=0?c->selection.items[route]:c->selection.equipped[role]);
            if(!asset)continue;
            // TryOn retains the real item's native sheath point in a preview;
            // only world weapons and our own preview extras use routed homes.
            const bool passthroughPreview=c->token&&route<0;
            const unsigned side=role==0||(role==2&&(asset->inventory==25||asset->inventory==26));
            const int home=passthroughPreview?sheathPointOriginal(asset->sheath,side):
                selectedWeaponHome(c->selection,role,route);
            if(home<0||point!=static_cast<unsigned>(home))continue;
            const bool owned=c->token?(c->extra[7+role]==child||
                (passthroughPreview&&findChildHook(reinterpret_cast<void*>(parent),nullptr,point)==child)):
                c->nativeChildren[role]==child;
            if(owned&&weaponModelMatches(child,asset->model)){
                slot=7+role;break;
            }
        }
    }
    if(slot<0)return false;
    BagMatrix back,torso,local,render;
    if(!animatedAttachmentMatrix(parent,28,back,&torso)||!read(model+0xBC,local)||
       !read(parent+0xFC,render)||!read(parent+0x30,resource)||!read(resource+0x130,header)||
       !read(header+0x110,lookup)||!read(lookup+56,index)||!read(header+0x108,records))return false;
    std::array<float,3> anchor;
    if(!read(records+48*index+8,anchor))return false;
    for(unsigned i=0;i<16;++i){
        bool matches=true;
        for(unsigned axis=0;axis<3;++axis)
            if(!std::isfinite(anchor[axis])||std::fabs(anchor[axis]-bagFits[i].anchor[axis])>.00001f)matches=false;
        if(!matches)continue;
        const auto& fit=c->guid==getPlayer()?weaponTuningEntries[slot][i]:c->sharedFits[slot];
        return fit.enabled&&
            placementTuning(base,local,torso,render,fit.values,out);
    }
    return false;
}
static int __fastcall setWeaponPhysicsLua(void* L){
    if(!isNumber(L,1)||(toNumber(L,1)!=0&&toNumber(L,1)!=1))return result(L,-2);
    const auto owner=getPlayer();if(!owner)return result(L,-1);
    const bool enabled=toNumber(L,1)==1;
    WeaponPhysicsSettings settings;
    unsigned* amounts[]={&settings.bounce,&settings.rocking,&settings.jumpLift};
    for(unsigned i=0;i<3;++i)if(isNumber(L,2+i)){
        const double value=toNumber(L,2+i);
        if(!std::isfinite(value)||value<0||value>200||value!=std::floor(value))return result(L,-2);
        *amounts[i]=static_cast<unsigned>(value);
    }
    if(weaponPhysicsOwner!=owner||weaponPhysicsEnabled!=enabled){for(auto& c:weaponContexts)c.rigidWeapons={};}
    if(weaponPhysicsOwner!=owner)weaponSlotPhysics={};
    weaponPhysicsOwner=owner;weaponPhysicsEnabled=enabled;weaponPhysicsSettings=settings;return result(L,1);
}
static int __fastcall setWeaponSlotPhysicsLua(void* L){
    unsigned values[5];for(unsigned i=0;i<5;++i){
        if(!isNumber(L,i+1))return result(L,-2);
        const double v=toNumber(L,i+1);
        if(!std::isfinite(v)||v<0||v>(i==0?110:i==1?2:200)||v!=std::floor(v))return result(L,-2);
        values[i]=static_cast<unsigned>(v);
    }
    if(values[0]<101||values[0]==107)return result(L,-2);
    const auto owner=getPlayer();if(!owner)return result(L,-1);
    if(weaponPhysicsOwner!=owner){weaponSlotPhysics={};weaponPhysicsEnabled=false;weaponPhysicsSettings={};weaponPhysicsOwner=owner;}
    auto& slot=weaponSlotPhysics[values[0]-101];
    if(slot.mode!=values[1])for(auto& c:weaponContexts){c.rigidWeapons[values[0]-101]={};if(values[0]>=108)c.rigidWeapons[values[0]-108+10]={};}
    slot={values[1],{values[2],values[3],values[4]}};return result(L,1);
}
static bool weaponPhysicsForSlot(const WeaponContext& c,unsigned index,WeaponPhysicsSettings& settings){
    const unsigned slot=index>=10?index-3:index;
    if(slot>=10||slot==6)return false;
    const bool local=c.guid==getPlayer();if(local&&weaponPhysicsOwner!=c.guid)return false;
    const auto& custom=local?weaponSlotPhysics[slot]:c.sharedWeaponSlotPhysics[slot];
    settings=custom.mode?custom.settings:(local?weaponPhysicsSettings:c.sharedWeaponPhysicsSettings);
    return custom.mode?custom.mode==2:(local?weaponPhysicsEnabled:c.sharedWeaponPhysics);
}
static bool positionWeaponPhysics(void* child,const float* matrix,BagMatrix& out,bool visible){
    const auto model=reinterpret_cast<std::uintptr_t>(child);std::uintptr_t parent=0;unsigned point=0;
    if(!read(model+0x1CC,parent)||!read(model+0x1D0,point))return false;
    auto* c=weaponContext(parent);if(!c||!weaponContextActive(*c))return false;
    int slot=-1;const WeaponAsset* asset=nullptr;
    for(unsigned i=0;i<c->extra.size();++i)if(c->extra[i]==child&&i!=6){slot=i;asset=weaponAsset(c->selection.items[i]);break;}
    if(slot<0)for(unsigned role=0;role<3;++role)if(c->nativeChildren[role]==child){
        slot=10+role;const int route=c->routes[role];asset=weaponAsset(route>=0?c->selection.items[route]:c->selection.equipped[role]);break;
    }
    if(slot<0)return false;
    auto& entry=c->rigidWeapons[slot];
    WeaponPhysicsSettings settings;const bool enabled=weaponPhysicsForSlot(*c,slot,settings);
    // Hands, quivers, unrelated props and stationary previews keep native poses.
    if(!enabled||c->token||!visible||point<26||point>33||!asset||asset->kind==5||!weaponModelMatches(child,asset->model)){entry={};return false;}
    if(entry.child!=model||entry.point!=point){entry={};entry.child=model;entry.point=point;}
    BagMatrix base,local,render,world;std::uintptr_t scene=0;
    if(!matrix||!read(model+0xBC,local)||!read(parent+0xFC,render)||!read(parent+0x2C,scene)||!scene||!read(scene+0x9C,world)){entry={};return false;}
    // The caller may pass a just-computed stack matrix (staff/bow/tuner fixes).
    for(unsigned i=0;i<16;++i)base[i]=matrix[i];
    return rigidWeaponPhysics(entry.motion,base,local,render,world,out,model,slot,point,bagClockMilliseconds(),bagIsRunning(*c),bagMotionActive(*c),bagAirLiftTarget(*c),settings);
}
static void updateWeaponAttachment(void* model,const float* matrix,const float* color,const float* lighting,float alpha){
    std::array<float,16> adjusted;
    std::uintptr_t parent=0;read(reinterpret_cast<std::uintptr_t>(model)+0x1CC,parent);
    const auto* context=weaponContext(parent);
    const bool clonedBag=!context&&clonedBagOwner(parent)&&
        (clonedBagChild(reinterpret_cast<std::uintptr_t>(model))||
         weaponModelMatches(model,"Interface\\AddOns\\SaureksCloset\\Models\\DarkSchoolbag.mdx"));
    bool instance=false;if(context)for(const auto& bag:context->bags)if(bag.child==model)instance=true;
    if((context&&context->backpack==model)||instance||clonedBag){
        // Wait for valid animated bones instead of briefly drawing a shield pose.
        if(positionBackpack(model,adjusted,true)){
            updateAttachedOriginal(model,adjusted.data(),color,lighting,alpha);
            applyBagResponseBones(model);
        }
        else updateAttachedOriginal(model,matrix,color,lighting,0);
        return;
    }
    // Zero effective alpha skips the native mesh draw, while its normal update
    // still runs. No native ownership or visibility state is changed; removing
    // the custom quiver restores the original alpha on the next frame.
    if(hideNativeQuiver(model)||hideStoredWeapon(model))alpha=0;
    const bool positioned=positionStoredStaff(model,adjusted)||positionStoredBackWeapon(model,adjusted)||positionStoredBow(model,matrix,adjusted)||
       positionStoredQuiver(model,matrix,adjusted);
    if(positioned)matrix=adjusted.data();
    std::array<float,16> tuned,base;
    if(positioned)base=adjusted;
    if((positioned||read(reinterpret_cast<std::uintptr_t>(matrix),base))&&tuneStoredPlacement(model,base,tuned))matrix=tuned.data();
    BagMatrix physics;
    if(positionWeaponPhysics(model,matrix,physics,alpha>0))matrix=physics.data();
    updateAttachedOriginal(model,matrix,color,lighting,alpha);
}
static bool ownedBagUpdate(void* model){
    const auto child=reinterpret_cast<std::uintptr_t>(model);std::uintptr_t parent=0;
    if(!read(child+0x1CC,parent))return false;
    if(const auto* context=weaponContext(parent)){
        if(!weaponContextActive(*context))return false;
        if(context->backpack==model)return true;
        for(const auto& bag:context->bags)if(bag.child==model)return true;
    }
    return clonedBagOwner(parent)==getPlayer()&&getPlayer()&&
        (clonedBagChild(child)||weaponModelMatches(model,"Interface\\AddOns\\SaureksCloset\\Models\\DarkSchoolbag.mdx"));
}
static bool ownedWeaponPhysicsUpdate(void* child){
    const auto model=reinterpret_cast<std::uintptr_t>(child);std::uintptr_t parent=0;unsigned point=0;
    if(!read(model+0x1CC,parent)||!read(model+0x1D0,point))return false;
    auto* c=weaponContext(parent);if(!c||!weaponContextActive(*c)||c->token)return false;
    if(point<26||point>33){for(auto& entry:c->rigidWeapons)if(entry.child==model)entry={};return false;}
    WeaponPhysicsSettings settings;
    for(unsigned i=0;i<c->extra.size();++i)if(i!=6&&c->extra[i]==child)return weaponPhysicsForSlot(*c,i,settings);
    for(unsigned role=0;role<3;++role)if(c->nativeChildren[role]==child)return weaponPhysicsForSlot(*c,10+role,settings);
    return false;
}
static bool positionHaircraftHat(void* child,const float* matrix,BagMatrix& out){
    if(!bagTuningOwner||bagTuningOwner!=getPlayer())return false;
    const auto model=reinterpret_cast<std::uintptr_t>(child);
    std::uintptr_t parent=0;unsigned point=0;
    if(!read(model+0x1D0,point)||point!=11||!read(model+0x1CC,parent))return false;
    Player p;if(!snapshot(p))return false;
    unsigned race=0,sex=0;
    if(parent==p.model&&p.display==p.native&&nativeModel(p.native)){
        if(!p.component||!read(p.component+0x18,race)||!read(p.component+0x1C,sex))return false;
    }else{
        const auto* preview=previews.find(parent);
        if(!preview||preview->guid!=p.guid)return false;
        race=preview->body.race;sex=preview->body.sex;
    }
    if(!bagTuningKey(111,race,sex))return false;
    const auto& fit=hatTuningEntries[(race-1)*2+sex];if(!fit.enabled)return false;
    // Spell effects can also attach to point 11. Adjust head equipment only.
    std::uintptr_t resource=0;std::array<char,260> path{};BagMatrix base;
    if(!read(model+0x30,resource)||!resource||
       !read(resource+0x20,path)||!read(reinterpret_cast<std::uintptr_t>(matrix),base))return false;
    for(auto& ch:path)ch=armorPathCharacter(ch);
    if(!armorPathPrefix(path,"item\\objectcomponents\\head\\"))return false;
    return hatPlacement(base,fit.values,out);
}
static void updateAttachmentForCaller(void* model,const float* matrix,const float* color,const float* lighting,float alpha,std::uintptr_t caller){
    if(hairMask::capture)hairMask::capture(model,matrix);
    BagMatrix hat;
    if(positionHaircraftHat(model,matrix,hat)){
        updateAttachedOriginal(model,hat.data(),color,lighting,alpha);return;
    }
    // 0x714000 can lazily evaluate a child after its parent is already current
    // (returns 0x71415D/0x714183). Those calls overwrite its complete palette
    // too, so every owned-bag update must restore local deformation afterward.
    // Opted-in rigid weapons must also retain their pose on lazy updates.
    // Unrelated equipment keeps the recursive-only routing restriction.
    if(caller==0x718761||ownedBagUpdate(model)||ownedWeaponPhysicsUpdate(model))
        updateWeaponAttachment(model,matrix,color,lighting,alpha);
    else updateAttachedOriginal(model,matrix,color,lighting,alpha);
}
static void __fastcall updateAttachedHook(void* model,void*,const float* matrix,const float* color,const float* lighting,float alpha){
    updateAttachmentForCaller(model,matrix,color,lighting,alpha,reinterpret_cast<std::uintptr_t>(__builtin_return_address(0)));
}
static void releaseExtras(WeaponContext& c){
    c.rigidWeapons={};
    const auto extra=c.extra;c.extra.fill(nullptr);
    for(auto child:extra)if(child){
        std::uintptr_t parent=0;
        if(read(reinterpret_cast<std::uintptr_t>(child)+0x1CC,parent)&&parent==c.parent)detachChild(child);
        releaseModel(child);
    }
}
static void releasePassthroughQuiver(WeaponContext& c){
    auto* child=c.passthroughQuiver;c.passthroughQuiver=nullptr;
    if(!child)return;
    std::uintptr_t parent=0;
    if(read(reinterpret_cast<std::uintptr_t>(child)+0x1CC,parent)&&parent==c.parent)detachChild(child);
    releaseModel(child);
}
static void releaseBackpack(WeaponContext& c){
    auto* child=c.backpack;c.backpack=nullptr;c.bagMotion={};
    if(!child)return;
    std::uintptr_t parent=0;
    if(read(reinterpret_cast<std::uintptr_t>(child)+0x1CC,parent)&&parent==c.parent)detachChild(child);
    releaseModel(child);
}
static void releaseBagInstance(WeaponContext& context,BagInstance& bag){
    auto* child=bag.child;bag.child=nullptr;bag.motion={};bag.response={};bag.bodyBinding={};bag.bodyBound=false;
    if(!child)return;
    std::uintptr_t parent=0;
    if(read(reinterpret_cast<std::uintptr_t>(child)+0x1CC,parent)&&parent==context.parent)detachChild(child);
    releaseModel(child);
}
static void releaseBagInstances(WeaponContext& context){
    if(hasBagInstances(context))context.bagGeneration=nextBagGeneration();
    for(auto& bag:context.bags){releaseBagInstance(context,bag);bag={};}
}
static void forgetWeapons(std::uintptr_t model){
    for(auto& entry:clonedBagChildren)if(entry.child==model)entry={};
    for(auto& entry:clonedBagPreviews)if(entry.model==model)entry={};
    for(auto& c:weaponContexts){
        for(auto& entry:c.rigidWeapons)if(entry.child==model)entry={};
        for(auto& child:c.nativeChildren)if(reinterpret_cast<std::uintptr_t>(child)==model)child=nullptr;
        for(auto& bag:c.bags)if(reinterpret_cast<std::uintptr_t>(bag.child)==model){bag.child=nullptr;bag.motion={};}
    }
#if !defined(SAUREKS_WEAPON_TEST) || defined(SAUREKS_SHARING_TEST)
    for(auto& pair:sharedWeaponContexts){auto& c=pair.second;
        for(auto& entry:c.rigidWeapons)if(entry.child==model)entry={};
        for(auto& child:c.nativeChildren)if(reinterpret_cast<std::uintptr_t>(child)==model)child=nullptr;
        for(auto& child:c.extra)if(reinterpret_cast<std::uintptr_t>(child)==model)child=nullptr;
        for(auto& bag:c.bags)if(reinterpret_cast<std::uintptr_t>(bag.child)==model){bag.child=nullptr;bag.motion={};bag.response={};bag.bodyBinding={};bag.bodyBound=false;}
        if(reinterpret_cast<std::uintptr_t>(c.passthroughQuiver)==model)c.passthroughQuiver=nullptr;
        if(reinterpret_cast<std::uintptr_t>(c.backpack)==model){c.backpack=nullptr;c.bagMotion={};}
    }
#endif
    if(auto* c=weaponContext(model)){releaseExtras(*c);releasePassthroughQuiver(*c);releaseBackpack(*c);releaseBagInstances(*c);*c={};}
}
static void discardInheritedPreviewWeapons(std::uintptr_t parent){
    const auto* preview=previews.find(parent);
    if(!preview||preview->guid!=getPlayer())return;
    const auto* context=weaponContext(parent);
    std::uintptr_t child=0;read(parent+0x1DC,child);
    for(unsigned n=0;child&&n<128;++n){
        unsigned point=0;std::uintptr_t owner=0,next=0;
        if(!read(child+0x1CC,owner)||owner!=parent||!read(child+0x1D0,point)||!read(child+0x1E4,next))return;
        const bool weaponPoint=point<=2||(point>=26&&point<=33);
        if(weaponPoint&&(!context||!ownedExtra(*context,reinterpret_cast<void*>(child))))
            detachChild(reinterpret_cast<void*>(child));
        if(next==child)return;
        child=next;
    }
}
static void* __fastcall findChildHook(void* parent,void*,unsigned point){
    const auto* c=weaponContext(reinterpret_cast<std::uintptr_t>(parent));
    if(!c)return findChildOriginal(parent,point);
    std::uintptr_t child=0;read(c->parent+0x1DC,child);
    for(unsigned n=0;child&&n<128;++n){
        unsigned id=0;std::uintptr_t next=0;
        if(!read(child+0x1D0,id)||!read(child+0x1E4,next))return nullptr;
        if(id==point&&!ownedExtra(*c,reinterpret_cast<void*>(child)))return reinterpret_cast<void*>(child);
        if(next==child)break;
        child=next;
    }
    return nullptr;
}
static void __fastcall clearChildrenHook(void* parent,void*,unsigned point){
    const auto address=reinterpret_cast<std::uintptr_t>(parent);
    if(loadingExtraParent==address)return;
    const auto* c=weaponContext(address);
    if(!c){clearChildrenOriginal(parent,point);return;}
    std::uintptr_t child=0;read(address+0x1DC,child);
    for(unsigned n=0;child&&n<128;++n){
        unsigned id=0;std::uintptr_t next=0;
        if(!read(child+0x1D0,id)||!read(child+0x1E4,next))return;
        if(id==point&&!ownedExtra(*c,reinterpret_cast<void*>(child)))detachChild(reinterpret_cast<void*>(child));
        if(next==child)break;
        child=next;
    }
}
static int __fastcall sheathPointHook(unsigned sheath,unsigned side){
    return scopedSheathPoint>=0?scopedSheathPoint:sheathPointOriginal(sheath,side);
}
static void* weaponDisplay(const WeaponAsset* asset){
    std::uintptr_t table=0,row=0;unsigned max=0,id=0;
    if(asset&&read(0xC0DC10,table)&&read(0xC0DC14,max)&&asset->display<=max&&
       read(table+4*asset->display,row)&&read(row,id)&&id==asset->display)return reinterpret_cast<void*>(row);
    return nullptr;
}
using WeaponInfo=const unsigned char* (__thiscall *)(void*,unsigned,unsigned);
static WeaponInfo weaponInfoOriginal=nullptr;
static const unsigned char* weaponInfoAt(void* unit,unsigned role,unsigned raw,std::uintptr_t caller){
    const auto* original=weaponInfoOriginal(unit,role,raw);
    // Only audited visual consumers of the player virtual-item getter. Other
    // callers (item requirements, skills, spell checks, inventory) get real data.
    const bool visual=caller==0x60B5BB||caller==0x60B797||caller==0x611E24||
        caller==0x61183B||caller==0x6118DF||caller==0x61199F||caller==0x611A45||
        caller==0x611BCE||caller==0x60BAA4||caller==0x624B35||caller==0x5DEF00||
        caller==0x5FD4A5||caller==0x5FE019||(caller==0x60A4FE&&scopedProjectileAppearance);
    if(!original||role!=2||raw||!visual)return original;
    Player p;
    if(!weaponPlayer(unit,p)||p.display!=p.native)return original;
    auto* c=weaponContext(p.model);
    if(!c||c->token||c->guid!=p.guid||c->unit!=p.unit||c->routes[2]<0)return original;
    const auto* asset=weaponAsset(c->selection.items[c->routes[2]]);
    if(!asset||asset->kind!=4||!weaponDisplay(asset))return original;
    // Return a private copy; never alter the unit's cached info/update fields.
    for(unsigned i=0;i<8;++i)c->rangedInfo[i]=original[i];
    c->rangedInfo[0]=2;c->rangedInfo[1]=asset->subclass;
    c->rangedInfo[3]=asset->inventory;c->rangedInfo[4]=asset->sheath;
    return c->rangedInfo.data();
}
static const unsigned char* __fastcall weaponInfoHook(void* unit,void*,unsigned role,unsigned raw){
    return weaponInfoAt(unit,role,raw,reinterpret_cast<std::uintptr_t>(__builtin_return_address(0)));
}
using UnitAnimation=void (__thiscall *)(void*,unsigned);
static UnitAnimation unitAnimationOriginal=nullptr;
static unsigned weaponAnimation(void* unit,unsigned animation){
    Player p;
    if(!weaponPlayer(unit,p)||p.display!=p.native)return animation;
    auto* c=weaponContext(p.model);
    if(!c||c->token||c->guid!=p.guid||c->unit!=p.unit||c->routes[2]<0)return animation;
    const auto* selected=weaponAsset(c->selection.items[c->routes[2]]);
    const auto* equipped=weaponAsset(c->selection.equipped[2]);
    if(!selected||selected->kind!=4||!weaponDisplay(selected))return animation;
    const auto* info=weaponInfoOriginal?weaponInfoOriginal(unit,2,0):nullptr;
    const unsigned subclass=info&&info[0]==2?info[1]:(equipped?equipped->subclass:255);
    return rangedAppearanceAnimation(animation,subclass,selected->subclass);
}
static void __fastcall unitAnimationHook(void* unit,void*,unsigned animation){
    // The stock animation scheduler still owns timing and completion; no
    // update-field mutation, spell replacement or per-frame model rebuild.
    unitAnimationOriginal(unit,weaponAnimation(unit,animation));
}
static int __fastcall weaponComposeHook(void* parent,void* display,unsigned slot,unsigned sheath,unsigned stored,unsigned shield,unsigned rangedRight){
    auto* c=weaponContext(reinterpret_cast<std::uintptr_t>(parent));
    const int old=scopedSheathPoint;
    scopedSheathPoint=-1;
    if(c&&!c->token&&weaponContextActive(*c)&&slot>=15&&slot<=17){
        const int route=c->routes[slot-15];
        if(c->selection.stowedMask>=0)scopedSheathPoint=selectedWeaponHome(c->selection,slot-15,route);
        if(route>=0){
            const auto* a=weaponAsset(c->selection.items[route]);
            if(void* row=weaponDisplay(a)){
                display=row;scopedSheathPoint=selectedWeaponHome(c->selection,slot-15,route);sheath=a->sheath;
                shield=a->kind==3;
                if(slot==17)rangedRight=a->inventory==25||a->inventory==26;
            }
        }
    }
    const int result=weaponComposeOriginal(parent,display,slot,sheath,stored,shield,rangedRight);
    // 47A218 returns the composed attachment; all rejected inputs return -1 at
    // 47A223. Success clears that attachment before loading, so an unrelated
    // preexisting prop cannot become a tracked weapon when composition fails.
    if(c&&!c->token&&weaponContextActive(*c)&&slot>=15&&slot<=17)
        c->nativeChildren[slot-15]=result>=0&&result<=33?findChildHook(parent,nullptr,static_cast<unsigned>(result)):nullptr;
    scopedSheathPoint=old;return result;
}
static void __fastcall moveWeaponHook(void* unit,void*,unsigned role,unsigned stored){
    std::uintptr_t parent=0;read(reinterpret_cast<std::uintptr_t>(unit)+0xD8,parent);
    auto* c=weaponContext(parent);const int old=scopedSheathPoint;
    // A ranged move can rebuild melee weapons recursively. Each role must
    // choose its own home instead of inheriting the outer ranged override.
    scopedSheathPoint=-1;
    if(c&&!c->token&&c->unit==reinterpret_cast<std::uintptr_t>(unit)&&weaponContextActive(*c)&&role<3&&(c->routes[role]>=0||c->selection.stowedMask>=0))
        scopedSheathPoint=selectedWeaponHome(c->selection,role,c->routes[role]);
    moveWeaponOriginal(unit,role,stored);scopedSheathPoint=old;
}
static void __fastcall rebuildWeaponHook(void* unit,void*,unsigned role){
    std::uintptr_t parent=0;read(reinterpret_cast<std::uintptr_t>(unit)+0xD8,parent);
    auto* c=weaponContext(parent);const int old=scopedSheathPoint;
    scopedSheathPoint=-1;
    if(c&&!c->token&&c->unit==reinterpret_cast<std::uintptr_t>(unit)&&weaponContextActive(*c)&&role<3&&(c->routes[role]>=0||c->selection.stowedMask>=0))
        scopedSheathPoint=selectedWeaponHome(c->selection,role,c->routes[role]);
    // The client's missing-child fallback also computes the sheath point for
    // weapon effects before composing. Scope that entire role's rebuild.
    rebuildWeaponOriginal(unit,role);scopedSheathPoint=old;
}
static void __fastcall sheathTransitionHook(void* unit,void*){
    const auto address=reinterpret_cast<std::uintptr_t>(unit);
    unsigned previous=3;read(address+0xD3C,previous);
    sheathTransitionOriginal(unit);
    // Build 5875's immediate ranged -> unarmed transition deletes the held
    // weapon instead of moving it. NPC talk and loot animations take this path
    // without calling moveWeapon, so restore its routed back position afterward.
    unsigned mode=3;Player p;
    if(previous!=2||!read(address+0xD40,mode)||mode!=0||!weaponPlayer(unit,p)||
       p.unit!=address||p.display!=p.native)return;
    const auto* c=weaponContext(p.model);
    if(!c||c->token||c->unit!=address||c->guid!=p.guid||!weaponContextActive(*c))return;
    for(unsigned role=0;role<2;++role){
        const int home=selectedWeaponHome(c->selection,role,c->routes[role]);
        if(home>=0&&hasPoint(reinterpret_cast<void*>(p.model),static_cast<unsigned>(home))&&
           !findChildHook(reinterpret_cast<void*>(p.model),nullptr,static_cast<unsigned>(home)))
            rebuildWeaponHook(unit,nullptr,role);
    }
    const int route=c->routes[2];
    if((route<0&&c->selection.stowedMask<0)||route>=static_cast<int>(weaponPoints.size()))return;
    const auto* asset=weaponAsset(route>=0?c->selection.items[route]:c->selection.equipped[2]);
    const int home=selectedWeaponHome(c->selection,2,route);
    unsigned loaded=0;
    if(!asset||asset->kind!=4||home<0||!read(p.model+0x10,loaded)||!loaded||
       !hasPoint(reinterpret_cast<void*>(p.model),static_cast<unsigned>(home))||
       findChildHook(reinterpret_cast<void*>(p.model),nullptr,static_cast<unsigned>(home)))return;
    // The ranged refresh releases its old held-model reference and retains the
    // new stored child. The generic role-2 rebuild does nothing while unarmed.
    refreshRanged(unit,1);
}
static bool ensureExtras(WeaponContext& c){
    bool complete=true;
    for(unsigned i=0;i<c.extra.size();++i){
        const auto* a=weaponAsset(c.selection.items[i]);if(!a)continue;
        if(i<7&&c.selection.carriedMode==0)continue;
        if(i>=7&&(!c.token||!c.selection.independent||c.routes[i-7]!=static_cast<int>(i)))continue;
        unsigned point=i<7?weaponPoints[i]:i==7?1:i==8?(a->kind==3?0:2):(a->inventory==25||a->inventory==26?1:2);
        if(i>=7&&(i==9?c.previewMode!=2:c.previewMode!=1)){
            if(c.selection.carriedMode!=0&&c.selection.stowedMask<0)continue;
            const int home=selectedWeaponHome(c.selection,i-7,static_cast<int>(i));
            if(home<0)continue;
            point=static_cast<unsigned>(home);
        }
        if(!hasPoint(reinterpret_cast<void*>(c.parent),point)){complete=false;continue;}
        bool routed=false;for(auto route:c.routes)if(route==static_cast<int>(i))routed=true;
        if(!c.token&&routed)continue;
        if(c.extra[i]){
            std::uintptr_t parent=0;
            if(read(reinterpret_cast<std::uintptr_t>(c.extra[i])+0x1CC,parent)&&!parent)
                attachChild(c.extra[i],reinterpret_cast<void*>(c.parent),point);
            continue;
        }
        // Keep preexisting children, then identify the factory's newly attached
        // child by its parent-list head. Our own reference survives stock clears.
        std::uintptr_t before=0,after=0;read(c.parent+0x1DC,before);
        loadingExtraParent=c.parent;
        loadChild(reinterpret_cast<void*>(c.parent),point,a->model,a->texture,0);
        loadingExtraParent=0;read(c.parent+0x1DC,after);
        unsigned actualPoint=0;std::uintptr_t parent=0;
        if(after&&after!=before&&read(after+0x1D0,actualPoint)&&actualPoint==point&&read(after+0x1CC,parent)&&parent==c.parent){
            c.extra[i]=reinterpret_cast<void*>(after);retainChild(c.extra[i]);
        }else complete=false;
    }
    return complete;
}
static bool ensurePassthroughQuiver(WeaponContext& c){
    const auto* asset=weaponAsset(c.actualQuiver);
    if(!c.token||c.selection.carriedMode==1||(c.selection.items[6]&&c.selection.carriedMode!=0)||!asset){releasePassthroughQuiver(c);return true;}
    if(!hasPoint(reinterpret_cast<void*>(c.parent),weaponPoints[6]))return false;
    if(c.passthroughQuiver){
        std::uintptr_t parent=0;unsigned loaded=0;
        if(!read(reinterpret_cast<std::uintptr_t>(c.passthroughQuiver)+0x1CC,parent))return false;
        if(!parent)attachChild(c.passthroughQuiver,reinterpret_cast<void*>(c.parent),weaponPoints[6]);
        return (parent==0||parent==c.parent)&&read(reinterpret_cast<std::uintptr_t>(c.passthroughQuiver)+0x10,loaded)&&loaded;
    }
    // Undress/clone cleanup removes inherited weapon children. Recreate the
    // actual equipped quiver explicitly, so race previews never borrow a skin
    // from a custom world quiver. Keep one owned instance across option changes.
    std::uintptr_t before=0,after=0;read(c.parent+0x1DC,before);
    loadingExtraParent=c.parent;
    loadChild(reinterpret_cast<void*>(c.parent),weaponPoints[6],asset->model,asset->texture,0);
    loadingExtraParent=0;read(c.parent+0x1DC,after);
    std::uintptr_t parent=0;unsigned point=0;
    if(!after||after==before||!read(after+0x1CC,parent)||parent!=c.parent||
       !read(after+0x1D0,point)||point!=weaponPoints[6])return false;
    c.passthroughQuiver=reinterpret_cast<void*>(after);retainChild(c.passthroughQuiver);
    unsigned loaded=0;return read(after+0x10,loaded)&&loaded;
}
static bool ensureBackpack(WeaponContext& c){
    if(!c.backBag){releaseBackpack(c);return true;}
    if(!hasPoint(reinterpret_cast<void*>(c.parent),28))return false;
    if(!c.backpack){
        std::uintptr_t before=0,after=0;read(c.parent+0x1DC,before);
        loadingExtraParent=c.parent;
        loadChild(reinterpret_cast<void*>(c.parent),28,
            "Interface\\AddOns\\SaureksCloset\\Models\\DarkSchoolbag.mdx",
            "Interface\\AddOns\\SaureksCloset\\Models\\DarkSchoolbag.blp",0);
        loadingExtraParent=0;read(c.parent+0x1DC,after);
        std::uintptr_t owner=0;unsigned point=0;
        if(!after||after==before||!read(after+0x1CC,owner)||owner!=c.parent||!read(after+0x1D0,point)||point!=28)return false;
        c.backpack=reinterpret_cast<void*>(after);retainChild(c.backpack);
    }
    std::uintptr_t parent=0;unsigned loaded=0;
    if(!read(reinterpret_cast<std::uintptr_t>(c.backpack)+0x1CC,parent))return false;
    if(parent&&parent!=c.parent){releaseBackpack(c);return false;}
    unsigned point=0;
    if(parent&&(!read(reinterpret_cast<std::uintptr_t>(c.backpack)+0x1D0,point)||point!=28)){
        detachChild(c.backpack);parent=0;c.bagMotion={};
    }
    if(!parent)attachChild(c.backpack,reinterpret_cast<void*>(c.parent),28);
    if(!read(reinterpret_cast<std::uintptr_t>(c.backpack)+0x10,loaded)||!loaded)return false;
    if(!weaponModelMatches(c.backpack,"Interface\\AddOns\\SaureksCloset\\Models\\DarkSchoolbag.mdx")){
        releaseBackpack(c);return false;
    }
    return true;
}
static bool ensureBagInstances(WeaponContext& context);
static int __fastcall setWeapons(void* L){
    unsigned values[11]{};
    for(int i=0;i<11;++i){
        if(!isNumber(L,i+1))return result(L,-2);
        const double v=toNumber(L,i+1);
        if(!std::isfinite(v)||v<0||v>2147483647||v!=static_cast<unsigned>(v))return result(L,-2);
        values[i]=static_cast<unsigned>(v);
    }
    bool options[3]{};
    // Optional for older addon callers. The addon always passes numeric 0/1.
    for(int i=0;i<3;++i)if(isNumber(L,12+i)){
        const double value=toNumber(L,12+i);
        if(value!=0&&value!=1)return result(L,-2);
        options[i]=value==1;
    }
    unsigned actualQuiver=0;
    if(isNumber(L,15)){
        const double value=toNumber(L,15);
        if(!std::isfinite(value)||value<0||value>2147483647||value!=static_cast<unsigned>(value))return result(L,-2);
        actualQuiver=static_cast<unsigned>(value);
        const auto* asset=weaponAsset(actualQuiver);
        if(actualQuiver&&(!asset||asset->kind!=5))return result(L,-2);
    }
    unsigned backBag=0;
    if(isNumber(L,16)){
        const double value=toNumber(L,16);
        if(value!=0&&value!=1)return result(L,-2);
        backBag=static_cast<unsigned>(value);
    }
    WeaponSelection selection;
    for(unsigned i=0;i<7;++i)selection.items[i]=values[i+1];
    for(unsigned i=0;i<3;++i)selection.equipped[i]=values[i+8];
    if(isNumber(L,20)){
        const double mode=toNumber(L,20);
        if(mode!=0&&mode!=1)return result(L,-2);
        selection.independent=mode==1;
        for(int i=0;i<3;++i){
            if(!isNumber(L,17+i))return result(L,-2);
            const double value=toNumber(L,17+i);
            if(!std::isfinite(value)||value<0||value>2147483647||value!=static_cast<unsigned>(value))return result(L,-2);
            selection.items[7+i]=static_cast<unsigned>(value);
        }
    }
    if(isNumber(L,22)){
        const double mode=toNumber(L,22);
        if(mode!=0&&mode!=1)return result(L,-2);
        selection.carriedMode=static_cast<int>(mode);
        selection.independent=true;
    }
    if(!selection.valid())return result(L,-2);
    // Explicitly disabled decoration never routes into a hand or loads extras.
    if(selection.carriedMode==0)for(unsigned i=0;i<7;++i)selection.items[i]=0;
    unsigned previewMode=0;
    if(isNumber(L,21)){
        const double mode=toNumber(L,21);
        if(mode!=0&&mode!=1&&mode!=2)return result(L,-2);
        previewMode=static_cast<unsigned>(mode);
    }
    if(isNumber(L,23)){
        const double mask=toNumber(L,23);
        if(!std::isfinite(mask)||mask<0||mask>7||mask!=static_cast<int>(mask))return result(L,-2);
        selection.stowedMask=static_cast<int>(mask);
    }
    Player p;if(!snapshot(p))return result(L,-1);
    std::uintptr_t parent=p.model;const unsigned token=values[0];
    if(token){
        parent=0;
        for(auto& e:previews.entries)if(e.token==token&&e.guid==p.guid&&e.status==1)parent=e.model;
        if(!parent)return result(L,-1);
    }else if(p.display!=p.native){
        if(auto* c=weaponContext(parent)){releaseExtras(*c);releasePassthroughQuiver(*c);releaseBackpack(*c);releaseBagInstances(*c);*c={};}
        return result(L,0);
    }
    unsigned loaded=0;if(!parent||!read(parent+0x10,loaded)||!loaded)return result(L,0);
    auto* c=weaponContext(parent);
    const bool keepContext=!selection.empty()||selection.stowedMask>=0||selection.carriedMode==1||options[0]||
        (selection.carriedMode<0&&(options[1]||options[2]))||(token&&actualQuiver)||backBag||(c&&hasBagInstances(*c));
    if(!c&&!keepContext)return result(L,1);
    if(!c){
        for(auto& entry:weaponContexts)if(!entry.parent){c=&entry;break;}
        if(!c)return result(L,-1);
        c->parent=parent;c->unit=token?0:p.unit;c->guid=p.guid;c->token=token;
        c->bagGeneration=nextBagGeneration();
    }
    if(c->guid!=p.guid||c->token!=token)return result(L,-1);
    if(token&&c->previewMode!=previewMode){releaseExtras(*c);c->previewMode=previewMode;}
    if(backBag&&hasBagInstances(*c))releaseBagInstances(*c);
    if(c->backBag!=backBag){releaseBackpack(*c);c->backBag=backBag;}
    // Visual options alone must not destroy or rebuild any weapon children.
    c->quiverHorizontal=options[0];c->hideRangedWhenStored=selection.carriedMode<0&&options[1];
    c->hideMeleeWhenStored=selection.carriedMode<0&&options[2];
    if(c->actualQuiver!=actualQuiver){releasePassthroughQuiver(*c);c->actualQuiver=actualQuiver;}
    // An option-only context still records actual equipment for mesh matching,
    // without rebuilding stock weapons just because those IDs were first read.
    if(c->selection.empty()&&selection.empty()&&c->selection.carriedMode==selection.carriedMode&&
        (c->selection.stowedMask>=0)==(selection.stowedMask>=0)&&
        (selection.stowedMask<0||c->selection.equipped==selection.equipped))c->selection=selection;
    if(!(c->selection==selection)){
        releaseExtras(*c);
        // The catalog may not know a server item's sheath/model. Remove only
        // its observed native stored child before an appearance changes homes.
        if(!token&&(c->selection.carriedMode>=0||selection.carriedMode>=0))
            for(auto child:c->nativeChildren)if(child){
                std::uintptr_t owner=0;unsigned point=0;
                const auto model=reinterpret_cast<std::uintptr_t>(child);
                if(read(model+0x1CC,owner)&&owner==parent&&read(model+0x1D0,point)&&
                    point>=26&&point<=33&&!ownedExtra(*c,child))detachChild(child);
            }
        // Remove the old routed weapons at their old homes before rerouting.
        if(!token)for(unsigned role=0;role<3;++role){
            const int home=selectedWeaponHome(c->selection,role,c->routes[role]);
            if(home>=0)clearChildrenHook(reinterpret_cast<void*>(parent),nullptr,static_cast<unsigned>(home));
        }
        if(!token){
            // A first override may use a different home than the stock weapon.
            // Clear both generations before rebuilding, or its old child remains.
            for(const auto* settings:{&c->selection,&selection})for(unsigned role=0;role<3;++role){
                const auto* a=weaponAsset(settings->equipped[role]);if(!a)continue;
                const unsigned side=role==0||(role==2&&(a->inventory==25||a->inventory==26));
                const int point=sheathPointOriginal(a->sheath,side);
                if(point>=0)clearChildrenHook(reinterpret_cast<void*>(parent),nullptr,static_cast<unsigned>(point));
            }
        }
        c->selection=selection;c->routes=selection.routes();
        for(unsigned role=0;role<3;++role){
            auto& route=c->routes[role];
            const int home=selectedWeaponHome(selection,role,route);
            // Native sheath 0 deliberately has no stored child (bows/wands).
            // It still supports an explicit appearance while held.
            if(route>=0&&(!weaponDisplay(weaponAsset(selection.items[route]))||
                (home>=0&&!hasPoint(reinterpret_cast<void*>(parent),static_cast<unsigned>(home)))))route=-1;
        }
        if(!token){
            unsigned mode=0;read(p.unit+0xD40,mode);
            refreshMelee(reinterpret_cast<void*>(p.unit),0);refreshMelee(reinterpret_cast<void*>(p.unit),1);
            if(mode==2){moveWeaponHook(reinterpret_cast<void*>(p.unit),nullptr,0,1);moveWeaponHook(reinterpret_cast<void*>(p.unit),nullptr,1,1);}
            refreshRanged(reinterpret_cast<void*>(p.unit),mode!=2);
        }
    }
    c->selection.stowedMask=selection.stowedMask;
    const bool extrasComplete=ensureExtras(*c);
    const bool bagComplete=ensureBackpack(*c);
    const bool instancesComplete=ensureBagInstances(*c);
    const bool complete=ensurePassthroughQuiver(*c)&&extrasComplete&&bagComplete&&instancesComplete;
    if(!keepContext){releasePassthroughQuiver(*c);*c={};}
    return result(L,complete?1:0);
}

// Bag selections and fits are private per render context. They never enter
// inventory/visible-item fields, and preview fits cannot overwrite world fits.
static bool ensureBagInstances(WeaponContext& context){
    bool complete=true;
    for(auto& bag:context.bags){
        const auto* asset=bagAsset(bag.model);
        if(!asset){releaseBagInstance(context,bag);continue;}
        const unsigned point=bagAttachment(bag.mount);
        if(!hasPoint(reinterpret_cast<void*>(context.parent),point)){complete=false;continue;}
        if(bag.child){
            std::uintptr_t owner=0;unsigned actualPoint=0;
            const auto child=reinterpret_cast<std::uintptr_t>(bag.child);
            if(!read(child+0x1CC,owner)){complete=false;continue;}
            if(owner&&owner!=context.parent){
                // The other parent owns its attachment reference. Drop only our
                // retain, then create a replacement on our own character.
                releaseBagInstance(context,bag);
            }else if(owner&&(!read(child+0x1D0,actualPoint)||actualPoint!=point)){
                detachChild(bag.child);attachChild(bag.child,reinterpret_cast<void*>(context.parent),point);
                bag.motion={};bag.response={};
            }
        }
        if(!bag.child){
            std::uintptr_t before=0,after=0;read(context.parent+0x1DC,before);
            loadingExtraParent=context.parent;
            loadChild(reinterpret_cast<void*>(context.parent),point,asset->model,asset->texture,0);
            loadingExtraParent=0;read(context.parent+0x1DC,after);
            std::uintptr_t owner=0;unsigned actualPoint=0;
            if(!after||after==before||!read(after+0x1CC,owner)||owner!=context.parent||
                !read(after+0x1D0,actualPoint)||actualPoint!=point){complete=false;continue;}
            bag.child=reinterpret_cast<void*>(after);retainChild(bag.child);
        }
        std::uintptr_t owner=0;unsigned loaded=0;
        const auto child=reinterpret_cast<std::uintptr_t>(bag.child);
        if(!read(child+0x1CC,owner)){complete=false;continue;}
        if(!owner)attachChild(bag.child,reinterpret_cast<void*>(context.parent),point);
        if((owner&&owner!=context.parent)||!read(child+0x10,loaded)||!loaded){complete=false;continue;}
        if(!weaponModelMatches(bag.child,asset->model)){releaseBagInstance(context,bag);complete=false;}
    }
    return complete;
}
static bool bagLuaUnsigned(void* L,int index,unsigned maximum,unsigned& out){
    if(!isNumber(L,index))return false;
    const double value=toNumber(L,index);
    if(!std::isfinite(value)||value<0||value>maximum||value!=static_cast<unsigned>(value))return false;
    out=static_cast<unsigned>(value);return true;
}
static int bagSelectionResult(void* L,int status,unsigned generation){
    pushNumber(L,status);pushNumber(L,generation);return 2;
}
static int __fastcall setBags(void* L){
    unsigned token=0;std::array<unsigned,8> models{},mounts{};
    if(!bagLuaUnsigned(L,1,2147483647,token)||isNumber(L,18))return result(L,-2);
    bool any=false;
    for(unsigned i=0;i<8;++i){
        if(!bagLuaUnsigned(L,2+2*i,2147483647,models[i])||
            !bagLuaUnsigned(L,3+2*i,2,mounts[i])||
            (models[i]&&!bagAsset(models[i]))||(!models[i]&&mounts[i]))return result(L,-2);
        any=any||models[i]!=0;
    }
    Player player;if(!snapshot(player))return result(L,-1);
    std::uintptr_t parent=player.model;
    if(token){
        parent=0;
        for(const auto& entry:previews.entries)if(entry.token==token&&entry.guid==player.guid&&entry.status==1)parent=entry.model;
        if(!parent)return result(L,-1);
    }else if(player.display!=player.native){
        if(auto* context=weaponContext(parent)){releaseExtras(*context);releasePassthroughQuiver(*context);
            releaseBackpack(*context);releaseBagInstances(*context);*context={};}
        return result(L,0);
    }
    unsigned loaded=0;if(!parent||!read(parent+0x10,loaded)||!loaded)return result(L,0);
    auto* context=weaponContext(parent);
    if(!context&&!any)return bagSelectionResult(L,1,0);
    if(!context){
        for(auto& entry:weaponContexts)if(!entry.parent){context=&entry;break;}
        if(!context)return result(L,-1);
        context->parent=parent;context->unit=token?0:player.unit;context->guid=player.guid;context->token=token;
        context->bagGeneration=nextBagGeneration();
    }
    if(context->guid!=player.guid||context->token!=token)return result(L,-1);
    releaseBackpack(*context);context->backBag=0;
    bool changed=false;
    for(unsigned i=0;i<8;++i){
        auto& bag=context->bags[i];
        if(bag.model!=models[i]||bag.mount!=mounts[i]){
            releaseBagInstance(*context,bag);bag={};bag.model=models[i];bag.mount=mounts[i];changed=true;
        }
    }
    if(changed)context->bagGeneration=nextBagGeneration();
    const bool complete=ensureBagInstances(*context);
    if(!any&&context->selection.empty()&&context->selection.stowedMask<0&&context->selection.carriedMode!=1&&!context->quiverHorizontal&&
        !context->hideRangedWhenStored&&!context->hideMeleeWhenStored&&!(token&&context->actualQuiver)&&!context->backBag){
        releaseExtras(*context);releasePassthroughQuiver(*context);*context={};
    }
    return bagSelectionResult(L,complete?1:0,context->bagGeneration);
}
static int __fastcall setBagInstanceFit(void* L){
    unsigned token=0,slot=0,race=0,sex=0,enabled=0;
    if(!bagLuaUnsigned(L,1,2147483647,token)||!bagLuaUnsigned(L,2,8,slot)||!slot||
        !bagLuaUnsigned(L,3,8,race)||!race||!bagLuaUnsigned(L,4,1,sex)||!bagLuaUnsigned(L,5,1,enabled))return result(L,-2);
    BagTuningValues values;
    if(enabled){
        float* fields[]={&values.left,&values.inset,&values.up,&values.pitch,&values.roll,&values.yaw,&values.scale};
        for(int i=0;i<7;++i){
            if(!isNumber(L,6+i))return result(L,-2);
            const double value=toNumber(L,6+i);
            if(!std::isfinite(value)||value<(i==2?-3:i<3?-1:i<6?-180:25)||value>(i<3?1:i<6?180:200))return result(L,-2);
            *fields[i]=static_cast<float>(value);
        }
        unsigned motion=0;if(!bagLuaUnsigned(L,13,1,motion))return result(L,-2);
        values.motion=motion==1;
        if(isNumber(L,14)){const double amplitude=toNumber(L,14);if(!std::isfinite(amplitude)||amplitude<0||amplitude>200)return result(L,-2);values.amplitude=static_cast<float>(amplitude*.01);}
    }
    Player player;if(!snapshot(player))return result(L,-1);
    const auto guid=player.guid;std::uintptr_t parent=player.model;
    if(token){
        parent=0;for(const auto& entry:previews.entries)
            if(entry.token==token&&entry.guid==guid&&entry.status==1)parent=entry.model;
    }else if(player.display!=player.native)return result(L,-1);
    auto* context=weaponContext(parent);
    if(context&&(context->guid!=guid||context->token!=token))return result(L,-1);
    if(!context)return result(L,enabled?-1:1);
    if(!context->bags[slot-1].model)return result(L,enabled?-1:1);
    auto& fit=context->bags[slot-1].fits[(race-1)*2+sex];
    if(fit.enabled==(enabled==1)&&(!enabled||fit.values==values))return result(L,1);
    fit.enabled=enabled==1;if(enabled)fit.values=values;++fit.revision;
    return result(L,1);
}
