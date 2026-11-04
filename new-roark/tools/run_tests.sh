#!/bin/sh
# Checks every story path on your computer (no 3DS needed). Run from the project folder.
cd "$(dirname "$0")/.." || exit 1
cc -I source tools/test_story.c source/story.c source/cast.c source/routes.c source/bg.c -o tools/test_story && ./tools/test_story
