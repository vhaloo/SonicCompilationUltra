"""Static source/guide/pipeline checks; no application, GPU or device execution."""
from pathlib import Path
import re
r=Path(__file__).resolve().parents[1]
read=lambda p:(r/p).read_text(encoding='utf-8')
keys=set(re.findall(r'T\("([^"]+)"',read('src/localization.cpp')))
help_keys=set(re.findall(r'H\("([^"]+)"',read('src/setting_help.cpp')))
options={'autoperformance','performancetarget','minimumrenderscale','maximumrenderscale',
         'performancepixellock','thermalaware','questshaderquality'}
assert options<=keys and options<=help_keys
settings=read('src/settings.h');serializer=read('src/settings.cpp')
for key in ['autoPerformance','performanceTarget','minimumRenderScale','maximumRenderScale','performancePixelLock','thermalAware','questShaderQuality']:
    assert key in settings and 'X('+key in serializer
assert re.search(r'autoPerformance\s*=\s*true',settings)
assert re.search(r'performanceTarget\s*=\s*60',settings)
world=read('src/world.cpp');ui=read('src/ui.cpp');main=read('src/main.cpp')
assert 'canvas.pixels = terrainPixels;' not in world
assert 'actorsDamage.restore' in world and 'textureDamage.merge(actorsDamage)' in world
assert 'terrainAttributes.sample' in world and 'terrainGeometryRevision(s.ram' in world
assert 'core.world.uploadRegions' in ui and 'createdWorldTexture' in ui
assert 'FrameDeadline pacing' in main and 'observe(sample, rawElapsed)' in main
assert 'core.executionDisabled = a.menuVerificationOnly' in main
timer=read('src/xr_gpu_timer.h')
assert 'GL_QUERY_RESULT_AVAILABLE_EXT' in timer and 'GL_GPU_DISJOINT_EXT' in timer
assert 'p.valid' in timer and 'p.sequence>lastSequence' in re.sub(r'\s+','',timer)
assert 'glFinish' not in timer and 'sleep' not in timer
shader=read('src/xr_stage.cpp')
assert '#ifdef SCU_FAST' in shader and 'programs[index]' in shader and 'selectShader' in shader
assert 'renderWidth, renderHeight' in shader and 'gpuTimer.finishEye' in shader
assert 'content.targetFrameMs' in shader and 'shaderUpgradeAfter' in shader
for p in ['android/app/src/main/java/com/vhaloo/soniccompilationultra/quest/QuestActivity.java',
          'android/mobile/src/main/java/com/vhaloo/soniccompilationultra/mobile/MobileActivity.java']:
    assert 'getCurrentThermalStatus()' in read(p)
print('PASS performance source/guide audit: 7 localized illustrated controls, active-work governor, damage uploads, cached geometry, optional non-blocking/disjoint-safe GPU timing and precompiled shader variants.')
