#pragma once
#include <chrono>
// Main-thread capture only. Baking happens from Lua after composition has
// settled, never inside a render hook and never on shared live vertex buffers.
namespace hairMaskRuntime {
struct Identity {unsigned race=0,sex=0,style=0,group=0,display=0;std::uint64_t owner=0;
    bool operator==(const Identity& b)const{return race==b.race&&sex==b.sex&&style==b.style&&group==b.group&&display==b.display&&owner==b.owner;}};
inline bool requested=false,ready=false;
inline std::uintptr_t requestedModel=0;
inline Identity captured{},applied{};
inline BagTuningValues capturedFit{};
inline bool capturedFitEnabled=false;
inline hairMask::Envelope envelope;
inline std::vector<hairMask::Point> hatVertices;
inline BagMatrix modelToHat{};
inline hairMask::Options options=hairMask::defaults(),appliedOptions=hairMask::defaults();
inline std::string appliedCape;
inline unsigned appliedMode=0;
inline hairMask::PlaneOptions appliedPlane=hairMask::planeDefaults();
inline unsigned changedTriangles=0,generation=0,captureRevision=0,appliedRevision=0;
inline std::chrono::steady_clock::time_point loadDeadline;
inline int loadedStatus(const Player& p){
    std::uintptr_t resource=0;unsigned loaded=0;std::array<char,260> name{};
    if(read(p.model+0x10,loaded)&&loaded&&read(p.model+0x30,resource)&&resource&&read(resource+0x20,name)){
        std::string expected=hairMask::activePath;if(expected.size()>3)expected.resize(expected.size()-3);
        if(std::find(name.begin(),name.end(),char(0))!=name.end()&&capeMotion::pathEqual(name.data(),expected.c_str()))return 1;
    }
    return std::chrono::steady_clock::now()<loadDeadline?3:-6;
}
inline int reply(void* L,int status){pushNumber(L,status);pushNumber(L,changedTriangles);pushNumber(L,generation);return 3;}
inline bool identity(Player& p,Identity& id){
    unsigned dirty=1,row=0;unsigned char initializing=1;
    if(!snapshot(p)||p.display!=p.native||!nativeModel(p.native)||!p.component||!p.model||
       !read(p.component+0x10,dirty)||dirty||!read(p.component+8,initializing)||initializing||
       !read(p.component+0x18,id.race)||!read(p.component+0x1c,id.sex)||!read(p.component+0x34,id.style)||
       !read(p.component+0x4a8,row))return false;
    id.owner=p.guid;id.group=hairGroup(reinterpret_cast<void*>(p.component+0x18));
    if(row&&!read(row,id.display))return false;return id.race>=1&&id.race<=8&&id.sex<=1;
}
inline bool sameFit(const BagTuningValues& a,const BagTuningValues& b){return a.left==b.left&&a.inset==b.inset&&a.up==b.up&&a.pitch==b.pitch&&a.roll==b.roll&&a.yaw==b.yaw&&a.scale==b.scale;}
inline void capture(void* child,const float* input){
    if(!requested||ready)return;
    std::uintptr_t parent=0,resource=0;unsigned point=0,loaded=0;const auto m=reinterpret_cast<std::uintptr_t>(child);
    if(!read(m+0x1cc,parent)||parent!=requestedModel||!read(m+0x1d0,point)||point!=11)return;
    Player p;Identity id;if(!identity(p,id)||!id.display||parent!=p.model)return;
    if(!read(m+0x10,loaded)||!loaded||!read(m+0x30,resource)||!resource)return;
    const auto& fit=hatTuningEntries[(id.race-1)*2+id.sex];const bool fitted=bagTuningOwner==p.guid&&fit.enabled;
    if(ready&&id==captured&&fitted==capturedFitEnabled&&(!fitted||sameFit(fit.values,capturedFit)))return;
    std::array<char,260> path{};if(!read(resource+0x20,path))return;for(auto& c:path)c=armorPathCharacter(c);if(!armorPathPrefix(path,"item\\objectcomponents\\head\\"))return;
    std::uintptr_t bodyResource=0,bodyHeader=0,lookup=0,attachments=0,palette=0,header=0,vertices=0;
    unsigned nlookup=0,nattach=0,nv=0,nb=0;std::uint16_t index=0,bone=0;
    if(!read(p.model+0x30,bodyResource)||!read(bodyResource+0x130,bodyHeader)||!read(bodyHeader+0x10c,nlookup)||nlookup<=11||nlookup>512||!read(bodyHeader+0x110,lookup)||!read(lookup+22,index)||
       !read(bodyHeader+0x104,nattach)||nattach>512||index>=nattach||!read(bodyHeader+0x108,attachments)||!read(attachments+48*index+4,bone)||!read(bodyHeader+0x34,nb)||bone>=nb||nb>2048||!read(p.model+0x94,palette))return;
    BagMatrix head,headInverse,hat,hatToRest;
    if(!read(palette+64*bone,head)||!bagAffineInverse(head,headInverse)||!read(reinterpret_cast<std::uintptr_t>(input),hat))return;
    if(fitted){BagMatrix adjusted;if(!hatPlacement(hat,fit.values,adjusted))return;hat=adjusted;}
    hatToRest=bagMatrixProduct(headInverse,hat);if(!bagAffineInverse(hatToRest,modelToHat))return;
    if(!read(resource+0x130,header)||!read(header+0x44,nv)||nv<3||nv>65535||!read(header+0x48,vertices))return;
    std::vector<hairMask::Point> points;points.reserve(nv);
    for(unsigned i=0;i<nv;++i){hairMask::Point v;if(!read(vertices+48*i,v))return;for(auto n:v)if(!std::isfinite(n))return;points.push_back(v);}
    hatVertices.swap(points);captured=id;capturedFit=fit.values;capturedFitEnabled=fitted;ready=true;++captureRevision;
}
inline void clear(Player& p){if(hairMask::activePath.empty())return;hairMask::activePath.clear();applied={};++generation;if(p.model&&p.display==p.native)refresh(p,true);}
}
static int __fastcall setHairMaskLua(void* L){
    using namespace hairMaskRuntime;
    if(!isNumber(L,1))return result(L,-2);const double on=toNumber(L,1);if(on!=0&&on!=1)return result(L,-2);
    auto next=hairMask::defaults();for(unsigned i=0;i<4;++i){if(!isNumber(L,i+2))return result(L,-2);const auto v=toNumber(L,i+2);if(!std::isfinite(v)||v<0||v>150||v!=std::floor(v))return result(L,-2);next[i]=unsigned(v);}if(!hairMask::valid(next))return result(L,-2);
    unsigned mode=0;auto plane=hairMask::planeDefaults();
    if(isNumber(L,6)){const auto v=toNumber(L,6);if(v!=0&&v!=1)return result(L,-2);mode=unsigned(v);}
    if(mode){for(unsigned i=0;i<3;++i){if(!isNumber(L,7+i))return result(L,-2);const auto v=toNumber(L,7+i);if(!std::isfinite(v)||v< -80||v>100||v!=std::floor(v))return result(L,-2);plane[i]=int(v);}if(!hairMask::validPlane(plane))return result(L,-2);}
    Player p;if(!snapshot(p)||state.busy)return result(L,-1);
    requestedModel=p.model;
    requested=on==1&&haircraft::owner==p.guid&&haircraft::enabled;
    if(!requested){ready=false;clear(p);return reply(L,1);}
    Identity id;if(!identity(p,id))return reply(L,0);
    if(!id.display){ready=false;clear(p);return reply(L,2);}
    const auto& fit=hatTuningEntries[(id.race-1)*2+id.sex];const bool fitted=bagTuningOwner==p.guid&&fit.enabled;
    if(!ready||!(captured==id)||fitted!=capturedFitEnabled||(fitted&&!sameFit(capturedFit,fit.values))){ready=false;clear(p);return reply(L,0);}
    const auto* ordinary=raceModels[(id.race-1)*2+id.sex].filename;
    const auto* cape=localCapeMotionModel(ordinary);const std::string capeKey=cape;
    if(applied==id&&appliedOptions==next&&appliedMode==mode&&appliedPlane==plane&&appliedCape==capeKey&&appliedRevision==captureRevision&&!hairMask::activePath.empty())return reply(L,loadedStatus(p));
    if(!(mode?hairMask::cuttingPlane(hatVertices,modelToHat,plane,envelope):hairMask::envelope(hatVertices,modelToHat,next,envelope)))return result(L,-3);
    char base[128];std::snprintf(base,sizeof(base),"Interface/AddOns/SaureksCloset/CapeMotion/B%02u.m2",(id.race-1)*2+id.sex+1);
    hairMask::Bytes source,baked;if(!capeMotion::readFile(capeMotion::pathEqual(cape,ordinary)?base:cape,source)||!hairMask::build(source,id.group,envelope,baked,changedTriangles))return result(L,-5);
    const auto* path=hairMask::save(baked);if(!path)return result(L,-5);
    const bool changed=hairMask::activePath!=path;
    hairMask::activePath=path;hairMask::activeOwner=p.guid;hairMask::activeBody=(id.race-1)*2+id.sex+1;hairMask::activeStyle=id.style;
    applied=id;appliedOptions=next;appliedMode=mode;appliedPlane=plane;appliedCape=capeKey;options=next;appliedRevision=captureRevision;if(changed){++generation;loadDeadline=std::chrono::steady_clock::now()+std::chrono::seconds(5);refresh(p,true);}
    Player current;if(snapshot(current))return reply(L,loadedStatus(current));
    return reply(L,1);
}

static int __fastcall hairMaskVersionLua(void* L){return result(L,2);}
