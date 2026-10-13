"""ROM-free UI smoke test, suitable for every build host."""
import argparse,json,pathlib,subprocess,os
p=argparse.ArgumentParser();p.add_argument('--build',type=pathlib.Path,required=True);p.add_argument('--package',type=pathlib.Path);a=p.parse_args()
build=a.build.resolve();exe=build/('Release/SonicCompilationUltra.exe' if os.name=='nt' else 'SonicCompilationUltra')
if a.package:
    package=a.package.resolve()
    app=package/'Sonic Compilation Ultra.app/Contents'
    if app.exists():
        exe=app/'MacOS/SonicCompilationUltra';font=app/'Resources/assets/SCUInterface.ttf'
    else:
        exe=package/('SonicCompilationUltra.exe' if os.name=='nt' else 'SonicCompilationUltra.sh');font=package/'assets/SCUInterface.ttf'
    if not font.exists():raise RuntimeError('Interface font missing from portable package')
out=build/('package-smoke' if a.package else 'ui-smoke');out.mkdir(exist_ok=True)
rows=[]
for language in ['en','fr','es','ja']:
    for size in ['640x360','320x240','1280x720']:
        case=out/(language+'-'+size);case.mkdir(exist_ok=True)
        timeline=case/'pages.txt'
        timeline.write_text('\n'.join(f'{page*3} tab {page}\n{page*3+2} capture {case/f"page-{page}.bmp"}' for page in range(9)))
        command=[str(exe),'--data-dir',str(case),'--headless','--software','--frames','27','--size',size,'--menu','--language',language,'--script',str(timeline)]
        result=subprocess.run(command,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True,timeout=60)
        if result.returncode:raise RuntimeError(f'UI smoke failed {language} {size}: {result.stdout}')
        for page in range(9):
            capture=case/f'page-{page}.bmp'
            data=capture.read_bytes()
            if data[:2]!=b'BM':raise RuntimeError('Capture is not a bitmap')
            width=int.from_bytes(data[18:22],'little',signed=True);height=abs(int.from_bytes(data[22:26],'little',signed=True))
            if (width,height)!=tuple(map(int,size.split('x'))):raise RuntimeError('Wrong drawable dimensions')
            rows.append({'language':language,'size':size,'page':page,'exit':0,'capture':True})
(out/'report.json').write_text(json.dumps({'rom_data_used':False,'cases':rows},indent=2))
print(f'PASS: {len(rows)} ROM-free page / language / window-size / software-renderer captures')
