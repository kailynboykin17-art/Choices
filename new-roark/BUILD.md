# Peppa Pig Baddies: New Roark — 3DS Choices game

Pick any teen in New Roark, play their Chapter 1, make choices that change how it ends.
Your character art is used exactly as you drew it — the game just shows the cut-out PNGs.

## Build the .3dsx (on your Mac)

You already have devkitPro set up, so:

1. Open Terminal and go into this folder:
   ```
   cd ~/Desktop/new-roark
   ```
2. (Optional but smart) check every story for mistakes first:
   ```
   sh tools/run_tests.sh
   ```
   It plays every possible choice path and tells you about typos like a wrong label or a character with no picture.
3. Build:
   ```
   make
   ```
4. You get **NewRoark.3dsx**. Copy it to your SD card at `/3ds/NewRoark/NewRoark.3dsx` and launch it from the Homebrew Launcher.

The pictures and stories are packed inside the .3dsx, so that one file is all you need.
Saves go to `sd:/3ds/NewRoark/save.dat`.

## Controls

| Button | What it does |
|---|---|
| A / tap | Next line, pick a choice |
| D-pad | Move between choices / stories |
| B | Show the whole line instantly / go back |
| SELECT | Back to the title (your spot is saved) |
| START | Save and quit |

## Roarstagram

The in-game Instagram (tap **Roarstagram** on the title screen, or press Y). It has Candy's posts with Simon.
Stories can also pop posts up mid-scene. To add a post: put the picture in `art_src/posts/`, make a 192×240
copy at `gfx/post_<name>.png` (plus a `.t3s` like the other post files), and add a line to `source/feed.c`.

## Stars

You start with 10 stars. Finishing a chapter earns 5–7 stars (more for the "best" endings).
Gold choices cost stars and unlock the juiciest scenes.

## Your art

- `art_src/` = your original family lineups.
- `gfx/<name>.png` = one cut-out per character (made by `tools/crop_portraits.py`).
- To fix a crop: change that character's numbers in `BOXES` inside `tools/crop_portraits.py`, then run
  `python3 tools/crop_portraits.py wendy` (just that character) and rebuild.
- To swap in a brand-new drawing: just replace `gfx/<name>.png` with your own PNG (transparent background,
  about 224 pixels tall max so it fits the top screen).
- New character? Add `gfx/<name>.png`, copy any `.t3s` file in `gfx/` to `<name>.t3s` and change the
  file name inside it, then add them to `source/cast.c`.
- Picture backgrounds: drop a 400×240 PNG named `gfx/bg_hallway.png` (or `bg_<any bg name>`) plus a matching
  `.t3s`, and the game uses it instead of the colored gradient.

## Adding stories

See `STORY_FORMAT.md`. Write `romfs/story/<name>.txt`, add one line to `source/routes.c`, run the tests, `make`.
