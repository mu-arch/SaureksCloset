#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
#include "../native/StaffPlacement.h"

static const BagMatrix identity{{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1}};
// Staff X is its long axis. The radial Z axis points into the torso here.
static const BagMatrix sheathed{{0,0,-1,0,0,1,0,0,1,0,0,0,-.4f,.08f,1.1f,1}};
static const std::array<float,4> shaft{{-.02f,.02f,-.03f,.03f}};

static void near(const BagMatrix& actual,const BagMatrix& expected,float tolerance=.00008f){
    for(unsigned i=0;i<16;++i)assert(std::fabs(actual[i]-expected[i])<tolerance);
}
static void preservesPose(const BagMatrix& actual,const BagMatrix& original){
    for(unsigned i=0;i<12;++i)assert(std::fabs(actual[i]-original[i])<.00008f);
    assert(std::fabs(actual[15]-original[15])<.00001f);
}
static BagMatrix shifted(BagMatrix matrix,float x,float y=0,float z=0){
    matrix[12]+=x;matrix[13]+=y;matrix[14]+=z;return matrix;
}
static BagMatrix turn(float angle){
    auto matrix=identity;matrix[0]=std::cos(angle);matrix[1]=std::sin(angle);
    matrix[4]=-matrix[1];matrix[5]=matrix[0];return matrix;
}
static BagMatrix pitch(float angle){
    auto matrix=identity;matrix[5]=std::cos(angle);matrix[6]=std::sin(angle);
    matrix[9]=-matrix[6];matrix[10]=matrix[5];return matrix;
}
static bool fit(const BagMatrix& attachment,const BagMatrix& local,const BagMatrix& torso,
                const BagMatrix& view,float depth,const std::array<float,4>& bounds,BagMatrix& out){
    return staffContactPlacement(attachment,local,torso,view,depth,bounds,out);
}

