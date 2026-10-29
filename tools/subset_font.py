"""Rebuild the OFL interface subset from the pinned, locally supplied font."""
from pathlib import Path
import argparse,hashlib
from fontTools.ttLib import TTFont
from fontTools.varLib.instancer import instantiateVariableFont
from fontTools import subset
p=argparse.ArgumentParser();p.add_argument('--source',type=Path,required=True);a=p.parse_args()
assert hashlib.sha256(a.source.read_bytes()).hexdigest()=='c2f3b4d463500a2ddcd3849cded1fceeb9fd6d1c32e6cbecd568453ba50fc68f','Unexpected font source; see docs/VENDOR.md'
root=Path(__file__).resolve().parents[1]
text=''.join(p.read_text(encoding='utf-8') for p in (root/'src').glob('*') if p.suffix in {'.cpp','.h'})
chars=set(range(32,256))|set(map(ord,text))|set(range(0x2000,0x2070))|{0x2212,0x25A1,0x2026}
font=instantiateVariableFont(TTFont(a.source),{'wght':450},inplace=True)
options=subset.Options();options.name_IDs=['*']
sub=subset.Subsetter(options=options);sub.populate(unicodes=chars);sub.subset(font)
for name in font['name'].names:
    if name.nameID in {1,2,3,4,6,16,17}:
        value='Regular' if name.nameID in {2,17} else 'SCUInterface-Regular' if name.nameID==6 else 'SCU Interface'
        name.string=value.encode(name.getEncoding(),errors='replace')
target=root/'assets/SCUInterface.ttf';temporary=target.with_suffix('.ttf.new')
font.save(temporary);temporary.replace(target)
ui=''.join((root/'src'/p).read_text(encoding='utf-8') for p in ['localization.cpp','setting_help.cpp'])
cmap=font.getBestCmap();missing=sorted(c for c in set(ui) if ord(c)>32 and ord(c) not in cmap)
assert not missing,'Missing UI glyphs: '+ascii(''.join(missing))
print('PASS: all localization and setting-guide characters present;',target.stat().st_size,'font bytes')
