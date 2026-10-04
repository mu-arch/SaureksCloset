// Run two copies against a loopback WSS relay using an isolated Wine prefix.
// Fixture identities/keys only. Certificate trust is scoped to that test prefix.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wincrypt.h>
#include <fstream>
#include <iterator>
#include <iostream>
#include "../native/SharingTransport.h"
static DWORD WINAPI connectFixture(void* p){const auto epoch=sharingMailbox.generation;SharingSocketApi api;if(!api.load())return 2;sharingConnect(api,*static_cast<SharingConfig*>(p),epoch);return 0;}
int main(int argc,char** argv){
    if(argc==3&&std::string(argv[1])=="trust"){
        std::ifstream f(argv[2],std::ios::binary);std::vector<unsigned char> der((std::istreambuf_iterator<char>(f)),{});
        auto store=CertOpenStore(CERT_STORE_PROV_SYSTEM_A,0,0,CERT_SYSTEM_STORE_CURRENT_USER,"ROOT");
        if(!store||!CertAddEncodedCertificateToStore(store,X509_ASN_ENCODING,der.data(),static_cast<DWORD>(der.size()),CERT_STORE_ADD_REPLACE_EXISTING,nullptr))return 3;
        CertCloseStore(store,0);return 0;
    }
    if(argc!=2)return 2;const bool first=std::string(argv[1])=="10";const auto guid=first?10u:20u,other=first?20u:10u;
    SharingConfig config;config.url="wss://localhost:19443/v1/sharing";config.token=std::string(64,first?'a':'b');config.server="server";config.realm="realm";config.name="TEST_ONLY";config.game="1.12.1 / 5875";config.guid=guid;config.flags=3;
    sharing::Look look;look.items[0]=guid;look.flags=4;look.weaponSlots[7]={{2,150,25,0}};look.cape={{1,50,25,100,75}};look.capeAdvanced={{55,85,60,35,65,60}};look.weaponMotion={{50,125,200}};
    sharingMailbox.config=config;sharingMailbox.generation=1;sharingMailbox.status=1;sharingMailbox.inbox.self=guid;sharingMailbox.inbox.receive=true;sharingMailbox.inbox.subscriptions({other});sharingMailbox.debounce.update(look,GetTickCount());
    auto thread=CreateThread(nullptr,0,connectFixture,&config,0,nullptr);if(!thread)return 4;
    DWORD start=GetTickCount(),changed=0;bool initial=false,updated=false;
    while(GetTickCount()-start<30000){
        {std::lock_guard<std::mutex> lock(sharingMailbox.mutex);auto i=sharingMailbox.inbox.looks.find(other);
        if(i!=sharingMailbox.inbox.looks.end()){
            if(i->second.look.weaponSlots==look.weaponSlots&&i->second.look.cape==look.cape&&i->second.look.capeAdvanced==look.capeAdvanced&&i->second.look.items[0]==other&&i->second.look.flags==4&&i->second.look.weaponMotion==std::array<unsigned char,3>{{50,125,200}})initial=true;
            if(initial&&!changed){changed=GetTickCount();look.items[0]=guid+100;look.weaponMotion={{200,0,75}};sharingMailbox.debounce.update(look,changed);}
            if(i->second.look.items[0]==other+100&&i->second.look.weaponMotion==std::array<unsigned char,3>{{200,0,75}}){updated=true;break;}
        }}Sleep(25);
    }
    std::cerr<<"Exchange: initial="<<initial<<" updated="<<updated<<" status="<<sharingMailbox.status<<"\n";
    // Both clients must stay alive long enough for the partner to receive the
    // debounced publication even if one finishes its receive loop first.
    if(updated)Sleep(1000);
    {std::lock_guard<std::mutex> lock(sharingMailbox.mutex);++sharingMailbox.generation;sharingMailbox.config.flags=0;}
    if(WaitForSingleObject(thread,10000)!=WAIT_OBJECT_0){std::cerr<<"FAIL: socket cancellation did not finish\n";return 5;}
    CloseHandle(thread);
    if(!initial||!updated||sharingMailbox.fatal){std::cerr<<"FAIL: WSS exchange; status="<<sharingMailbox.status<<" initial="<<initial<<" updated="<<updated<<"\n";return 1;}
    std::cout<<"PASS: real WinHTTP WSS handshake, authenticated appearance exchange, five-second debounce and receive cancellation\n";return 0;
}
