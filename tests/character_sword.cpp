// Stock C-window clones are not addon-preview contexts. Exercise real hooks.
#define main weaponBaseTests
#include "weapon_renderer.cpp"
#undef main
static void skeleton(std::uintptr_t parent,const SwordAttachmentFixture& f,const BagMatrix& bone){
    const auto data=parent+0x100000,header=data+0x1000,lookup=data+0x2000,records=data+0x3000,palette=data+0x5000;
    memory[parent+0x30]=data;memory[data+0x130]=header;
    memory[header+0x10C]=37;memory[header+0x110]=lookup;memory[lookup+2*f.point]=f.index;
    memory[header+0x104]=34;memory[header+0x108]=records;memory[header+0x34]=128;
    const auto record=records+48*f.index;
    memory[record]=f.point;memory[record+4]=f.bone;positions[record+8]=f.position;
    memory[parent+0x94]=palette;matrices[palette+64*f.bone]=bone;
}
int main(){
    weaponBaseTests();
    for(auto& c:weaponContexts)if(c.parent)forgetWeapons(c.parent);
    clonedBagPreviews={};clonedBackWeapons={};
    const auto* sword=weaponAsset(4939);assert(sword&&sword->kind==2&&sword->sheath==1);
    const BagMatrix raw{{1,0,0,0,0,1,0,0,0,0,1,0,-10,-20,-30,1}};
    unsigned cases=0;
    for(const auto& f:swordFixtures){
        auto& c=weaponContexts[0];c={};c.parent=player.model;c.guid=player.guid;c.unit=player.unit;
        const unsigned role=f.point==26?0:1,home=role?31:30;
        c.selection.stowedMask=7;c.selection.equipped[role]=sword->item;
        factory(pointer(c.parent),home,sword->model,sword->texture,0);
        auto* source=findChildOriginal(pointer(c.parent),home);c.nativeChildren[role]=source;
        // The source world transform must never be copied into the UI camera.
        skeleton(c.parent,f,raw);
        const std::uintptr_t parent=0xED0000,nested=0xEE0000;
        factory(pointer(parent),home,sword->model,sword->texture,0);
        auto* child=findChildOriginal(pointer(parent),home);
        rememberClonedBagPreview(address(source),address(child));
        assert(clonedBackWeapon(address(child)));
        BagMatrix out;assert(!positionStoredBackWeapon(child,out)); // parent not registered yet
        rememberClonedBagPreview(c.parent,parent);
        assert(!weaponContext(parent)&&clonedBagOwner(parent)==player.guid);
        for(const BagMatrix& bone:{BagMatrix{{1,0,0,0,0,1,0,0,0,0,1,0,1,2,3,1}},
            BagMatrix{{0,2,0,0,-2,0,0,0,0,0,2,0,8,-4,7,1}}}){
            skeleton(parent,f,bone);
            auto expected=bone;
            for(unsigned axis=0;axis<3;++axis)expected[12+axis]+=f.position[0]*bone[axis]+f.position[1]*bone[4+axis]+f.position[2]*bone[8+axis];
            assert(positionStoredBackWeapon(child,out)&&out==expected);
            captureAttachmentMatrix=true;
            for(unsigned caller:{0x718761u,0x71415Du,0x714183u}){
                updateAttachmentForCaller(child,raw.data(),nullptr,nullptr,.75f,caller);
                assert(capturedAttachmentMatrix==expected&&attachmentUpdate.alpha==.75f);
            }
            captureAttachmentMatrix=false;
            assert(memory[address(child)+0x1D0]==home); // logical ownership unchanged
            // Nested native clones keep their own live skeleton and camera.
            factory(pointer(nested),home,sword->model,sword->texture,0);
            auto* copy=findChildOriginal(pointer(nested),home);
            rememberClonedBagPreview(address(child),address(copy));rememberClonedBagPreview(parent,nested);
            skeleton(nested,f,bone);assert(positionStoredBackWeapon(copy,out)&&out==expected);
            detach(copy);assert(!clonedBackWeapon(address(copy)));forgetWeapons(nested);
        }
        // A snapshot remains valid after the source player's selection changes.
        c.selection.equipped[role]=36;assert(positionStoredBackWeapon(child,out));
        memory[address(child)+0x1D0]=1;assert(!positionStoredBackWeapon(child,out));memory[address(child)+0x1D0]=home;
        modelName(child,weaponAsset(36)->model);assert(!positionStoredBackWeapon(child,out));modelName(child,sword->model);
        memory[address(child)+0x1CC]=0xEF0000;assert(!positionStoredBackWeapon(child,out));memory[address(child)+0x1CC]=parent;
        ++player.guid;assert(!positionStoredBackWeapon(child,out));--player.guid;
        forgetWeapons(parent);assert(!positionStoredBackWeapon(child,out));
        detach(child);assert(!clonedBackWeapon(address(child)));
        detach(source);forgetWeapons(c.parent);++cases;
    }
    assert(cases==32);
    std::cout<<"PASS: C-window stowed swords, both back sides/all 16 bodies, recursive/lazy updates, independent clone camera/skeleton, nested clones, hands/mesh/owner isolation and cleanup\n";
}
