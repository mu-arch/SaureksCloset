#define main existingWeaponRegression
#include "weapon_renderer.cpp"
#undef main
int main(){
    existingWeaponRegression();
    weaponContexts={};player={0x4100000,0x4101000,123,49,49};
    auto& local=weaponContexts[0];local.parent=player.model;local.unit=player.unit;local.guid=player.guid;
    Player found;
    assert(weaponPlayer(pointer(player.unit),found));
    assert(!weaponPlayer(pointer(player.unit+0x10000),found));
    assert(weaponContextActive(local));
    local.token=12;assert(weaponContextActive(local)); // registered local previews
    local.token=0;
    Lua on{{1}};assert(setWeaponPhysicsLua(&on)==1);
    WeaponPhysicsSettings settings;assert(weaponPhysicsForSlot(local,10,settings));
    auto foreign=local;foreign.guid=456;foreign.unit+=0x10000;foreign.parent+=0x10000;
    assert(!weaponContextActive(foreign));
    assert(!weaponPhysicsForSlot(foreign,10,settings));
    assert(!weaponContext(foreign.parent));
    auto identity=BagMatrix{{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1}};
    auto* child=pointer(0x4200000);const auto* asset=weaponAsset(35);assert(asset);
    modelName(child,asset->model);memory[address(child)+0x1cc]=foreign.parent;memory[address(child)+0x1d0]=30;
    BagMatrix output{};assert(!positionWeaponPhysics(child,identity.data(),output,true));
    // Old owner state cannot apply after logging into a different character.
    player.guid=456;assert(!weaponContextActive(local));assert(!weaponPhysicsForSlot(local,10,settings));
    player.guid=0;WeaponContext empty;assert(!weaponContextActive(empty));assert(!weaponPhysicsForSlot(empty,10,settings));
    std::cout<<"PASS: only current player/preview contexts are eligible; foreign and stale actors remain untouched\n";
}
