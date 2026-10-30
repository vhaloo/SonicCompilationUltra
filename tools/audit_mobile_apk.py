"""Static phone/tablet APK audit. No application, emulator or device execution."""
from pathlib import Path,PurePosixPath
import argparse,hashlib,io,json,os,re,subprocess,tempfile,zipfile,zlib
p=argparse.ArgumentParser();p.add_argument('apk',type=Path);p.add_argument('--sdk',type=Path,required=True)
p.add_argument('--jdk',type=Path);p.add_argument('--out',type=Path,required=True);p.add_argument('--allow-private',action='store_true')
a=p.parse_args();env=os.environ.copy()
if a.jdk:env.update(JAVA_HOME=str(a.jdk),PATH=str(a.jdk/'bin')+os.pathsep+env.get('PATH',''))
def run(args):
    args=[str(x) for x in args]
    if os.name=='nt' and args[0].endswith('.bat'):args=['cmd.exe','/d','/c',*args]
    result=subprocess.run(args,env=env,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True,timeout=120)
    if result.returncode:raise RuntimeError(result.stdout)
    return result.stdout
build=a.sdk/'build-tools/36.0.0';suffix='.exe' if os.name=='nt' else ''
signature=run([build/('apksigner.bat' if os.name=='nt' else 'apksigner'),'verify','--verbose','--print-certs',a.apk])
badging=run([build/('aapt2'+suffix),'dump','badging',a.apk])
for expected in ["name='com.vhaloo.soniccompilationultra.mobile'","versionCode='800'","versionName='0.8.0-mobile-preview'","minSdkVersion:'24'","targetSdkVersion:'36'",'MobileActivity']:
    assert expected in badging,expected
manifest=run([build/('aapt2'+suffix),'dump','xmltree','--file','AndroidManifest.xml',a.apk])
for banned in ['oculus','openxr','IMMERSIVE_HMD','android.permission.CAMERA','android.permission.RECORD_AUDIO',
               'android.permission.INTERNET','android.permission.READ_EXTERNAL_STORAGE','android.permission.WRITE_EXTERNAL_STORAGE']:
    assert banned not in manifest,banned
assert re.search(r'resizeableActivity[^\n]*=(?:true|0xffffffff)',manifest)
alignment=run([build/('zipalign'+suffix),'-c','-P','16','-v','4',a.apk])
assert 'Verification successful' in alignment
host='windows-x86_64' if os.name=='nt' else 'linux-x86_64'
llvm=a.sdk/'ndk/29.0.14206865/toolchains/llvm/prebuilt'/host/'bin'
readelf=llvm/('llvm-readelf'+suffix);nm=llvm/('llvm-nm'+suffix)
system={'libc.so','libm.so','libdl.so','liblog.so','libandroid.so','libEGL.so','libGLESv1_CM.so',
        'libGLESv2.so','libGLESv3.so','libOpenSLES.so','libaaudio.so','libvulkan.so','libz.so'}
abis={'arm64-v8a':(2,183),'armeabi-v7a':(1,40),'x86_64':(2,62)}
deps={};cartridges=[]
with zipfile.ZipFile(a.apk) as z,tempfile.TemporaryDirectory(prefix='scu-mobile-audit-') as temp:
    assert z.testzip() is None
    names=z.namelist();assert len(names)==len(set(names))
    for n in names:
        q=PurePosixPath(n);assert not q.is_absolute() and '..' not in q.parts,n
        assert q.suffix not in {'.jks','.keystore','.key'},n
    libs=[n for n in names if n.startswith('lib/') and n.endswith('.so')]
    assert {n.split('/')[1] for n in libs}==set(abis)
    for abi,(elf_class,machine) in abis.items():
        bundled={PurePosixPath(n).name for n in libs if n.startswith('lib/'+abi+'/')}
        required={'libmain.so','libSDL2.so','libc++_shared.so'}
        assert required<=bundled and bundled<=required|{'libomp.so'},(abi,bundled)
        for name in libs:
            if not name.startswith('lib/'+abi+'/'):continue
            content=z.read(name);assert content[:6]==b'\x7fELF'+bytes([elf_class,1])
            assert int.from_bytes(content[18:20],'little')==machine
            f=Path(temp)/PurePosixPath(name).name;f.write_bytes(content)
            needed=re.findall(r'Shared library: \[([^]]+)\]',run([readelf,'--dynamic','--wide',f]))
            assert set(needed)<=bundled|system,(name,needed)
            deps[name]=needed
            if elf_class==2:
                loads=[line for line in run([readelf,'--program-headers','--wide',f]).splitlines() if line.strip().startswith('LOAD ')]
                assert loads and all(int(line.split()[-1],16)>=16384 for line in loads),name
            if f.name=='libmain.so':assert re.search(r'\bSDL_main$',run([nm,'--dynamic','--defined-only',f]),re.M)
    prefix='assets/scu/';private=[n for n in names if n.startswith(prefix+'ROMs/') or n.startswith(prefix+'UserData/')]
    font=z.read(prefix+'assets/SCUInterface.ttf');assert len(font)>100000
    if not a.allow_private:assert not private,'Personal data in public APK'
    else:
        lines=z.read(prefix+'UserData/library.tsv').decode('utf-8').splitlines()
        for line in lines:
            game,crc,path,member=line.split('\t');q=PurePosixPath(path)
            assert not q.is_absolute() and '..' not in q.parts and q.parts[0]=='ROMs'
            with zipfile.ZipFile(io.BytesIO(z.read(prefix+path))) as cartridge:raw=cartridge.read(member)
            assert zlib.crc32(raw)&0xffffffff==int(crc,16),(game,path,member)
            cartridges.append({'id':game,'crc32':crc,'sha256':hashlib.sha256(raw).hexdigest()})
        assert len(cartridges)==12
        settings=dict(line.split('=',1) for line in z.read(prefix+'UserData/settings.ini').decode('utf-8').splitlines() if '=' in line)
        assert settings['mobileLayout']=='0' and settings['mobileTouch']=='0' and settings['view']=='2'
    for n in names:
        if n.startswith('assets/'):
            raw=z.read(n);assert b'C:\\Users\\' not in raw and b'E:\\Emulation' not in raw,n
report={'version':'0.8.0-mobile-preview','package':'com.vhaloo.soniccompilationultra.mobile',
        'bytes':a.apk.stat().st_size,'sha256':hashlib.sha256(a.apk.read_bytes()).hexdigest(),
        'signature_verified':True,'signature':signature,'abis':list(abis),'native_dependencies':deps,
        'elf64_load_alignment_minimum':16384,'zip_16k_alignment_verified':True,'font_bytes':len(font),
        'private_game_data_included':a.allow_private,'cartridges':cartridges,'application_executed':False,'device_runtime_qualified':False}
a.out.parent.mkdir(parents=True,exist_ok=True);a.out.write_text(json.dumps(report,indent=2),encoding='utf-8')
print('PASS static mobile APK: signature, non-XR manifest, 3 ABI dependency closures, SDL entry points, ELF/ZIP 16 KB alignment, font and library paths.')
print('PRIVATE' if a.allow_private else 'ROM-FREE',len(cartridges),'cartridge entries;',a.apk.stat().st_size,'bytes; no application executed.')
