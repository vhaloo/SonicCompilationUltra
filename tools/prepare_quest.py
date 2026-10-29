"""Fetch the pinned SDL Android sources. Does not build or run the application."""
from pathlib import Path
import argparse, hashlib, tarfile, urllib.request

p=argparse.ArgumentParser()
p.add_argument('--out',type=Path,default=Path(__file__).resolve().parents[1]/'android/.deps')
a=p.parse_args();a.out.mkdir(parents=True,exist_ok=True)
archive=a.out/'SDL2-2.32.10.tar.gz'
expected='5f5993c530f084535c65a6879e9b26ad441169b3e25d789d83287040a9ca5165'
if not archive.exists():
    urllib.request.urlretrieve('https://github.com/libsdl-org/SDL/releases/download/release-2.32.10/SDL2-2.32.10.tar.gz',archive)
if hashlib.sha256(archive.read_bytes()).hexdigest()!=expected:
    raise SystemExit('SDL source checksum mismatch')
source=a.out/'SDL2-2.32.10'
if not (source/'CMakeLists.txt').exists():
    with tarfile.open(archive) as tar:
        for member in tar.getmembers():
            target=(a.out/member.name).resolve()
            if not target.is_relative_to(a.out.resolve()):raise RuntimeError('Unsafe archive path')
            # The Android JNI source symlink is unnecessary with this CMake build;
            # skipping links also permits extraction on ordinary Windows accounts.
            if not member.isfile() and not member.isdir():continue
            if member.isdir():target.mkdir(parents=True,exist_ok=True)
            else:
                target.parent.mkdir(parents=True,exist_ok=True)
                target.write_bytes(tar.extractfile(member).read())
print(source.resolve())
