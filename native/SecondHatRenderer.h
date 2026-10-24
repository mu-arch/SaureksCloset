#pragma once
#include <string>
// Uses the same build-5875 DBC fields and path rules as 4799A0. No inventory,
// compositor equipment, geoset masks or real head-slot records are changed.
static bool secondHatText(std::uintptr_t address,std::string& output){
    output.clear();
    for(unsigned i=0;i<128;++i){char c=0;if(!read(address+i,c))return false;
        if(!c)return !output.empty();
        if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_'||c=='.'))return false;
        output+=c;
    }
    return false;
}
static std::uintptr_t secondHatRow(std::uintptr_t dbc,unsigned id){
    std::uintptr_t table=0,row=0;unsigned maximum=0,actual=0;
    return read(dbc,table)&&table&&read(dbc+4,maximum)&&id<=maximum&&
        read(table+4*id,row)&&row&&read(row,actual)&&actual==id?row:0;
}
static bool secondHatPaths(unsigned display,unsigned race,unsigned sex,std::string& model,std::string& texture){
    if(race<1||race>8||sex>1)return false;
    const auto row=secondHatRow(0xC0DC10,display),raceRow=secondHatRow(0xC0DEE0,race);
    std::uintptr_t name=0,skin=0,code=0;std::string stem,color,body;
    if(!row||!raceRow||!read(row+4,name)||!read(row+12,skin)||!read(raceRow+24,code)||
       !secondHatText(name,stem)||!secondHatText(skin,color)||!secondHatText(code,body)||body.size()!=2||
       stem.compare(0,5,"Helm_")!=0)return false;
    const auto dot=stem.find('.');if(dot==std::string::npos)return false;
    const auto extension=stem.substr(dot);
    if(extension!=".mdx"&&extension!=".m2")return false;
    stem.resize(dot);
    model="Item\\ObjectComponents\\Head\\"+stem+"_"+body+(sex?"F":"M")+".mdx";
    texture="Item\\ObjectComponents\\Head\\"+color+".blp";
    return model.size()<260&&texture.size()<260;
}
static void* cloneWithoutSecondHat(void* scene,void* source,unsigned flags){
    auto* context=weaponContext(reinterpret_cast<std::uintptr_t>(source));
    auto* hat=context?context->secondHat:nullptr;
    std::uintptr_t parent=0;
    if(!hat||!read(reinterpret_cast<std::uintptr_t>(hat)+0x1CC,parent)||parent!=reinterpret_cast<std::uintptr_t>(source))
        return cloneModelOriginal(scene,source,flags);
    // Addon previews dress their own second hat after Undress. Avoid inheriting
    // a duplicate, while stock Character-window clones still show both hats.
    retainChild(hat);detachChild(hat);
    auto* copy=cloneModelOriginal(scene,source,flags);
    attachChild(hat,source,11);releaseModel(hat);
    return copy;
}
static int __fastcall secondHatVersionLua(void* L){return result(L,1);}
static int __fastcall setSecondHatLua(void* L){
    unsigned token=0,display=0;
    if(!bagLuaUnsigned(L,1,2147483647,token)||!bagLuaUnsigned(L,2,2147483647,display))return result(L,-2);
    Player p;if(!snapshot(p))return result(L,-1);
    std::uintptr_t parent=p.model,component=p.component;
    if(token){
        parent=0;component=0;
        for(const auto& e:previews.entries)if(e.token==token&&e.guid==p.guid&&e.status==1){parent=e.model;component=e.component;}
        if(!parent)return result(L,-1);
    }else if(p.display!=p.native){
        if(auto* c=weaponContext(parent)){releaseSecondHat(*c);c->secondHatDisplay=0;}
        return result(L,0);
    }
    unsigned race=0,sex=0;std::string model,texture;
    if(display&&(!component||!read(component+0x18,race)||!read(component+0x1C,sex)||
        !secondHatPaths(display,race,sex,model,texture)))return result(L,-2);
    auto* c=weaponContext(parent);
    if(!c&&!display)return result(L,1);
    unsigned loaded=0;if(!parent||!read(parent+0x10,loaded)||!loaded)return result(L,0);
    if(!c){
        for(auto& entry:weaponContexts)if(!entry.parent){c=&entry;break;}
        if(!c)return result(L,-1);
        c->parent=parent;c->guid=p.guid;c->unit=token?0:p.unit;c->token=token;
    }
    if(c->guid!=p.guid||c->token!=token)return result(L,-1);
    if(c->secondHatDisplay!=display||c->secondHatRace!=race||c->secondHatSex!=sex){
        releaseSecondHat(*c);c->secondHatDisplay=display;c->secondHatRace=race;c->secondHatSex=sex;
    }
    if(!display){releaseSecondHat(*c);return result(L,1);}
    if(!hasPoint(reinterpret_cast<void*>(parent),11))return result(L,0);
    if(c->secondHat){
        std::uintptr_t owner=0;
        if(!read(reinterpret_cast<std::uintptr_t>(c->secondHat)+0x1CC,owner))return result(L,0);
        if(!owner)attachChild(c->secondHat,reinterpret_cast<void*>(parent),11);
        return result(L,(owner==0||owner==parent)&&read(reinterpret_cast<std::uintptr_t>(c->secondHat)+0x10,loaded)&&loaded?1:0);
    }
    std::uintptr_t before=0,after=0;read(parent+0x1DC,before);
    const auto previous=loadingExtraParent;loadingExtraParent=parent;
    loadChild(reinterpret_cast<void*>(parent),11,model.c_str(),texture.c_str(),0);
    loadingExtraParent=previous;read(parent+0x1DC,after);
    unsigned point=0;std::uintptr_t owner=0;
    if(after&&after!=before&&read(after+0x1CC,owner)&&owner==parent&&read(after+0x1D0,point)&&point==11){
        c->secondHat=reinterpret_cast<void*>(after);retainChild(c->secondHat);
        return result(L,read(after+0x10,loaded)&&loaded?1:0);
    }
    return result(L,0);
}
