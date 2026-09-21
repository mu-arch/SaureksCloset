#pragma once
#include "WeaponState.h"
#include "BowPlacement.h"
#include "BagPlacement.h"
#include "PlacementTuning.h"
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
#endif
static const auto retainChild=weaponFunction<DestroyModel>(0x710390);
static const auto detachChild=weaponFunction<DestroyModel>(0x713020);
static const auto attachChild=weaponFunction<AttachChild>(0x712F70);
static const auto hasPoint=weaponFunction<HasPoint>(0x712CB0);
static const auto loadChild=weaponFunction<LoadChild>(0x4798C0);
static const auto refreshMelee=weaponFunction<void (__thiscall *)(void*,unsigned)>(0x605DA0);
static const auto refreshRanged=weaponFunction<void (__thiscall *)(void*,unsigned)>(0x611E10);
struct WeaponContext {
    std::uintptr_t parent=0,unit=0;std::uint64_t guid=0;unsigned token=0;
    WeaponSelection selection;
    bool quiverHorizontal=false,hideRangedWhenStored=false,hideMeleeWhenStored=false;
    unsigned actualQuiver=0;
    void* passthroughQuiver=nullptr;
    unsigned backBag=0;
    void* backpack=nullptr;
    BagMotion bagMotion;
    std::array<int,3> routes{{-1,-1,-1}};
    std::array<void*,7> extra{};
    std::array<unsigned char,8> rangedInfo{};
};
static std::array<WeaponContext,9> weaponContexts{};
static int scopedSheathPoint=-1;
static std::uintptr_t loadingExtraParent=0;
static WeaponContext* weaponContext(std::uintptr_t model){
    for(auto& c:weaponContexts)if(model&&c.parent==model)return &c;
    return nullptr;
}
static bool ownedExtra(const WeaponContext& c,void* child){
    if(child&&c.backpack==child)return true;
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
static bool bagIsAirborne(const WeaponContext& context){
    // Build 5875 starts jump/drop motion with 0x2000 at 7C620B; extended
    // falling adds 0x4000 at 633240. Landing clears both at 7C629B.
    // 7C61D6 excludes nonground modes from natural fall start. Do not reject
    // walking, transport or feather-fall: these can still be real jumps.
    unsigned flags=0;
    constexpr unsigned nonBallistic=0x400|0x800|0x200000|0x800000|0x1000000|0x8000000;
    return !context.token&&context.unit&&read(context.unit+0x9E8,flags)&&(flags&0x6000)&&!(flags&nonBallistic);
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
static bool bagTuningLuaKey(void* L,unsigned& bag,unsigned& race,unsigned& sex){
    unsigned values[3]{};
    for(int i=0;i<3;++i){
        if(!isNumber(L,i+1))return false;
        const double value=toNumber(L,i+1);
        if(!std::isfinite(value)||value<0||value>107||value!=static_cast<unsigned>(value))return false;
        values[i]=static_cast<unsigned>(value);
    }
    bag=values[0];race=values[1];sex=values[2];
    return bagTuningKey(bag,race,sex);
}
static int __fastcall getBagFitDefaults(void* L){
    unsigned bag=0,race=0,sex=0;BagTuningValues values;
    if(!bagTuningLuaKey(L,bag,race,sex)||!bagTuningDefaults(bag,race,sex,values))return result(L,-2);
    pushNumber(L,1);
    for(float value:{values.left,values.inset,values.up,values.pitch,values.roll,values.yaw,values.scale})pushNumber(L,value);
    return 8;
}
static int __fastcall setBagFit(void* L){
    unsigned bag=0,race=0,sex=0;
    if(!bagTuningLuaKey(L,bag,race,sex)||!isNumber(L,4))return result(L,-2);
    const double enabled=toNumber(L,4);
    if(enabled!=0&&enabled!=1)return result(L,-2);
    BagTuningValues values;
    if(enabled==1){
        float* fields[]={&values.left,&values.inset,&values.up,&values.pitch,&values.roll,&values.yaw,&values.scale};
        for(int i=0;i<7;++i){
            if(!isNumber(L,i+5))return result(L,-2);
            const double value=toNumber(L,i+5);
            const double minimum=i<3?-1:(i<6?-180:25),maximum=i<3?1:(i<6?180:200);
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
    if(!read(model+0x1CC,parent)||!read(model+0x1D0,point)||point!=28)return false;
    auto* context=weaponContext(parent);
    if(!context||context->guid!=getPlayer()||context->backpack!=child||context->backBag!=1)return false;
    bagTuningUseOwner(context->guid);
    std::array<float,16> back,torso,local,modelToRender;std::array<float,3> position;
    if(!animatedAttachmentMatrix(parent,28,back,&torso)||!read(model+0xBC,local)||!read(parent+0xFC,modelToRender)||
       !read(parent+0x30,data)||!read(data+0x130,header)||!read(header+0x110,lookup)||
       !read(lookup+56,index)||!read(header+0x108,attachments)||!read(attachments+48*index+8,position)){
        context->bagMotion={};return false;
    }
    // 7076BE copies the graphics view to scene+0x9C; 714389 composes it
    // through root/attachment placement into model+0xFC. Cancel that shared
    // basis to get world vertical even on a mount, independent of the camera.
    // If the scene is between updates, retain a rigid visible bag until ready.
    BagMatrix worldToRender;std::uintptr_t scene=0;
    if(smooth&&(!read(parent+0x2C,scene)||!scene||!read(scene+0x9C,worldToRender))){
        context->bagMotion={};smooth=false;
    }
    const bool valid=bagPlacement(back,torso,local,position,adjusted,context->backBag,
        smooth?&context->bagMotion:nullptr,smooth?bagClockMilliseconds():0,header,bagIsRunning(*context),&modelToRender,smooth?&worldToRender:nullptr,bagAirLiftTarget(*context));
    if(!valid)context->bagMotion={};
    return valid;
}
static bool positionStoredBow(void* child,const float* attachment,std::array<float,16>& adjusted){
    std::uintptr_t parent=0;unsigned point=0;
    const auto model=reinterpret_cast<std::uintptr_t>(child);
    if(!read(model+0x1CC,parent)||!read(model+0x1D0,point)||point!=weaponPoints[5])return false;
    const auto* c=weaponContext(parent);
    if(!c||c->guid!=getPlayer())return false;
    const auto* asset=weaponAsset(c->selection.items[5]);
    if(!asset||asset->kind!=4||asset->subclass!=2)return false;
    if(c->extra[5]!=child&&(c->token||c->routes[2]!=5||findChildOriginal(reinterpret_cast<void*>(parent),point)!=child))return false;
    std::array<float,3> back;
    if(!animatedBackPosition(parent,back)||!read(reinterpret_cast<std::uintptr_t>(attachment),adjusted))return false;
    // Keep the ranged sheath's animated orientation, but center the bow's grip
    // on this model's authored back anchor. Logical point 27 stays independent
    // of shields at 28, preserving ownership and native draw/sheath callbacks.
    for(unsigned axis=0;axis<3;++axis)adjusted[12+axis]=back[axis];
    return true;
}
static bool positionStoredBackWeapon(void* child,std::array<float,16>& adjusted){
    std::uintptr_t parent=0;unsigned point=0;
    const auto model=reinterpret_cast<std::uintptr_t>(child);
    if(!read(model+0x1CC,parent)||!read(model+0x1D0,point)||(point!=30&&point!=31))return false;
    const auto* c=weaponContext(parent);
    if(!c||c->guid!=getPlayer())return false;
    const unsigned position=point==30?2:3;
    const auto* asset=weaponAsset(c->selection.items[position]);
    if(!asset||asset->kind!=2||asset->sheath!=1)return false;
    if(c->extra[position]!=child){
        if(c->token)return false;
        bool routed=false;for(auto route:c->routes)if(route==static_cast<int>(position))routed=true;
        if(!routed||findChildOriginal(reinterpret_cast<void*>(parent),point)!=child)return false;
    }
    // Points 30/31 have a staff-style pose. Keep those independent logical
    // homes, but draw type-1 two-handers using the model's sword pose at 26/27.
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
    if(!c||c->guid!=getPlayer())return false;
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
    if(!c||c->guid!=getPlayer()||ownedExtra(*c,child))return false;
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
static bool hideStoredWeapon(void* child){
    const auto model=reinterpret_cast<std::uintptr_t>(child);
    std::uintptr_t parent=0;unsigned point=0;
    if(!read(model+0x1CC,parent)||!read(model+0x1D0,point)||point<26||point>33)return false;
    const auto* c=weaponContext(parent);
    if(!c||c->guid!=getPlayer())return false;
    const auto hidden=[c](const WeaponAsset* asset){
        return asset&&((asset->kind==4&&c->hideRangedWhenStored)||
            ((asset->kind==1||asset->kind==2)&&c->hideMeleeWhenStored));
    };
    for(unsigned i=0;i<c->extra.size();++i)if(c->extra[i]==child)
        return point==weaponPoints[i]&&hidden(weaponAsset(c->selection.items[i]));
    if(c->passthroughQuiver==child)return false;
    // A native child's point alone is ambiguous. Match its selected/equipped
    // model as well, and leave hands (0/1/2), shields, quivers and unknown props.
    for(unsigned role=0;role<3;++role){
        const int route=c->routes[role];
        const auto* asset=weaponAsset(route>=0?c->selection.items[route]:c->selection.equipped[role]);
        if(!hidden(asset))continue;
        const unsigned side=role==0||(role==2&&(asset->inventory==25||asset->inventory==26));
        const int home=route>=0?static_cast<int>(weaponPoints[route]):sheathPointOriginal(asset->sheath,side);
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
static bool tuneStoredPlacement(void* child,const BagMatrix& base, std::array<float,16>& out){
    const auto model=reinterpret_cast<std::uintptr_t>(child);
    std::uintptr_t parent=0,resource=0,header=0,lookup=0,records=0;
    unsigned point=0;std::uint16_t index=0;
    if(!read(model+0x1CC,parent)||!read(model+0x1D0,point))return false;
    const auto* c=weaponContext(parent);
    if(!c||c->guid!=getPlayer())return false;
    int slot=-1;
    for(unsigned i=0;i<7;++i)if(point==weaponPoints[i]&&c->selection.items[i]){
        if(c->extra[i]==child)slot=i;
        else if(!c->token&&findChildOriginal(reinterpret_cast<void*>(parent),point)==child)
            for(auto route:c->routes)if(route==static_cast<int>(i))slot=i;
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
        const auto& fit=weaponTuningEntries[slot][i];
        return fit.enabled&&
            placementTuning(base,local,torso,render,fit.values,out);
    }
    return false;
}
static void updateWeaponAttachment(void* model,const float* matrix,const float* color,const float* lighting,float alpha){
    std::array<float,16> adjusted;
    std::uintptr_t parent=0;read(reinterpret_cast<std::uintptr_t>(model)+0x1CC,parent);
    const auto* context=weaponContext(parent);
    if(context&&context->backpack==model){
        // Wait for valid animated bones instead of briefly drawing a shield pose.
        if(positionBackpack(model,adjusted,true))updateAttachedOriginal(model,adjusted.data(),color,lighting,alpha);
        else updateAttachedOriginal(model,matrix,color,lighting,0);
        return;
    }
    // Zero effective alpha skips the native mesh draw, while its normal update
    // still runs. No native ownership or visibility state is changed; removing
    // the custom quiver restores the original alpha on the next frame.
    if(hideNativeQuiver(model)||hideStoredWeapon(model))alpha=0;
    const bool positioned=positionStoredBackWeapon(model,adjusted)||positionStoredBow(model,matrix,adjusted)||
       positionStoredQuiver(model,matrix,adjusted);
    if(positioned)matrix=adjusted.data();
    std::array<float,16> tuned,base;
    if(positioned)base=adjusted;
    if((positioned||read(reinterpret_cast<std::uintptr_t>(matrix),base))&&tuneStoredPlacement(model,base,tuned))matrix=tuned.data();
    updateAttachedOriginal(model,matrix,color,lighting,alpha);
}
static void __fastcall updateAttachedHook(void* model,void*,const float* matrix,const float* color,const float* lighting,float alpha){
    // Exact recursive child update after the animated attachment is resolved.
    if(reinterpret_cast<std::uintptr_t>(__builtin_return_address(0))==0x718761)
        updateWeaponAttachment(model,matrix,color,lighting,alpha);
    else updateAttachedOriginal(model,matrix,color,lighting,alpha);
}
static void releaseExtras(WeaponContext& c){
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
static void forgetWeapons(std::uintptr_t model){
    if(auto* c=weaponContext(model)){releaseExtras(*c);releasePassthroughQuiver(*c);releaseBackpack(*c);*c={};}
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
        caller==0x611BCE||caller==0x60BAA4||caller==0x624B35||caller==0x5DEF00;
    if(!original||role!=2||raw||!visual)return original;
    Player p;
    if(!snapshot(p)||p.unit!=reinterpret_cast<std::uintptr_t>(unit)||p.display!=p.native)return original;
    auto* c=weaponContext(p.model);
    if(!c||c->token||c->guid!=p.guid||c->unit!=p.unit||c->routes[2]!=5)return original;
    const auto* asset=weaponAsset(c->selection.items[5]);
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
    if(!snapshot(p)||p.unit!=reinterpret_cast<std::uintptr_t>(unit)||p.display!=p.native)return animation;
    auto* c=weaponContext(p.model);
    if(!c||c->token||c->guid!=p.guid||c->unit!=p.unit||c->routes[2]!=5)return animation;
    const auto* selected=weaponAsset(c->selection.items[5]);
    const auto* equipped=weaponAsset(c->selection.equipped[2]);
    if(!selected||!equipped||selected->kind!=4||equipped->kind!=4||!weaponDisplay(selected))return animation;
    return rangedAppearanceAnimation(animation,equipped->subclass,selected->subclass);
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
    if(c&&!c->token&&c->guid==getPlayer()&&slot>=15&&slot<=17){
        const int route=c->routes[slot-15];
        if(route>=0){
            const auto* a=weaponAsset(c->selection.items[route]);
            if(void* row=weaponDisplay(a)){display=row;scopedSheathPoint=weaponPoints[route];sheath=a->sheath;}
        }
    }
    const int result=weaponComposeOriginal(parent,display,slot,sheath,stored,shield,rangedRight);
    scopedSheathPoint=old;return result;
}
static void __fastcall moveWeaponHook(void* unit,void*,unsigned role,unsigned stored){
    std::uintptr_t parent=0;read(reinterpret_cast<std::uintptr_t>(unit)+0xD8,parent);
    auto* c=weaponContext(parent);const int old=scopedSheathPoint;
    // A ranged move can rebuild melee weapons recursively. Each role must
    // choose its own home instead of inheriting the outer ranged override.
    scopedSheathPoint=-1;
    if(c&&!c->token&&c->unit==reinterpret_cast<std::uintptr_t>(unit)&&c->guid==getPlayer()&&role<3&&c->routes[role]>=0)
        scopedSheathPoint=weaponPoints[c->routes[role]];
    moveWeaponOriginal(unit,role,stored);scopedSheathPoint=old;
}
static void __fastcall rebuildWeaponHook(void* unit,void*,unsigned role){
    std::uintptr_t parent=0;read(reinterpret_cast<std::uintptr_t>(unit)+0xD8,parent);
    auto* c=weaponContext(parent);const int old=scopedSheathPoint;
    scopedSheathPoint=-1;
    if(c&&!c->token&&c->unit==reinterpret_cast<std::uintptr_t>(unit)&&c->guid==getPlayer()&&role<3&&c->routes[role]>=0)
        scopedSheathPoint=weaponPoints[c->routes[role]];
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
    if(previous!=2||!read(address+0xD40,mode)||mode!=0||!snapshot(p)||
       p.unit!=address||p.display!=p.native)return;
    const auto* c=weaponContext(p.model);
    if(!c||c->token||c->unit!=address||c->guid!=p.guid||c->guid!=getPlayer())return;
    const int route=c->routes[2];
    if(route<0||route>=static_cast<int>(weaponPoints.size()))return;
    const auto* asset=weaponAsset(c->selection.items[route]);
    unsigned loaded=0;
    if(!asset||asset->kind!=4||!read(p.model+0x10,loaded)||!loaded||
       !hasPoint(reinterpret_cast<void*>(p.model),weaponPoints[route])||
       findChildHook(reinterpret_cast<void*>(p.model),nullptr,weaponPoints[route]))return;
    // The ranged refresh releases its old held-model reference and retains the
    // new stored child. The generic role-2 rebuild does nothing while unarmed.
    refreshRanged(unit,1);
}
static bool ensureExtras(WeaponContext& c){
    bool complete=true;
    for(unsigned i=0;i<7;++i){
        const auto* a=weaponAsset(c.selection.items[i]);if(!a)continue;
        if(!hasPoint(reinterpret_cast<void*>(c.parent),weaponPoints[i])){complete=false;continue;}
        bool routed=false;for(auto route:c.routes)if(route==static_cast<int>(i))routed=true;
        if(!c.token&&routed)continue;
        if(c.extra[i]){
            std::uintptr_t parent=0;
            if(read(reinterpret_cast<std::uintptr_t>(c.extra[i])+0x1CC,parent)&&!parent)
                attachChild(c.extra[i],reinterpret_cast<void*>(c.parent),weaponPoints[i]);
            continue;
        }
        // Keep preexisting children, then identify the factory's newly attached
        // child by its parent-list head. Our own reference survives stock clears.
        std::uintptr_t before=0,after=0;read(c.parent+0x1DC,before);
        loadingExtraParent=c.parent;
        loadChild(reinterpret_cast<void*>(c.parent),weaponPoints[i],a->model,a->texture,0);
        loadingExtraParent=0;read(c.parent+0x1DC,after);
        unsigned point=0;std::uintptr_t parent=0;
        if(after&&after!=before&&read(after+0x1D0,point)&&point==weaponPoints[i]&&read(after+0x1CC,parent)&&parent==c.parent){
            c.extra[i]=reinterpret_cast<void*>(after);retainChild(c.extra[i]);
        }else complete=false;
    }
    return complete;
}
static bool ensurePassthroughQuiver(WeaponContext& c){
    const auto* asset=weaponAsset(c.actualQuiver);
    if(!c.token||c.selection.items[6]||!asset){releasePassthroughQuiver(c);return true;}
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
    if(!parent)attachChild(c.backpack,reinterpret_cast<void*>(c.parent),28);
    if((parent&&parent!=c.parent)||!read(reinterpret_cast<std::uintptr_t>(c.backpack)+0x10,loaded)||!loaded)return false;
    if(!weaponModelMatches(c.backpack,"Interface\\AddOns\\SaureksCloset\\Models\\DarkSchoolbag.mdx")){
        releaseBackpack(c);return false;
    }
    return true;
}
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
    if(!selection.valid())return result(L,-2);
    Player p;if(!snapshot(p))return result(L,-1);
    std::uintptr_t parent=p.model;const unsigned token=values[0];
    if(token){
        parent=0;
        for(auto& e:previews.entries)if(e.token==token&&e.guid==p.guid&&e.status==1)parent=e.model;
        if(!parent)return result(L,-1);
    }else if(p.display!=p.native){
        if(auto* c=weaponContext(parent)){releaseExtras(*c);releasePassthroughQuiver(*c);releaseBackpack(*c);*c={};}
        return result(L,0);
    }
    unsigned loaded=0;if(!parent||!read(parent+0x10,loaded)||!loaded)return result(L,0);
    auto* c=weaponContext(parent);
    const bool keepContext=!selection.empty()||options[0]||options[1]||options[2]||(token&&actualQuiver)||backBag;
    if(!c&&!keepContext)return result(L,1);
    if(!c){
        for(auto& entry:weaponContexts)if(!entry.parent){c=&entry;break;}
        if(!c)return result(L,-1);
        c->parent=parent;c->unit=token?0:p.unit;c->guid=p.guid;c->token=token;
    }
    if(c->guid!=p.guid||c->token!=token)return result(L,-1);
    if(c->backBag!=backBag){releaseBackpack(*c);c->backBag=backBag;}
    // Visual options alone must not destroy or rebuild any weapon children.
    c->quiverHorizontal=options[0];c->hideRangedWhenStored=options[1];c->hideMeleeWhenStored=options[2];
    if(c->actualQuiver!=actualQuiver){releasePassthroughQuiver(*c);c->actualQuiver=actualQuiver;}
    // An option-only context still records actual equipment for mesh matching,
    // without rebuilding stock weapons just because those IDs were first read.
    if(c->selection.empty()&&selection.empty())c->selection=selection;
    if(!(c->selection==selection)){
        releaseExtras(*c);
        // Remove the old routed weapons at their old homes before rerouting.
        if(!token)for(auto route:c->routes)if(route>=0)clearChildrenHook(reinterpret_cast<void*>(parent),nullptr,weaponPoints[route]);
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
        for(auto& route:c->routes)if(route>=0&&(!weaponDisplay(weaponAsset(selection.items[route]))||!hasPoint(reinterpret_cast<void*>(parent),weaponPoints[route])))route=-1;
        if(!token){
            unsigned mode=0;read(p.unit+0xD40,mode);
            refreshMelee(reinterpret_cast<void*>(p.unit),0);refreshMelee(reinterpret_cast<void*>(p.unit),1);
            if(mode==2){moveWeaponHook(reinterpret_cast<void*>(p.unit),nullptr,0,1);moveWeaponHook(reinterpret_cast<void*>(p.unit),nullptr,1,1);}
            refreshRanged(reinterpret_cast<void*>(p.unit),mode!=2);
        }
    }
    const bool extrasComplete=ensureExtras(*c);
    const bool bagComplete=ensureBackpack(*c);
    const bool complete=ensurePassthroughQuiver(*c)&&extrasComplete&&bagComplete;
    if(!keepContext){releasePassthroughQuiver(*c);*c={};}
    return result(L,complete?1:0);
}
