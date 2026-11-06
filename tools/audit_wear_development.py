"""Offline source/guide consistency; never start SDL, an app, a core or a device."""
from pathlib import Path
import re,struct,xml.etree.ElementTree as ET
r=Path(__file__).resolve().parents[1]
read=lambda p:(r/p).read_text(encoding='utf-8')
java=read('android/mobile/src/main/java/com/vhaloo/soniccompilationultra/mobile/MobileActivity.java')
for token in ['FEATURE_WATCH','SOURCE_ROTARY_ENCODER','AXIS_SCROLL','isScreenRound()','nativeRotary(delta)','dispatchGenericMotionEvent','romInboxPath','resolveActivity']:
    assert token in java,token
manifest=read('android/wear/src/main/AndroidManifest.xml');ET.fromstring(manifest)
assert 'android.hardware.type.watch' in manifest and 'com.google.android.wearable.standalone' in manifest
assert 'android:value="true"' in manifest and 'OpenXR' not in manifest and 'oculus' not in manifest
assert 'INTERNET' not in manifest and 'EXTERNAL_STORAGE' not in manifest
gradle=read('android/mobile/build.gradle')
assert "project.name == 'wear'" in gradle and 'SCU_WEAR_BUILD=' in gradle and 'scuWearPrivateAssets' in gradle
assert "':wear'" in read('android/settings.gradle')
translations=set(re.findall(r'T\("([^"]+)"',read('src/localization.cpp')))
help_text=read('src/setting_help.cpp')
helps=set(re.findall(r'H\("([^"]+)"',help_text))
aliases=set()
for group in re.findall(r'for\s*\(auto alias\s*:\s*\{([^}]+)\}',help_text): aliases.update(re.findall(r'"([^"]+)"',group))
aliases.add('questsurroundbrightness')
controls=set()
for file in ['ui.cpp','mobile.cpp','quest.cpp']:
    controls.update(re.findall(r'(?:slider|slide|choice|tick|check)\("([^"]+)"',read('src/'+file)))
assert controls<=translations,sorted(controls-translations)
assert controls<=helps|aliases,sorted(controls-helps-aliases)
for field in ['wearZoom','wearCrownSpeed','wearLensStrength','wearFisheye','wearCrownReverse']:
    assert 'X('+field in read('src/settings.cpp')
assert 'wearActive)' not in read('src/settings.cpp') and 'wearZoomValue)' not in read('src/settings.cpp')
ui=read('src/ui.cpp');assert 'settingBadge(key)' in ui and 'ImGuiHoveredFlags_DelayNormal' in ui
assert 'settings-preview.bmp' in ui and 'loadSettingsPreview' in ui and 'quickqualitygroup' in ui
bmp=(r/'assets/settings-preview.bmp').read_bytes()
assert bmp[:2]==b'BM' and struct.unpack_from('<ii',bmp,18)==(1280,900) and len(bmp)<1300000
assert 'SDL_RenderGeometryRaw' in read('src/mobile.cpp') and 'controlsDraw' in read('src/mobile.cpp')
assert 'LensMesh::update' in read('src/wear_display.cpp') and 'crownZoom' in read('src/wear_display.cpp')
print(f'PASS Wear/source guide audit: {len(controls)} explained localized controls, 19 original examples, crown detection, standalone non-XR watch manifest and transient device facts.')
