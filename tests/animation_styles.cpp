#include "../native/CapeAnimation.h"
#include <filesystem>
#include <cassert>
#include <iostream>
using namespace animationStyles;
static Bytes file(const std::filesystem::path& p){Bytes b;assert(readFile(p.string(),b));return b;}
int main(int argc,char** argv){
    const auto root=std::filesystem::absolute(argc>1?argv[1]:"addon/SaureksCloset/Animations/Styles");
    unsigned tested=0;
    for(unsigned r=1;r<=16;++r){
        char name[30];std::snprintf(name,sizeof(name),"B%02u.m2",r);const auto base=file(root/name);const auto checksum=crc(base);const auto offset=u32(base,56),bones=u32(base,52);
        unsigned capeCount=r==9||r==10||r==12||r==16?3:r==11?4:2;const unsigned original=bones-capeCount;
        for(unsigned donor=1;donor<=16;++donor){if(donor==r)continue;Bytes bodyModel=base,capeModel=base;
            for(bool cape:{false,true}){
                std::snprintf(name,sizeof(name),"%c%02u_%02u.sca",cape?'C':'A',r,donor);auto patch=file(root/name);auto& model=cape?capeModel:bodyModel;
                assert(apply(model,patch,base.size(),checksum,cape));std::vector<bool> changed(base.size(),false);unsigned p=16;
                for(unsigned i=0;i<u32(patch,12);++i){const auto b=u16(patch,p),t=u16(patch,p+2),len=u32(patch,p+4);assert(cape?b>=original:b<original);assert(t==40||(!cape&&t==12));for(unsigned j=0;j<28;++j)changed[offset+b*108+t+j]=true;p+=8+len;}
                for(unsigned i=0;i<base.size();++i)if(!changed[i])assert(base[i]==model[i]);
                // Malformed patches are transactional; the recipient stays intact.
                Bytes invalid=patch;invalid.pop_back();Bytes untouched=base;assert(!apply(untouched,invalid,base.size(),checksum,cape)&&untouched==base);
                ++tested;
            }
            Bytes combined=bodyModel;std::snprintf(name,sizeof(name),"C%02u_%02u.sca",r,donor);auto patch=file(root/name);assert(apply(combined,patch,base.size(),checksum,true));
            // Appended offsets differ, but original body track headers remain exact.
            for(unsigned i=0;i<original;++i)assert(!std::memcmp(combined.data()+offset+i*108,bodyModel.data()+offset+i*108,108));
        }
    }
    assert(tested==480);
    assert(!valid(17,0)&&!valid(0,17));
    assert(cachePath("Interface\\AddOns\\SaureksCloset\\Animations\\Cache\\R02_B03_C04_G0_0123abcd.m2"));
    assert(!cachePath("Interface/AddOns/SaureksCloset/Animations/Cache/../../x.m2"));
    assert(!cachePath("Interface/AddOns/SaureksCloset/Animations/Cache/R02_B17_C04_G0_0123abcd.m2"));
    assert(animationRecipient("Character\\Human\\Female\\HumanFemale.m2")==2);
    assert(!animationRecipient("Creature\\Bear\\Bear.m2"));
    // End-to-end generation uses the same file parser and cache as the DLL.
    auto temporary=std::filesystem::temp_directory_path()/"closet-animation-style-test";std::filesystem::remove_all(temporary);
    auto directory=temporary/"Interface/AddOns/SaureksCloset/Animations";std::filesystem::create_directories(directory);std::filesystem::create_directory_symlink(root,directory/"Styles");
    auto cwd=std::filesystem::current_path();std::filesystem::current_path(temporary);
    const auto* generated=prepare(2,3,7,false);assert(generated&&cachePath(generated));const std::string stable=generated;assert(prepare(2,3,7,false)==generated);
    assert(prepare(2,3,0,true));assert(!prepare(0,3,7,false));assert(prepare(2,0,7,false)&&prepare(2,3,0,false));
    Bytes loaded;assert(readFile(stable,loaded));std::filesystem::current_path(cwd);std::filesystem::remove_all(temporary);
    animationCompression::Bytes invalid={'S','L','Z','1',8,0,0,0,0,0,8,0,1,0},out;assert(!animationCompression::unpack(invalid,out));
    std::cout<<"PASS: 480 race/gender transfers, independent cape/body tracks, unchanged meshes/pivots/attachments/timing, malformed inputs and on-demand cache\n";
}
