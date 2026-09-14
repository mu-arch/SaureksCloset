#include <cassert>
#include <iostream>
#include <limits>
#include "../native/BagPlacement.h"
static const BagMatrix identity{{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1}};
static void near(const BagMatrix& a,const BagMatrix& b,float tolerance=.0001f){
    for(unsigned i=0;i<16;++i)assert(std::fabs(a[i]-b[i])<tolerance);
}
static BagMatrix placed(unsigned fit,BagMotion* motion=nullptr,unsigned time=0,bool running=false,
                        BagMatrix body=identity,BagMatrix view=identity){
    auto back=body;const auto anchor=bagFits[fit].anchor;
    for(unsigned row=0;row<3;++row)for(unsigned k=0;k<3;++k)back[12+row]+=body[k*4+row]*anchor[k];
    BagMatrix out;
    assert(bagPlacement(bagMatrixProduct(view,back),bagMatrixProduct(view,body),identity,anchor,out,
        1,motion,time,123,running,&view));
    return out;
}
int main(){
    std::array<BagMatrix,16> baseline;
    for(unsigned fit=0;fit<16;++fit)baseline[fit]=placed(fit);
    for(unsigned fit=0;fit<16;++fit){
        const unsigned race=fit/2+1,sex=fit%2;
        BagTuningValues values;assert(bagTuningDefaults(1,race,sex,values));
        assert(bagTuningSet(1,race,sex,true,values));
        near(placed(fit),baseline[fit]); // Defaults are identical to shipped fits.
        values.left+=.1f;values.inset+=.05f;values.up-=.03f;
        assert(bagTuningSet(1,race,sex,true,values));
        auto moved=baseline[fit];moved[12]+=.05f;moved[13]+=.1f;moved[14]-=.03f;
        near(placed(fit),moved);
        for(unsigned other=0;other<16;++other)if(other!=fit)near(placed(other),baseline[other]);
        const auto movedCenter=placed(fit);
        for(float size:{25.f,85.f,100.f,200.f}){
            values.scale=size;assert(bagTuningSet(1,race,sex,true,values));const auto resized=placed(fit);
            for(unsigned row=0;row<3;++row)assert(resized[12+row]==movedCenter[12+row]);
            for(unsigned col:{0u,4u,8u}){
                float norm=0;for(unsigned row=0;row<3;++row)norm+=resized[col+row]*resized[col+row];
                assert(std::fabs(std::sqrt(norm)-.45f*(size/100))<.000001f);
            }
        }
        values.pitch=values.roll=values.yaw=0;values.scale=100;
        assert(bagTuningSet(1,race,sex,true,values));const auto flat=placed(fit);
        for(unsigned axis=0;axis<3;++axis){
            auto rotated=values;(axis==0?rotated.pitch:(axis==1?rotated.roll:rotated.yaw))=90;
            assert(bagTuningSet(1,race,sex,true,rotated));auto expected=flat;
            if(axis==0){expected[0]=0;expected[2]=.45;expected[8]=-.45;expected[10]=0;}
            if(axis==1){expected[5]=0;expected[6]=-.45;expected[9]=.45;expected[10]=0;}
            if(axis==2){expected[0]=0;expected[1]=.45;expected[4]=-.45;expected[5]=0;}
            near(placed(fit),expected); // All rotations pivot about bag center.
        }
        assert(bagTuningSet(1,race,sex,false));near(placed(fit),baseline[fit]);
    }
    BagTuningValues values;assert(bagTuningDefaults(1,2,0,values));
    BagMotion history;placed(2,&history,1000);
    auto body=identity;body[12]=.008f;placed(2,&history,1016,false,body);
    assert(history.position[0]==placed(2,nullptr,0,false,body)[12]);
    values.left+=.005f;assert(bagTuningSet(1,2,0,true,values));
    near(placed(2,&history,1032,false,body),placed(2,nullptr,0,false,body)); // Immediate edit; no old-pose lag.
    const auto fitKey=history.fit;
    assert(bagTuningSet(1,2,0,true,values));body[12]+=.008f;
    const auto anchored=placed(2,&history,1048,false,body);
    assert(history.fit==fitKey);near(anchored,placed(2,nullptr,0,false,body)); // Translation is always immediate.
    const float turn=.025f;
    body[0]=std::cos(turn);body[1]=std::sin(turn);body[4]=-body[1];body[5]=body[0];
    placed(2,&history,1056,false,body);
    auto uninterrupted=history;
    assert(bagTuningSet(1,2,0,true,values));
    near(placed(2,&history,1060,false,body),placed(2,&uninterrupted,1060,false,body)); // Idempotent setter keeps angular history.
    auto unrelated=values;unrelated.up+=.1f;assert(bagTuningSet(1,2,1,true,unrelated));
    body[12]+=.008f;
    near(placed(2,&history,1064,false,body),placed(2,&uninterrupted,1064,false,body));assert(history.fit==fitKey);
    values.motion=false;assert(bagTuningSet(1,2,0,true,values));
    for(unsigned i=0;i<30;++i){body[12]+=.005f;near(placed(2,&history,1080+i*16,true,body),placed(2,nullptr,0,false,body));assert(!history.ready);}
    values.motion=true;assert(bagTuningSet(1,2,0,true,values));
    near(placed(2,&history,1600,true,body),placed(2,nullptr,0,false,body));
    assert(history.ready&&history.runWeight==0);
    // Tuned rotations/offsets obey animated torso axes, then the current view.
    values.pitch=27;values.roll=-14;values.yaw=33;values.scale=73;
    assert(bagTuningSet(1,2,0,true,values));
    BagMotion bodyHistory,viewHistory;
    for(unsigned frame=0;frame<100;++frame){
        body=identity;const float turn=.06f*std::sin(frame*.2f);
        body[0]=std::cos(turn);body[1]=std::sin(turn);body[4]=-body[1];body[5]=body[0];body[12]=.02f*std::sin(frame*.4f);
        auto view=identity;const float angle=frame*.13f;
        view[0]=-std::cos(angle)*1.2f;view[1]=-std::sin(angle)*1.2f;view[4]=-std::sin(angle)*.8f;view[5]=std::cos(angle)*.8f;
        view[12]=frame*.5f;view[14]=-static_cast<float>(frame)*.2f;
        near(placed(2,&viewHistory,frame*16,true,body,view),bagMatrixProduct(view,placed(2,&bodyHistory,frame*16,true,body)));
    }
    const auto revision=bagTuningEntries[2].revision;
    assert(!bagTuningSet(2,2,0,true,values)&&!bagTuningSet(1,0,0,true,values)&&!bagTuningSet(1,9,0,true,values)&&!bagTuningSet(1,2,2,true,values));
    auto invalid=values;invalid.scale=std::numeric_limits<float>::infinity();assert(!bagTuningSet(1,2,0,true,invalid));
    assert(bagTuningEntries[2].revision==revision);
    bagTuningUseOwner(1);assert(!bagTuningEntries[2].enabled&&!bagTuningEntries[3].enabled);
    near(placed(2),baseline[2]);
    std::cout<<"PASS: bag tuning defaults, all race/sex isolation, position, center-pivot rotation/scale, immediate edits, idempotence, pause/resume and camera invariance\n";
}
