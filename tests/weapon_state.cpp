#include <cassert>
#include <iostream>
#include "../native/WeaponState.h"
int main(){
    unsigned kinds[6]{};unsigned guns=0,bows=0,swords=0,daggers=0;
    for(const auto& a:weaponAssets){
        assert(weaponAsset(a.item)==&a);kinds[a.kind]++;
        if(a.kind==4&&a.subclass==3)guns=a.item;
        if(a.kind==4&&a.subclass==2)bows=a.item;
        if(a.kind==1&&a.subclass==7)swords=a.item;
        if(a.kind==1&&a.subclass==15)daggers=a.item;
        for(unsigned i=0;i<7;i++)if(acceptsWeapon(i,&a)){
            WeaponSelection s;s.items[i]=a.item;assert(s.valid());
        }
    }
    for(unsigned kind=1;kind<=5;kind++)assert(kinds[kind]);
    assert(!weaponAsset(0)&&!weaponAsset(999999));
    WeaponSelection s;s.items={{swords,daggers,35,35,143,guns,0}};
    assert(s.valid());assert((s.routes()==std::array<int,3>{{-1,-1,-1}}));
    s.equipped={{swords,daggers,0}};
    assert((s.routes()==std::array<int,3>{{0,1,-1}}));
    s.items[1]=swords;
    assert((s.routes()==std::array<int,3>{{1,0,-1}}));
    s.items[0]=0;
    assert((s.routes()==std::array<int,3>{{1,-1,-1}}));
    s.equipped={{35,0,guns}};
    assert((s.routes()==std::array<int,3>{{2,-1,5}}));
    s.equipped[2]=bows;assert(s.routes()[2]==-1);
    s.items[0]=guns;assert(!s.valid());
    // One-handed weapons can be stored on the back; selected location wins.
    s={};s.items[2]=daggers;s.equipped[0]=daggers;assert(s.routes()[0]==2);
    for(unsigned i=0;i<7;i++)for(unsigned j=0;j<i;j++)assert(weaponPoints[i]!=weaponPoints[j]);
    std::cout<<"PASS: weapon asset validation, physical compatibility, independent routes, real-equipment matching\n";
}
