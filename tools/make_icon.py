"""Render the project's own geometric U mark; no game artwork is used."""
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont
root = Path(__file__).resolve().parents[1]
image = Image.new('RGBA', (256, 256), (7, 22, 41, 255))
draw = ImageDraw.Draw(image)
draw.rounded_rectangle((2, 2, 253, 253), radius=48, fill=(7, 22, 41), outline=(27, 88, 114), width=4)
draw.ellipse((30, 30, 226, 226), fill=(5, 20, 34), outline=(239, 207, 103), width=9)
font = ImageFont.truetype(str(root / 'assets/SCUInterface.ttf'), 158)
draw.text((128, 123), 'U', font=font, anchor='mm', fill=(105, 239, 222), stroke_width=1)
image.save(root / 'assets/SCU.ico', sizes=[(16,16),(24,24),(32,32),(48,48),(64,64),(128,128),(256,256)])
image.save(root / 'assets/SCU.icns')
