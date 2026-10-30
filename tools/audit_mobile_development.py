"""Source/translation/manifest consistency only. No SDL, emulator or game execution."""
from pathlib import Path
import re,xml.etree.ElementTree as ET
r=Path(__file__).resolve().parents[1]
java=(r/'android/mobile/src/main/java/com/vhaloo/soniccompilationultra/mobile/MobileActivity.java').read_text(encoding='utf-8')
gradle=(r/'android/mobile/build.gradle').read_text(encoding='utf-8')
manifest=(r/'android/mobile/src/main/AndroidManifest.xml').read_text(encoding='utf-8')
assert 'minSdk 24' in gradle and 'targetSdk 36' in gradle
assert "'arm64-v8a', 'armeabi-v7a', 'x86_64'" in gradle
assert 'SCU_QUEST_BUILD=OFF' in gradle and 'scuMobilePrivateAssets' in gradle and 'scuPrivateAssets' not in gradle
assert 'OpenXR' not in java and 'openxr' not in gradle and 'oculus' not in manifest
assert 'android:resizeableActivity="true"' in manifest
assert 'INTERNET' not in manifest and 'EXTERNAL_STORAGE' not in manifest
ET.fromstring(manifest);ET.parse(r/'docs/illustrations/mobile-layouts.svg')
assert 'registerOnBackInvokedCallback' in java
assert 'Context.RECEIVER_NOT_EXPORTED' in gradle and 'mobileJavaPatchRevision' in gradle
assert 'systemGestures()' in java and 'displayCutout()' in java
assert 'staging.renameTo(target)' in java
assert 'copyTree("scu", root)' in java and '!target.exists()' in java
source=(r/'src/mobile.cpp').read_text(encoding='utf-8')
keys=set(re.findall(r'"(mobile[a-z]+)"',source))
translations=set(re.findall(r'T\("([^"]+)"',(r/'src/localization.cpp').read_text(encoding='utf-8')))
assert keys<=translations,sorted(keys-translations)
options=set(re.findall(r'app\.(?:choice|slider|tick)\("([^"]+)"',source))
help_keys=set(re.findall(r'H\("([^"]+)"',(r/'src/setting_help.cpp').read_text(encoding='utf-8')))
assert options<=help_keys, sorted(options-help_keys)
assert 'core.executionDisabled = a.menuVerificationOnly' in (r/'src/main.cpp').read_text(encoding='utf-8')
print(f'PASS mobile source audit: {len(options)} translated/illustrated settings; 3 ABIs; separate non-XR package; adaptive safe areas; private/public isolation.')
