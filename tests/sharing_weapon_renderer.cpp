#define SAUREKS_SHARING_TEST
#define main existingWeaponRegression
#include "weapon_renderer.cpp"
#undef main
static void* forceRefresh=nullptr;
static unsigned sharedRefreshes=0;
static void updateDisplay(void*){++sharedRefreshes;}
static void rebuildComponent(void*){++sharedRefreshes;}
static void* visibleItemOriginal(void*,int slot){const auto base=0x1200000+48*slot;for(unsigned i=0;i<12;++i)memory[base+4*i]=0;memory[base+8]=slot>=15&&slot<=17?realIDs[slot-15]:0;return pointer(base);}
#include "../native/SharingWorld.h"
int main(){existingWeaponRegression();
    Player p{0x9000,0x8000,456,49,49,0xa000,0xb000};sharedObjects[p.guid]=p.unit;
    memory[p.unit+0x14]=4;memory[p.unit+8]=p.fields;memory[p.fields]=p.guid;memory[p.fields+0x83*4]=49;memory[p.fields+0x84*4]=49;memory[p.fields+0x24*4]=1;memory[p.fields+0xc1*4]=0;memory[p.fields+0xc2*4]=0;memory[p.unit+0xd8]=p.model;memory[p.unit+0xd30]=p.component;memory[p.unit+0xd40]=0;memory[p.model+0x10]=1;
    unsigned bow=0;for(const auto& asset:weaponAssets)if(asset.kind==4&&asset.subclass==2){bow=asset.item;break;}
    realIDs[0]=25;realIDs[1]=0;realIDs[2]=bow;
    sharing::Look look;look.items[15]=25;look.items[17]=bow;look.stowed=5;look.flags=1;look.carried[2]=35;
    look.fitMask=128;look.fits[7]={0,0,0,0,0,0,12500};
    look.bags[0]={12,2,4,{1000,0,-1000,0,0,0,7000}};
    assert(sharedValidLook(look));std::map<std::uint64_t,sharing::Remote> incoming{{p.guid,{1,look}}};
    const auto owner=bagTuningOwner;const auto localFits=weaponTuningEntries[7][0].values;
    sharedApply(incoming);auto* c=weaponContext(p.model);assert(c&&weaponContextActive(*c)&&c->guid!=getPlayer());assert(c->extra[2]&&c->bags[4].child);assert(c->sharedFits[7].values.scale==125);assert(c->bags[4].fits[0].values.scale==70);assert(weaponTuningEntries[7][0].values==localFits&&bagTuningOwner==owner);
    auto child=c->bags[4].child;const auto beforeLoads=loads,beforeRefreshes=sharedRefreshes;sharedApply(incoming);assert(loads==beforeLoads&&sharedRefreshes==beforeRefreshes&&c->bags[4].child==child);
    detach(child);releaseModel(child);assert(!c->bags[4].child&&refs[address(child)]==0);sharedApply(incoming);child=c->bags[4].child;assert(child&&refs[address(child)]==2);
    auto rangedChild=c->nativeChildren[2];assert(rangedChild&&!hideStoredWeapon(rangedChild));incoming[p.guid].look.stowed=1;sharedApply(incoming);rangedChild=c->nativeChildren[2];assert(rangedChild&&hideStoredWeapon(rangedChild));
    incoming[p.guid].look.bags[0]={};incoming[p.guid].look.carried[2]=0;sharedApply(incoming);assert(!c->bags[4].child&&!c->extra[2]&&refs[address(child)]==0);
    assert(bagTuningOwner==owner&&weaponTuningEntries[7][0].values==localFits);sharedClearAll();assert(sharedAppearances.empty()&&sharedWeaponContexts.empty()&&!weaponContext(p.model));
    auto invalid=look;invalid.bags[0].model=999;assert(!sharedValidLook(invalid));invalid=look;invalid.fits[7][6]=0;assert(!sharedValidLook(invalid));
    std::cout<<"PASS: complete remote weapon/bag context, independent fits, idempotent updates, stowed eyes, attachment removal, disconnect restore and local-profile isolation\n";
}
