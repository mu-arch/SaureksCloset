// Real spell records distinguish activities from magic sharing the same art.
#define main castingBaseTests
#include "casting.cpp"
#undef main
#include "casting_activity_fixtures.h"
int main(){
    castingBaseTests();settings();castingKits.clear();
    unsigned preserved=0,magic=0;
    for(const auto& fixture:castingActivityCases){
        const auto& spell=fixture.spell;const auto id=spell[0];
        for(unsigned i=0;i<173;++i)put(0x8000+4*i,spell[i]);
        dbcRows[{0xC0D788,id}]=0x8000;sourceVisual=fixture.visual;
        put(player.unit+0xC8C,id);put(0x7228,id);put(player.unit+0xD40,1u);
        castingDrewWeapon=true;simulateKit=false;
        assert(castingSpell(id)==fixture.castable);
        auto output=sourceVisual;
        castingAugmentVisual(ptr(player.unit),reinterpret_cast<const unsigned*>(0x8000),output.data(),output.data());
        if(!fixture.castable)assert(output==sourceVisual);
        unsigned index=0;
        for(const auto& data:fixture.kits){
            const unsigned phase=index==0?4:index==1?0:2;++index;
            if(!data[0])continue;
            simulateKit=true;call(data,phase);simulateKit=false;
            if(fixture.castable){assert(receivedKit!=reinterpret_cast<const unsigned*>(0x9000));assert(receivedPoint==weaponCasting::marker+22);}
            else{
                assert(receivedKit==reinterpret_cast<const unsigned*>(0x9000));
                assert(receivedPoint==22); // cups/props remain on the native hand
                loop(data[2]);assert(receivedAnimation==data[2]);
            }
        }
        if(!fixture.castable){
            castingSheathHook(ptr(player.unit),nullptr,0,1,1);
            unsigned mode=9;assert(read(player.unit+0xD40,mode)&&mode==0&&receivedNotify==1);
            float out[3]{};castingOriginHook(out,ptr(player.unit),id);
            assert(receivedModel==player.unit&&receivedPoint==id);
            ++preserved;
        }else ++magic;
    }
    assert(preserved==30&&magic==10);
    std::cout<<"PASS: 30 real food/drink, bandage, potion, crafting/gathering/fishing and triggered-food records preserve native props/animation; 10 genuine spell controls retain weapon casting\n";
}
