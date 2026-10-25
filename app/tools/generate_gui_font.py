from pathlib import Path
from PIL import Image,ImageDraw,ImageFont
r=Path(__file__).resolve().parent.parent;(r/'tools').mkdir(exist_ok=True)
lines=['/* Aileron Regular raster subset, rendered by Pillow. CC0 / No Rights Reserved. */','#pragma once','#include <stdint.h>']
for idx,size in enumerate([18,22,30]):
 font=ImageFont.load_default(size=size);w=40;h=40;raw=[];adv=[]
 for cp in range(32,256):
  image=Image.new('L',(w,h));draw=ImageDraw.Draw(image);draw.text((1,-1),chr(cp),font=font,fill=255)
  raw.extend(image.tobytes());adv.append(round(font.getlength(chr(cp))))
 lines.append('static const unsigned char gui_font_%d[%d]={%s};'%(idx,len(raw),','.join(map(str,raw))))
 lines.append('static const unsigned char gui_advance_%d[224]={%s};'%(idx,','.join(map(str,adv))))
(r/'include/gui_font.h').write_text('\n'.join(lines)+'\n')
(r/'licenses/Aileron.txt').write_text('Aileron Regular by Sora Sagano (dot colon).\nhttps://dotcolon.net/fonts/aileron\n\nCC0 / No Rights Reserved. The font embedded in Pillow identifies its copyright as "No Rights Reserved."\nThis application includes rasterized glyphs from Pillow ImageFont.load_default, not a system or Nintendo font.\n')
print('Generated three antialiased font sizes; Latin-1 plus UTF-8 fallback.')
