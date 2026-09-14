#pragma once
// Build 5875 disables ordinary loose-file searches at startup. Route only our
// two shipped bag assets to the client's existing disk-file handle path.
using ResolveAssetFile=int (__fastcall *)(const char*,char*,unsigned,unsigned,unsigned*,void**);
static ResolveAssetFile resolveAssetFileOriginal=nullptr;
static const char* bagAssetFile(const char* filename){
    if(!filename)return nullptr;
    for(const char* allowed:{"Interface\\AddOns\\SaureksCloset\\Models\\DarkSchoolbag.m2",
                             "Interface\\AddOns\\SaureksCloset\\Models\\DarkSchoolbag.blp"}){
        unsigned i=0;
        for(;allowed[i]&&filename[i];++i){
            auto a=allowed[i],b=filename[i];
            if(a>='A'&&a<='Z')a+=32;
            if(b>='A'&&b<='Z')b+=32;
            if(b=='/')b='\\';
            if(a!=b)break;
        }
        if(!allowed[i]&&!filename[i])return allowed;
    }
    return nullptr;
}
static int __fastcall resolveAssetFileHook(const char* filename,char* output,unsigned capacity,
                                         unsigned flags,unsigned* kind,void** archive){
    const auto* path=bagAssetFile(filename);
    if(path&&output&&kind){
        const auto length=std::strlen(path);
        const auto attributes=GetFileAttributesA(path);
        if(length<capacity&&attributes!=INVALID_FILE_ATTRIBUTES&&!(attributes&FILE_ATTRIBUTE_DIRECTORY)){
            std::memcpy(output,path,length+1);
            *kind=0; // Client-owned disk file: normal open/read/async/close lifetime.
            if(archive)*archive=nullptr;
            return 1;
        }
    }
    return resolveAssetFileOriginal(filename,output,capacity,flags,kind,archive);
}
