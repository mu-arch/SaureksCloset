#pragma once
#include <cstddef>
#include <cstring>
struct UpdateRelease { unsigned major=0,minor=0,patch=0,dll=0; };
inline bool updateNumber(const char*& p,const char* end,unsigned& out,unsigned limit){
    if(p==end||*p<'0'||*p>'9')return false;
    out=0;
    while(p<end&&*p>='0'&&*p<='9'){
        const unsigned digit=static_cast<unsigned>(*p++-'0');
        if(out>(limit-digit)/10)return false;
        out=out*10+digit;if(out>limit)return false;
    }
    return true;
}
// Data only: strict, bounded manifest. Never evaluate Lua, JSON scripts or URLs.
inline bool parseUpdateManifest(const char* text,std::size_t size,UpdateRelease& result){
    if(!text||!size||size>1024)return false;
    UpdateRelease value;unsigned seen=0;
    const char* p=text;const char* end=text+size;
    while(p<end){
        const char* line=p;while(p<end&&*p!='\n')++p;
        const char* stop=p;if(p<end)++p;
        if(stop>line&&stop[-1]=='\r')--stop;
        if(line==stop)continue;
        unsigned flag=0;
        if(stop-line>=7&&std::memcmp(line,"schema=",7)==0){
            flag=1;line+=7;unsigned schema=0;
            if(!updateNumber(line,stop,schema,9)||schema!=1)return false;
        }else if(stop-line>=6&&std::memcmp(line,"addon=",6)==0){
            flag=2;line+=6;
            if(!updateNumber(line,stop,value.major,65535)||line==stop||*line++!='.'||
               !updateNumber(line,stop,value.minor,65535)||line==stop||*line++!='.'||
               !updateNumber(line,stop,value.patch,65535)||!value.major)return false;
        }else if(stop-line>=4&&std::memcmp(line,"dll=",4)==0){
            flag=4;line+=4;
            if(!updateNumber(line,stop,value.dll,999999)||!value.dll)return false;
        }else return false;
        if(line!=stop||(seen&flag))return false;
        seen|=flag;
    }
    if(seen!=7)return false;
    result=value;return true;
}
// Compatibility with releases published before update-version.txt existed.
// Extract only the exact version declarations; never execute downloaded source.
inline bool parsePublishedVersion(const char* text,std::size_t size,bool dll,UpdateRelease& out){
    if(!text||!size||size>65536)return false;
    const char* prefix=dll?"static int __fastcall version(void* L){return result(L,":"## Version: ";
    const std::size_t length=std::strlen(prefix);bool found=false;
    const char* p=text;const char* end=text+size;
    while(p<end){
        const char* line=p;while(p<end&&*p!='\n')++p;
        const char* stop=p;if(p<end)++p;
        if(stop>line&&stop[-1]=='\r')--stop;
        if(static_cast<std::size_t>(stop-line)<length||std::memcmp(line,prefix,length))continue;
        if(found)return false;
        found=true;line+=length;
        // Development releases use 0.x.y.z; keep the existing three-number wire ABI.
        if(!dll&&stop-line>2&&line[0]=='0'&&line[1]=='.')line+=2;
        if(dll){
            if(!updateNumber(line,stop,out.dll,999999)||!out.dll||stop-line!=3||std::memcmp(line,");}",3))return false;
        }else if(!updateNumber(line,stop,out.major,65535)||line==stop||*line++!='.'||
                 !updateNumber(line,stop,out.minor,65535)||line==stop||*line++!='.'||
                 !updateNumber(line,stop,out.patch,65535)||!out.major||line!=stop)return false;
    }
    return found;
}
