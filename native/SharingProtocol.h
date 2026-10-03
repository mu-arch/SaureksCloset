#pragma once
#include <array>
#include <cstdint>
#include <vector>
#include <string>
#include <algorithm>
#include <map>
namespace sharing {
constexpr unsigned maxVisible=256,maxMessage=8192,appearanceSize=364;
constexpr std::uint32_t inherit=0xffffffffu;
using Bytes=std::vector<unsigned char>;
inline void put(Bytes& b,std::uint64_t v,unsigned n){for(unsigned i=0;i<n;++i)b.push_back(static_cast<unsigned char>(v>>(8*i)));}
inline std::uint64_t get(const unsigned char* p,unsigned n){std::uint64_t v=0;for(unsigned i=0;i<n;++i)v|=std::uint64_t(p[i])<<(8*i);return v;}
using Fit=std::array<std::int16_t,7>;
inline bool validFit(const Fit& f){return f[0]>=-10000&&f[0]<=10000&&f[1]>=-10000&&f[1]<=10000&&f[2]>=-30000&&f[2]<=10000&&f[3]>=-18000&&f[3]<=18000&&f[4]>=-18000&&f[4]<=18000&&f[5]>=-18000&&f[5]<=18000&&f[6]>=2500&&f[6]<=20000;}
struct Bag{unsigned model=0,mount=0,slot=0;Fit fit{};bool operator==(const Bag& b)const{return model==b.model&&mount==b.mount&&slot==b.slot&&fit==b.fit;}};
struct Look{
    std::array<unsigned char,8> body{};
    std::array<std::uint32_t,19> items{};
    unsigned char stowed=7;
    unsigned flags=0,fitMask=0;
    std::array<std::uint32_t,7> carried{};
    std::array<Fit,10> fits{};
    std::array<Bag,5> bags{};
    Look(){items.fill(inherit);}
    bool operator==(const Look& b)const{return body==b.body&&items==b.items&&stowed==b.stowed&&flags==b.flags&&fitMask==b.fitMask&&carried==b.carried&&fits==b.fits&&bags==b.bags;}
    Bytes encode()const{Bytes b(body.begin(),body.end());for(auto id:items)put(b,id,4);b.push_back(stowed);b.insert(b.end(),3,0);put(b,flags,4);for(auto id:carried)put(b,id,4);put(b,fitMask,4);
        for(const auto& fit:fits)for(auto v:fit)put(b,static_cast<std::uint16_t>(v),2);
        for(const auto& bag:bags){put(b,bag.model,2);b.push_back(static_cast<unsigned char>(bag.mount));b.push_back(static_cast<unsigned char>(bag.slot));put(b,0,2);for(auto v:bag.fit)put(b,static_cast<std::uint16_t>(v),2);}return b;}
    static bool decode(const unsigned char* b,unsigned n,Look& out){
        if(n!=appearanceSize||b[0]>1||b[2]>1||(b[0]&&(b[1]<1||b[1]>8))||b[84]>7||b[85]||b[86]||b[87])return false;
        Look value;std::copy(b,b+8,value.body.begin());
        for(unsigned i=0;i<19;++i){auto id=static_cast<std::uint32_t>(get(b+8+4*i,4));if(id!=inherit&&id>1000000)return false;value.items[i]=id;}
        value.stowed=b[84];value.flags=static_cast<unsigned>(get(b+88,4));value.fitMask=static_cast<unsigned>(get(b+120,4));
        if(value.flags>3||value.fitMask>1023)return false;
        for(unsigned i=0;i<7;++i){value.carried[i]=static_cast<unsigned>(get(b+92+4*i,4));if(value.carried[i]>1000000)return false;}
        auto readFit=[](const unsigned char* p,Fit& f){for(unsigned i=0;i<7;++i){const auto v=get(p+2*i,2);f[i]=static_cast<std::int16_t>(v>=32768?int(v)-65536:int(v));}};
        for(unsigned i=0;i<10;++i){readFit(b+124+14*i,value.fits[i]);if((value.fitMask&(1u<<i))&&!validFit(value.fits[i]))return false;}
        unsigned slots=0;
        for(unsigned i=0;i<5;++i){const auto* p=b+264+20*i;auto& bag=value.bags[i];bag.model=static_cast<unsigned>(get(p,2));bag.mount=p[2];bag.slot=p[3];readFit(p+6,bag.fit);
            if(p[4]||p[5]||bag.model>64||bag.mount>2||bag.slot>7)return false;
            if(bag.model){if((slots&(1u<<bag.slot))||!validFit(bag.fit))return false;slots|=1u<<bag.slot;}}
        out=value;return true;
    }
};
inline Bytes message(unsigned char kind){return {'S','C',1,kind};}
inline Bytes publish(const Look& look,unsigned sequence){auto b=message(4);put(b,sequence,4);const auto p=look.encode();b.insert(b.end(),p.begin(),p.end());return b;}
inline Bytes visible(const std::vector<std::uint64_t>& ids){auto b=message(3);for(auto id:ids)put(b,id,8);return b;}
struct Remote{std::uint64_t revision=0;Look look;};
// Consume only current subscriptions. Unknown GUIDs, self and out-of-order
// results never reach the renderer. An epoch reset discards all cached looks.
struct Inbox{
    std::map<std::uint64_t,Remote> looks;
    std::vector<std::uint64_t> visible;
    std::uint64_t self=0;
    bool receive=false;
    void subscriptions(std::vector<std::uint64_t> next){std::sort(next.begin(),next.end());visible=std::move(next);for(auto i=looks.begin();i!=looks.end();)if(!allows(i->first))i=looks.erase(i);else ++i;}
    bool allows(std::uint64_t guid)const{return receive&&guid&&guid!=self&&std::binary_search(visible.begin(),visible.end(),guid);}
    bool accept(const Bytes& b){
        if(b.size()<4||b[0]!='S'||b[1]!='C'||b[2]!=1)return false;
        if((b[3]==129||b[3]==133)&&b.size()==4)return true;
        if(b[3]==131&&b.size()==12){looks.erase(get(b.data()+4,8));return true;}
        if(b[3]!=130||b.size()!=20+appearanceSize)return false;
        const auto guid=get(b.data()+4,8),revision=get(b.data()+12,8);Look next;
        if(!revision||!Look::decode(b.data()+20,appearanceSize,next))return false;
        if(!allows(guid))return true;
        auto i=looks.find(guid);if(i!=looks.end()&&revision<=i->second.revision)return true;
        looks[guid]={revision,next};return true;
    }
};
// Monotonic milliseconds. Five seconds after the last actual content change,
// not after each polling tick. A new connection publishes immediately.
struct Debounce{
    Look pending,sent;std::uint32_t changedAt=0;bool initialized=false,haveSent=false;
    void update(const Look& look,std::uint32_t now){if(!initialized||!(pending==look)){pending=look;changedAt=now;initialized=true;}}
    bool ready(std::uint32_t now,bool immediate=false)const{return initialized&&(!haveSent||!(pending==sent))&&(immediate||std::uint32_t(now-changedAt)>=5000);}
    void published(){sent=pending;haveSent=true;}
};
}
