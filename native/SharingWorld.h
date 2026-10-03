#pragma once
static void sharedClear(std::uint64_t guid){
    auto it=sharedAppearances.find(guid);if(it==sharedAppearances.end())return;
    Player p;const bool live=sharedPlayer(guid,p);const bool body=it->second.snapshot.look.body[0];
    sharedModelOwners.erase(it->second.model);sharedAppearances.erase(it);
    auto c=sharedWeaponContexts.find(guid);
    if(c!=sharedWeaponContexts.end()){releaseExtras(c->second);releasePassthroughQuiver(c->second);releaseBackpack(c->second);releaseBagInstances(c->second);sharedWeaponContexts.erase(c);}
    if(live&&p.display==p.native){if(body){forceRefresh=reinterpret_cast<void*>(p.unit);updateDisplay(forceRefresh);forceRefresh=nullptr;}else rebuildComponent(reinterpret_cast<void*>(p.unit));
        refreshMelee(reinterpret_cast<void*>(p.unit),0);refreshMelee(reinterpret_cast<void*>(p.unit),1);unsigned mode=0;read(p.unit+0xd40,mode);refreshRanged(reinterpret_cast<void*>(p.unit),mode!=2);
    }
}
static void sharedClearAll(){while(!sharedAppearances.empty())sharedClear(sharedAppearances.begin()->first);}
static sharing::Fit sharedPackFit(const BagTuningValues& v){
    const float fields[]={v.left,v.inset,v.up,v.pitch,v.roll,v.yaw,v.scale};sharing::Fit out{};
    for(unsigned i=0;i<7;++i)out[i]=static_cast<std::int16_t>(std::lround(fields[i]*(i<3?10000.f:100.f)));
    return out;
}
static BagTuningValues sharedUnpackFit(const sharing::Fit& f){return {f[0]/10000.f,f[1]/10000.f,f[2]/10000.f,f[3]/100.f,f[4]/100.f,f[5]/100.f,f[6]/100.f,true};}
static void sharedCapture(sharing::Look& look){
    Player p;if(!snapshot(p))return;look.flags=(weaponPhysicsEnabled&&weaponPhysicsOwner==p.guid)?4u:0u;const auto* c=weaponContext(p.model);if(!c||c->guid!=p.guid||c->token)return;
    const auto* native=nativeModel(p.native);if(!native)return;
    const unsigned race=look.body[0]?look.body[1]:native->race,sex=look.body[0]?look.body[2]:native->sex,index=(race-1)*2+sex;
    look.flags|=(c->selection.carriedMode==1?1u:0u)|(c->quiverHorizontal?2u:0u);
    if(look.flags&1)for(unsigned i=0;i<7;++i)look.carried[i]=c->selection.items[i];
    for(unsigned i=0;i<10;++i){const auto& fit=weaponTuningEntries[i][index];if(fit.enabled){look.fitMask|=1u<<i;look.fits[i]=sharedPackFit(fit.values);}}
    unsigned count=0;
    for(unsigned i=0;i<c->bags.size()&&count<5;++i){const auto& bag=c->bags[i];if(!bag.model)continue;
        BagTuningValues fit;if(bag.fits[index].enabled)fit=bag.fits[index].values;else if(!bagInstanceTuningDefaults(bag.mount,race,sex,fit))continue;
        look.bags[count++]={bag.model,bag.mount,i,sharedPackFit(fit),static_cast<unsigned>(std::lround(fit.amplitude*100)),fit.motion};
    }
}
static bool sharedValidLook(const sharing::Look& look){
    if(look.body[0]&&!sharedBody(look).valid())return false;
    for(unsigned i=0;i<7;++i)if(look.carried[i]&&!acceptsWeapon(i,weaponAsset(look.carried[i])))return false;
    for(unsigned i=0;i<3;++i){const auto id=look.items[15+i];if(id!=sharing::inherit&&id&&!acceptsWeapon(7+i,weaponAsset(id)))return false;}
    for(unsigned i=0;i<10;++i)if((look.fitMask&(1u<<i))&&!bagTuningValid(sharedUnpackFit(look.fits[i])))return false;
    for(const auto& bag:look.bags)if(bag.model&&(!bagAsset(bag.model)||!bagTuningValid(sharedUnpackFit(bag.fit),true)))return false;
    return true;
}
static void sharedWeapons(const Player& p,const sharing::Look& look){
    auto& c=sharedWeaponContexts[p.guid];
    WeaponSelection next;next.independent=true;next.carriedMode=(look.flags&1)?1:0;next.stowedMask=look.stowed;
    for(unsigned i=0;i<7;++i)next.items[i]=next.carriedMode?look.carried[i]:0;
    for(unsigned role=0;role<3;++role){std::array<std::uint32_t,12> item{};auto* real=visibleItemOriginal(reinterpret_cast<void*>(p.unit),15+role);if(real&&read(reinterpret_cast<std::uintptr_t>(real),item))next.equipped[role]=item[2];
        const auto choice=look.items[15+role];if(choice!=sharing::inherit&&choice)next.items[7+role]=choice;
    }
    const bool changed=c.parent!=p.model||!(c.selection==next)||c.selection.stowedMask!=next.stowedMask;
    if(c.parent!=p.model)c={};
    if(changed){
        releaseExtras(c);
        if(c.parent==p.model){
            for(auto child:c.nativeChildren)if(child){std::uintptr_t owner=0;unsigned point=0;
                if(read(reinterpret_cast<std::uintptr_t>(child)+0x1cc,owner)&&owner==p.model&&read(reinterpret_cast<std::uintptr_t>(child)+0x1d0,point)&&point>=26&&point<=33)detachChild(child);}
            for(unsigned role=0;role<3;++role){
                const int home=selectedWeaponHome(c.selection,role,c.routes[role]);if(home>=0)clearChildrenHook(reinterpret_cast<void*>(p.model),nullptr,static_cast<unsigned>(home));
                for(const auto* selection:{&c.selection,&next})if(const auto* asset=weaponAsset(selection->equipped[role])){
                    const unsigned side=role==0||(role==2&&(asset->inventory==25||asset->inventory==26));const int point=sheathPointOriginal(asset->sheath,side);
                    if(point>=0)clearChildrenHook(reinterpret_cast<void*>(p.model),nullptr,static_cast<unsigned>(point));
                }
            }
        }
    }
    if(c.sharedWeaponPhysics!=bool(look.flags&4))c.rigidWeapons={};
    c.sharedWeaponPhysics=(look.flags&4)!=0;
    c.parent=p.model;c.unit=p.unit;c.guid=p.guid;c.selection=next;c.routes=next.routes();c.quiverHorizontal=(look.flags&2)!=0;
    for(unsigned i=0;i<10;++i){auto& fit=c.sharedFits[i];fit.enabled=(look.fitMask&(1u<<i))!=0;if(fit.enabled)fit.values=sharedUnpackFit(look.fits[i]);}
    for(unsigned i=0;i<c.bags.size();++i){const sharing::Bag* source=nullptr;for(const auto& bag:look.bags)if(bag.model&&bag.slot==i)source=&bag;
        auto& bag=c.bags[i];if(!source){if(bag.model){releaseBagInstance(c,bag);bag={};}continue;}
        if(bag.model!=source->model||bag.mount!=source->mount){releaseBagInstance(c,bag);bag={};bag.model=source->model;bag.mount=source->mount;}
        auto values=sharedUnpackFit(source->fit);values.amplitude=source->amplitude*.01f;values.motion=source->physics;
        for(auto& fit:bag.fits){if(!fit.enabled||!(fit.values==values)){fit.enabled=true;fit.values=values;++fit.revision;}}
    }
    if(changed){refreshMelee(reinterpret_cast<void*>(p.unit),0);refreshMelee(reinterpret_cast<void*>(p.unit),1);unsigned mode=0;read(p.unit+0xd40,mode);refreshRanged(reinterpret_cast<void*>(p.unit),mode!=2);}
    ensureExtras(c);ensureBagInstances(c);
}
static void sharedApply(const std::map<std::uint64_t,sharing::Remote>& incoming){
    for(auto i=sharedAppearances.begin();i!=sharedAppearances.end();){const auto guid=i->first;++i;if(!incoming.count(guid))sharedClear(guid);}
    unsigned budget=4;
    for(const auto& pair:incoming){
        Player p;if(!sharedPlayer(pair.first,p)||!p.model||!p.component)continue;
        const auto& look=pair.second.look;if(!sharedValidLook(look)){sharedClear(pair.first);continue;}
        auto i=sharedAppearances.find(pair.first);bool changed=i==sharedAppearances.end()||!(i->second.snapshot.look==look);
        bool newModel=i!=sharedAppearances.end()&&i->second.model!=p.model;
        if(changed||newModel){if(!budget)continue;--budget;
            bool bodyChanged=look.body[0]!=0;
            if(i!=sharedAppearances.end())bodyChanged=i->second.snapshot.look.body!=look.body;
            auto& entry=sharedAppearances[p.guid];entry.snapshot=pair.second;sharedBind(p.guid,entry,p);
            if(p.display==p.native&&nativeModel(p.native)){
                if(bodyChanged||newModel){forceRefresh=reinterpret_cast<void*>(p.unit);updateDisplay(forceRefresh);forceRefresh=nullptr;}
                else rebuildComponent(reinterpret_cast<void*>(p.unit));
                if(!sharedPlayer(pair.first,p))continue;
                sharedBind(p.guid,entry,p);
            }
        }
        if(p.display==p.native&&nativeModel(p.native))sharedWeapons(p,look);
    }
}
