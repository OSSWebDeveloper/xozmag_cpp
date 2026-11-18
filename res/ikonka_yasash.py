"""Xozmag ikonkasini yasaydi: ko'k kvadrat, oq «X» va pastda yashil chiziq (kassa)."""
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

ILDIZ = Path(__file__).resolve().parent
rasmlar = []
for o in (16, 24, 32, 48, 64, 128, 256):
    im = Image.new("RGBA", (o, o), (21, 101, 192, 255))
    d = ImageDraw.Draw(im)
    d.rectangle([0, int(o * 0.80), o, o], fill=(46, 125, 50, 255))
    f = ImageFont.truetype(r"C:\Windows\Fonts\segoeuib.ttf", int(o * 0.78))
    b = d.textbbox((0, 0), "X", font=f)
    x = (o - (b[2] - b[0])) / 2 - b[0]
    y = (o * 0.80 - (b[3] - b[1])) / 2 - b[1]
    d.text((x, y), "X", font=f, fill=(255, 255, 255, 255))
    rasmlar.append(im)
rasmlar[-1].save(ILDIZ / "xozmag.ico", sizes=[(r.width, r.height) for r in rasmlar],
                 append_images=rasmlar[:-1])
rasmlar[-1].save(ILDIZ / "xozmag.png")
print("tayyor")
