// Exercise actual ranged visual preparation and missile hooks against sparse,
// read-only client DBC memory. Original callbacks record their arguments; this
// validates buffer ownership and dispatch behavior, not the machine-code ABI.
#define __fastcall
#define __thiscall
#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <map>
#include <string>
#include "../native/WeaponState.h"

static std::map<std::uintptr_t,std::uint64_t> memory;
static std::map<std::uintptr_t,std::array<unsigned,16>> readableVisuals;
template<typename T> static bool read(std::uintptr_t address,T& out){
    const auto i=memory.find(address);if(i==memory.end())return false;
    out=static_cast<T>(i->second);return true;
}
static bool read(std::uintptr_t address,std::array<unsigned,16>& out){
    const auto i=readableVisuals.find(address);
    if(i!=readableVisuals.end()){out=i->second;return true;}
    std::array<unsigned,16> copy{};
    for(unsigned field=0;field<copy.size();++field)
        if(!read(address+4*field,copy[field]))return false;
    out=copy;return true;
}
struct Player {std::uintptr_t model=0,unit=0;std::uint64_t guid=0;unsigned display=0,native=0;};
struct WeaponContext {
    std::uintptr_t parent=0,unit=0;std::uint64_t guid=0;unsigned token=0;
    WeaponSelection selection;
    std::array<int,3> routes{{-1,-1,-1}};
};
static Player player;
static WeaponContext context;
static bool playerAvailable=true,contextAvailable=true,scopedProjectileAppearance=false;
static bool snapshot(Player& out){out=player;return playerAvailable;}
static WeaponContext* weaponContext(std::uintptr_t model){
    return contextAvailable&&model==context.parent?&context:nullptr;
}
static std::map<unsigned,std::uintptr_t> displayRows;
static void* weaponDisplay(const WeaponAsset* asset){
    const auto i=displayRows.find(asset->display);
    return i==displayRows.end()?nullptr:reinterpret_cast<void*>(i->second);
}
#include "../native/ProjectileRenderer.h"

