#define main originalWeaponTests
#include "weapon_renderer.cpp"
#undef main
static unsigned captures=0,filters=0;
static bool intercept=false;
static void captureProbe(void*,const float*){++captures;}
static bool castingProbe(void*,const float* matrix,BagMatrix& out){
    ++filters;if(!intercept)return false;
    for(unsigned i=0;i<16;++i)out[i]=matrix[i];out[12]+=1;return true;
}
int main(){
    originalWeaponTests();
    weaponContexts={};clonedBagPreviews={};clonedBagChildren={};clonedBackWeapons={};
    player={0x4100000,0x4101000,123,49,49};bagTuningOwner=player.guid;
    updateAttachedOriginal=&observeAttachment;captureAttachmentMatrix=true;evaluateAttachmentBones=false;
    hairMask::capture=&captureProbe;castingAttachmentFilter=&castingProbe;
    const auto model=pointer(0x4200000);memory[address(model)+0x1cc]=0x4300000;
    BagMatrix input{{1,0,0,0,0,1,0,0,0,0,1,0,2,3,4,1}};
    for(unsigned point:{1u,11u,30u})for(unsigned caller:{0x718761u,0x71415Du,0x714183u}){
        memory[address(model)+0x1d0]=point;const auto calls=attachmentUpdate.calls;
        captures=filters=0;intercept=false;
        updateAttachmentForCaller(model,input.data(),nullptr,nullptr,.7f,caller);
        assert(attachmentUpdate.calls==calls+1&&capturedAttachmentMatrix==input&&attachmentUpdate.alpha==.7f);
        assert(captures==1&&filters==1);
        // Spell children are attached to weapons, so have no direct player
        // context. Their directional correction must precede the fast path.
        intercept=true;updateAttachmentForCaller(model,input.data(),nullptr,nullptr,.7f,caller);
        auto corrected=input;corrected[12]+=1;assert(capturedAttachmentMatrix==corrected);
        assert(captures==2&&filters==2);
    }
    hairMask::capture=nullptr;castingAttachmentFilter=nullptr;
    std::cout<<"PASS: unrelated attachments pass through unchanged; nested casting and hair capture still run before the fast path\n";
}
