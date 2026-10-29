"""Static APK/signature/dependency audit. Never installs or executes the application."""
from pathlib import Path, PurePosixPath
import argparse, hashlib, json, os, re, subprocess, tempfile, zipfile, io, zlib

p=argparse.ArgumentParser()
p.add_argument('apk',type=Path)
p.add_argument('--sdk',type=Path,required=True)
p.add_argument('--jdk',type=Path)
p.add_argument('--out',type=Path,required=True)
p.add_argument('--allow-private',action='store_true')
a=p.parse_args();env=os.environ.copy()
if a.jdk:
    env['JAVA_HOME']=str(a.jdk)
    env['PATH']=str(a.jdk/'bin')+os.pathsep+env.get('PATH','')
tools=a.sdk/'build-tools/36.0.0'
def run(command):
    command=[str(x) for x in command]
    if os.name=='nt' and command[0].endswith('.bat'):command=['cmd.exe','/d','/c',*command]
    result=subprocess.run(command,env=env,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True,timeout=120)
    if result.returncode:raise RuntimeError(result.stdout)
    return result.stdout
signer=tools/('apksigner.bat' if os.name=='nt' else 'apksigner')
signature=run([signer,'verify','--verbose','--print-certs',a.apk])
aapt=tools/('aapt2.exe' if os.name=='nt' else 'aapt2')
badging=run([aapt,'dump','badging',a.apk])
assert "package: name='com.vhaloo.soniccompilationultra.quest'" in badging
assert "versionCode='500'" in badging and "versionName='0.5.0-quest-preview'" in badging
assert "native-code: 'arm64-v8a'" in badging
assert 'QuestActivity' in badging
manifest=run([aapt,'dump','xmltree','--file','AndroidManifest.xml',a.apk])
assert 'IMMERSIVE_HMD' in manifest and 'PASSTHROUGH' in manifest
assert 'com.oculus.permission.USE_SCENE' in manifest and 'com.oculus.permission.USE_ANCHOR_API' in manifest
for forbidden in ['android.permission.CAMERA','android.permission.RECORD_AUDIO','android.permission.INTERNET',
                  'android.permission.READ_EXTERNAL_STORAGE','android.permission.WRITE_EXTERNAL_STORAGE']:
    assert forbidden not in manifest,forbidden
host='windows-x86_64' if os.name=='nt' else 'linux-x86_64'
llvm=a.sdk/'ndk/29.0.14206865/toolchains/llvm/prebuilt'/host/'bin'
readelf=llvm/('llvm-readelf.exe' if os.name=='nt' else 'llvm-readelf')
nm=llvm/('llvm-nm.exe' if os.name=='nt' else 'llvm-nm')
system={'libc.so','libm.so','libdl.so','liblog.so','libandroid.so','libEGL.so','libGLESv1_CM.so',
        'libGLESv2.so','libGLESv3.so','libOpenSLES.so','libaaudio.so','libvulkan.so','libz.so'}
dependencies={};cartridges=[]
with zipfile.ZipFile(a.apk) as z, tempfile.TemporaryDirectory(prefix='scu-quest-static-') as temp:
    assert z.testzip() is None
    names=z.namelist();assert len(names)==len(set(names))
    for name in names:
        q=PurePosixPath(name)
        assert not q.is_absolute() and '..' not in q.parts,name
    libraries=[n for n in names if n.startswith('lib/') and n.endswith('.so')]
    assert libraries and all(n.startswith('lib/arm64-v8a/') for n in libraries)
    bundled={PurePosixPath(n).name for n in libraries}
    assert {'libmain.so','libSDL2.so','libopenxr_loader.so','libc++_shared.so'}<=bundled
    for name in libraries:
        file=Path(temp)/PurePosixPath(name).name
        content=z.read(name);file.write_bytes(content)
        assert content[:6]==b'\x7fELF\x02\x01' and int.from_bytes(content[18:20],'little')==183,name
        dynamic=run([readelf,'--dynamic','--wide',file])
        needed=re.findall(r'Shared library: \[([^]]+)\]',dynamic)
        assert set(needed)<=bundled|system,(name,needed)
        dependencies[file.name]=needed
        if file.name=='libmain.so':
            symbols=run([nm,'--dynamic','--defined-only',file])
            assert re.search(r'\bSDL_main$',symbols,re.M),'Android SDL entry point missing'
    font=z.read('assets/scu/assets/SCUInterface.ttf');assert len(font)>100000
    assert 'assets/scu/licences/OpenXR-loader-Apache-2.0.txt' in names
    prefix='assets/scu/'
    private=[n for n in names if n.startswith(prefix+'ROMs/') or n.startswith(prefix+'UserData/')]
    if not a.allow_private:
        assert not private,'Private cartridges/user data included in ROM-free APK'
        assert prefix+'personal-wall-preset-0500.txt' not in names,'Personal preset in ROM-free APK'
    else:
        assert prefix+'personal-wall-preset-0500.txt' in names,'Personal wall profile flag missing'
        settings=dict(line.split('=',1) for line in z.read(prefix+'UserData/settings.ini').decode().splitlines() if '=' in line)
        for key,value in {'questMode':'2','questPassthrough':'1','questWallHeight':'200','questRefresh':'0','questWallLocked':'1','questCompanion':'1'}.items():
            assert settings.get(key)==value,(key,settings.get(key))
        lines=z.read(prefix+'UserData/library.tsv').decode().splitlines()
        for line in lines:
            game,crc,path,member=line.split('\t')
            q=PurePosixPath(path)
            assert not q.is_absolute() and '..' not in q.parts and q.parts[0]=='ROMs'
            with zipfile.ZipFile(io.BytesIO(z.read(prefix+path))) as cartridge:raw=cartridge.read(member)
            assert zlib.crc32(raw)&0xffffffff==int(crc,16),(game,crc)
            cartridges.append({'id':game,'crc32':crc,'sha256':hashlib.sha256(raw).hexdigest()})
        assert len(cartridges)==12
    for name in names:
        if name.startswith('assets/'):
            data=z.read(name)
            assert b'C:\\Users\\' not in data and b'E:\\Emulation' not in data,name
report={'apk':str(a.apk),'sha256':hashlib.sha256(a.apk.read_bytes()).hexdigest(),'bytes':a.apk.stat().st_size,
        'signature_verified':True,'signature':signature,'package':'com.vhaloo.soniccompilationultra.quest',
        'version':'0.5.0-quest-preview','abi':'arm64-v8a','native_dependencies':dependencies,
        'private_game_data_included':a.allow_private,'cartridges':cartridges,
        'font_bytes':len(font),'application_executed':False,'headset_runtime_qualified':False}
a.out.parent.mkdir(parents=True,exist_ok=True);a.out.write_text(json.dumps(report,indent=2))
print('PASS static APK audit: signature, manifest, ARM64 libraries, SDL entry point, font and local-data boundary')
print('PRIVATE' if a.allow_private else 'ROM-FREE',len(cartridges),'cartridge entries;',a.apk.stat().st_size,'bytes')
print('No game, installer or headset session was executed.')
