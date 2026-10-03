// Small deterministic LZ stream used only for shipped offline animation patches.
#include "../native/AnimationCompression.h"
#include <fstream>
#include <array>
#include <cassert>
using animationCompression::Bytes;
static void u16(Bytes& b,unsigned n){b.push_back(n&255);b.push_back(n>>8);}
int main(int argc,char** argv){
    for(int arg=1;arg<argc;++arg){
        std::ifstream f(argv[arg],std::ios::binary|std::ios::ate);if(!f)return 1;auto size=f.tellg();if(size<0||size>32*1024*1024)return 2;Bytes in(static_cast<unsigned>(size));f.seekg(0);f.read(reinterpret_cast<char*>(in.data()),in.size());f.close();
        Bytes out={'S','L','Z','1'};for(unsigned i=0;i<4;++i)out.push_back(unsigned(size)>>(8*i));
        std::array<int,65536> last;last.fill(-1);unsigned pos=0,start=0;
        auto emit=[&](unsigned end,unsigned match,unsigned distance){while(end-start>65535){u16(out,65535);u16(out,0);u16(out,0);out.insert(out.end(),in.begin()+start,in.begin()+start+65535);start+=65535;}u16(out,end-start);u16(out,match);u16(out,distance);out.insert(out.end(),in.begin()+start,in.begin()+end);start=end+match;};
        while(pos+8<=in.size()){
            unsigned key=0;for(unsigned i=0;i<4;++i)key=key*257+in[pos+i];key=(key^(key>>16))&65535;
            const int previous=last[key];last[key]=pos;unsigned match=0;
            if(previous>=0&&pos-unsigned(previous)<=65535){while(match<65535&&pos+match<in.size()&&in[previous+match]==in[pos+match])++match;}
            if(match>=8){emit(pos,match,pos-previous);pos+=match;}else ++pos;
        }
        if(start<in.size())emit(in.size(),0,0);
        Bytes verified;assert(animationCompression::unpack(out,verified)&&verified==in);
        std::ofstream save(argv[arg],std::ios::binary|std::ios::trunc);if(!save.write(reinterpret_cast<const char*>(out.data()),out.size()))return 3;
    }
}
