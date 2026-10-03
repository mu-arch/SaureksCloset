#define main existingWeaponRegression
#include "weapon_renderer.cpp"
#undef main
static BagMatrix neutralPose(){return {{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1}};}
static float difference(const BagMatrix& a,const BagMatrix& b){float d=0;for(unsigned i=0;i<16;++i)d=std::fmax(d,std::fabs(a[i]-b[i]));return d;}
static void rigid(const BagMatrix& m,float scale){for(unsigned a=0;a<3;++a)for(unsigned b=a;b<3;++b){float dot=0;for(unsigned k=0;k<3;++k)dot+=m[a*4+k]*m[b*4+k];assert(std::fabs(dot-(a==b?scale*scale:0))<.0001f);}}
int main(){
    existingWeaponRegression();const auto identity=neutralPose();
    for(unsigned dt:{8u,16u,33u}){
        BagMotion motion,cameraMotion;float peak=0;BagMatrix out;
        auto local=identity;local[12]=.08f;local[14]=-.12f;
        for(unsigned t=0;t<6000;t+=dt){
            auto pose=identity;const float angle=.09f*std::sin(t*.013f);pose[0]=pose[5]=std::cos(angle);pose[1]=std::sin(angle);pose[4]=-pose[1];pose[12]=-.23f;pose[14]=1.4f+.035f*std::sin(t*.0176f);
            assert(rigidWeaponPhysics(motion,pose,local,identity,identity,out,77,2,30,t,true,true,0));
            rigid(bagMatrixProduct(out,local),1);peak=std::fmax(peak,difference(out,pose));
            const auto once=out;assert(rigidWeaponPhysics(motion,pose,local,identity,identity,out,77,2,30,t,true,true,0));assert(difference(out,once)<.00001f);
            auto render=identity;render[12]=40+t*.01f;render[13]=-12;render[14]=8;
            BagMatrix view;assert(rigidWeaponPhysics(cameraMotion,bagMatrixProduct(render,pose),local,render,render,view,77,2,30,t,true,true,0));
            BagMatrix inverse;assert(bagAffineInverse(render,inverse));assert(difference(bagMatrixProduct(inverse,view),out)<.00006f);
        }
        assert(peak>.005f&&peak<.7f);
        auto stopped=identity;stopped[12]=-.23f;stopped[14]=1.4f;
        for(unsigned t=6000;t<8500;t+=dt)assert(rigidWeaponPhysics(motion,stopped,local,identity,identity,out,77,2,30,t,false,false,0));
        assert(difference(out,stopped)<.0001f);
        for(unsigned t=8500;t<10000;t+=dt){assert(rigidWeaponPhysics(motion,stopped,local,identity,identity,out,77,2,30,t,false,true,1));rigid(bagMatrixProduct(out,local),1);}
    }
    // Quiet torso mounts must still produce visible secondary movement. The
    // previous heavy setting failed these displacement/rocking floors.
    for(bool running:{false,true})for(float scale:{.35f,1.f})for(unsigned dt:{8u,16u,33u}){
        BagMotion motion;float bob=0,rock=0;
        for(unsigned t=0;t<6000;t+=dt){
            auto pose=identity;const float angle=.025f*std::sin(t*.013f);
            pose[0]=pose[5]=std::cos(angle)*scale;pose[1]=std::sin(angle)*scale;pose[4]=-pose[1];pose[10]=scale;
            pose[12]=-.25f;pose[14]=1.4f+.012f*std::sin(t*.0176f);BagMatrix moved;
            assert(rigidWeaponPhysics(motion,pose,identity,identity,identity,moved,77,2,30,t,running,true,0));rigid(moved,scale);
            if(t>1000){
                bob=std::fmax(bob,std::fabs(moved[14]-pose[14]));
                const float dot=std::fabs(bagRotationDot(bagRotation(pose,scale),bagRotation(moved,scale)));
                rock=std::fmax(rock,2*std::acos(std::fmin(1.f,dot))*57.29578f);
            }
        }
        assert(bob>(running?.009f:.0055f)&&bob<.04f);
        assert(rock>2.8f&&rock<8.f);
    }
    // Each control changes just its own response, including at zero and 200%.
    for(unsigned dt:{8u,16u,33u}){
        BagMotion full,noBounce,noRock,zero,maximum;float peak=0;
        for(unsigned t=0;t<6000;t+=dt){
            auto pose=identity;float angle=.05f*std::sin(t*.013f);
            pose[0]=pose[5]=std::cos(angle);pose[1]=std::sin(angle);pose[4]=-pose[1];
            pose[12]=-.25f;pose[14]=1.4f+.02f*std::sin(t*.0176f);
            BagMatrix a,b,r,z,m;
            assert(rigidWeaponPhysics(full,pose,identity,identity,identity,a,77,2,30,t,true,true,0));
            assert(rigidWeaponPhysics(noBounce,pose,identity,identity,identity,b,77,2,30,t,true,true,0,{0,100,100}));
            assert(rigidWeaponPhysics(noRock,pose,identity,identity,identity,r,77,2,30,t,true,true,0,{100,0,100}));
            assert(rigidWeaponPhysics(zero,pose,identity,identity,identity,z,77,2,30,t,true,true,0,{0,0,0}));
            assert(rigidWeaponPhysics(maximum,pose,identity,identity,identity,m,77,2,30,t,true,true,0,{200,200,200}));
            rigid(m,1);assert(difference(z,pose)<.00001f);
            for(unsigned i=0;i<12;++i){assert(std::fabs(b[i]-a[i])<.00001f);assert(std::fabs(r[i]-pose[i])<.00001f);}
            for(unsigned i=12;i<15;++i){assert(std::fabs(b[i]-pose[i])<.00001f);assert(std::fabs(r[i]-a[i])<.00001f);assert(std::fabs(m[i]-pose[i]-2*(a[i]-pose[i]))<.00001f);}
            peak=std::fmax(peak,difference(a,z));
        }
        assert(peak>.01f);
    }
    BagMotion jumpOff,jumpOn,jumpDouble;auto mount=identity;mount[12]=-.25f;mount[14]=1.4f;float jumpPeak=0;
    for(unsigned t=0;t<2000;t+=16){
        BagMatrix off,on,high;
        assert(rigidWeaponPhysics(jumpOff,mount,identity,identity,identity,off,77,2,30,t,false,true,1,{0,0,0}));
        assert(rigidWeaponPhysics(jumpOn,mount,identity,identity,identity,on,77,2,30,t,false,true,1,{0,0,100}));
        assert(rigidWeaponPhysics(jumpDouble,mount,identity,identity,identity,high,77,2,30,t,false,true,1,{0,0,200}));
        assert(difference(off,mount)<.00001f);rigid(on,1);rigid(high,1);
        jumpPeak=std::fmax(jumpPeak,difference(on,off));
        assert(difference(high,off)+.00001f>=difference(on,off));
    }
    assert(jumpPeak>.01f);
    // Real attachment dispatch: native equipped weapons and extra decorations.
    player={0x4100000,0x4101000,123,49,49};weaponContexts[0]={};auto& c=weaponContexts[0];c.parent=player.model;c.unit=player.unit;c.guid=player.guid;c.selection.stowedMask=7;
    const auto scene=0x4102000u;memory[c.parent+0x2c]=scene;matrices[c.parent+0xfc]=identity;matrices[scene+0x9c]=identity;memory[c.unit+0x9e8]=1;
    Lua on{{1}},off{{0}},invalid{{2}};assert(setWeaponPhysicsLua(&off)==1&&setWeaponPhysicsLua(&invalid)==-2);
    Lua tuned{{1,50,125,200}};assert(setWeaponPhysicsLua(&tuned)==1);
    assert(weaponPhysicsSettings.bounce==50&&weaponPhysicsSettings.rocking==125&&weaponPhysicsSettings.jumpLift==200);
    for(double value:{-1.,201.,1.5,std::numeric_limits<double>::infinity(),std::numeric_limits<double>::quiet_NaN()}){
        for(unsigned column=1;column<=3;++column){Lua bad{{1,100,100,100}};bad.values[column]=value;assert(setWeaponPhysicsLua(&bad)==-2);assert(weaponPhysicsSettings.bounce==50&&weaponPhysicsSettings.rocking==125&&weaponPhysicsSettings.jumpLift==200);}
    }
    assert(setWeaponPhysicsLua(&off)==1&&weaponPhysicsSettings.bounce==100&&weaponPhysicsSettings.rocking==100&&weaponPhysicsSettings.jumpLift==100);
    for(unsigned kind:{1u,2u,4u}){
        const WeaponAsset* asset=nullptr;for(const auto& a:weaponAssets)if(a.kind==kind){asset=&a;break;}assert(asset);
        void* child=pointer(0x4200000+kind*0x1000);modelName(child,asset->model);memory[address(child)+0x1cc]=c.parent;memory[address(child)+0x1d0]=30;matrices[address(child)+0xbc]=identity;
        c.nativeChildren={};c.extra={};c.nativeChildren[0]=child;c.routes={{-1,-1,-1}};c.selection.equipped[0]=asset->item;
        auto pose=identity;pose[14]=1.2f;BagMatrix out;
        assert(!positionWeaponPhysics(child,pose.data(),out,true));assert(setWeaponPhysicsLua(&on)==1);
        for(bagTestTime=1000;bagTestTime<3000;bagTestTime+=16){pose[14]=1.2f+.04f*std::sin(bagTestTime*.02f);assert(positionWeaponPhysics(child,pose.data(),out,true));rigid(out,1);}
        assert(c.rigidWeapons[10].motion.ready);
        assert(setWeaponPhysicsLua(&tuned)==1&&c.rigidWeapons[10].motion.ready);
        assert(setWeaponPhysicsLua(&on)==1&&c.rigidWeapons[10].motion.ready);

        // A later native lazy update must not overwrite the physics pose.
        bagTestTime-=16;captureAttachmentMatrix=true;evaluateAttachmentBones=false;
        for(auto caller:{0x718761u,0x71415Du,0x714183u}){
            updateAttachmentForCaller(child,pose.data(),nullptr,nullptr,1,caller);
            assert(difference(capturedAttachmentMatrix,out)<.00001f);
        }
        captureAttachmentMatrix=false;
        // Drawing, hiding and previews cancel history immediately.
        memory[address(child)+0x1d0]=1;assert(!positionWeaponPhysics(child,pose.data(),out,true)&&!c.rigidWeapons[10].motion.ready);
        memory[address(child)+0x1d0]=30;assert(!positionWeaponPhysics(child,pose.data(),out,false));
        c.token=1;assert(!positionWeaponPhysics(child,pose.data(),out,true));c.token=0;
        c.extra[2]=child;c.selection.items[2]=asset->item;c.nativeChildren={};assert(positionWeaponPhysics(child,pose.data(),out,true));
        assert(c.rigidWeapons[2].motion.ready);forgetWeapons(address(child));assert(!c.rigidWeapons[2].child);
        assert(setWeaponPhysicsLua(&off)==1);assert(!positionWeaponPhysics(child,pose.data(),out,true));
    }
    BagMotion bad;BagMatrix out,skew=identity;skew[0]=2;assert(!rigidWeaponPhysics(bad,skew,identity,identity,identity,out,1,1,30,0,true,true,0));
    std::cout<<"PASS: rigid weapon bob/rock, jump, quiet idle, camera/duplicate-frame invariance, native/decorative ownership, drawing, hiding, preview, disable and destruction cleanup\n";
}
