#define main castingBaseTests
#include "casting.cpp"
#undef main
#include "casting_catalog_fixtures.h"
int main(){
    castingBaseTests();simulateKit=false;
    const std::array<float,16> grip{{1,0,0,0,0,1,0,0,0,0,1,0,.03f,-.02f,.01f,1}};
    put(child+0xBC,grip);
    unsigned cases=0;
    for(unsigned item:{36u,647u,35u})for(bool effects:{false,true}){
        settings(true,item,effects,false);
        const auto* asset=weaponAsset(item);const auto ready=weaponCasting::animation(51,asset->kind,asset->subclass);
        for(const auto& f:castingCases){
            put(0x8000,f.spell);dbcRows[{0xC0D788,f.spell}]=0x8000;
            put(player.unit+0xC8C,f.spell);sourceVisual=f.visual;
            call(f.kit,f.phase);assert(receivedKit[2]==ready);
            for(unsigned i=0;i<35;++i)if(i!=2&&(i!=6&&i!=7||!effects))assert(receivedKit[i]==f.kit[i]);
            std::array<float,16> actual;assert(read(child+0xBC,actual)&&actual==grip);
            unsigned point=0;std::uintptr_t parent=0;assert(read(child+0x1D0,point)&&point==1&&read(child+0x1CC,parent)&&parent==player.model);
            if((f.phase==4||f.phase==2||(f.phase==0&&f.visual[5]))&&f.kit[2]&&f.kit[2]!=~0u){
                for(unsigned time:{100u,3500u,30000u}){loop(f.kit[2],time);assert(receivedAnimation==ready);}
            }
            ++cases;
        }
    }
    assert(cases==6*std::size(castingCases));
    std::cout<<"PASS: swing-off retains native one-hand mace/2H/staff grip through every caster role and delayed loop, with effects on/off and no hand transform edits\n";
}
