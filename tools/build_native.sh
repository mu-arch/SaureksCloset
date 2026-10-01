#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p native/build
# Optional self-contained LLVM-MinGW (MSVCRT) toolchain; otherwise use GCC.
if [ -n "${CLOSET_TOOLCHAIN:-}" ]; then
  closet_cc="$CLOSET_TOOLCHAIN/bin/i686-w64-mingw32-clang"
  closet_cxx="$CLOSET_TOOLCHAIN/bin/i686-w64-mingw32-clang++"
  closet_objdump="$CLOSET_TOOLCHAIN/bin/i686-w64-mingw32-objdump"
  closet_ar="$CLOSET_TOOLCHAIN/bin/llvm-ar"
  closet_crt=""
  closet_static="-static"
else
  closet_ar=i686-w64-mingw32-ar
  closet_cc=i686-w64-mingw32-gcc
  closet_cxx=i686-w64-mingw32-g++
  closet_objdump=i686-w64-mingw32-objdump
  closet_crt="-mcrtdll=msvcrt-os"
  closet_static="-static-libgcc -static-libstdc++"
fi
for source in buffer hook trampoline hde/hde32; do
  "$closet_cc" $closet_crt -O2 -c "native/vendor/minhook/src/$source.c" -Inative/vendor/minhook/include -o "native/build/$(basename "$source").o"
done
python3 tools/build_nvcloth.py --windows --cxx "$closet_cxx" --ar "$closet_ar" --output native/build/nvcloth-win
# Keep third-party headers out of the bridge's warning policy. NvCloth itself
# is compiled separately; all original integration code retains -Werror.
"$closet_cxx" $closet_crt -std=c++17 -DNDEBUG -DNV_CLOTH_IMPORT= -DNV_CLOTH_ENABLE_CUDA=0 -DNV_CLOTH_ENABLE_DX11=0 -msse2 -fno-exceptions -fno-rtti -O2 -Wall -Wextra -Werror \
  -isystem native/vendor/nvcloth/NvCloth/include -isystem native/vendor/nvcloth/NvCloth/include/NvCloth/ps \
  -isystem native/vendor/nvcloth/NvCloth/extensions/include -isystem native/vendor/nvcloth/PxShared/include \
  -c native/CapeNvCloth.cpp -o native/build/CapeNvCloth.o
"$closet_cxx" $closet_crt -std=c++17 -fno-exceptions -fno-rtti -O2 -Wall -Wextra -Werror -c native/CapeWorker.cpp -o native/build/CapeWorker.o
"$closet_cxx" $closet_crt -std=c++17 -fno-exceptions -fno-rtti -Os -Wall -Wextra -Werror -shared $closet_static -Inative/vendor/minhook/include native/SaureksCloset.cpp native/build/*.o native/build/nvcloth-win/libnvcloth.a -Wl,--no-insert-timestamp -o native/SaureksCloset.dll -lwinhttp -lshell32
"$closet_objdump" -p native/SaureksCloset.dll | sed -n '/DLL Name/p'
