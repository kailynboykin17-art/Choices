# CHOICES — Illustrated Portrait Guide

The game's UI is finished. The character art is meant to be **painted, comic-book-style portrait illustrations**, loaded as PNG files.
Until a character has a PNG, the game shows a simple stand-in bust so it still runs. Those stand-ins are not the intended art.

## How to add a portrait

1. Save the painting as `art/portraits/<id>.png` using the **id** from the table below (for example `art/portraits/odette.png`).
2. Optional talking frame (mouth open): `art/portraits/<id>_talk.png`. The game flips between the two frames while that character speaks.
3. Rebuild (`make`, or push to GitHub). The Makefile converts every PNG to `romfs/gfx/<id>.t3x` with tex3ds, and the game picks it up automatically.

## Canvas spec (important for the 3DS)

- **Size: 256 × 320 px** (bust portrait, head and shoulders, with room above the hair). The game fits it into a polaroid frame of about 106×116 on screen, so paint at 256×320 and fine detail will survive.
- PNG with a transparent **or** painted background. Painted sunset or campus backgrounds look best inside the polaroid.
- Keep the face in the **upper 55%** of the canvas. The bottom of the image is covered by the polaroid caption strip.
- Keep files at 512 px on the longest side or less (3DS texture limit is 1024; smaller loads faster).
- Lighting: warm golden-hour key light from the left, dusty-rose rim light on the right, cocoa shadows. Use this on **every** portrait so the cast looks like one series.

## The player character

The player customizes skin, hair and outfit, so the art uses outfit-based portraits:
- `me_0.png` silk cami · `me_1.png` baby tee · `me_2.png` velour tracksuit · `me_3.png` sundress · `me_4.png` cropped blazer · `me_5.png` cardigan & pearls
- Or a single `me.png` used for every outfit.
- `cover.png` (about 204×214, or 408×428 for detail) is the cover girl on the title screen.

## Style direction (give this to your artist or use it for your own paintings)

> Modern comic-book cover illustration with the polish of a glossy 2004 fashion editorial. Realistic proportions, expressive almond eyes with detailed lashes and catchlights, glossy detailed lips, defined brows, painted hair with individual strands and warm highlights, confident ink linework with painterly rendering, cinematic golden-hour lighting. Wardrobe is Y2K: baby tees, low-rise denim, velour tracksuits, gold hoops, pearls, designer shoulder bags. Palette: dusty rose, mauve, cream, warm ivory, cocoa, soft plum, champagne gold. No chibi, no flat vector, no anime-sprite style.

## Every character that needs a portrait

| Story | id (file name) | Character |
|---|---|---|
| Story Land / Boutique | `shopkeep` | Bijou |
| Story Land / Boutique | `narrator` | Story Land |
| Nanny Diaries | `agency` | Ms. Dolly |
| Nanny Diaries | `bell` | Mrs. Bell |
| Nanny Diaries | `lulu` | Lulu Bell |
| Nanny Diaries | `rivera` | Mr. Rivera |
| Nanny Diaries | `trip` | The Triplets |
| Nanny Diaries | `okafor` | Mr. Okafor |
| Nanny Diaries | `zuri` | Zuri |
| Nanny Diaries | `tiff` | Tiffani |
| 2000s Teen Queen | `mom` | Mom |
| 2000s Teen Queen | `jade` | Jade |
| 2000s Teen Queen | `brooke` | Brooke |
| 2000s Teen Queen | `tyler` | Tyler |
| 2000s Teen Queen | `maddie` | Maddie |
| Prima Ballerina | `madame` | Madame Petrova |
| Prima Ballerina | `odette` | Odette |
| Prima Ballerina | `nia` | Nia |
| Pastor's Daughter | `pastor` | Pastor Davis |
| Pastor's Daughter | `firstlady` | Mama Davis |
| Pastor's Daughter | `marcus` | Marcus |
| Pastor's Daughter | `kiara` | Kiara |
| The Friend Group | `dani` | Dani |
| The Friend Group | `kels` | Kelsey |
| The Friend Group | `rae` | Rae |
| The Friend Group | `sophie` | Sophie |
| Pop Star Dreams | `manager` | Manager Max |
| Pop Star Dreams | `lexi` | Lexi Starr |
| Pop Star Dreams | `dj` | DJ Remi |
| Boarding School | `headmistress` | Headmistress Grey |
| Boarding School | `ivy` | Ivy |
| Boarding School | `pri` | Priya |
| Cheer Captain | `coach` | Coach Tammy |
| Cheer Captain | `bree` | Bree |
| Cheer Captain | `tasha` | Tasha |
| Fashion Intern NYC | `miranda` | Ms. Valentina |
| Fashion Intern NYC | `jules` | Jules |
| Fashion Intern NYC | `chad` | Chad |
| Summer at Grandma's | `grandma` | Grandma Pearl |
| Summer at Grandma's | `leo` | Leo |
| Summer at Grandma's | `coco` | Coco |
| Sleepover Secrets | `mya` | Mya |
| Sleepover Secrets | `hailey` | Hailey |
| Sleepover Secrets | `gabby` | Gabby |
| Prom Night | `amber` | Amber |
| Prom Night | `jay` | Jay |
| Prom Night | `tia` | Tia |
| Mall Job | `boss` | Boss Brenda |
| Mall Job | `kai` | Kai |
| Mall Job | `karen` | Angry Customer |
| Roller Rink Royalty | `roxy` | Roxy |
| Roller Rink Royalty | `dj2` | DJ Spinz |
| Roller Rink Royalty | `skate` | Skater Sam |
| Small-Town Pageant | `mama` | Mama June-Belle |
| Small-Town Pageant | `judge` | Judge Harold |
| Small-Town Pageant | `caroline` | Caroline |
| Pink Diamond Heist | `boss2` | Madame Noir |
| Pink Diamond Heist | `pixel` | Pixel |
| Pink Diamond Heist | `detective` | Det. Rose |
| Soap Opera Star | `director` | Director Vince |
| Soap Opera Star | `diva` | Veronica |
| Soap Opera Star | `costar` | Ricky |
| Beauty Salon Boss | `auntie` | Auntie Bernice |
| Beauty Salon Boss | `client` | Shay |
| Beauty Salon Boss | `rival2` | Glamazon Salon |
| College Freshman | `roomie` | Jordan |
| College Freshman | `ra` | RA Destiny |
| College Freshman | `prof` | Professor Lin |
| Witchy Girl | `luna` | Grandma Luna |
| Witchy Girl | `cat` | Midnight (cat) |
| Witchy Girl | `hex` | Hexa |

Total: **71 characters** + player portraits + title cover.

Tip: start with the player (`me_*.png`), the cover, and one story's cast, so you can see the full look in-game early.
