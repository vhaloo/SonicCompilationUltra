"""Fail publication checks if private game data, generated game C or paths appear."""
from pathlib import Path
import argparse,subprocess,re,hashlib,json
root=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--staged',action='store_true');a=p.parse_args()
command=['git','diff','--cached','--name-only','-z'] if a.staged else ['git','ls-files','-z']
files=[x for x in subprocess.check_output(command,cwd=root).decode().split('\0') if x]
bad=[]
banned={'.bin','.gen','.smd','.rom','.rsdk','.srm','.state','.ram','.wav','.mp3','.ogg','.tga','.bmp','.exe','.dll','.pdb','.obj','.lib','.apk','.aab','.jks','.keystore'}
texttypes={'.md','.txt','.json','.py','.cpp','.c','.h','.yml','.cmake','.in','.svg','.csv','.gradle','.java','.xml','.properties','.inl'}
rows=[]
for name in files:
    path=root/name
    if not path.is_file():continue
    data=path.read_bytes()
    original_preview=name=='assets/settings-preview.bmp' and hashlib.sha256(data).hexdigest()=='dd663cf8fcb966ea47124bf9920cf0d79ce2f13d1c6ff761a0f5addb8cd96239'
    original_icon=name=='assets/SCU.bmp' and hashlib.sha256(data).hexdigest()=='14c2250708f34f4b44a64551364c8f3f92a3e3d734858b08a2e9f32f74ffdd38'
    if (path.suffix.lower() in banned and not original_preview and not original_icon) or path.name in {'settings.ini','library.tsv','quest-wall-anchor.txt'} or re.search(r'^(generated|private|private_assets|build[^/]*)/', name):bad.append(name)
    if path.suffix.lower() in texttypes and not name.startswith('vendor/'):
        text=data.decode('utf-8',errors='replace')
        if re.search(r'(?i)(?:[A-Z]:\\Users\\|E:[/\\]Emulation|gh[pousr]_[A-Za-z0-9]{20,}|github_pat_[A-Za-z0-9]{30,})',text):bad.append(name+' (private path/token)')
    rows.append({'file':name,'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest()})
if bad:raise SystemExit('PUBLIC AUDIT FAILED:\n'+'\n'.join(bad))
print(f'PASS: {len(rows)} tracked/staged files; no ROM, private state/path or compiled binary in source')
