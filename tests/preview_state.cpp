#include "../native/PreviewState.h"
#include <cassert>
#include <cstdio>
int main(){
    Appearance body;
    for(const auto& o:bodyOptions)if(o.race==1&&o.sex==0&&o.kind==0){body.skin=o.a;body.face=o.b;break;}
    for(const auto& o:bodyOptions)if(o.race==1&&o.sex==0&&o.kind==1){body.hairStyle=o.a;body.hairColor=o.b;break;}
    for(const auto& o:bodyOptions)if(o.race==1&&o.sex==0&&o.kind==2){body.facial=o.a;break;}
    assert(body.valid());PreviewRegistry registry;
    const auto token=registry.bind(0x1234,77,body);assert(token&&registry.query(token,77)==0);
    assert(registry.query(token,78)==-1&&!registry.bind(0x1234,77,body));
    registry.find(0x1234)->status=1;assert(registry.query(token,77)==1);
    std::array<std::uint32_t,91> original;for(unsigned i=0;i<91;++i)original[i]=0xabc000+i;
    const auto copy=previewDescriptor(original,body,0x7890);
    assert(original[8]==0xabc008&&copy[8]==0x7890);
    for(unsigned i=0;i<91;++i)if(i!=0&&i!=1&&i!=2&&i!=3&&i!=5&&i!=6&&i!=7&&i!=8)assert(copy[i]==original[i]);
    registry.forget(0x1234);assert(registry.query(token,77)==-1);
    const auto next=registry.bind(0x1234,77,body);assert(next!=token&&registry.query(token,77)==-1);
    for(unsigned i=1;i<registry.entries.size();++i)assert(registry.bind(0x1234+i,77,body));
    assert(!registry.bind(0x9999,77,body));registry.forget(0x1236);assert(registry.bind(0x9999,77,body));
    std::puts("PASS: bounded preview registry, ownership, stale tokens, release/reuse and copied descriptors.");
}
