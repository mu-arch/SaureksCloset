// Actual attachment, native clear/find hooks, ownership and Lua bridge.
#define main weaponBaseTests
#include "weapon_renderer.cpp"
#undef main
static bool cloneSucceeds=true;
static void* cloneForTest(void*,void* source,unsigned flags){
    assert(flags==7);
    const auto* c=weaponContext(address(source));
    if(c&&c->secondHat)assert(memory[address(c->secondHat)+0x1CC]==0);
    return cloneSucceeds?pointer(0xDEAD00):nullptr;
}
static const auto cloneModelOriginal=&cloneForTest;
#include "../native/SecondHatRenderer.h"
static void textAt(std::uintptr_t at,const char* value){
    for(unsigned i=0;;++i){memory[at+i]=static_cast<unsigned char>(value[i]);if(!value[i])return;}
}
static int chooseHat(unsigned display,unsigned token=0){Lua L{{double(token),double(display)}};return setSecondHatLua(&L);}
int main(){
    weaponBaseTests();
    for(auto& c:weaponContexts)if(c.parent)forgetWeapons(c.parent);
    previews.entries={};player.display=player.native=49;player.component=0xD01000;
    memory[player.model+0x10]=1;memory[player.component+0x18]=1;memory[player.component+0x1C]=1;
    memory[0xC0DC10]=0xD02000;memory[0xC0DC14]=100;
    memory[0xD02000+4*75]=0xD03000;memory[0xD03000]=75;memory[0xD03004]=0xD04000;memory[0xD0300C]=0xD05000;
    textAt(0xD04000,"Helm_Leather_B_01.mdx");textAt(0xD05000,"Helm_Leather_B_01Black");
    memory[0xC0DEE0]=0xD06000;memory[0xC0DEE4]=8;
    const char* codes[]={"Hu","Or","Dw","Ni","Sc","Ta","Gn","Tr"};
    for(unsigned race=1;race<=8;++race){
        const auto row=0xD07000+0x100*race;memory[0xD06000+4*race]=row;memory[row]=race;memory[row+24]=row+32;textAt(row+32,codes[race-1]);
        for(unsigned sex=0;sex<2;++sex){
            std::string model,texture;assert(secondHatPaths(75,race,sex,model,texture));
            assert(model==std::string("Item\\ObjectComponents\\Head\\Helm_Leather_B_01_")+codes[race-1]+(sex?"F":"M")+".mdx");
            assert(texture=="Item\\ObjectComponents\\Head\\Helm_Leather_B_01Black.blp");
        }
    }
    factory(pointer(player.model),11,"Item\\ObjectComponents\\Head\\Helm_Cloth_HuF.mdx","",0);
    auto* primary=find(pointer(player.model),11);assert(primary);
    assert(chooseHat(75)==1);auto* c=weaponContext(player.model);assert(c&&c->secondHat&&c->secondHat!=primary);
    auto* secondary=c->secondHat;const auto count=loads;
    assert(refs[address(secondary)]==2&&findChildHook(pointer(player.model),nullptr,11)==primary);
    assert(weaponModelMatches(secondary,"Item\\ObjectComponents\\Head\\Helm_Leather_B_01_HuF.mdx"));
    for(unsigned i=0;i<10;++i)assert(chooseHat(75)==1&&c->secondHat==secondary&&loads==count);
    // Default weapon/bag synchronization must retain the hat-only context.
    WeaponSelection none;auto weapons=request(0,none);assert(setWeapons(&weapons)==1&&c->secondHat==secondary);
    Lua bags;bags.values.resize(17,0);assert(setBagsStatus(&bags)==1&&c->secondHat==secondary);
    clearChildrenHook(pointer(player.model),nullptr,11);
    assert(!refs[address(primary)]&&refs[address(secondary)]==2&&findChildHook(pointer(player.model),nullptr,11)==nullptr);
    factory(pointer(player.model),11,"Item\\ObjectComponents\\Head\\Helm_Cloth_HuF.mdx","",0);
    primary=findChildHook(pointer(player.model),nullptr,11);assert(primary&&primary!=secondary&&c->secondHat==secondary);
    // Head tuning and hair trimming must keep treating the first hat as primary.
    BagMatrix input{{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1}},output;
    assert(!positionHaircraftHat(secondary,input.data(),output));
    for(bool success:{true,false}){
        cloneSucceeds=success;assert(cloneWithoutSecondHat(nullptr,pointer(player.model),7)==(success?pointer(0xDEAD00):nullptr));
        assert(memory[address(secondary)+0x1CC]==player.model&&refs[address(secondary)]==2);
    }
    // Clear/retry of asynchronously loading art does not duplicate attachments.
    assert(chooseHat(0)==1&&!c->secondHat&&refs[address(secondary)]==0&&findChildHook(pointer(player.model),nullptr,11)==primary);
    factoryModelsLoaded=false;assert(chooseHat(75)==0);factoryModelsLoaded=true;
    secondary=c->secondHat;assert(secondary);const auto pendingLoads=loads;assert(chooseHat(75)==0&&loads==pendingLoads);
    memory[address(secondary)+0x10]=1;assert(chooseHat(75)==1&&loads==pendingLoads);
    // Invalid display/path/arguments cannot replace a good selection.
    assert(chooseHat(76)==-2&&c->secondHat==secondary);
    textAt(0xD04000,"../Helm_Bad.mdx");assert(chooseHat(75)==-2&&c->secondHat==secondary);textAt(0xD04000,"Helm_Leather_B_01.mdx");
    Lua bad{{0,-1}};assert(setSecondHatLua(&bad)==-2);bad.values={0,.5};assert(setSecondHatLua(&bad)==-2);
    ++c->guid;assert(chooseHat(0)==-1&&c->secondHat==secondary);--c->guid;
    // Every body gets a correctly suffixed attachment, independent of previews.
    for(unsigned race=1;race<=8;++race)for(unsigned sex=0;sex<2;++sex){
        memory[player.component+0x18]=race;memory[player.component+0x1C]=sex;
        assert(chooseHat(75)==1&&c->secondHatRace==race&&c->secondHatSex==sex);
    }
    const auto worldHat=c->secondHat;
    const auto token=previews.bind(0xD10000,player.guid,Appearance{});auto* e=previews.find(0xD10000);assert(token&&e);
    e->status=1;e->component=0xD11000;memory[e->component+0x18]=1;memory[e->component+0x1C]=0;memory[e->model+0x10]=1;
    assert(chooseHat(75,token)==1);auto* preview=weaponContext(e->model);assert(preview&&preview->secondHat!=worldHat);
    assert(chooseHat(0,token)==1&&c->secondHat==worldHat);
    assert(chooseHat(75,99999)==-1&&c->secondHat==worldHat);
    forgetWeapons(player.model);assert(!refs[address(worldHat)]&&!weaponContext(player.model));
    std::cout<<"PASS: second hat paths for 16 bodies, two independent hats, native rebuild/clear/find, async loading, preview/clone ownership, look/weapon/bag isolation, removal and destruction\n";
}
