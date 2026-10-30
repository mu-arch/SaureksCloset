// Actual combined dispatch hook, pinned client mount-kit fixtures, fake client
// memory/callbacks. Does not replace in-game audio verification.
#define main castingBaseTests
#include "casting.cpp"
#undef main
#include "../native/MountSoundsRenderer.h"
#include "mount_sound_fixtures.h"
static std::map<unsigned,weaponCasting::Visual> mountVisuals;
static unsigned* mountVisual(void*,const unsigned* row,unsigned* output){
    unsigned id=0;if(!read(reinterpret_cast<std::uintptr_t>(row),id)||!mountVisuals.count(id))return nullptr;
    std::copy(mountVisuals[id].begin(),mountVisuals[id].end(),output);return output;
}
static const MountSpellFixture& fixture(unsigned id){for(const auto& f:mountSpellFixtures)if(f.id==id)return f;assert(false);return mountSpellFixtures[0];}
static const weaponCasting::Kit& fixtureKit(unsigned id){for(const auto& k:mountKitFixtures)if(k[0]==id)return k;assert(false);return mountKitFixtures[0];}
static unsigned primary(const weaponCasting::Visual& v){for(unsigned i:{2u,3u,4u,5u})if(v[i])return i;return 0;}
int main(){
    castingBaseTests();settings(false);simulateKit=false;unitSpellVisualOriginal=mountVisual;
    for(const auto& f:mountSpellFixtures){
        const auto row=0x100000+0x1000*f.id;dbcRows[{0xC0D788,f.id}]=row;put(row,f.id);mountVisuals[f.id]=f.visual;
        for(unsigned i=0;i<3;++i){put(row+4*(61+i),f.effects[i]);put(row+4*(91+i),f.auras[i]);}
    }
    for(const auto& k:mountKitFixtures){const auto row=0x70000000+0x1000*k[0];dbcRows[{0xC0D74C,k[0]}]=row;put(row,k);put(row+52,k[13]);}
    unsigned cases=0;std::vector<std::pair<const unsigned*,weaponCasting::Kit>> retained;
    for(const auto& target:mountSoundEntries){
        Lua selection{{double(target.display)}};assert(setMountSoundLua(&selection)==1&&selection.status==1);
        const auto& selected=fixture(target.spell).visual;
        for(const auto& source:mountSpellFixtures)for(unsigned role=1;role<=5;++role){
            if(!source.visual[role])continue;
            const auto& original=fixtureKit(source.visual[role]);
            const unsigned phase=role==1?4:role==2?0:role==3?1:2;
            const auto* spell=reinterpret_cast<const unsigned*>(dbcRows[{0xC0D788,source.id}]);
            const auto* kit=reinterpret_cast<const unsigned*>(dbcRows[{0xC0D74C,original[0]}]);
            assert(mountSoundKitHook(ptr(player.unit),nullptr,spell,kit,phase,987)==77);
            assert(receivedSpell==spell&&receivedPhase==phase&&receivedArg==987);
            unsigned targetRole=role;
            if(role==primary(source.visual))targetRole=primary(selected);
            else if(role!=1&&role==primary(selected)&&primary(source.visual)!=primary(selected))targetRole=0;
            const unsigned expected=targetRole&&selected[targetRole]?fixtureKit(selected[targetRole])[13]:~0u;
            if(expected==original[13])assert(receivedKit==kit);
            else{
                assert(receivedKit!=kit&&receivedKit[13]==expected);
                for(unsigned i=0;i<35;++i)if(i!=13)assert(receivedKit[i]==original[i]);
                retained.push_back({receivedKit,[&](){auto k=original;k[13]=expected;return k;}()});
            }
            weaponCasting::Kit unchanged{};assert(read(reinterpret_cast<std::uintptr_t>(kit),unchanged)&&unchanged==original);
            // Remote players and unrelated phases retain their exact native kit.
            mountSoundKitHook(ptr(0x9999),nullptr,spell,kit,phase,987);assert(receivedKit==kit);
            mountSoundKitHook(ptr(player.unit),nullptr,spell,kit,5,987);assert(receivedKit==kit);
            ++cases;
        }
    }
    // Known independent summon sounds: horse -> wolf/ram/mechanostrider/kodo.
    const auto horse=reinterpret_cast<const unsigned*>(dbcRows[{0xC0D788,458}]);
    const auto horseKit=reinterpret_cast<const unsigned*>(dbcRows[{0xC0D74C,1604}]);
    for(auto pair:std::array<std::pair<unsigned,unsigned>,4>{{{207,4069},{2785,4068},{14372,4070},{11641,7814}}}){
        Lua choice{{double(pair.first)}};setMountSoundLua(&choice);
        mountSoundKitHook(ptr(player.unit),nullptr,horse,horseKit,0,987);assert(receivedKit[13]==pair.second);
    }
    const auto before=mountSoundKits.size();Lua clear{{0}};setMountSoundLua(&clear);
    mountSoundKitHook(ptr(player.unit),nullptr,horse,horseKit,0,987);assert(receivedKit==horseKit&&mountSoundKits.size()==before);
    for(const auto& r:retained)assert(std::equal(r.second.begin(),r.second.end(),r.first)); // no eviction on selection/disable
    for(double value:{-1.,.5,999999.,std::numeric_limits<double>::infinity()}){Lua bad{{value}};setMountSoundLua(&bad);assert(bad.status==-2&&!mountSoundDisplay);}
    Lua wolf{{207}};setMountSoundLua(&wolf);
    const auto pass=[&](){mountSoundKitHook(ptr(player.unit),nullptr,horse,horseKit,0,987);assert(receivedKit==horseKit);};
    ++player.guid;pass();--player.guid;player.display=1;pass();player.display=player.native;snapshotOK=false;pass();snapshotOK=true;
    const auto source=dbcRows[{0xC0D788,458}];put(source+4*91,0u);pass();put(source+4*91,78u);
    const auto target=dbcRows[{0xC0D788,578}];dbcRows.erase({0xC0D788,578});pass();dbcRows[{0xC0D788,578}]=target;
    const auto targetKit=dbcRows[{0xC0D74C,1610}];memory.erase(targetKit+52);pass();put(targetKit+52,4069u);
    // A nonmount cast keeps the existing weapon-casting feature, independently.
    settings();unitSpellVisualOriginal=originalVisual;
    put(0x8000,133u);sourceVisual={{67,30,38,286,0,0}};
    weaponCasting::Kit ordinary{};ordinary[0]=30;ordinary[2]=51;
    put(0x9000,ordinary);dbcRows[{0xC0D788,133}]=0x8000;dbcRows[{0xC0D74C,30}]=0x9000;
    mountSoundKitHook(ptr(player.unit),nullptr,reinterpret_cast<const unsigned*>(0x8000),reinterpret_cast<const unsigned*>(0x9000),4,987);
    assert(receivedKit[2]==28);
    std::cout<<"PASS: "<<cases<<" native mount sound dispatches across all 109 selections/126 mount spells, precast/arrival/impact/aura sounds, exact field preservation, missing data, owner isolation, disable, retained kit lifetime and weapon-casting coexistence\n";
}
