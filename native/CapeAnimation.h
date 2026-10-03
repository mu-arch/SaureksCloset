#pragma once
#include <cstring>
static constexpr const char* calmCapeModel="Interface\\AddOns\\SaureksCloset\\Animations\\HumanFemaleCapeCalm.mdx";
static constexpr const char* calmCapeFile="Interface\\AddOns\\SaureksCloset\\Animations\\HumanFemaleCapeCalm.m2";
static bool capePathEqual(const char* a,const char* b){
    if(!a||!b)return false;
    for(;*a&&*b;++a,++b){auto x=*a,y=*b;if(x>='A'&&x<='Z')x+=32;if(y>='A'&&y<='Z')y+=32;if(x=='/')x='\\';if(y=='/')y='\\';if(x!=y)return false;}
    return !*a&&!*b;
}
static const char* capeAnimationAsset(const char* path){return capePathEqual(path,calmCapeFile)?calmCapeFile:nullptr;}
static const char* capeAnimationModel(const char* original,bool enabled){
    return enabled&&(capePathEqual(original,"Character\\Human\\Female\\HumanFemale.mdx")||capePathEqual(original,"Character\\Human\\Female\\HumanFemale.m2"))?calmCapeModel:original;
}
