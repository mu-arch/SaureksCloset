#pragma once
#include <array>
#include <cstdint>
#include "BodyOptions.h"
#include "RaceModels.h"
struct Appearance {
    unsigned race=1,sex=0,skin=0,face=0,hairStyle=0,hairColor=0,facial=0;
    const RaceModel* model() const {
        for(const auto& m:raceModels)if(m.race==race&&m.sex==sex)return &m;
        return nullptr;
    }
    bool valid() const {
        if(!model())return false;
        bool skinFace=false,hair=false,features=false;
        for(const auto& o:bodyOptions){
            if(o.race!=race||o.sex!=sex)continue;
            if(o.kind==0&&o.a==skin&&o.b==face)skinFace=true;
            if(o.kind==1&&o.a==hairStyle&&o.b==hairColor)hair=true;
            if(o.kind==2&&o.a==facial)features=true;
        }
        return skinFace&&hair&&features;
    }
    bool operator==(const Appearance& b) const {
        return race==b.race&&sex==b.sex&&skin==b.skin&&face==b.face&&hairStyle==b.hairStyle&&hairColor==b.hairColor&&facial==b.facial;
    }
    // Input to CCharacterComponent::Initialize, not a player update-field buffer.
    // Preserve the M2 reference, equipment, geosets, flags and every other word.
    void compose(std::array<std::uint32_t,91>& descriptor) const {
        descriptor[0]=race;descriptor[1]=sex;descriptor[2]=hairColor;
        descriptor[3]=skin;descriptor[5]=face;descriptor[6]=facial;descriptor[7]=hairStyle;
    }
};
inline const RaceModel* nativeModel(unsigned display) {
    for(const auto& m:raceModels)if(m.display==display)return &m;
    return nullptr;
}

inline void scaleBasis(std::array<float,16>& matrix,float ratio) {
    // Row-vector 4x4 basis only; preserve translation and homogeneous entries.
    for(unsigned row=0;row<3;++row)for(unsigned col=0;col<3;++col)matrix[row*4+col]*=ratio;
}
