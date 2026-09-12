"""Exercise the production snapshot and Lua body reader with build-5875 fields.

The fixture uses absolute field indices from the vanilla UpdateFields layout:
https://github.com/cmangos/mangos-classic/blob/master/src/game/Entities/UpdateFields.h
It leaves spell-cost multipliers at 1.0, reproducing the old all-1 UI result.
"""
from pathlib import Path
import subprocess
import tempfile
import sys

root = Path(__file__).resolve().parents[1]
source = Path(sys.argv[1]) if len(sys.argv) > 1 else root / 'native/SaureksCloset.cpp'
text = source.read_text()
snapshot = text[text.index('struct Player {'):text.index('static bool applies(')]
body_info = text[text.index('static int __fastcall bodyInfo('):text.index('static int __fastcall inspect(')]
harness = r'''
#include <array>
#include <vector>
#include <cstdint>
#include <cstring>
#include <cassert>
#define __fastcall
static std::array<unsigned char, 0xD40> unit{};
static std::array<std::uint32_t, 0x100> fields{};
static std::vector<double> returned;
static std::uint64_t getPlayer() { return 42; }
static void* objectPtr(unsigned, const char*, std::uint64_t, int) { return unit.data(); }
template<typename T> static bool read(std::uintptr_t address, T& value) {
    if (!address) return false;
    std::memcpy(&value, reinterpret_cast<void*>(address), sizeof(value));
    return true;
}
static void pushNumber(void*, double value) { returned.push_back(value); }
static int result(void*, int status) { returned.push_back(status); return 1; }
'''
harness += snapshot + body_info
harness += r'''
int main() {
    std::uintptr_t fieldPointer = reinterpret_cast<std::uintptr_t>(fields.data());
    std::memcpy(unit.data()+8, &fieldPointer, sizeof(fieldPointer));
    unsigned type=4;
    std::memcpy(unit.data()+0x14, &type, sizeof(type));
    fields[0]=42;
    fields[0x24]=7; // Male gnome.
    fields[0x83]=fields[0x84]=1563;
    fields[0xB5]=fields[0xB6]=0x3F800000; // Unrelated spell multipliers: float 1.0.
    fields[0xC1]=0x05040302; // Skin 2, face 3, hair style 4, hair color 5.
    fields[0xC2]=0x02010006; // Facial 6; higher bytes are unrelated state.
    assert(bodyInfo(nullptr)==7);
    assert((returned==std::vector<double>{7,0,2,3,4,5,6}));
    // A second character must replace all values, including genuine zero choices.
    fields[0x24]=4 | (1<<16);
    fields[0xC1]=0;
    fields[0xC2]=0x02010000;
    returned.clear();
    assert(bodyInfo(nullptr)==7);
    assert((returned==std::vector<double>{4,1,0,0,0,0,0}));
}
'''
with tempfile.TemporaryDirectory(prefix='closet-native-body-') as directory:
    cpp = Path(directory) / 'native_body.cpp'
    binary = Path(directory) / 'native_body'
    cpp.write_text(harness)
    subprocess.run(['clang++', '-std=c++17', '-Wall', '-Wextra', '-Werror', str(cpp), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
print('Native snapshot and real-body regression passed.')
