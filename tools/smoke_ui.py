"""ROM-free UI smoke test, suitable for every build host."""
import argparse,json,pathlib,subprocess,os
p=argparse.ArgumentParser();p.add_argument('--build',type=pathlib.Path,required=True);a=p.parse_args()
build=a.build.resolve();exe=build/('Release/SonicCompilationUltra.exe' if os.name=='nt' else 'SonicCompilationUltra')
out=build/'ui-smoke';out.mkdir(exist_ok=True)
rows=[]
for language in ['en','fr','es','ja']:
    for size in ['640x360','320x240','1280x720']:
        case=out/(language+'-'+size);case.mkdir(exist_ok=True)
        capture=case/'capture.bmp'
        command=[str(exe),'--data-dir',str(case),'--headless','--software','--frames','6','--size',size,'--menu','--language',language,'--capture',str(capture)]
        result=subprocess.run(command,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True,timeout=40)
        if result.returncode or not capture.exists():raise RuntimeError(f'UI smoke failed {language} {size}: {result.stdout}')
        data=capture.read_bytes()
        if data[:2]!=b'BM':raise RuntimeError('Capture is not a bitmap')
        width=int.from_bytes(data[18:22],'little',signed=True);height=abs(int.from_bytes(data[22:26],'little',signed=True))
        if (width,height)!=tuple(map(int,size.split('x'))):raise RuntimeError('Wrong drawable dimensions')
        rows.append({'language':language,'size':size,'exit':0,'capture':True})
(out/'report.json').write_text(json.dumps({'rom_data_used':False,'cases':rows},indent=2))
print(f'PASS: {len(rows)} ROM-free language / window-size / software-renderer cases')