namespace {
constexpr std::uintptr_t spellTable=0xC0D788,visualTable=0xC0D738;
constexpr std::uintptr_t effectTable=0xC0D760,itemDisplayTable=0xC0DC10;
constexpr unsigned standardSpell=75,specialSpell=19434;
constexpr unsigned families[4]={2,3,18,19}; // Bow, gun, crossbow, wand.
constexpr unsigned allRanged=(1u<<2)|(1u<<3)|(1u<<18)|(1u<<19);
std::uintptr_t nextTable=0x2000000,nextRow=0x3000000;
unsigned scenarios=0;
std::string scenario;
void check(bool condition,const char* what){
    if(!condition){std::cerr<<"FAIL: "<<scenario<<": "<<what<<'\n';std::abort();}
}
const WeaponAsset* assetOf(unsigned subclass,unsigned differentDisplay=0){
    for(const auto& asset:weaponAssets)
        if(asset.kind==4&&asset.subclass==subclass&&asset.display!=differentDisplay)return &asset;
    std::abort();
}
std::uintptr_t row(std::uintptr_t dbc,unsigned id){
    auto& table=memory[dbc];if(!table){table=nextTable;nextTable+=0x100000;}
    memory[dbc+4]=std::max(memory[dbc+4],static_cast<std::uint64_t>(id));
    const auto address=nextRow;nextRow+=0x1000;
    memory[table+id*4]=address;memory[address]=id;return address;
}
void deleteRow(std::uintptr_t dbc,unsigned id){memory.erase(memory.at(dbc)+4*id);}
struct Recorded {
    void* unit=nullptr;
    const void *guid=nullptr,*cast=nullptr,*targets=nullptr,*flags=nullptr;
    unsigned ammo=0,inventory=0,arg5=0,arg6=0,arg7=0,spell=0,calls=0;
    const unsigned* visualPointer=nullptr;
    std::array<unsigned,16> visual{};
    bool scope=false;
} recorded;
bool copyRecordedVisual=true;
void original(void* unit,const void* guid,const void* cast,unsigned ammo,unsigned inventory,
    unsigned arg5,unsigned arg6,unsigned arg7,const unsigned* visual,unsigned spell,
    const void* targets,const void* flags){
    ++recorded.calls;recorded.unit=unit;recorded.guid=guid;recorded.cast=cast;
    recorded.ammo=ammo;recorded.inventory=inventory;
    recorded.arg5=arg5;recorded.arg6=arg6;recorded.arg7=arg7;recorded.spell=spell;
    recorded.visualPointer=visual;recorded.targets=targets;recorded.flags=flags;
    recorded.scope=scopedProjectileAppearance;
    if(visual&&copyRecordedVisual)std::copy(visual,visual+16,recorded.visual.begin());
}
struct VisualRecorded {
    void* unit=nullptr;
    const unsigned* spell=nullptr;
    unsigned* output=nullptr;
    unsigned calls=0;
    std::array<unsigned,16> native{};
    // Native failures may return null or a different buffer. Both are passthrough.
    unsigned returnMode=0;
    std::array<unsigned,16> foreign{};
} visualRecorded;
unsigned* originalVisual(void* unit,const unsigned* spell,unsigned* output){
    ++visualRecorded.calls;visualRecorded.unit=unit;visualRecorded.spell=spell;
    visualRecorded.output=output;
    if(output)std::copy(visualRecorded.native.begin(),visualRecorded.native.end(),output);
    if(visualRecorded.returnMode==1)return nullptr;
    if(visualRecorded.returnMode==2)return visualRecorded.foreign.data();
    return output;
}
struct Fixture {
    const WeaponAsset* selected=nullptr;
    std::uintptr_t display=0,weaponVisual=0,spell=0;
    unsigned spellId=standardSpell,visualId=0,missile=405,sound=0,precast=0,fire=0,reload=0;
    std::array<unsigned,16> input{};
    const unsigned* inputPointer=nullptr;
    void* unit=nullptr;
    unsigned equippedSubclass=0;
    unsigned char guid=1,cast=2,targets=3,flags=4;

