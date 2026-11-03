#!/bin/sh
# Convert your recorded voice lines into the format the game plays.
# Usage: ./tools/wav_to_raw.sh hello.wav   ->  romfs/voice/hello.raw
# Needs ffmpeg. Then in story.txt:  say me | Hi!! | hello
for f in "$@"; do
  n=$(basename "${f%.*}")
  ffmpeg -y -i "$f" -ac 1 -ar 22050 -f s16le "romfs/voice/$n.raw"
done
