#include <cassert>
#include <iostream>
#include "../native/BagPlacement.h"
static const BagMatrix identity{{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1}};
static void near(const BagMatrix& a,const BagMatrix& b,float tolerance=.00012f){
    for(unsigned i=0;i<16;++i)assert(std::fabs(a[i]-b[i])<tolerance);
}
static BagMatrix rotation(float angle){
    auto m=identity;m[0]=std::cos(angle);m[1]=std::sin(angle);m[4]=-m[1];m[5]=m[0];return m;
}
int main(){
    // Exercise identical body animation through radically different moving view
    // transforms, including reflection, orbit, camera pitch, zoom and travel.
    for(unsigned fit=0;fit<16;++fit){
        const auto anchor=bagFits[fit].anchor;
        BagMotion bodyHistory,viewHistory;
        for(unsigned frame=0;frame<240;++frame){
            const float t=frame*.016f;
            auto torso=rotation(.07f*std::sin(t*11));torso[12]=.025f*std::sin(t*19);torso[14]=.02f*std::sin(t*13);
            auto back=torso;
            for(unsigned row=0;row<3;++row)back[12+row]+=torso[row]*anchor[0]+torso[4+row]*anchor[1]+torso[8+row]*anchor[2];
            auto view=rotation(t*4.7f);
            auto pitch=identity;pitch[5]=std::cos(t*2);pitch[6]=std::sin(t*2);pitch[9]=-pitch[6];pitch[10]=pitch[5];
            view=bagMatrixProduct(view,pitch);
            for(unsigned row=0;row<3;++row){view[row]*=-1.3f;view[4+row]*=.7f;view[8+row]*=1.1f;}
            view[12]=40*std::sin(t);view[13]=12*std::cos(t*2);view[14]=-15+t*3;
            auto local=rotation(.35f);for(unsigned i=0;i<12;++i)local[i]*=.8f;local[12]=.15f;local[14]=-.04f;
            BagMatrix plain,rendered;
            const bool running=frame>10&&frame<190;
            const bool airborne=frame>20&&frame<140;
            assert(bagPlacement(back,torso,local,anchor,plain,1,&bodyHistory,frame*16,123,running,&identity,&identity,airborne));
            assert(bagPlacement(bagMatrixProduct(view,back),bagMatrixProduct(view,torso),local,anchor,rendered,1,&viewHistory,frame*16,123,running,&view,&view,airborne));
            near(rendered,bagMatrixProduct(view,plain));
            for(unsigned i=0;i<3;++i)assert(std::fabs(bodyHistory.position[i]-viewHistory.position[i])<.00002f);
            assert(std::fabs(bodyHistory.runWeight-viewHistory.runWeight)<.000001f);
            assert(std::fabs(bodyHistory.verticalOffset-viewHistory.verticalOffset)<.00002f);
            assert(std::fabs(bodyHistory.airborneWeight-viewHistory.airborneWeight)<.000001f);
        }
    }
    // A stationary character remains fixed to the body even when only the view
    // changes; camera movement must not add lag or restart the motion filter.
    const auto anchor=bagFits[2].anchor;auto back=identity;for(unsigned i=0;i<3;++i)back[12+i]=anchor[i];
    BagMotion state;BagMatrix rest;assert(bagPlacement(back,identity,identity,anchor,rest));
    for(unsigned frame=0;frame<100;++frame){
        auto view=rotation(frame*.2f);view[12]=frame*.5f;view[14]=-frame*.3f;
        BagMatrix result;assert(bagPlacement(bagMatrixProduct(view,back),view,identity,anchor,result,1,&state,frame*16,1,false,&view,&view));
        near(result,bagMatrixProduct(view,rest));
    }
    // Root orientation (including an attached/mounted actor) and bag tilt must
    // not tilt the spring's direction. Compare through a second moving camera.
    for(unsigned fit=0;fit<16;++fit)for(float lean:{0.f,.7f,1.570796327f}){
        const auto fitAnchor=bagFits[fit].anchor;
        auto actor=identity;actor[0]=std::cos(lean);actor[2]=-std::sin(lean);actor[8]=std::sin(lean);actor[10]=actor[0];
        const std::array<float,3> up{{-std::sin(lean),0,std::cos(lean)}};
        BagMotion normal,camera;float greatest=0;
        for(unsigned frame=0;frame<200;++frame){
            const float t=frame*.016f,bounce=.045f*std::sin(t*18);
            auto body=rotation(.6f); // The fitted bag is independently tilted.
            for(unsigned row=0;row<3;++row)body[12+row]=up[row]*bounce;
            body[13]+=.06f*std::sin(t*11); // Perpendicular travel is immediate.
            auto attachment=body;
            for(unsigned row=0;row<3;++row)for(unsigned k=0;k<3;++k)attachment[12+row]+=body[k*4+row]*fitAnchor[k];
            actor[12]=frame*.1f;actor[14]=.5f*std::sin(t*3); // Whole-player travel and jumping are not filtered.
            auto view=rotation(t*4);auto pitch=identity;
            pitch[5]=std::cos(t*2);pitch[6]=std::sin(t*2);pitch[9]=-pitch[6];pitch[10]=pitch[5];
            view=bagMatrixProduct(view,pitch);view[12]=8;view[13]=-6;view[14]=12;
            const auto renderedActor=bagMatrixProduct(view,actor);
            BagMatrix result,other,raw;
            assert(bagPlacement(bagMatrixProduct(actor,attachment),bagMatrixProduct(actor,body),identity,fitAnchor,result,1,&normal,frame*16,22,true,&actor,&identity));
            assert(bagPlacement(bagMatrixProduct(renderedActor,attachment),bagMatrixProduct(renderedActor,body),identity,fitAnchor,other,1,&camera,frame*16,22,true,&renderedActor,&view));
            assert(bagPlacement(bagMatrixProduct(actor,attachment),bagMatrixProduct(actor,body),identity,fitAnchor,raw,1,nullptr,0,22,true,&actor,&identity));
            near(other,bagMatrixProduct(view,result));
            assert(std::fabs(result[12]-raw[12])<.00002f&&std::fabs(result[13]-raw[13])<.00002f);
            const float vertical=result[14]-raw[14];assert(std::fabs(vertical)<=1.239f*.45f*.85f*.0401f);
            greatest=std::fmax(greatest,std::fabs(vertical));
        }
        assert(greatest>.003f); // Visible give along ground-up, including a sideways actor.
    }
    // An actor carried through changing root orientation or travel, with no
    // attachment animation, must not acquire bounce from that root transform.
    state={};
    for(unsigned frame=0;frame<120;++frame){
        auto actor=identity;const float tilt=frame*.03f;
        actor[0]=std::cos(tilt);actor[2]=-std::sin(tilt);actor[8]=std::sin(tilt);actor[10]=actor[0];
        actor[12]=frame*.2f;actor[14]=frame*.1f;
        BagMatrix result;
        assert(bagPlacement(bagMatrixProduct(actor,back),actor,identity,anchor,result,1,&state,frame*16,1,false,&actor,&identity));
        near(result,bagMatrixProduct(actor,rest));
    }
    // A tilted actor with different scale on each axis still distinguishes
    // world-horizontal animation from vertical bounce; normalizing the up
    // direction alone is insufficient for the velocity measurement here.
    auto stretched=identity;const float lean=.8f;
    stretched[0]=std::cos(lean)*.6f;stretched[2]=-std::sin(lean)*.6f;
    stretched[8]=std::sin(lean)*1.5f;stretched[10]=std::cos(lean)*1.5f;stretched[5]=1.2f;
    BagMatrix inverseStretched;assert(bagAffineInverse(stretched,inverseStretched));
    for(bool vertical:{false,true}){
        BagMotion moving;float greatest=0;
        for(unsigned frame=0;frame<160;++frame){
            auto body=identity;const float travel=.05f*std::sin(frame*.3f);
            for(unsigned row=0;row<3;++row)body[12+row]=inverseStretched[(vertical?8:0)+row]*travel;
            const auto attachment=bagMatrixProduct(body,back);
            BagMatrix result,raw;
            assert(bagPlacement(bagMatrixProduct(stretched,attachment),bagMatrixProduct(stretched,body),identity,anchor,result,1,&moving,frame*16,1,false,&stretched,&identity));
            assert(bagPlacement(bagMatrixProduct(stretched,attachment),bagMatrixProduct(stretched,body),identity,anchor,raw,1,nullptr,0,1,false,&stretched,&identity));
            assert(std::fabs(result[12]-raw[12])<.00001f&&std::fabs(result[13]-raw[13])<.00001f);
            greatest=std::fmax(greatest,std::fabs(result[14]-raw[14]));
        }
        if(vertical)assert(greatest>.003f);else assert(greatest<.00001f);
    }
    // Missing/invalid direction information keeps the bag rigid and visible.
    BagMatrix invalidScene{},rigidResult;
    assert(bagPlacement(back,identity,identity,anchor,rigidResult,1,&state,2000,1,false,&identity,&invalidScene));
    near(rigidResult,rest);assert(!state.ready);
    auto invalid=identity;invalid[0]=0;BagMatrix out;assert(!bagPlacement(back,identity,identity,anchor,out,1,&state,2000,1,false,&invalid));
    invalid=identity;invalid[3]=.2f;assert(!bagAffineInverse(invalid,out));
    std::cout<<"PASS: world-vertical give across all races, sideways actors, camera/view invariance, reflections, zoom, immediate actor travel and safe missing-scene behavior\n";
}