    Fixture(unsigned equipped=19,unsigned appearance=3){
        memory.clear();readableVisuals.clear();displayRows.clear();recorded={};visualRecorded={};
        equippedSubclass=equipped;
        nextTable=0x2000000;nextRow=0x3000000;
        player={0x1000,0x2000,0x1234567812345678ULL,49,49};
        playerAvailable=true;contextAvailable=true;scopedProjectileAppearance=false;
        context={};context.parent=player.model;context.unit=player.unit;context.guid=player.guid;
        context.selection.independent=true;
        context.selection.equipped[2]=assetOf(equipped)->item;
        choose(assetOf(appearance));
        spell=row(spellTable,spellId);
        memory[spell+0x18]=2;memory[spell+0xE8]=2;memory[spell+0xEC]=1u<<equipped;
        row(itemDisplayTable,2414);row(itemDisplayTable,2418);
        for(unsigned i=0;i<input.size();++i)input[i]=700+i*13;
        // A wand source deliberately supplies a bolt and other sources ammo.
        input[6]=equipped==19?1:0;input[7]=equipped==19?151:0;
        inputPointer=input.data();makeInputReadable();
        unit=reinterpret_cast<void*>(player.unit);
        unitMissileOriginal=&original;unitSpellVisualOriginal=&originalVisual;copyRecordedVisual=true;
    }
    void choose(const WeaponAsset* asset){
        selected=asset;context.selection.items[9]=asset->item;
        context.routes=context.selection.routes();
        display=row(itemDisplayTable,asset->display);displayRows[asset->display]=display;
        switch(asset->subclass){
        case 2:visualId=5;precast=7;fire=164;sound=4222;missile=0;break;
        case 3:visualId=224;precast=161;fire=167;sound=0;missile=0;break;
        case 18:visualId=743;precast=803;fire=804;sound=4222;missile=0;break;
        case 19:visualId=2799;precast=372;fire=2973;sound=0;missile=405;break;
        default:std::abort();
        }
        memory[display+0x28]=visualId;
        weaponVisual=row(visualTable,visualId);
        for(unsigned i=0;i<16;++i)memory[weaponVisual+4*i]=0;
        memory[weaponVisual]=visualId;
        memory[weaponVisual+4]=precast;memory[weaponVisual+8]=fire;
        memory[weaponVisual+0x24]=1;
        if(asset->subclass==2)memory[weaponVisual+0xC]=1947;
        memory[weaponVisual+0x18]=asset->subclass==19?1:0;
        memory[weaponVisual+0x1C]=missile;memory[weaponVisual+0x28]=sound;
        memory[weaponVisual+0x38]=reload;
        if(missile)row(effectTable,missile);
    }
    void makeInputReadable(){readableVisuals[reinterpret_cast<std::uintptr_t>(inputPointer)]=input;}
    void runUpstream(bool replaced,bool dispatch=false,unsigned returnMode=0,
        bool nullOutput=false,const unsigned* overrideSpell=nullptr){
        ++scenarios;
        struct GuardedVisual {
            std::array<unsigned,3> before{{0xFEDCBA98,0x76543210,0x89ABCDEF}};
            std::array<unsigned,16> fields{};
            std::array<unsigned,5> after{{0x01234567,0xABCDEF01,0x23456789,0xDEAD1234,0x5678BEEF}};
        } buffer;
        const auto beforeGuard=buffer.before;const auto afterGuard=buffer.after;
        buffer.fields.fill(0xBAADF00D);
        const auto untouched=buffer.fields;
        visualRecorded={};visualRecorded.native=input;visualRecorded.returnMode=returnMode;
        visualRecorded.foreign.fill(0xDEADC0DE);
        const auto beforeMemory=memory;
        const auto beforeDisplays=displayRows;
        const auto beforeReadable=readableVisuals;
        const auto beforeInput=input;
        const auto beforeSelection=context.selection;
        const auto beforeRoutes=context.routes;
        const auto beforeForeign=visualRecorded.foreign;
        const auto spellPointer=overrideSpell?overrideSpell:reinterpret_cast<const unsigned*>(spell);
        auto* output=nullOutput?nullptr:buffer.fields.data();
        auto* result=unitSpellVisualHook(unit,nullptr,spellPointer,output);
        auto* expectedReturn=returnMode==1?nullptr:returnMode==2?visualRecorded.foreign.data():output;
        check(result==expectedReturn,"visual hook returns exact native return pointer");
        check(visualRecorded.calls==1&&visualRecorded.unit==unit&&visualRecorded.spell==spellPointer&&
            visualRecorded.output==output,"native visual merger called once with original arguments");
        check(buffer.before==beforeGuard&&buffer.after==afterGuard,"caller buffer canaries unchanged");
        check(input==beforeInput&&memory==beforeMemory&&displayRows==beforeDisplays&&
            readableVisuals==beforeReadable&&visualRecorded.foreign==beforeForeign,
            "visual hook never modifies input, shared DBC data, or foreign return buffer");
        check(context.selection==beforeSelection&&context.routes==beforeRoutes,"visual hook retains weapon state");
        if(nullOutput){check(buffer.fields==untouched,"null output never writes caller buffer");return;}
        auto expected=input;
        if(replaced){
            expected[1]=precast;expected[2]=fire;expected[6]=1;
            expected[7]=selected->subclass==19?missile:0;expected[10]=sound;expected[14]=reload;
        }
        check(buffer.fields==expected,"only selected precast/fire/reload/flight and missile fields change");
        // The client checks this before reaching the late missile constructor.
        // A wand sends no ammo; its replacement must survive this early gate.
        if(dispatch){
            const unsigned sourceAmmo=equippedSubclass==19?0:777;
            check(buffer.fields[6]||sourceAmmo,"upstream visual survives native zero-ammo dispatch gate");
            check(replaced,"dispatch simulation requires a selected ranged replacement");
            readableVisuals[reinterpret_cast<std::uintptr_t>(buffer.fields.data())]=buffer.fields;
            const auto beforeBuffer=buffer.fields;
            recorded={};
            unitMissileHook(unit,nullptr,&guid,&cast,sourceAmmo,equippedSubclass==19?0:24,
                113,227,449,buffer.fields.data(),spellId,&targets,&flags);
            check(recorded.calls==1,"missile dispatch reaches native constructor once");
            check(recorded.ammo==(selected->subclass==19?0:selected->subclass==3?2418:2414),
                "early preparation and late constructor select the same projectile family");
            check(recorded.inventory==(selected->subclass==19?0u:24u),"projectile inventory matches selected ammo");
            check(recorded.visual==expected,"late constructor retains chosen firing sounds and all ability fields");
            check(buffer.fields==beforeBuffer,"late constructor preserves caller-owned firing visual");
            check(recorded.visualPointer!=buffer.fields.data(),"missile constructor uses its own scoped copy");
            check(recorded.scope&&!scopedProjectileAppearance,"metadata scope ends with missile constructor");
            readableVisuals.erase(reinterpret_cast<std::uintptr_t>(buffer.fields.data()));
        }
    }
    void run(bool replaced,bool previousScope=false){
        ++scenarios;
        const auto beforeMemory=memory;
        const auto beforeDisplays=displayRows;
        const auto beforeReadable=readableVisuals;
        const auto beforeInput=input;
        const auto beforeSelection=context.selection;
        const auto beforeRoutes=context.routes;
        scopedProjectileAppearance=previousScope;
        unitMissileHook(unit,nullptr,&guid,&cast,777,26,113,227,449,inputPointer,
            spellId,&targets,&flags);
        check(recorded.calls==1,"original missile constructor called exactly once");
        check(recorded.unit==unit&&recorded.guid==&guid&&recorded.cast==&cast&&
              recorded.targets==&targets&&recorded.flags==&flags,"unit and every target/cast pointer forwarded exactly");
        check(recorded.arg5==113&&recorded.arg6==227&&recorded.arg7==449&&
              recorded.spell==spellId,"timing/other arguments and original spell forwarded exactly");
        check(recorded.scope==replaced,"appearance metadata scope active only during replacement");
        check(scopedProjectileAppearance==previousScope,"previous metadata scope restored after original call");
        check(input==beforeInput&&memory==beforeMemory&&displayRows==beforeDisplays&&
              readableVisuals==beforeReadable,"input and shared DBC rows remain unmodified");
        check(context.selection==beforeSelection&&context.routes==beforeRoutes,"weapon ownership and selections unchanged");
        if(replaced){
            const bool wand=selected->subclass==19;
            const unsigned ammo=wand?0:selected->subclass==3?2418:2414;
            check(recorded.ammo==ammo&&recorded.inventory==(wand?0u:24u),"selected type controls ammo display and ammo inventory type");
            check(recorded.visualPointer!=inputPointer,"replacement uses private visual array");
            for(unsigned i=0;i<input.size();++i){
                const unsigned expected=i==7?(wand?missile:0):i==10?sound:beforeInput[i];
                check(recorded.visual[i]==expected,"only missile and missile sound change among all 16 visual fields");
            }
        }else{
            check(recorded.ammo==777&&recorded.inventory==26,"fallback retains original ammo arguments");
            check(recorded.visualPointer==inputPointer,"fallback retains exact original visual pointer");
            if(inputPointer&&copyRecordedVisual)check(recorded.visual==beforeInput,"fallback retains all visual fields");
        }
    }
};
template<typename Change> void fallback(const char* name,Change change,unsigned selected=3){
    scenario=name;Fixture f(19,selected);change(f);f.run(false);f.runUpstream(false);
}
}

