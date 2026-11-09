"""Build a factual comparison sheet from unretouched renders/reference pixels."""
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont
import argparse
parser=argparse.ArgumentParser()
parser.add_argument('--game',type=Path,help='Actual 2560x1440 PuckDetail capture; never a studio render')
args=parser.parse_args()
root=Path(__file__).resolve().parent.parent
out=root/'Deliverables/Neon-Standard-2026-11-08'
out.mkdir(exist_ok=True)
new=root/'AssetDevelopment/Pucks/Neon/renders/neon-standard-studio.png'
reference=root/'Deliverables/Neon Standard Puck.png'
before=out/'neon-before-refinement.png'
font=ImageFont.truetype('C:/Windows/Fonts/arialbd.ttf',30)
small=ImageFont.truetype('C:/Windows/Fonts/arial.ttf',20)
def sheet(panels,name):
    size=900;gap=20;top=85;bottom=60
    im=Image.new('RGB',(gap+(size+gap)*len(panels),top+size+bottom),(5,15,24));d=ImageDraw.Draw(im)
    for i,(path,label,subtitle) in enumerate(panels):
        x=gap+i*(size+gap)
        d.text((x,14),label,font=font,fill=(244,248,255));d.text((x,51),subtitle,font=small,fill=(137,217,240))
        picture=Image.open(path).convert('RGB');picture.thumbnail((size,size),Image.Resampling.LANCZOS)
        im.paste(picture,(x+(size-picture.width)//2,top+(size-picture.height)//2))
    d.text((gap,top+size+17),'FLICK / NEON STANDARD   |   Actual model renders. Reference shown unchanged apart from uniform scaling.',font=small,fill=(193,210,223))
    im.save(out/name)
sheet([(new,'REFINED IMPLEMENTATION','Studio render of the mesh imported into the game'),(reference,'YOUR REFERENCE','Original target artwork')],'neon-refined-vs-reference.png')
sheet([(before,'BEFORE','Implementation at the start of this refinement'),(new,'AFTER','Refined lenses, metal, matrix and reflections'),(reference,'REFERENCE','Original target artwork')],'neon-before-after-reference.png')
print(out/'neon-refined-vs-reference.png')
if args.game:
    game=Image.open(args.game).convert('RGB')
    assert game.size==(2560,1440),'Use the documented full-resolution close-up framing.'
    game.save(out/'neon-in-game-closeup.png')
    # Crop surrounding HUD/empty stage only. Preserve all rendered puck pixels;
    # no recoloring, compositing onto a new background or perspective warping.
    panels=[(game.crop((570,325,1990,1300)),'IN-GAME / NEON STANDARD','Actual Knockout screenshot crop'),
            (Image.open(reference).convert('RGB').crop((90,275,1440,1250)),'YOUR REFERENCE','Original target artwork crop')]
    width=1000;height=755;gap=24;top=88
    result=Image.new('RGB',(gap*3+width*2,top+height+60),(5,15,24));draw=ImageDraw.Draw(result)
    for i,(pic,title,note) in enumerate(panels):
        x=gap+i*(width+gap)
        draw.text((x,15),title,font=font,fill=(244,248,255))
        draw.text((x,53),note,font=small,fill=(137,217,240))
        pic.thumbnail((width,height),Image.Resampling.LANCZOS)
        result.paste(pic,(x+(width-pic.width)//2,top+(height-pic.height)//2))
    draw.text((gap,top+height+18),'FLICK / NEON   |   Cropping and uniform scaling only. Different arena and studio lighting remain visible.',font=small,fill=(193,210,223))
    result.save(out/'neon-in-game-vs-reference.png')
    print(out/'neon-in-game-vs-reference.png')