int main(){
    BagMatrix out;
    // Ordinary identity is a valid orientation: the radial section has no
    // inward extent when staff X itself points into the body.
    assert(fit(identity,identity,identity,identity,.08f,shaft,out));
    near(out,shifted(identity,.068f));preservesPose(out,identity);

    // A sheathed staff's radius, rather than its long-axis bounding box, sets
    // contact. Leave exactly 0.012 model units between its shaft and the body.
    assert(fit(sheathed,identity,identity,identity,.08f,shaft,out));
    near(out,shifted(sheathed,.038f));preservesPose(out,sheathed);
    assert(std::fabs((out[12]-sheathed[12])+shaft[3]+.012f-.08f)<.00001f);

    // Use asymmetric section bounds and the correct inward-facing side,
    // including reflected staff axes; do not substitute a centered radius.
    const std::array<float,4> asymmetric{{-.01f,.025f,-.015f,.04f}};
    assert(fit(sheathed,identity,identity,identity,.08f,asymmetric,out));
    near(out,shifted(sheathed,.028f));
    auto reversed=sheathed;
    for(unsigned row=0;row<3;++row)reversed[8+row]*=-1;
    assert(fit(reversed,identity,identity,identity,.08f,asymmetric,out));
    near(out,shifted(reversed,.053f));preservesPose(out,reversed);

    // An inward shift is bounded, and cannot push a staff outward when the
    // authored pose already touches or intersects the requested surface.
    assert(fit(sheathed,identity,identity,identity,.5f,shaft,out));
    near(out,shifted(sheathed,.12f));
    for(float depth:{.042f,.03f,.001f,0.f,-.01f})
        assert(!fit(sheathed,identity,identity,identity,depth,shaft,out));

    // Normalize the torso direction in model space. Bone scale and torso
    // translation must not scale the physical gap or shift the target origin.
    auto torso=identity;torso[0]=2;torso[5]=.7f;torso[10]=1.5f;
    torso[12]=12;torso[13]=-5;torso[14]=3;
    assert(fit(sheathed,identity,torso,identity,.08f,shaft,out));
    near(out,shifted(sheathed,.038f));

    // The child factory's local offset and scale are part of the shaft's
    // physical contact. Keep that local pose and attachment scale intact.
    auto local=identity;local[0]=1.8f;local[5]=.7f;local[10]=2;
    local[12]=.2f;local[13]=-.07f;local[14]=.01f;
    assert(fit(sheathed,local,identity,identity,.11f,shaft,out));
    near(out,shifted(sheathed,.028f));preservesPose(out,sheathed);
    const auto finalChild=bagMatrixProduct(out,local);
    const auto originalChild=bagMatrixProduct(sheathed,local);
    near(finalChild,shifted(originalChild,.028f));

    // Local rotation can move either radial axis toward the player. The
    // maximum transformed corner, including local translation, sets support.
    auto rotatedLocal=identity;
    rotatedLocal[5]=0;rotatedLocal[6]=1.5f;
    rotatedLocal[9]=-.7f;rotatedLocal[10]=0;
    rotatedLocal[12]=.2f;rotatedLocal[13]=-.06f;rotatedLocal[14]=.008f;
    const std::array<float,4> rotatedBounds{{-.01f,.02f,-.03f,.04f}};
    assert(fit(sheathed,rotatedLocal,identity,identity,.1f,rotatedBounds,out));
    near(out,shifted(sheathed,.05f));preservesPose(out,sheathed);

    // Both radial axes can project inward at once; contact is the supporting
    // corner of their transformed section, not the larger individual radius.
    auto diagonalLocal=pitch(.6f);diagonalLocal[14]=.008f;
    const float diagonalShift=.1f-.012f-.008f-.025f*std::sin(.6f)-.04f*std::cos(.6f);
    assert(fit(sheathed,diagonalLocal,identity,identity,.1f,asymmetric,out));
    near(out,shifted(sheathed,diagonalShift));preservesPose(out,sheathed);

    // Attachment scale participates in radial support, but the output retains
    // precisely that scale. The contact correction only changes translation.
    auto scaled=sheathed;
    for(unsigned row=0;row<3;++row){scaled[row]*=.8f;scaled[4+row]*=1.3f;scaled[8+row]*=2;}
    assert(fit(scaled,identity,identity,identity,.1f,shaft,out));
    near(out,shifted(scaled,.028f));preservesPose(out,scaled);

    // Model animation may rotate the body's inward direction. Camera orbit,
    // pitch, reflection, nonuniform zoom, and travel must not change the fit.
    for(unsigned frame=0;frame<180;++frame){
        const float time=frame*.037f;
        auto body=bagMatrixProduct(turn(time),pitch(.45f*std::sin(time)));
        body[12]=.05f*std::cos(time);body[13]=.08f*std::sin(time);body[14]=.02f*time;
        const auto attachment=bagMatrixProduct(body,sheathed);
        BagMatrix plain;
        assert(fit(attachment,rotatedLocal,body,identity,.1f,rotatedBounds,plain));
        near(plain,shifted(attachment,.05f*body[0],.05f*body[1],.05f*body[2]));
        preservesPose(plain,attachment);

        auto view=bagMatrixProduct(turn(time*2.7f),pitch(time*.9f));
        for(unsigned row=0;row<3;++row){view[row]*=-1.7f;view[4+row]*=.6f;view[8+row]*=2.2f;}
        view[12]=25*std::sin(time);view[13]=-17*std::cos(time);view[14]=-8+time*3;
        const auto rendered=bagMatrixProduct(view,attachment);
        const auto renderedTorso=bagMatrixProduct(view,body);
        assert(fit(rendered,rotatedLocal,renderedTorso,view,.1f,rotatedBounds,out));
        near(out,bagMatrixProduct(view,plain));preservesPose(out,rendered);
        const auto first=out;
        for(unsigned repeat=0;repeat<10;++repeat){
            assert(fit(rendered,rotatedLocal,renderedTorso,view,.1f,rotatedBounds,out));
            near(out,first); // Repeated renders use the authored pose, never accumulate.
        }
    }

    // Do not claim a valid placement from corrupt asset bounds, invalid body
    // depth, non-affine transforms, missing bones or singular scene matrices.
    const float nan=std::numeric_limits<float>::quiet_NaN();
    const float infinity=std::numeric_limits<float>::infinity();
    for(float invalid:{nan,infinity,-infinity}){
        assert(!fit(sheathed,identity,identity,identity,invalid,shaft,out));
        for(unsigned axis=0;axis<4;++axis){
            auto bounds=shaft;bounds[axis]=invalid;
            assert(!fit(sheathed,identity,identity,identity,.08f,bounds,out));
        }
        for(unsigned which=0;which<4;++which)for(unsigned coefficient=0;coefficient<16;++coefficient){
            BagMatrix matrices[4]={sheathed,identity,identity,identity};
            matrices[which][coefficient]=invalid;
            assert(!fit(matrices[0],matrices[1],matrices[2],matrices[3],.08f,shaft,out));
        }
    }
    for(unsigned first:{0u,2u}){
        auto bounds=shaft;bounds[first]=bounds[first+1]+.01f;
        assert(!fit(sheathed,identity,identity,identity,.08f,bounds,out));
    }
    for(unsigned which=0;which<4;++which){
        for(unsigned coefficient:{3u,7u,11u,15u}){
            BagMatrix matrices[4]={sheathed,identity,identity,identity};
            matrices[which][coefficient]+=.1f;
            assert(!fit(matrices[0],matrices[1],matrices[2],matrices[3],.08f,shaft,out));
        }
        BagMatrix matrices[4]={sheathed,identity,identity,identity};
        for(unsigned row=0;row<3;++row)matrices[which][row]=0;
        assert(!fit(matrices[0],matrices[1],matrices[2],matrices[3],.08f,shaft,out));
    }
    std::cout<<"PASS: staff shaft contact, bounded inward-only correction, local factory transforms, preserved pose, camera/view invariance, no accumulation and invalid-input rejection\n";
}
