import argparse
import json
from pathlib import Path
import textwrap
from PIL import Image, ImageDraw, ImageFont

parser = argparse.ArgumentParser()
parser.add_argument('report', type=Path)
args = parser.parse_args()
rows = [json.loads(p.read_text(encoding='utf-8')) for p in sorted(args.report.glob('*/audit.json'))]
rows = [row for row in rows if row['snapshot']]
font = ImageFont.truetype('C:/Windows/Fonts/segoeui.ttf', 15)
for offset in range(0, len(rows), 20):
    page = Image.new('RGB', (1360, 1540), '#121923')
    draw = ImageDraw.Draw(page)
    for i, row in enumerate(rows[offset:offset + 20]):
        x, y = (i % 4) * 340, (i // 4) * 308
        label = f'{offset+i+1}. {row["rom"]}'
        for line, text in enumerate(textwrap.wrap(label, 40)[:3]):
            draw.text((x + 6, y + line * 17), text, font=font, fill='white')
        with Image.open(args.report / row['snapshot']) as original:
            image = original.convert('RGB')
            image.thumbnail((326, 240))
            page.paste(image, (x + 7, y + 57))
    path = args.report / f'contact-{offset//20+1}.jpg'
    page.save(path, quality=95)
    print(path)
    print('\n'.join(f'{offset+i+1}: {row["rom"]}' for i,row in enumerate(rows[offset:offset+20])))
