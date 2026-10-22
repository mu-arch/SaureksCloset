// Exercise actual hooks against client-derived caster-role/animation variants.
#define main castingBaseTests
#include "casting.cpp"
#undef main
#include "casting_catalog_fixtures.h"
int main(){
    castingBaseTests();settings();simulateKit=false;
    // Start a fresh simulated process/DBC after the synthetic base scenarios.
    castingKits.clear();
    unsigned channels=0,none=0,unusual=0;
    for(const auto& fixture:castingCases){
        put(0x8000,fixture.spell);dbcRows[{0xC0D788,fixture.spell}]=0x8000;
        put(player.unit+0xC8C,fixture.spell);
        sourceVisual=fixture.visual;
        simulateKit=fixture.phase==2&&(weaponCasting::effect(fixture.kit[6])||weaponCasting::effect(fixture.kit[7]));
        assert(call(fixture.kit,fixture.phase)==77);
        if(simulateKit)assert(receivedPoint==weaponCasting::marker+22);
        simulateKit=false;
        assert(receivedKit!=reinterpret_cast<const unsigned*>(0x9000));
        const bool release=fixture.phase==0&&!fixture.visual[5];
        assert(receivedKit[2]==(release?19u:28u));
        for(unsigned i=0;i<35;++i)if(i!=2&&i!=6&&i!=7)assert(receivedKit[i]==fixture.kit[i]);
        assert(receivedKit[6]==~0u);
        const auto& k=fixture.kit;
        assert(receivedKit[7]==(weaponCasting::effect(k[7])?k[7]:k[6]));
        if(fixture.phase==2)++channels;
        if(k[2]==~0u)++none;
        else if(!weaponCasting::ready(k[2])&&!weaponCasting::release(k[2])&&!weaponCasting::channel(k[2]))++unusual;
        // The same kit on a victim/ground/aura dispatch must not be hijacked.
        for(unsigned phase:{1u,3u,5u}){
            call(fixture.kit,phase);assert(receivedKit==reinterpret_cast<const unsigned*>(0x9000));
        }
        if(fixture.phase==2){
            put(player.unit+0xC8C,0u);put(0x7228,0u);
            call(fixture.kit,2);assert(receivedKit==reinterpret_cast<const unsigned*>(0x9000));
        }
    }
    assert(channels&&none&&unusual);
    put(0x8000,133u);sourceVisual={};sourceVisual[3]=286;sourceVisual[10]=1234;
    auto output=sourceVisual;
    assert(castingAugmentVisual(ptr(player.unit),reinterpret_cast<const unsigned*>(0x8000),output.data(),output.data())==output.data());
    assert(output[2]==21);
    for(unsigned i=0;i<16;++i)if(i!=2)assert(output[i]==sourceVisual[i]);
    weaponCasting::Kit blank{};blank[0]=21;blank[2]=53;for(unsigned i=3;i<35;++i)blank[i]=~0u;
    call(blank,0);assert(receivedKit[2]==19&&receivedKit[7]==~0u);
    // A missing visual also gets only a silent weapon swing, never a missile.
    visualAvailable=false;call(blank,0);assert(receivedKit[2]==19);
    output.fill(999);castingAugmentVisual(ptr(player.unit),reinterpret_cast<const unsigned*>(0x8000),output.data(),nullptr);
    assert(output[2]==21);for(unsigned i=0;i<16;++i)if(i!=2)assert(output[i]==0);
    visualAvailable=true;
    sourceVisual[5]=729;output=sourceVisual;
    castingAugmentVisual(ptr(player.unit),reinterpret_cast<const unsigned*>(0x8000),output.data(),output.data());
    assert(output==sourceVisual);
    sourceVisual[5]=0;output=sourceVisual;
    castingAugmentVisual(ptr(0x9999),reinterpret_cast<const unsigned*>(0x8000),output.data(),output.data());assert(output==sourceVisual);
    settings(false);output=sourceVisual;
    castingAugmentVisual(ptr(player.unit),reinterpret_cast<const unsigned*>(0x8000),output.data(),output.data());assert(output==sourceVisual);
    settings(true,35,true,false);output=sourceVisual;
    castingAugmentVisual(ptr(player.unit),reinterpret_cast<const unsigned*>(0x8000),output.data(),output.data());assert(output==sourceVisual);
    std::cout<<"PASS: "<<std::size(castingCases)<<" client caster-role fixtures, all Blizzard ranks, channel-only spells, custom/no-animation kits, target isolation and silent missing-caster fallback\n";
}
