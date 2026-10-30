"""Offline performance probes, not an in-game FPS benchmark.

Runs production attachment dispatch through the existing memory fixture and
counts reader calls. Times cape baking with shipped models. No game or addon
files are written. Run from any directory with Python 3 and c++ installed.
"""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
fixture = (root / 'tests/weapon_renderer.cpp').read_text()
# Instrument the test reader only; production still uses ReadProcessMemory.
fixture, count = re.subn(r'(static bool read\(std::uintptr_t a,[^\n]*?\)\{)',
                        r'\1++profileReads;', fixture)
assert count == 5, 'Update the probe for new reader overloads'
attachment = '''
static unsigned long long profileReads=0;
#define main unusedWeaponRegression
''' + fixture + r'''
#undef main
int main(){
    updateAttachedOriginal=&observeAttachment;
    player={0x4100000,0x4101000,123,49,49};bagTuningOwner=player.guid;
    const auto foreignParent=0x4200000u;auto* child=pointer(0x4300000);
    memory[address(child)+0x1cc]=foreignParent;
    BagMatrix input{{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1}};
    matrices[reinterpret_cast<std::uintptr_t>(input.data())]=input;
    captureAttachmentMatrix=true;
    for(unsigned point:{1u,11u,30u})for(unsigned caller:{0x718761u,0x71415Du}){
        memory[address(child)+0x1d0]=point;profileReads=0;attachmentUpdate={};
        for(unsigned i=0;i<1000;++i)updateAttachmentForCaller(child,input.data(),nullptr,nullptr,1,caller);
        assert(attachmentUpdate.calls==1000&&capturedAttachmentMatrix==input);
        std::cout<<"Foreign attachment: point="<<point<<", "<<(caller==0x718761?"recursive":"lazy")
                 <<", reader calls/update="<<profileReads/1000.<<"\n";
    }
}
'''
cape = r'''
#include "CapeMotion.h"
#include <chrono>
#include <cassert>
#include <iostream>
#include <algorithm>
int main(int argc,char** argv){
    assert(argc==2);using namespace capeMotion;
    for(unsigned body:{2u,12u}){
        char suffix[80];std::snprintf(suffix,sizeof(suffix),"/addon/SaureksCloset/CapeMotion/B%02u.m2",body);
        Bytes source;assert(readFile(std::string(argv[1])+suffix,source));
        std::vector<double> timings;unsigned checksum=0;std::size_t bytes=0;
        for(unsigned i=0;i<11;++i){
            Bytes output;const auto start=std::chrono::steady_clock::now();
            assert(build(source,body,{{100,75,100,100}},output,{{55,85,60,35,65,60}}));
            checksum^=crc(output);bytes=output.size();
            const auto end=std::chrono::steady_clock::now();
            if(i)timings.push_back(std::chrono::duration<double,std::milli>(end-start).count());
        }
        std::sort(timings.begin(),timings.end());
        std::cout<<"Cape build + CRC: body="<<body<<", bytes="<<bytes<<", median_ms="<<timings[timings.size()/2]
                 <<", max_ms="<<timings.back()<<", checksum="<<checksum<<"\n";
    }
}
'''
with tempfile.TemporaryDirectory(prefix='closet-performance-') as folder:
    for name, source, args in [('attachment', attachment, []), ('cape', cape, [str(root)])]:
        cpp = Path(folder) / (name + '.cpp')
        exe = Path(folder) / name
        cpp.write_text(source)
        subprocess.run(['c++', '-std=c++17', '-Os', '-I'+str(root/'tests'), '-I'+str(root/'native'), str(cpp), '-o', str(exe)], check=True)
        subprocess.run([str(exe), *args], check=True)
print('Reader counts exclude optional casting/hair callbacks and real client work. Cape timings exclude disk I/O and model reloads; host CPU timings are not game frame times.')
