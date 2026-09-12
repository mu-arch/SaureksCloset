#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p native/build
for source in buffer hook trampoline hde/hde32; do
  i686-w64-mingw32-gcc -mcrtdll=msvcrt-os -O2 -c "native/vendor/minhook/src/$source.c" -Inative/vendor/minhook/include -o "native/build/$(basename "$source").o"
done
i686-w64-mingw32-g++ -mcrtdll=msvcrt-os -std=c++17 -fno-exceptions -fno-rtti -Os -Wall -Wextra -Werror -shared -static-libgcc -static-libstdc++ -Inative/vendor/minhook/include native/SaureksCloset.cpp native/build/*.o -Wl,--no-insert-timestamp -o native/SaureksCloset.dll
i686-w64-mingw32-objdump -p native/SaureksCloset.dll | sed -n '/DLL Name/p'
