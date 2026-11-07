"""Publish only original generated fixtures from the ROM-free mesh helper."""
from pathlib import Path
import argparse, json, subprocess, hashlib
from PIL import Image

parser=argparse.ArgumentParser()
parser.add_argument('--helper',type=Path,required=True)
parser.add_argument('--temporary',type=Path,required=True)
args=parser.parse_args()
assert args.helper.is_file() and args.helper.stem=='voxel_preview'
repo=Path(__file__).resolve().parents[1]
temporary=args.temporary.resolve();temporary.mkdir(parents=True,exist_ok=True)
subprocess.run([str(args.helper.resolve()),str(temporary)],check=True)
target=repo/'docs/illustrations';target.mkdir(parents=True,exist_ok=True)
manifest=[]
for name in ['flat','layered','sculpted']:
    image=Image.open(temporary/(name+'.ppm')).convert('RGB')
    assert image.size==(960,540)
    path=target/('voxel-'+name+'.png');image.save(path,optimize=True)
    manifest.append({'file':path.name,'sha256':hashlib.sha256(path.read_bytes()).hexdigest()})
proof={'provenance':'Original fixture pixels and actual procedural mesh; CPU reference rasterizer, not an application or device capture.',
       'rom_or_emulator_input':False,'gpu_or_application_executed':False,'files':manifest}
(target/'voxel-provenance.json').write_text(json.dumps(proof,indent=2)+'\n',encoding='utf-8')
print('Saved three original mesh-fixture views and their provenance. No game assets.')
