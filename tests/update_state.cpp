#include <cassert>
#include <cstring>
#include <iostream>
#include <fstream>
#include <iterator>
#include <string>
#include "../native/UpdateState.h"
int main(int argc,char** argv){
    UpdateRelease value;
    const char* good="schema=1\r\naddon=3.4.33\r\ndll=30433\r\n";
    assert(parseUpdateManifest(good,std::strlen(good),value)&&value.major==3&&value.minor==4&&value.patch==33&&value.dll==30433);
    for(const char* bad:{"schema=1\naddon=3.4.33\n", "schema=2\naddon=3.4.33\ndll=30433\n",
        "schema=1\naddon=3.4.33\ndll=30433\ndll=30432", "schema=1\naddon=3.4.33\ndll=30433\nurl=evil",
        "schema=1\naddon=3.4.33-beta\ndll=30433", "schema=1\naddon=3.4.33\ndll=99999999999999999",
        "<html>error</html>", "schema=1\naddon=3.4.33\ndll=-1", "schema=1\naddon=3.4.33\ndll=0"})
        assert(!parseUpdateManifest(bad,std::strlen(bad),value));
    assert(!parseUpdateManifest(good,1025,value));
    const char* toc="## Interface: 11200\n## Version: 3.4.15\r\nCore.lua\n";
    const char* dll="// source\nstatic int __fastcall version(void* L){return result(L,30400);}\n";
    assert(parsePublishedVersion(toc,std::strlen(toc),false,value)&&value.patch==15);
    assert(parsePublishedVersion(dll,std::strlen(dll),true,value)&&value.dll==30400);
    assert(!parsePublishedVersion("## Version: 3.4.15junk",21,false,value));
    const char* dev="## Version: 0.3.6.7\n";
    assert(parsePublishedVersion(dev,std::strlen(dev),false,value)&&value.major==3&&value.minor==6&&value.patch==7);
    assert(!parsePublishedVersion("## Version: 0.3.6.7.8",21,false,value));
    UpdateMailbox box;unsigned ticket=0;
    assert(!box.begin(ticket)&&box.status()==-2);
    box.enable(true);assert(box.begin(ticket)&&box.status()==1);
    unsigned other=0;assert(!box.begin(other));
    box.enable(false);assert(box.cancelled(ticket)&&box.status()==-2);
    box.enable(true);box.finish(ticket,value,0);
    assert(box.status()==0); // stale response cannot appear after re-enabling
    assert(box.begin(ticket));box.finish(ticket,value,0);
    assert(box.status()==2&&box.release.dll==30400);
    assert(box.begin(ticket));box.finish(ticket,{},12007);assert(box.status()==-1&&box.error==12007);
    if(argc==3){
        std::ifstream addonFile(argv[1]),dllFile(argv[2]);assert(addonFile&&dllFile);
        const std::string addonText((std::istreambuf_iterator<char>(addonFile)),{});
        const std::string dllText((std::istreambuf_iterator<char>(dllFile)),{});
        assert(parsePublishedVersion(addonText.data(),addonText.size(),false,value));
        assert(parsePublishedVersion(dllText.data(),dllText.size(),true,value));
        std::cout<<"Published GitHub files parsed: addon "<<value.major<<'.'<<value.minor<<'.'<<value.patch<<", DLL "<<value.dll<<'\n';
    }
    std::cout<<"PASS: strict version parsers, published legacy files, bounded input, single worker and cancelled-generation isolation\n";
}
