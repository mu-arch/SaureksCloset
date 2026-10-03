#include "../native/SharingProtocol.h"
#include <cassert>
#include <iostream>
using namespace sharing;
int main(){Look a;a.body={1,1,0,0,0,0,0,0};a.items[0]=0;a.items[2]=1234;a.flags=7;a.weaponMotion={{0,65,200}};a.carried[0]=17;a.fitMask=1;a.fits[0]={100,0,-300,400,500,600,8500};a.bags[0]={12,1,4,{1000,-1000,-20000,0,0,0,7000}};a.bags[0].amplitude=35;a.bags[0].physics=false;auto b=a.encode();assert(b.size()==appearanceSize);Look decoded;assert(Look::decode(b.data(),b.size(),decoded)&&decoded==a);for(unsigned n=0;n<b.size();++n)assert(!Look::decode(b.data(),n,decoded));b[84]=8;assert(!Look::decode(b.data(),b.size(),decoded));
b=a.encode();b[264+20]=12;b[264+20+3]=4;assert(!Look::decode(b.data(),b.size(),decoded)); // Duplicate nonempty bag slot.
b=a.encode();b[124+12]=0;b[124+13]=0;assert(!Look::decode(b.data(),b.size(),decoded)); // Enabled zero-size fit.
b=a.encode();b[268]=201;assert(!Look::decode(b.data(),b.size(),decoded)); // Out-of-range amplitude.
for(unsigned i=85;i<88;++i){b=a.encode();b[i]=201;assert(!Look::decode(b.data(),b.size(),decoded));}
Look defaults;assert((defaults.weaponMotion==std::array<unsigned char,3>{{100,100,100}}));
Debounce d;d.update(a,0);assert(!d.ready(4999)&&d.ready(5000));d.published();d.update(a,6000);assert(!d.ready(12000));a.items[0]=42;d.update(a,7000);d.update(a,8000);assert(d.ready(12000));d.published();a.items[0]=0;d.update(a,0xfffffff0u);assert(d.ready(6000));
Inbox inbox;inbox.self=1;inbox.receive=true;inbox.subscriptions({2,3});auto packet=message(130);put(packet,2,8);put(packet,7,8);auto p=a.encode();packet.insert(packet.end(),p.begin(),p.end());assert(inbox.accept(packet));assert(inbox.looks.size()==1);auto old=packet;old[2]=4;assert(!inbox.accept(old));packet[12]=6;packet[28]=42;assert(inbox.accept(packet));assert(inbox.looks[2].look.items[0]==0);inbox.subscriptions({3});assert(inbox.looks.empty());assert(inbox.accept(packet)&&inbox.looks.empty());inbox.receive=false;inbox.subscriptions({2});assert(inbox.accept(packet)&&inbox.looks.empty());
for(unsigned n=0;n<packet.size();++n){auto truncated=packet;truncated.resize(n);assert(!inbox.accept(truncated));}std::cout<<"PASS: binary snapshots, passthrough/hide, bounds, debounce, stale revisions, subscription and receive isolation\n";}
