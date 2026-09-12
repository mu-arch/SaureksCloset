#pragma once
#include "Appearance.h"
struct PreviewEntry {
    std::uintptr_t model=0;
    std::uint64_t guid=0;
    Appearance body;
    unsigned token=0;
    int status=0;
};
struct PreviewRegistry {
    std::array<PreviewEntry,8> entries{};
    unsigned sequence=0;
    PreviewEntry* freeEntry(){for(auto& e:entries)if(!e.model)return &e;return nullptr;}
    PreviewEntry* find(std::uintptr_t model){for(auto& e:entries)if(model&&e.model==model)return &e;return nullptr;}
    unsigned bind(std::uintptr_t model,std::uint64_t guid,const Appearance& body){
        if(!model||!guid||!body.valid()||find(model))return 0;
        auto* e=freeEntry();if(!e)return 0;
        if(++sequence==0)++sequence;
        *e={model,guid,body,sequence,0};return sequence;
    }
    void forget(std::uintptr_t model){if(auto* e=find(model))*e={};}
    int query(unsigned token,std::uint64_t guid)const{
        for(const auto& e:entries)if(e.model&&e.token==token&&e.guid==guid)return e.status;
        return -1;
    }
};
inline std::array<std::uint32_t,91> previewDescriptor(const std::array<std::uint32_t,91>& source,const Appearance& body,std::uint32_t model){
    auto copy=source;body.compose(copy);copy[8]=model;return copy;
}
