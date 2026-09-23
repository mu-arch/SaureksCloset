#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p native/build
# Optional self-contained LLVM-MinGW (MSVCRT) toolchain; otherwise use GCC.
if [ -n "${CLOSET_TOOLCHAIN:-}" ]; then
  closet_cc="$CLOSET_TOOLCHAIN/bin/i686-w64-mingw32-clang"
  closet_cxx="$CLOSET_TOOLCHAIN/bin/i686-w64-mingw32-clang++"
  closet_objdump="$CLOSET_TOOLCHAIN/bin/i686-w64-mingw32-objdump"
  closet_crt=""
  closet_static="-static"
else
  closet_cc=i686-w64-mingw32-gcc
  closet_cxx=i686-w64-mingw32-g++
  closet_objdump=i686-w64-mingw32-objdump
  closet_crt="-mcrtdll=msvcrt-os"
  closet_static="-static-libgcc -static-libstdc++"
fi
for source in buffer hook trampoline hde/hde32; do
  "$closet_cc" $closet_crt -O2 -c "native/vendor/minhook/src/$source.c" -Inative/vendor/minhook/include -o "native/build/$(basename "$source").o"
done
"$closet_cxx" $closet_crt -std=c++17 -fno-exceptions -fno-rtti -Os -Wall -Wextra -Werror -shared $closet_static -Inative/vendor/minhook/include native/SaureksCloset.cpp native/build/*.o -Wl,--no-insert-timestamp -o native/SaureksCloset.dll -lwinhttp -lshell32
"$closet_objdump" -p native/SaureksCloset.dll | sed -n '/DLL Name/p'
