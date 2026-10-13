"""Portable distribution packager. Reads build outputs, never user data/ROMs."""
from __future__ import annotations
import argparse, hashlib, json, os, pathlib, platform, re, shutil, subprocess, tarfile, zipfile

ROOT = pathlib.Path(__file__).resolve().parents[1]
BANNED = {'.bin','.rom','.gen','.smd','.rsdk','.srm','.state','.ram','.wav','.mp3','.ogg','.tga','.bmp'}

def copy_file(source: pathlib.Path, destination: pathlib.Path):
    destination.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(source, destination)

def audit_tree(root: pathlib.Path):
    files = []
    for p in sorted(root.rglob('*')):
        if not p.is_file():
            continue
        rel = p.relative_to(root).as_posix()
        if p.suffix.lower() in BANNED or p.name in {'library.tsv','settings.ini','imgui.ini'}:
            raise RuntimeError(f'User/game-data file in package: {rel}')
        data = p.read_bytes()
        if p.suffix.lower() not in {'.c','.cpp','.h','.md','.txt','.json','.py','.svg','.yml','.in'}:
            if len(data) >= 512 and data[0x100:0x104] in (b'SEGA',b'ESAG'):
                raise RuntimeError(f'Cartridge signature in package: {rel}')
        files.append({'path':rel,'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest()})
    return files

def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--build',type=pathlib.Path,required=True)
    parser.add_argument('--out',type=pathlib.Path,default=ROOT/'dist')
    parser.add_argument('--platform',choices=['windows','linux','macos'],required=True)
    parser.add_argument('--arch',default=platform.machine())
    parser.add_argument('--version', default=re.search(r'project\(SonicCompilationUltra VERSION ([0-9.]+)', (ROOT/'CMakeLists.txt').read_text()).group(1))
    parser.add_argument('--source',action='store_true')
    args=parser.parse_args()
    build=args.build.resolve();out=args.out.resolve();out.mkdir(parents=True,exist_ok=True)
    label=f'SonicCompilationUltra-{args.version}-{args.platform}-{args.arch}'
    stage=out/label
    if stage.exists():
        raise RuntimeError(f'Package directory already exists; choose a fresh output path: {stage}')
    stage.mkdir()
    binary=build/'Release/SonicCompilationUltra.exe' if args.platform=='windows' else build/'SonicCompilationUltra'
    if not binary.exists():
        raise RuntimeError(f'Build output missing: {binary}')
    payload=stage
    if args.platform=='macos':
        payload=stage/'Sonic Compilation Ultra.app/Contents'
        copy_file(binary,payload/'MacOS/SonicCompilationUltra')
        resources=payload/'Resources'
        resources.mkdir(parents=True,exist_ok=True)
        shutil.copytree(ROOT/'assets',resources/'assets')
        copy_file(ROOT/'assets/SCU.icns',resources/'SCU.icns')
        info={'CFBundleName':'Sonic Compilation Ultra','CFBundleDisplayName':'Sonic Compilation Ultra','CFBundleIdentifier':'com.vhaloo.soniccompilationultra','CFBundleExecutable':'SonicCompilationUltra','CFBundleIconFile':'SCU.icns','CFBundlePackageType':'APPL','CFBundleShortVersionString':args.version,'CFBundleVersion':args.version,'NSHighResolutionCapable':True,'LSMinimumSystemVersion':'11.0' if args.arch=='arm64' else '10.15'}
        import plistlib
        (payload/'Info.plist').write_bytes(plistlib.dumps(info))
        result=subprocess.check_output(['otool','-L',str(binary)],text=True)
        sdl=next((line.strip().split(' ')[0] for line in result.splitlines() if 'SDL2' in line),None)
        if sdl:
            candidates=list(build.rglob('libSDL2*.dylib'))
            actual=pathlib.Path(sdl)
            if not actual.exists():
                if not candidates:raise RuntimeError('SDL dylib missing')
                actual=candidates[0]
            target=payload/'Frameworks'/actual.name
            copy_file(actual.resolve(),target)
            subprocess.run(['install_name_tool','-change',sdl,'@executable_path/../Frameworks/'+target.name,str(payload/'MacOS/SonicCompilationUltra')],check=True)
            subprocess.run(['install_name_tool','-id','@executable_path/../Frameworks/'+target.name,str(target)],check=True)
        subprocess.run(['codesign','--force','--deep','--sign','-',str(stage/'Sonic Compilation Ultra.app')],check=True)
    else:
        copy_file(binary,stage/binary.name)
        shutil.copytree(ROOT/'assets',stage/'assets')
        if args.platform=='windows':
            for name in ['SDL2.dll','vcomp140.dll','msvcp140.dll','vcruntime140.dll','vcruntime140_1.dll']:
                candidates=list(build.rglob(name))
                if not candidates and name=='vcomp140.dll':
                    candidate=pathlib.Path(os.environ.get('WINDIR','C:/Windows'))/'System32'/name
                    if candidate.exists():candidates=[candidate]
                if candidates:copy_file(candidates[0],stage/name)
            if not (stage/'SDL2.dll').exists():raise RuntimeError('SDL2.dll missing')
        else:
            candidates=list(build.rglob('libSDL2*.so*'))
            if candidates:
                actual=next((p for p in candidates if p.is_file() and not p.is_symlink()),candidates[0])
                copy_file(actual.resolve(),stage/'libSDL2-2.0.so.0')
            script='#!/bin/sh\nscu_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)\nexport LD_LIBRARY_PATH="$scu_dir${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"\nexec "$scu_dir/SonicCompilationUltra" "$@"\n'
            (stage/'SonicCompilationUltra.sh').write_text(script)
            (stage/'SonicCompilationUltra.sh').chmod(0o755)
    for name in ['README.md','LICENSE','THIRD_PARTY_NOTICES.md','CHANGELOG.md']:
        copy_file(ROOT/name,stage/name)
    shutil.copytree(ROOT/'licenses',stage/'licenses')
    shutil.copytree(ROOT/'docs',stage/'docs')
    manifest=audit_tree(stage)
    (stage/'MANIFEST.json').write_text(json.dumps({'version':args.version,'platform':args.platform,'architecture':args.arch,'files':manifest,'game_data_included':False},indent=2))
    if args.platform=='windows':
        archive=out/(label+'.zip')
        with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED,compresslevel=9) as z:
            for p in sorted(stage.rglob('*')):
                if p.is_file():z.write(p,p.relative_to(out).as_posix())
    else:
        archive=out/(label+'.tar.gz')
        with tarfile.open(archive,'w:gz') as tar:tar.add(stage,arcname=label)
    print(archive)
    if args.source:
        source=out/f'SonicCompilationUltra-{args.version}-source.zip'
        tracked=subprocess.check_output(['git','ls-files','-z'],cwd=ROOT).decode().split('\0')
        # Preserve the canonical archive, including upstream symlinks. Windows
        # extraction creates inaccessible Android links on some build hosts.
        candidates=list(build.rglob('SDL2-2.32.10.tar.gz'))
        bundled=ROOT/'vendor/SDL2-2.32.10.tar.gz'
        if bundled.exists():candidates.insert(0,bundled)
        if not candidates:raise RuntimeError('Pinned SDL archive required for corresponding source')
        sdl=candidates[0]
        if hashlib.sha256(sdl.read_bytes()).hexdigest()!='5f5993c530f084535c65a6879e9b26ad441169b3e25d789d83287040a9ca5165':
            raise RuntimeError('SDL source archive checksum mismatch')
        with zipfile.ZipFile(source,'w',zipfile.ZIP_DEFLATED,compresslevel=9) as z:
            for name in sorted(n for n in tracked if n):
                p=ROOT/name
                if p.is_file():z.write(p,'SonicCompilationUltra/'+name)
            z.write(sdl,'SonicCompilationUltra/vendor/SDL2-2.32.10.tar.gz')
        print(source)
    sums=[]
    for p in sorted(out.glob('*.zip'))+sorted(out.glob('*.tar.gz')):
        sums.append(hashlib.sha256(p.read_bytes()).hexdigest()+'  '+p.name)
    (out/'SHA256SUMS.txt').write_text('\n'.join(sums)+'\n')

if __name__=='__main__':main()
