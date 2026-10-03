#pragma once
#include <cstring>
#include "RaceModels.h"
#include "AnimationStyles.h"
static constexpr const char* calmCapeModel="Interface\\AddOns\\SaureksCloset\\Animations\\HumanFemaleCapeCalm.mdx";
static constexpr const char* calmCapeFile="Interface\\AddOns\\SaureksCloset\\Animations\\HumanFemaleCapeCalm.m2";
static bool capePathEqual(const char* a,const char* b){
    if(!a||!b)return false;
    for(;*a&&*b;++a,++b){auto x=*a,y=*b;if(x>='A'&&x<='Z')x+=32;if(y>='A'&&y<='Z')y+=32;if(x=='/')x='\\';if(y=='/')y='\\';if(x!=y)return false;}
    return !*a&&!*b;
}
static const char* capeAnimationAsset(const char* path){return capePathEqual(path,calmCapeFile)?calmCapeFile:animationStyles::cachePath(path)?path:nullptr;}
static const char* capeAnimationModel(const char* original,bool enabled){
    return enabled&&(capePathEqual(original,"Character\\Human\\Female\\HumanFemale.mdx")||capePathEqual(original,"Character\\Human\\Female\\HumanFemale.m2"))?calmCapeModel:original;
}

static unsigned animationRecipient(const char* original){
    for(unsigned i=0;i<16;++i){
        if(capePathEqual(original,raceModels[i].filename))return i+1;
        std::string m2=raceModels[i].filename;m2.replace(m2.size()-3,3,"m2");if(capePathEqual(original,m2.c_str()))return i+1;
    }
    return 0;
}
static const char* styledAnimationModel(const char* original,unsigned body,unsigned cape,bool calm){
    if(!body&&!cape)return capeAnimationModel(original,calm);
    const auto recipient=animationRecipient(original);if(!recipient)return original;
    const auto* file=animationStyles::prepare(recipient,body,cape,calm);return file?file:original;
}
