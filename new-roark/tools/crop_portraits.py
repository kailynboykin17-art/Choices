#!/usr/bin/env python3
"""Cut the family lineup art (art_src/*.png) into one PNG per character (gfx/<id>.png).
Your art is used as-is: this only crops, removes the white background, and shrinks it
so the tallest person in each family is 330px (shorter family members stay shorter).
The game shows the upper body; legs run off the bottom of the screen like in Choices. Edit BOXES to adjust a crop: (sheet, x0, x1)."""
import os, sys
import numpy as np
from PIL import Image
from scipy import ndimage as nd

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC, OUT = os.path.join(ROOT, "art_src"), os.path.join(ROOT, "gfx")
MAX_H = 330
# sheets with no adult in them get their own tallest height
SHEET_H = {"baddies": 315, "evie": 150}

BOXES = {
  # Pony
  "mrpony": ("pony", 27, 224), "mrspony": ("pony", 246, 427), "pedro": ("pony", 435, 657),
  "patricia": ("pony", 671, 851), "paloma": ("pony", 854, 1023),
  # Squirrel
  "mrsquirrel": ("squirrel", 6, 272), "mrssquirrel": ("squirrel", 272, 452),
  "simon": ("squirrel", 462, 652), "samantha": ("squirrel", 640, 837),
  # Fox
  "mrfox": ("fox", 16, 280), "mrsfox": ("fox", 282, 478), "freddy": ("fox", 452, 728), "fraya": ("fox", 700, 922),
  # Mole
  "mrmole": ("mole", 46, 260), "mrsmole": ("mole", 268, 462), "molly": ("mole", 466, 701), "malachi": ("mole", 714, 944),
  # Rabbit
  "mrrabbit": ("rabbit", 16, 176), "mrsrabbit": ("rabbit", 176, 334), "rebecca": ("rabbit", 353, 532),
  "richard": ("rabbit", 539, 677), "rosie": ("rabbit", 702, 824), "robbie": ("rabbit", 859, 997),
  # Pig
  "mrpig": ("pig", 37, 555), "mrspig": ("pig", 558, 1060), "peppa_home": ("pig", 1060, 1515), "george": ("pig", 1585, 2000),
  # Dobermann
  "derek": ("dobermann", 28, 282), "diana": ("dobermann", 282, 486), "danny": ("dobermann", 482, 727),
  # Cat
  "mscat": ("cat", 559, 1180), "candy": ("cat", 1170, 1624),
  # Kameelperd
  "mrk": ("kameelperd", 4, 160), "mrsk": ("kameelperd", 160, 310), "kanyisa": ("kameelperd", 300, 462),
  "kaya": ("kameelperd", 463, 650), "keva": ("kameelperd", 655, 840), "gerald": ("kameelperd", 832, 1010),
  # Elephant
  "mrelephant": ("elephant", 35, 695), "mrselephant": ("elephant", 720, 1400), "emily": ("elephant", 1400, 2054),
  # Kangaroo
  "katrina": ("kangaroo", 244, 437), "karter": ("kangaroo", 469, 672),
  # Sheep
  "mrssheep": ("sheep", 177, 549), "slater": ("sheep", 614, 1079), "suzy": ("sheep", 1149, 1546),
  # The Baddies sheet (Season outfits)
  "rebecca_b": ("baddies", 25, 560), "peggi": ("baddies", 600, 1120), "kylie": ("baddies", 1125, 1598),
  "peppa": ("baddies", 1600, 2130),
  # Baby Evie
  "evie": ("evie", 330, 820),
  # Wolf
  "watts": ("wolf", 30, 352), "winter": ("wolf", 268, 600), "waverly": ("wolf", 585, 915),
  "willow": ("wolf", 885, 1225), "wendy": ("wolf", 1195, 1582), "whitney": ("wolf", 1556, 1965),
  "vanya": ("wolf", 1960, 2370), "wyatt": ("wolf", 2300, 2795),
}

def cut(sheet, x0, x1):
    im = np.array(Image.open(os.path.join(SRC, sheet + ".png")).convert("RGBA"))
    im = im[:, x0:x1].copy()
    rgb, a = im[:, :, :3].astype(int), im[:, :, 3]
    whiteish = (rgb.min(axis=2) >= 236) | (a < 30)
    # background = white areas touching the crop's edge
    lab, _ = nd.label(whiteish)
    edge = set(np.unique(np.concatenate([lab[0], lab[-1], lab[:, 0], lab[:, -1]]))) - {0}
    bg = np.isin(lab, list(edge))
    fg = ~bg
    # keep the biggest piece (the character); drops name labels and stray bits
    lab2, n = nd.label(nd.binary_closing(fg, iterations=2))
    if n:
        sizes = nd.sum(fg, lab2, range(1, n + 1))
        keep = lab2 == (int(np.argmax(sizes)) + 1)
        fg &= keep
    # soften edge a little
    alpha = np.where(fg, a, 0).astype(np.uint8)
    im[:, :, 3] = alpha
    ys, xs = np.nonzero(alpha)
    im = im[ys.min():ys.max() + 1, xs.min():xs.max() + 1]
    return Image.fromarray(im, "RGBA")

if __name__ == "__main__":
    os.makedirs(OUT, exist_ok=True)
    only = set(sys.argv[1:])
    cuts = {cid: cut(*box) for cid, box in BOXES.items()}
    tallest = {}
    for cid, (sheet, _, _) in BOXES.items():
        tallest[sheet] = max(tallest.get(sheet, 0), cuts[cid].height)
    for cid, (sheet, _, _) in BOXES.items():
        if only and cid not in only: continue
        im = cuts[cid]
        s = SHEET_H.get(sheet, MAX_H) / tallest[sheet]
        im = im.resize((max(1, round(im.width * s)), max(1, round(im.height * s))), Image.LANCZOS)
        im.save(os.path.join(OUT, cid + ".png"))
        print("ok", cid, im.size)
