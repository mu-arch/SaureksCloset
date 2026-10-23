// Actual effect routing with all five build-5875 player Cone of Cold ranks.
#define main castingBaseTests
#include "casting.cpp"
#undef main
#include "casting_cone_fixtures.h"
int main(){
    castingBaseTests();settings();
    constexpr std::uintptr_t fx=0xABCD;
    put(fx+0x1CC,child);coneMatches=true;
    unsigned ranks=0;
    for(const auto& fixture:castingConeCases){
        for(unsigned i=0;i<173;++i)put(0x8000+4*i,fixture.spell[i]);
        const unsigned id=fixture.spell[0];dbcRows[{0xC0D788,id}]=0x8000;
        sourceVisual=fixture.visual;simulateKit=true;
        call(fixture.kits[0]);assert(receivedKit[2]==28&&receivedKit[7]==117);
        call(fixture.kits[1],0);
        assert(receivedKit[2]==19&&receivedKit[7]==625&&receivedKit[6]==~0u);
        assert(receivedPoint==weaponCasting::marker+22&&castingScope==0);
        // Correct direction throughout the effect's life, including later
        // lazy updates after the cast scope/current spell ID has cleared.
        put(player.unit+0xC8C,0u);put(0x7228,0u);
        for(float heading:{0.f,.6f,1.57f,3.14f,4.7f})for(float swing:{0.f,.7f,2.1f,4.5f}){
            const float c=std::cos(heading),s=std::sin(heading),scale=1.3f;
            std::array<float,16> root{{scale*c,scale*s,0,0,-scale*s,scale*c,0,0,0,0,scale,0,40,50,60,1}};
            put(player.model+0xFC,root);
            std::array<float,16> input{{std::cos(swing),0,std::sin(swing),0,0,1,0,0,-std::sin(swing),0,std::cos(swing),0,10+swing,20-swing,30,1}},out{};
            const auto untouched=input;
            assert(castingForwardAttachment(ptr(fx),input.data(),out));
            // The authored +X cone axis follows player facing/scale and has
            // zero vertical component, regardless of the staff swing.
            assert(std::fabs(out[0]-scale*c)<.00001f&&std::fabs(out[1]-scale*s)<.00001f&&out[2]==0);
            for(unsigned i=0;i<12;++i)assert(out[i]==root[i]);
            for(unsigned i=12;i<15;++i)assert(out[i]==input[i]);
            assert(out[15]==1&&input==untouched);
        }
        ++ranks;
    }
    assert(ranks==5);simulateKit=false;
    std::array<float,16> input{{1,0,0,0,0,1,0,0,0,0,1,0,10,20,30,1}},out{};
    coneMatches=false;assert(!castingForwardAttachment(ptr(fx),input.data(),out));coneMatches=true; // glows/other spell art
    put(fx+0x1CC,player.model);assert(!castingForwardAttachment(ptr(fx),input.data(),out)); // native, non-weapon effect
    put(fx+0x1CC,std::uintptr_t(0xDEAD));assert(!castingForwardAttachment(ptr(fx),input.data(),out));
    put(fx+0x1CC,child);
    childMatches=false;assert(!castingForwardAttachment(ptr(fx),input.data(),out));childMatches=true;
    player.display=1;assert(!castingForwardAttachment(ptr(fx),input.data(),out));player.display=49;
    ++castingOwner;assert(!castingForwardAttachment(ptr(fx),input.data(),out));--castingOwner;
    snapshotOK=false;assert(!castingForwardAttachment(ptr(fx),input.data(),out));snapshotOK=true;
    settings(true,35,false);assert(!castingForwardAttachment(ptr(fx),input.data(),out));
    settings(false);assert(!castingForwardAttachment(ptr(fx),input.data(),out));settings();
    auto broken=input;broken[0]=std::numeric_limits<float>::quiet_NaN();put(player.model+0xFC,broken);
    assert(!castingForwardAttachment(ptr(fx),input.data(),out));
    broken=input;broken[0]=broken[1]=broken[2]=0;put(player.model+0xFC,broken);
    assert(!castingForwardAttachment(ptr(fx),input.data(),out));
    put(player.model+0xFC,input);assert(!castingForwardAttachment(ptr(fx),nullptr,out));
    input[14]=std::numeric_limits<float>::infinity();assert(!castingForwardAttachment(ptr(fx),input.data(),out));
    std::cout<<"PASS: all five Cone of Cold ranks retain weapon swing/glow, forward blast through swing and facing changes, expired cast, isolation and safe fallbacks\n";
}
