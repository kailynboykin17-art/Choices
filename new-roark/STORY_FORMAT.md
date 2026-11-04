# How to write a story file

Story files live in `romfs/story/`. One line = one thing. Lines starting with `#` are notes to yourself.

```
title Wendy Wolf - Chapter 1: Static        shows on the menus

bg hallway Roark High - Main Hallway        background + place name (top-left tag)
show wendy left                             put a character on screen: left / center / right
show peppa right
hide peppa                                  take one off  (hide all = clear the stage)

peppa: Look, fresh meat.                    someone talks  (name = their id from source/cast.c)
narr: She says it loud enough on purpose.   narration box
The bell rings.                             a plain line is narration too
phone: Unknown - "practice friday?"         texts / TV / radio voices: phone, tv, radio, teacher, crowd, kid

choice                                      a choice (2-4 options)
> Stare her down | stare | rep +2           > text | label to jump to | stat change
> Keep walking | walk
> [5] Compliment her boots | boots | rep +3 [5] = costs 5 stars (gold button)
endchoice

label stare                                 a place to jump to
wendy: Sorry, couldn't hear you.
goto lunch                                  jump

set band 1                                  set a stat   (set band +1 adds)
if rep >= 3 goto popular                    check a stat: >= <= > < == !=
post candy post_candy1 him >>>               a Roarstagram post pops up (who, picture, caption)
post peggi none emily said respect my name  "none" = text-only post
reward 5                                    stars earned when the chapter ends
end                                         chapter over
```

Rules that keep it working:
- Only use a colon right after a speaker id (`wendy:`). In narration, use a dash instead.
- Keep dialogue lines under ~260 letters and choice text under ~64 so they fit the 3DS screen.
- Every path needs a `reward` before its `end`.
- Run `sh tools/run_tests.sh` after editing — it checks all of the above for every path.
