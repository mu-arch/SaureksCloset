"""Build the pinned CPU-only NvCloth subset; no downloads or GPU runtime needed."""
from pathlib import Path
import argparse
import concurrent.futures
import hashlib
import subprocess

ROOT = Path(__file__).resolve().parents[1]
VENDOR = ROOT / 'native/vendor/nvcloth'
NV = VENDOR / 'NvCloth'

def flags():
    result = ['-std=c++17', '-O2', '-DNDEBUG', '-DNV_CLOTH_IMPORT=',
              '-DNV_CLOTH_ENABLE_CUDA=0', '-DNV_CLOTH_ENABLE_DX11=0',
              '-fno-exceptions', '-fno-rtti', '-msse2']
    for folder in [NV/'include', NV/'include/NvCloth', NV/'include/NvCloth/ps',
                   NV/'src', NV/'extensions/include', NV/'extensions/src', VENDOR/'PxShared/include']:
        result += ['-isystem', str(folder)]
    return result

def build(output, cxx='c++', ar='ar', windows=False):
    output = Path(output).resolve()
    output.mkdir(parents=True, exist_ok=True)
    options = flags() + (['-fms-extensions'] if windows and 'clang' in cxx else [])
    sources = sorted((NV/'src').glob('*.cpp')) + sorted((NV/'extensions/src').glob('*.cpp'))
    sources += [NV/'include/NvCloth/ps/PsAllocator.cpp', NV/('src/ps/windows/PsWindowsAtomic.cpp' if windows else 'src/ps/unix/PsUnixAtomic.cpp')]
    digest = hashlib.sha256((' '.join(options)+cxx+ar).encode())
    for source in sorted(VENDOR.rglob('*')):
        if source.is_file():
            digest.update(str(source.relative_to(VENDOR)).encode())
            digest.update(source.read_bytes())
    stamp = output/'build.sha256'
    archive = output/'libnvcloth.a'
    if archive.exists() and stamp.exists() and stamp.read_text() == digest.hexdigest():
        return archive
    def compile_one(source):
        obj = output/(source.stem+'.o')
        subprocess.run([cxx, *options, '-w', '-c', str(source), '-o', str(obj)], check=True)
        return obj
    with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
        objects = list(pool.map(compile_one, sources))
    subprocess.run([ar, 'rcsD', str(archive), *map(str, objects)], check=True)
    stamp.write_text(digest.hexdigest())
    return archive

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', required=True)
    parser.add_argument('--cxx', default='c++')
    parser.add_argument('--ar', default='ar')
    parser.add_argument('--windows', action='store_true')
    args = parser.parse_args()
    print(build(args.output, args.cxx, args.ar, args.windows))
