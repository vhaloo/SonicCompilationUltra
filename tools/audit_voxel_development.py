"""Static language, font and original-fixture provenance checks; no runtime."""
from pathlib import Path
import ast, hashlib, json, re, struct

root=Path(__file__).resolve().parents[1]
def calls(text,name):
    result={}
    for match in re.finditer(r'\b'+name+r'\s*\(',text):
        start=match.end();quoted=False;escape=False;depth=1;parts=[];last=start
        for i in range(start,len(text)):
            c=text[i]
            if quoted:
                if escape:escape=False
                elif c=='\\':escape=True
                elif c=='"':quoted=False
            elif c=='"':quoted=True
            elif c=='(':depth+=1
            elif c==')':
                depth-=1
                if not depth:parts.append(text[last:i]);break
            elif c==',' and depth==1:parts.append(text[last:i]);last=i+1
        if not parts or not parts[0].strip().startswith('"'):continue
        strings=lambda p:''.join(ast.literal_eval(s) for s in re.findall(r'"(?:[^"\\]|\\.)*"',p))
        key=strings(parts[0]);assert key not in result,('duplicate',name,key)
        result[key]=[strings(p) for p in parts[1:]]
    return result

translations=calls((root/'src/localization.cpp').read_text(encoding='utf-8'),'T')
helps=calls((root/'src/setting_help.cpp').read_text(encoding='utf-8'),'H')
new={key:value for key,value in translations.items() if key.startswith('voxel') or key=='remote_depth'}
assert len(new)>=30
for key,values in translations.items():assert len(values)==4 and all(values),(key,'four nonempty language strings')
for key,values in helps.items():assert len(values)==5 and all(values[1:]),(key,'four complete guide texts')
controls={'voxelmode','voxelpreset','voxelthickness','voxelspacing','voxelrelief','voxellight',
          'voxelbackground','voxeldetail','voxelyaw','voxelpitch','voxelcrt','voxelexport'}
assert controls<=set(helps) and controls<=set(translations)
for key in controls:assert helps[key][0]=='',('Expected symbolic picture field',key)
required=set(''.join(''.join(v) for v in translations.values())+
             ''.join(''.join(v[1:]) for v in helps.values()))
required={ord(c) for c in required if ord(c)>32}
font=(root/'assets/SCUInterface.ttf').read_bytes()
u16=lambda offset:struct.unpack_from('>H',font,offset)[0]
u32=lambda offset:struct.unpack_from('>I',font,offset)[0]
tables={font[12+i*16:16+i*16]:u32(20+i*16) for i in range(u16(4))}
cmap=tables[b'cmap'];covered=set()
for index in range(u16(cmap+2)):
    table=cmap+u32(cmap+4+index*8+4);fmt=u16(table)
    if fmt==12:
        for group in range(u32(table+12)):
            at=table+16+group*12;start,end,glyph=u32(at),u32(at+4),u32(at+8)
            covered.update(c for c in required if start<=c<=end and glyph+c-start)
    elif fmt==4:
        count=u16(table+6)//2;ends=table+14;starts=ends+count*2+2;deltas=starts+count*2;ranges=deltas+count*2
        for i in range(count):
            start,end=u16(starts+i*2),u16(ends+i*2);delta=u16(deltas+i*2);offset=u16(ranges+i*2)
            for c in required:
                if not start<=c<=end:continue
                if offset:
                    address=ranges+i*2+offset+2*(c-start)
                    assert address+2<=len(font)
                    glyph=u16(address);glyph=(glyph+delta)&65535 if glyph else 0
                else:glyph=(c+delta)&65535
                if glyph:covered.add(c)
missing=required-covered
assert not missing,'Missing interface glyphs: '+ascii(''.join(map(chr,sorted(missing))))
provenance=json.loads((root/'docs/illustrations/voxel-provenance.json').read_text(encoding='utf-8'))
assert not provenance['rom_or_emulator_input'] and not provenance['gpu_or_application_executed']
for item in provenance['files']:
    data=(root/'docs/illustrations'/item['file']).read_bytes()
    assert hashlib.sha256(data).hexdigest()==item['sha256']
    assert data[:8]==b'\x89PNG\r\n\x1a\n' and struct.unpack_from('>II',data,16)==(960,540)
print(f'PASS static voxel guide audit: {len(new)} new four-language labels, {len(controls)} explained controls, {len(required)} font characters and three original mesh-fixture images.')
print('Language/provenance inspection only. Geometry, platform compilation and actual-device performance are separate checks.')
