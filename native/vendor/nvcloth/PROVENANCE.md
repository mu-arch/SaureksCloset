# NvCloth CPU dependency

Source: https://github.com/NVIDIAGameWorks/NvCloth
Pinned revision: `6e86b1838ae185c527744d475d34777ff69c1b8a`.

This directory contains the CPU runtime, fabric cooking extensions and
PxShared headers needed by Saurek's Closet. Samples, tools, documentation,
prebuilt binaries, GPU backends and other platform implementations are omitted.
The original notices and NVIDIA Source Code License are retained in each
component. The installed addon carries the license as
`Installation instructions/NVCLOTH-LICENSE.txt`.

One portability change: `NvCloth/src/SwSolverKernel.cpp` restricts the Windows
MSVC-specific SSE constraint overload to `PX_VC`. LLVM-MinGW and GCC use the
existing portable SIMD implementation, as the Linux build does. No solver
algorithm has been rewritten. CPU SSE2 is used; CUDA, DirectX compute and AVX
are not required. `tools/build_nvcloth.py` builds a static library without a
network download. The CPU build uses `-fno-strict-aliasing`: the SIMD helpers
access vector registers through float/integer array views. GCC's strict-aliasing
optimization produced invalid sphere-grid indices and intermittent crashes in
the capsule regression; sanitizer instrumentation alone masked that failure.
The sphere/capsule regression is therefore also run in the optimized ordinary
build. The game ships one SaureksCloset.dll as before.

NvCloth/PxShared remain under their own license, not the license covering the
project's original integration code.
