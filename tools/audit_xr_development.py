"""Source/guide consistency only. Never import SDL, contact ADB or run the app."""
from pathlib import Path
import csv,re,xml.etree.ElementTree as ET

root=Path(__file__).resolve().parents[1]
localization=(root/'src/localization.cpp').read_text(encoding='utf-8')
keys=re.findall(r'T\("([^"]+)"',localization)
assert len(keys)==len(set(keys)), 'Duplicate translation keys'
labels=set(re.findall(r'"([a-z][a-z0-9_]+)"',(root/'src/spatial_controls.cpp').read_text(encoding='utf-8')))
assert labels<=set(keys), 'Missing spatial translations: '+str(sorted(labels-set(keys)))
new_options={'questremote','questtransparent','questhandtracking','questhandjoystick','questremotescale',
             'questoptionsscale','questmoverotate','questcontrolhints','questinsetcrt','questinsetwidth','questeyedetail',
             'questtvmodel','questcoplanar','spinballtriggers','objectpreview','objectorigins','frozenpreviews'}
help_keys=set(re.findall(r'H\("([^"]+)"',(root/'src/setting_help.cpp').read_text(encoding='utf-8')))
assert new_options<=set(keys) and new_options<=help_keys, 'New option lacks translation or illustrated guide'
settings=(root/'src/settings.h').read_text(encoding='utf-8')
assert re.search(r'questRefresh\s*=\s*90\b',settings)
assert re.search(r'questTransparent\s*=\s*true\b',settings)
java=(root/'android/app/src/main/java/com/vhaloo/soniccompilationultra/quest/QuestActivity.java').read_text(encoding='utf-8')
assert 'questRefresh=90\\nquestTransparent=1' in java
ET.parse(root/'docs/illustrations/spatial-remote.svg')
ET.parse(root/'android/app/src/main/AndroidManifest.xml')
with (root/'docs/MOVING_OBJECTS.csv').open(encoding='utf-8',newline='') as f:rows=list(csv.DictReader(f))
assert len(rows)==152 and len({(r['profile'],r['object_id_hex']) for r in rows})==152
for p in (root/'src').iterdir():
    if p.suffix not in {'.cpp','.h','.inl'}:continue
    text=p.read_text(encoding='utf-8')
    assert chr(0xc2)+chr(0xb0) not in text, 'Mojibake degree mark: '+p.name
assert '0.7.0' in (root/'CMakeLists.txt').read_text()
print(f'PASS XR source/guide audit: {len(labels)} remote labels, {len(new_options)} illustrated options, 152 metadata rows; 90 Hz/transparent defaults.')
print('No application, cartridge or headset operation executed.')
