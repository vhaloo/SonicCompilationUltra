"""Original illustrative examples, not cartridge screenshots. No ROM/app inputs.

Run with Pillow to regenerate the indexed BMP used by every native frontend.
The small palette keeps the entire 20-picture atlas around 1.2 MB on disk.
"""
from pathlib import Path
import math
from PIL import Image, ImageDraw, ImageFont

ROOT=Path(__file__).resolve().parents[1]
FONT=ImageFont.load_default(size=14)
INK='#09172b'; CYAN='#66dce7'; GOLD='#ffca72'; TEAL='#248599'; MUTED='#496381'

def landscape():
    im=Image.new('RGB',(960,224),INK); d=ImageDraw.Draw(im)
    for y in range(224):
        d.line((0,y,960,y),fill=(11+y//18,25+y//9,53+y//5))
    for x,y in [(70,30),(250,45),(440,22),(580,50),(780,32),(890,60)]:
        d.rectangle((x,y,x+32,y+2),fill='#3f7190');d.rectangle((x+8,y-4,x+24,y+5),fill='#3f7190')
    for x in range(0,960,16):
        y=171-int(22*math.sin(x/70))
        d.rectangle((x,y,x+16,224),fill='#193f51')
    for i in range(30):
        x=i*32; y=179-(i//4%3)*14
        d.rectangle((x,y,x+32,224),fill='#183d44');d.rectangle((x,y,x+32,y+5),fill='#79c7a5')
        d.line((x+15,y+7,x+15,224),fill='#28535a')
    for x,y in [(65,125),(180,80),(300,125),(480,65),(650,115),(820,80)]:
        d.rectangle((x,y,x+70,y+7),fill='#aee6d1');d.rectangle((x+6,y+8,x+64,y+18),fill=TEAL)
        for z in range(3):d.ellipse((x+12+z*18,y-22,x+20+z*18,y-14),outline=GOLD,width=2)
    d.ellipse((327,103,343,119),fill=GOLD);d.rectangle((331,107,339,111),fill='#fff5d1')
    for x,y in [(125,165),(540,130),(755,150)]:
        d.rectangle((x,y-14,x+14,y),fill='#a17ea7');d.rectangle((x+3,y-11,x+5,y-8),fill=INK)
    return im

WORLD=landscape()
def scene(size,wide=False,pixel=False):
    source=WORLD if wide else WORLD.crop((180,0,500,224))
    if pixel:source=source.resize((40,28),Image.Resampling.BOX)
    return source.resize(size,Image.Resampling.NEAREST if pixel else Image.Resampling.BICUBIC)
def frame(im,rect,wide=False,pixel=False):
    x,y,w,h=rect; im.paste(scene((w,h),wide,pixel),(x,y))
    ImageDraw.Draw(im).rounded_rectangle((x-1,y-1,x+w,y+h),radius=4,outline=MUTED,width=1)
def circle_scene(im,xy,r,wide):
    tile=scene((r*2,r*2),wide)
    mask=Image.new('L',tile.size);ImageDraw.Draw(mask).ellipse((1,1,r*2-2,r*2-2),fill=255)
    im.paste(tile,(xy[0]-r,xy[1]-r),mask)
    ImageDraw.Draw(im).ellipse((xy[0]-r,xy[1]-r,xy[0]+r,xy[1]+r),outline=CYAN,width=3)

def picture(index):
    im=Image.new('RGB',(320,180),INK);d=ImageDraw.Draw(im)
    d.rounded_rectangle((3,3,316,176),radius=12,fill='#10243c',outline='#25516a',width=1)
    d.rounded_rectangle((14,15,44,35),radius=7,fill='#21546a');d.text((24,17),'A',font=FONT,fill=CYAN)
    d.rounded_rectangle((175,15,205,35),radius=7,fill='#665037');d.text((185,17),'B',font=FONT,fill=GOLD)
    if index==19:
        # These checked-in pictures are generated only by the standalone,
        # ROM-free mesh fixture. See voxel-provenance.json and its helper.
        for name,x in [('flat',17),('layered',176)]:
            source=Image.open(ROOT/('docs/illustrations/voxel-'+name+'.png')).convert('RGB')
            assert source.size==(960,540)
            im.paste(source.resize((126,71),Image.Resampling.NEAREST),(x,70))
            d.rounded_rectangle((x-1,69,x+126,142),radius=4,outline=MUTED,width=1)
    elif index in (12,14):
        for half in range(2):
            for n in range(54):
                x=19+half*159+n*2; y=104; a=abs(math.sin(n*.31))*27
                d.line((x,y-a,x,y+a),fill=CYAN if half==0 else GOLD,width=1)
        if index==14:
            for x in (220,243):d.polygon([(x,142),(x+14,133),(x+14,151)],fill=GOLD)
    elif index in (15,17):
        for half in range(2):
            for n in range(6):
                x=22+half*159+n%3*39;y=55+n//3*48
                d.rounded_rectangle((x,y,x+29,y+37),radius=4,fill='#193d52',outline=GOLD if n==half*2 else MUTED,width=2)
                d.rectangle((x+6,y+7,x+23,y+20),fill=TEAL if n%2 else '#826c96')
    elif index==13:
        for half in range(2):
            x=25+half*159
            d.rounded_rectangle((x,64,x+116,132),radius=22,fill=MUTED,outline=CYAN,width=2)
            d.line((x+16,97,x+48,97),fill=GOLD,width=7);d.line((x+32,81,x+32,113),fill=GOLD,width=7)
            for xx,yy in [(85,83),(97,97),(85,111),(73,97)]:d.ellipse((x+xx-4,yy-4,x+xx+4,yy+4),fill=GOLD)
    elif index==18:
        circle_scene(im,(83,102),55,False);circle_scene(im,(242,102),55,True)
        for x in (83,242):
            d.rectangle((x-18,39,x+18,46),fill=MUTED);d.rectangle((x-18,158,x+18,165),fill=MUTED)
            d.rounded_rectangle((x+55,93,x+61,106),radius=2,fill=GOLD)
    elif index==7:
        for half in range(2):
            x=20+half*159
            d.polygon([(x,136),(x+88,153),(x+120,56),(x+47,45)],fill=MUTED,outline=CYAN,width=2)
            for xx,yy in [(65,80),(80,110),(40,105)]:d.ellipse((x+xx-7,yy-7,x+xx+7,yy+7),outline=GOLD,width=3)
            d.line((x+24,138,x+24,165),fill=MUTED,width=5)
            d.line((x+87,153,x+87,172),fill=MUTED,width=5)
    elif index in (5,6,8,9):
        for half in range(2):
            x=17+half*159
            d.polygon([(x,153),(x+124,153),(x+110,41),(x+11,41)],outline=MUTED)
            w=109 if half else 73;h=70 if half else 50
            xx=x+(124-w)//2;yy=58 if half else 77
            if index==8:d.rounded_rectangle((xx-7,yy-8,xx+w+7,yy+h+13),radius=10,fill='#2a3e52',outline=CYAN,width=2)
            frame(im,(xx,yy,w,h),index in (5,6,9))
            if index==8:
                for row in range(yy+2,yy+h,3):d.line((xx,row,xx+w,row),fill='#0b2234',width=1)
            if index==9:d.line((x+16,146,x+108,146),fill=GOLD,width=2)
            if index==6:d.line((x,155,x+126,155),fill=CYAN,width=2)
    else:
        frame(im,(17,48,126,97),wide=index in (2,11,16),pixel=index==11)
        frame(im,(176,48,126,97),wide=index in (0,1,3,4,10,11,16))
        if index==2:
            d.rectangle((200,66,271,125),outline=GOLD,width=2);d.line((214,95,285,95),fill=GOLD,width=2)
        if index==3:
            for x,y in [(24,54),(219,101)]:d.rounded_rectangle((x,y,x+47,y+17),radius=4,fill=INK,outline=GOLD,width=1)
        if index==4:d.rounded_rectangle((254,131,299,143),radius=3,fill=INK,outline=CYAN,width=1)
        if index==10:
            for x in range(176,302,6):d.line((x,48,x,145),fill='#244c62',width=2)
        if index==16:
            for x in (204,238,273):d.rectangle((x,100,x+8,112),outline=GOLD,width=1)
        if index==11:
            d.rounded_rectangle((20,154,141,161),radius=3,fill=MUTED);d.rounded_rectangle((179,154,300,161),radius=3,fill=MUTED)
            d.rounded_rectangle((20,154,80,161),radius=3,fill=CYAN);d.rounded_rectangle((179,154,270,161),radius=3,fill=GOLD)
    d.line((158,48,158,155),fill='#31516a',width=1)
    return im

def main():
    atlas=Image.new('RGB',(1280,900),'#061222')
    for index in range(20):atlas.paste(picture(index),((index%4)*320,(index//4)*180))
    indexed=atlas.quantize(colors=192,method=Image.Quantize.MEDIANCUT)
    indexed.save(ROOT/'assets/settings-preview.bmp')
    gallery=ROOT/'docs/illustrations/settings-preview-gallery.png'
    gallery.parent.mkdir(parents=True,exist_ok=True);atlas.save(gallery,optimize=True)
    print('Generated 20 original A/B setting examples, no cartridge or app inputs;', (ROOT/'assets/settings-preview.bmp').stat().st_size,'BMP bytes')
if __name__=='__main__': main()
