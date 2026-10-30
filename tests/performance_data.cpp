#define main originalArmorTests
#include "armor_inspection.cpp"
#undef main
#include "../native/CapeMotion.h"
#include "../native/ReadBatch.h"

static unsigned originalCRC(const capeMotion::Bytes& bytes){
    unsigned c=~0u;for(auto v:bytes){c^=v;for(unsigned i=0;i<8;++i)c=(c>>1)^(0xedb88320u&-(c&1));}return ~c;
}
int main(){
    originalArmorTests();
    capeMotion::Bytes bytes;
    assert(capeMotion::crc(bytes)==0);
    for(char c:std::string("123456789"))bytes.push_back(c);
    assert(capeMotion::crc(bytes)==0xcbf43926u);
    unsigned random=42;
    for(unsigned size=0;size<96;++size){
        bytes.resize(size);for(auto& value:bytes){random=random*1664525+1013904223;value=random>>24;}
        assert(capeMotion::crc(bytes)==originalCRC(bytes));
    }
    // Every shipped body retains the exact checksum used in its cache filename.
    for(unsigned body=1;body<=16;++body){
        char path[100];std::snprintf(path,sizeof(path),"addon/SaureksCloset/CapeMotion/B%02u.m2",body);
        assert(capeMotion::readFile(path,bytes));assert(capeMotion::crc(bytes)==originalCRC(bytes));
    }
    // Strings can end at an inaccessible allocation/page boundary. Compare
    // successful and failing results to the original byte-at-a-time reader.
    for(unsigned length=0;length<=260;++length)for(bool padded:{false,true}){
        Memory memory;const unsigned base=0x8000;
        for(unsigned i=0;i<(padded?288:length+1);++i)memory.put<char>(base+i,i==length?0:(i%2?'A':'/'));
        std::array<char,260> expected{},actual{};
        bool matched=false;unsigned calls=0;
        for(unsigned i=0;i<expected.size();++i){char value=0;if(!memory.read(base+i,value))break;expected[i]=armorPathCharacter(value);if(!value){matched=true;break;}}
        const auto reader=[&](auto address,auto& value){++calls;return memory.read(address,value);};
        assert(armorReadPath(base,reader,actual)==matched&&actual==expected);
        if(padded&&length<256)assert(calls==length/32+1);
        if(!padded&&length==63)assert(calls==2);
        if(length>0){memory.bytes.erase(base+length/2);actual={};assert(!armorReadPath(base,reader,actual));}
    }
    // Strided vertices and triangle indices: bulk reads and sparse/failing
    // allocations produce exactly the same requested element values.
    for(bool sparse:{false,true}){
        Memory memory;const unsigned base=0x9000;
        for(unsigned i=0;i<65;++i){
            if(!sparse)for(unsigned byte=0;byte<48;++byte)memory.put<unsigned char>(base+48*i+byte,0);
            memory.put(base+48*i,std::array<float,3>{{float(i),float(i+1),float(i+2)}});
        }
        ReadBatch<std::array<float,3>,48> batch{base,65};unsigned calls=0;
        const auto reader=[&](auto address,auto& value){++calls;return memory.read(address,value);};
        for(unsigned i=0;i<65;++i){std::array<float,3> value;assert(batch.get(i,value,reader)&&value[0]==i&&value[2]==i+2);}
        assert(calls==(sparse?67u:3u));
        const auto before=calls;std::array<float,3> value;assert(!batch.get(65,value,reader)&&calls==before);
        memory.bytes.erase(base);ReadBatch<std::array<float,3>,48> missing{base,65};assert(!missing.get(0,value,reader));
    }
    std::cout<<"PASS: identical CRC/cache keys for all bodies; batched paths/mesh reads preserve values and inaccessible-boundary fallback\n";
}
