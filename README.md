# Choices (3DS)
Y2K story collection: pick 1 of 20 lives, earn Hearts, collect stickers.

## Get the .3dsx + .cia (no install needed)
1. Make a new GitHub repo and upload this whole folder (including `.github`).
2. Open the **Actions** tab. The "Build 3DSX + CIA" job runs on its own.
3. When it's green, download the **NannyDiaries** artifact zip, which has both files.

## Build on your PC instead
Install devkitPro (3ds-dev), plus makerom and bannertool. Then:
`make` builds the .3dsx and `make cia` builds the .cia.

## Install
- .3dsx: copy it to `SD:/3ds/` and open it from the Homebrew Launcher.
- .cia: install it with FBI.
- For voice and sound, your console needs `SD:/3ds/dspfirm.cdc` (dump it with DSP1).

## Editing the story
Everything is in `romfs/story.txt`, and the command list is at the top of that file.
Add your own voice clips with `tools/wav_to_raw.sh hi.wav`, then use `say me | Hi!! | hi`.

## Adding a story
Make a new `romfs/stories/21_yours.txt` with a `#yours_start` label. Then add
`> Your Story | | yours_start` to the Story Land menu in `romfs/story.txt`.

## Visual redesign (Maple Valley look)
- Palette, fonts, magazine screens, the polaroid frames and HUD are all in `source/main.cpp`.
- Fonts: `fonts/serif.ttf` + `fonts/italic.ttf` (Caladea, Apache 2.0 license), converted automatically when you build.
- Character art: see **ART_GUIDE.md**. Drop painted PNG portraits in `art/portraits/` and they replace the stand-in busts.
