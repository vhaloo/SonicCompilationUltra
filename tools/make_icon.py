"""Build platform icons from the project's original palm illustration."""
from pathlib import Path
from PIL import Image
root = Path(__file__).resolve().parents[1]
with Image.open(root / 'assets/SCU.png') as source:
    image = source.convert('RGBA')
assert image.width == image.height
image.resize((256, 256), Image.Resampling.NEAREST).save(
    root / 'assets/SCU.ico', sizes=[(16,16),(24,24),(32,32),(48,48),(64,64),(128,128),(256,256)])
image.resize((1024, 1024), Image.Resampling.NEAREST).save(root / 'assets/SCU.icns')
image.convert('RGB').resize((128, 128), Image.Resampling.NEAREST).save(root / 'assets/SCU.bmp')
for module in ('app', 'mobile'):
    target = root / 'android' / module / 'src/main/res/drawable-nodpi/scu_icon.png'
    target.parent.mkdir(parents=True, exist_ok=True)
    image.resize((512, 512), Image.Resampling.NEAREST).save(target)
