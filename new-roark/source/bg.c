// bg.c - background looks. Each scene "bg <key>" picks a gradient (top color, bottom color, accent).
// Drop a 400x240 PNG named gfx/bg_<key>.png to use a picture instead (see BUILD.md).
#include "bg.h"
#include <string.h>

const BgStyle BGS[] = {
    {"bedroom",    0xF6C9D8, 0x9E7BB5, 0xFFE6F0},
    {"kitchen",    0xFFE3B8, 0xE8A87C, 0xFFF4E0},
    {"livingroom", 0xF2D7C0, 0xA87C6A, 0xFFEBDC},
    {"street",     0xA9C6E8, 0x5C6B8A, 0xFFD36E},
    {"hallway",    0xD9E4F2, 0x7D8FB3, 0xF5B6CF},
    {"classroom",  0xE6F0D9, 0x7FA37A, 0xFFFFFF},
    {"cafeteria",  0xFFF0C2, 0xD9A05B, 0xFF9E9E},
    {"gym",        0xFFD9A8, 0xC0703A, 0xFFFFFF},
    {"library",    0xE8D6B8, 0x7A5A3A, 0xFFF0D0},
    {"office",     0xDDE2E8, 0x7A8796, 0xFFFFFF},
    {"dorm",       0xC9B79C, 0x4A3A2E, 0xE8D8B0},
    {"mall",       0xFFD1F0, 0x9AA8FF, 0xFFFFFF},
    {"park",       0xBFE8C9, 0x4E8A5A, 0xFFF3A8},
    {"rooftop",    0x2A2A5A, 0x0E0E24, 0xFFD36E},
};
const int BG_COUNT = sizeof BGS / sizeof BGS[0];

const BgStyle *bg_find(const char *key) {
    for (int i = 0; i < BG_COUNT; i++)
        if (strcmp(BGS[i].key, key) == 0) return &BGS[i];
    return NULL;
}