int main(){
    for(unsigned equipped:families)for(unsigned selected:families){
        scenario="equipped "+std::to_string(equipped)+" to selected "+std::to_string(selected);
        Fixture f(equipped,selected);f.run(true);f.runUpstream(true,true);
    }
    scenario="native arcane wand missile";
    {Fixture f(3,19);f.run(true);}
    scenario="distinct shadow wand missile";
    {Fixture f(2,19);f.choose(assetOf(19,f.selected->display));f.missile=151;f.sound=927;
        memory[f.weaponVisual+0x1C]=f.missile;memory[f.weaponVisual+0x28]=f.sound;
        row(effectTable,f.missile);f.run(true);}
    scenario="decorative wand without visual uses native lesser magic bolt";
    {Fixture f(3,19);memory[f.display+0x28]=0;
        f.run(true);}
    scenario="ranged special attack uses selected projectile";
    {Fixture f(19,3);f.spellId=specialSpell;
        const auto special=row(spellTable,specialSpell);memory[special+0x18]=0x400002;
        memory[special+0xE8]=2;memory[special+0xEC]=allRanged;f.run(true);}
    scenario="previous active scope restored after replacement";
    {Fixture f;f.run(true,true);}
    scenario="previous active scope suppressed during unrelated cast then restored";
    {Fixture f;memory[f.spell+0x18]=0;f.run(false,true);}

    fallback("no player snapshot",[](Fixture&){playerAvailable=false;});
    fallback("another unit",[](Fixture& f){f.unit=reinterpret_cast<void*>(0x3000);});
    fallback("transformed player",[](Fixture&){++player.display;});
    fallback("missing weapon context",[](Fixture&){contextAvailable=false;});
    fallback("different model context",[](Fixture&){++context.parent;});
    fallback("preview context",[](Fixture&){context.token=19;});
    fallback("stale unit ownership",[](Fixture&){++context.unit;});
    fallback("stale player GUID",[](Fixture&){++context.guid;});
    fallback("no ranged appearance route",[](Fixture&){context.routes[2]=-1;});
    fallback("route beyond selection array",[](Fixture&){context.routes[2]=10;});
    fallback("corrupt ranged appearance route",[](Fixture&){context.routes[2]=123456;});
    fallback("empty appearance",[](Fixture&){context.selection.items[9]=0;});
    fallback("unknown appearance",[](Fixture&){context.selection.items[9]=0xFFFFFF;});
    fallback("melee appearance is never a projectile",[](Fixture&){context.selection.items[9]=25;});
    fallback("zero spell",[](Fixture& f){f.spellId=0;memory[f.spell]=0;});
    fallback("unknown spell",[](Fixture& f){f.spellId=90000;memory[f.spell]=90000;});
    fallback("missing spell table",[](Fixture&){memory.erase(spellTable);});
    fallback("null spell table",[](Fixture&){memory[spellTable]=0;});
    fallback("missing spell row",[](Fixture& f){deleteRow(spellTable,f.spellId);});
    fallback("null spell row",[](Fixture& f){memory[memory.at(spellTable)+4*f.spellId]=0;});
    fallback("mismatched spell row",[](Fixture& f){memory[f.spell]=76;});
    fallback("unreadable spell attributes",[](Fixture& f){memory.erase(f.spell+0x18);});
    fallback("unrelated cast spell",[](Fixture& f){memory[f.spell+0x18]=0;});
    fallback("nonweapon spell requirement",[](Fixture& f){memory[f.spell+0xE8]=4;});
    fallback("unrestricted spell",[](Fixture& f){memory[f.spell+0xEC]=0;});
    fallback("melee ability",[](Fixture& f){memory[f.spell+0xEC]=1u<<7;});
    fallback("mixed melee and ranged ability",[](Fixture& f){memory[f.spell+0xEC]=allRanged|(1u<<7);});
    fallback("thrown ability",[](Fixture& f){memory[f.spell+0xEC]=1u<<16;});
    fallback("selected display unavailable",[](Fixture& f){displayRows.erase(f.selected->display);});
    fallback("unreadable display visual",[](Fixture& f){memory.erase(f.display+0x28);});
    fallback("zero nonwand visual",[](Fixture& f){memory[f.display+0x28]=0;});
    fallback("missing weapon visual",[](Fixture& f){deleteRow(visualTable,f.visualId);});
    fallback("mismatched weapon visual row",[](Fixture& f){memory[f.weaponVisual]=f.visualId+1;});
    fallback("unreadable missile",[](Fixture& f){memory.erase(f.weaponVisual+0x1C);});
    fallback("unreadable missile sound",[](Fixture& f){memory.erase(f.weaponVisual+0x28);});
    fallback("missing bullet display",[](Fixture&){deleteRow(itemDisplayTable,2418);});
    fallback("missing arrow display",[](Fixture&){deleteRow(itemDisplayTable,2414);},2);
    fallback("missing crossbow arrow display",[](Fixture&){deleteRow(itemDisplayTable,2414);},18);
    fallback("incorrect bullet display identity",[](Fixture&){memory[projectileDBRow(itemDisplayTable,2418)]=2414;});
    fallback("wand visual has no missile",[](Fixture& f){memory[f.weaponVisual+0x18]=0;},19);
    fallback("unreadable wand missile flag",[](Fixture& f){memory.erase(f.weaponVisual+0x18);},19);
    fallback("missing wand effect",[](Fixture& f){deleteRow(effectTable,f.missile);},19);
    fallback("mismatched wand effect",[](Fixture& f){memory[projectileDBRow(effectTable,f.missile)]=151;},19);
    fallback("decorative wand fallback unavailable",[](Fixture& f){memory[f.display+0x28]=0;deleteRow(visualTable,2799);},19);
    scenario="null caller missile visual";
    {Fixture f;f.inputPointer=nullptr;f.run(false);}
    scenario="unreadable caller missile visual";
    {Fixture f;readableVisuals.clear();copyRecordedVisual=false;
        f.inputPointer=reinterpret_cast<const unsigned*>(0xBAD);f.run(false);}
    scenario="native visual merger returns null";
    {Fixture f;f.runUpstream(false,false,1);}
    scenario="native visual merger returns foreign buffer";
    {Fixture f;f.runUpstream(false,false,2);}
    scenario="null caller output";
    {Fixture f;f.runUpstream(false,false,0,true);}
    scenario="unreadable caller spell row";
    {Fixture f;f.runUpstream(false,false,0,false,reinterpret_cast<const unsigned*>(0xBAD));}
    scenario="copied spell row identity rejected";
    {Fixture f;memory[0x12340000]=f.spellId;
        f.runUpstream(false,false,0,false,reinterpret_cast<const unsigned*>(0x12340000));}
    scenario="selected reload sound and animation transferred";
    {Fixture f;f.reload=918;memory[f.weaponVisual+0x38]=f.reload;f.runUpstream(true,true);}
    scenario="zero gun sound fields clear previous wand sounds";
    {Fixture f;f.input[10]=5414;f.input[14]=745;f.input[6]=0;f.makeInputReadable();
        f.runUpstream(true,true);}
    scenario="physical visual with stale bolt still uses cosmetic ammo";
    {Fixture f;memory[f.weaponVisual+0x1C]=151;f.runUpstream(true,true);}
    scenario="ranged ability preserves native impact and state visuals";
    {Fixture f;f.spellId=specialSpell;f.spell=row(spellTable,specialSpell);
        memory[f.spell+0x18]=0x400002;memory[f.spell+0xE8]=2;memory[f.spell+0xEC]=allRanged;
        f.runUpstream(true,true);}
    scenario="decorative wand upstream uses magic wand casting kits";
    {Fixture f(3,19);memory[f.display+0x28]=0;f.runUpstream(true,true);}
    scenario="shadow wand upstream uses its own firing kit";
    {Fixture f(2,19);f.missile=151;f.fire=2974;
        memory[f.weaponVisual+0x1C]=f.missile;memory[f.weaponVisual+8]=f.fire;
        row(effectTable,f.missile);f.runUpstream(true,true);}
    std::cout<<"PASS: "<<scenarios<<" projectile and firing-visual scenarios, including all 16 ranged family combinations\n";
}
