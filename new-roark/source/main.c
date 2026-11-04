// main.c - Peppa Pig Baddies: New Roark (3DS homebrew, citro2d)
// Top screen = the scene. Bottom screen = choices and menus.
#include <3ds.h>
#include <citro2d.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "story.h"
#include "routes.h"
#include "bg.h"
#include "feed.h"

#define TOP_W 400
#define BOT_W 320
#define SCR_H 240
#define START_STARS 10
#define SAVE_DIR  "sdmc:/3ds/NewRoark"
#define SAVE_PATH "sdmc:/3ds/NewRoark/save.dat"
#define SAVE_MAGIC 0x4E525231u   // "NRR1"
#define MAX_ROUTES 32

// ---------------------------------------------------------------- colors
static u32 rgb(unsigned int c, u8 a) { return C2D_Color32((c >> 16) & 0xFF, (c >> 8) & 0xFF, c & 0xFF, a); }
#define COL_INK     C2D_Color32(0x2B, 0x1E, 0x3A, 0xFF)
#define COL_WHITE   C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF)
#define COL_PINK    C2D_Color32(0xFF, 0x5F, 0xB0, 0xFF)
#define COL_LILAC   C2D_Color32(0xC9, 0xA8, 0xFF, 0xFF)
#define COL_SOFT    C2D_Color32(0xFF, 0xF4, 0xFA, 0xF0)
#define COL_GREY    C2D_Color32(0xB8, 0xB0, 0xC0, 0xFF)
#define COL_GOLD    C2D_Color32(0xFF, 0xC8, 0x3D, 0xFF)

// ---------------------------------------------------------------- save data
typedef struct {
    u32  magic;
    int  stars;
    u8   done[MAX_ROUTES];
    // story in progress
    int  inStory;
    char route[24];
    int  pc;
    int  pending;               // stars earned so far this chapter
    int  nvars;
    char varName[ST_MAX_VARS][24];
    int  varVal[ST_MAX_VARS];
    char bg[32], place[64], title[96];
    char slot[ST_SLOTS][24];
} SaveData;

static SaveData save;

static void save_write(void) {
    mkdir("sdmc:/3ds", 0777);
    mkdir(SAVE_DIR, 0777);
    FILE *f = fopen(SAVE_PATH, "wb");
    if (!f) return;
    fwrite(&save, sizeof save, 1, f);
    fclose(f);
}

static void save_read(void) {
    memset(&save, 0, sizeof save);
    FILE *f = fopen(SAVE_PATH, "rb");
    if (f) {
        if (fread(&save, sizeof save, 1, f) != 1 || save.magic != SAVE_MAGIC) memset(&save, 0, sizeof save);
        fclose(f);
    }
    if (save.magic != SAVE_MAGIC) { save.magic = SAVE_MAGIC; save.stars = START_STARS; }
}

// ---------------------------------------------------------------- portraits (cached sprite sheets)
typedef struct { char id[24]; C2D_SpriteSheet sheet; int tried; } Portrait;
#define MAX_PORTRAITS 10
static Portrait portraits[MAX_PORTRAITS];

static void portraits_flush(void) {
    for (int i = 0; i < MAX_PORTRAITS; i++) {
        if (portraits[i].sheet) C2D_SpriteSheetFree(portraits[i].sheet);
        memset(&portraits[i], 0, sizeof portraits[i]);
    }
}

// returns 1 and fills img if the portrait exists
static int portrait_get(const char *id, C2D_Image *img) {
    int freeSlot = -1;
    for (int i = 0; i < MAX_PORTRAITS; i++) {
        if (portraits[i].tried && !strcmp(portraits[i].id, id)) {
            if (!portraits[i].sheet) return 0;
            *img = C2D_SpriteSheetGetImage(portraits[i].sheet, 0);
            return 1;
        }
        if (!portraits[i].tried && freeSlot < 0) freeSlot = i;
    }
    if (freeSlot < 0) { // cache full: drop the oldest
        if (portraits[0].sheet) C2D_SpriteSheetFree(portraits[0].sheet);
        memmove(&portraits[0], &portraits[1], sizeof(Portrait) * (MAX_PORTRAITS - 1));
        memset(&portraits[MAX_PORTRAITS - 1], 0, sizeof(Portrait));
        freeSlot = MAX_PORTRAITS - 1;
    }
    Portrait *p = &portraits[freeSlot];
    strncpy(p->id, id, sizeof p->id - 1);
    p->tried = 1;
    char path[96];
    snprintf(path, sizeof path, "romfs:/gfx/%s.t3x", id);
    p->sheet = C2D_SpriteSheetLoad(path);
    if (!p->sheet) return 0;
    *img = C2D_SpriteSheetGetImage(p->sheet, 0);
    return 1;
}

// ---------------------------------------------------------------- text helpers
static C2D_TextBuf tbuf;

static void text(const char *s, float x, float y, float scale, u32 color, int flags, float wrap) {
    C2D_Text t;
    C2D_TextParse(&t, tbuf, s);
    C2D_TextOptimize(&t);
    if (flags & C2D_WordWrap) C2D_DrawText(&t, flags | C2D_WithColor, x, y, 0.5f, scale, scale, color, wrap);
    else C2D_DrawText(&t, flags | C2D_WithColor, x, y, 0.5f, scale, scale, color);
}

static float text_width(const char *s, float scale) {
    C2D_Text t; float w = 0, h = 0;
    C2D_TextParse(&t, tbuf, s);
    C2D_TextGetDimensions(&t, scale, scale, &w, &h);
    return w;
}

// UTF-8 safe prefix of n bytes
static int utf8_cut(const char *s, int n) {
    int len = strlen(s);
    if (n >= len) return len;
    while (n > 0 && (s[n] & 0xC0) == 0x80) n--;
    return n;
}

// ---------------------------------------------------------------- drawing pieces
static void pill(float x, float y, float w, float h, u32 fill) {
    float r = h / 2;
    C2D_DrawRectSolid(x + r, y, 0.4f, w - 2 * r, h, fill);
    C2D_DrawCircleSolid(x + r, y + r, 0.4f, r, fill);
    C2D_DrawCircleSolid(x + w - r, y + r, 0.4f, r, fill);
}

static void panel(float x, float y, float w, float h, u32 fill, u32 border) {
    C2D_DrawRectSolid(x - 2, y - 2, 0.3f, w + 4, h + 4, border);
    C2D_DrawRectSolid(x, y, 0.35f, w, h, fill);
}

static void star_icon(float cx, float cy, float r, u32 c) {
    // little 4-point sparkle (two thin diamonds)
    C2D_DrawTriangle(cx, cy - r, c, cx + r * 0.3f, cy, c, cx - r * 0.3f, cy, c, 0.6f);
    C2D_DrawTriangle(cx, cy + r, c, cx + r * 0.3f, cy, c, cx - r * 0.3f, cy, c, 0.6f);
    C2D_DrawTriangle(cx - r, cy, c, cx, cy - r * 0.3f, c, cx, cy + r * 0.3f, c, 0.6f);
    C2D_DrawTriangle(cx + r, cy, c, cx, cy - r * 0.3f, c, cx, cy + r * 0.3f, c, 0.6f);
}

static int portrait_get(const char *id, C2D_Image *img);

static void draw_bg(const char *key, int frame) {
    // a picture background (gfx/bg_<key>.png) wins over the gradient
    char picId[40]; C2D_Image pic;
    snprintf(picId, sizeof picId, "bg_%s", key);
    if (key[0] && portrait_get(picId, &pic)) {
        C2D_DrawImageAt(pic, 0, 0, 0.0f, NULL, TOP_W / (float)pic.subtex->width, SCR_H / (float)pic.subtex->height);
        return;
    }
    const BgStyle *b = bg_find(key);
    unsigned int top = b ? b->top : 0xF6C9D8, bot = b ? b->bottom : 0x9E7BB5, acc = b ? b->accent : 0xFFFFFF;
    C2D_DrawRectangle(0, 0, 0.0f, TOP_W, SCR_H, rgb(top, 255), rgb(top, 255), rgb(bot, 255), rgb(bot, 255));
    // soft Y2K sparkles drifting a little
    static const float sx[] = { 30, 120, 210, 300, 370, 70, 260, 345 };
    static const float sy[] = { 24, 60, 18, 48, 90, 110, 100, 20 };
    for (int i = 0; i < 8; i++) {
        float tw = ((frame / 6 + i * 7) % 20) / 20.0f;
        star_icon(sx[i], sy[i], 3 + 3 * tw, rgb(acc, 120 + (u8)(100 * tw)));
    }
}

static void draw_portrait(const char *id, float cx, int dim, float scale) {
    C2D_Image img;
    if (!portrait_get(id, &img)) {
        // placeholder silhouette + name, so missing art never crashes the game
        const CastMember *c = cast_find(id);
        u32 col = rgb(c ? c->color : 0x888888, dim ? 120 : 220);
        C2D_DrawCircleSolid(cx, 110, 0.2f, 34 * scale, col);
        C2D_DrawRectSolid(cx - 40 * scale, 150, 0.2f, 80 * scale, 90, col);
        text(c ? c->name : id, cx - text_width(c ? c->name : id, 0.45f) / 2, 100, 0.45f, COL_WHITE, 0, 0);
        return;
    }
    float w = img.subtex->width * scale, h = img.subtex->height * scale;
    C2D_ImageTint tint;
    C2D_PlainImageTint(&tint, C2D_Color32(0x30, 0x20, 0x40, 0xFF), dim ? 0.35f : 0.0f);
    // tallest family member's head sits at y=8; shorter people stand lower
    C2D_DrawImageAt(img, cx - w / 2, 8 + 330 * scale - h, 0.2f, &tint, scale, scale);
}

// ---------------------------------------------------------------- game state
typedef enum { S_TITLE, S_SELECT, S_STORY, S_END, S_FEED } Screen;

static Screen screen = S_TITLE;
static Story story;
static int routeIdx = 0;
static EvType ev = EV_NONE;
static int reveal = 0;          // bytes of dialogue shown (typewriter)
static int choiceSel = 0;
static int selCursor = 0;
static int frame = 0;
static int feedIdx = 0;
static int chapterStars = 0;
static char toast[96];
static int toastTimer = 0;

static int route_index(const char *id) {
    for (int i = 0; i < ROUTE_COUNT; i++) if (!strcmp(ROUTES[i].id, id)) return i;
    return -1;
}

static char *load_text_file(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET);
    char *b = malloc(n + 1);
    if (!b) { fclose(f); return NULL; }
    size_t got = fread(b, 1, n, f); b[got] = 0;
    fclose(f);
    return b;
}

static void autosave(void) {
    save.inStory = 1;
    strncpy(save.route, ROUTES[routeIdx].id, sizeof save.route - 1);
    save.pc = story.sayPc;
    save.pending = story.stars;
    save.nvars = story.nvars;
    memcpy(save.varName, story.varName, sizeof save.varName);
    memcpy(save.varVal, story.varVal, sizeof save.varVal);
    memcpy(save.bg, story.bg, sizeof save.bg);
    memcpy(save.place, story.place, sizeof save.place);
    memcpy(save.title, story.title, sizeof save.title);
    memcpy(save.slot, story.slot, sizeof save.slot);
    save_write();
}

static void advance(void) {
    ev = story_next(&story);
    reveal = 0;
    choiceSel = 0;
    if (ev == EV_SAY || ev == EV_CHOICE || ev == EV_POST) autosave();
    if (ev == EV_END || ev == EV_ERROR) {
        chapterStars = story.stars;
        save.stars += chapterStars;
        if (routeIdx < MAX_ROUTES) save.done[routeIdx] = 1;
        save.inStory = 0;
        save_write();
        screen = S_END;
    }
}

static int start_route(int idx, int resume) {
    char path[96];
    snprintf(path, sizeof path, "romfs:/story/%s.txt", ROUTES[idx].id);
    char *src = load_text_file(path);
    if (!src) { snprintf(toast, sizeof toast, "Missing story file: %s", ROUTES[idx].id); toastTimer = 180; return 0; }
    story_free(&story);
    int ok = story_load(&story, src);
    free(src);
    if (!ok) { snprintf(toast, sizeof toast, "Story error: %s", story.err); toastTimer = 180; return 0; }
    portraits_flush();
    routeIdx = idx;
    if (resume) {
        story.pc = save.pc;
        story.stars = save.pending;
        story.nvars = save.nvars;
        memcpy(story.varName, save.varName, sizeof save.varName);
        memcpy(story.varVal, save.varVal, sizeof save.varVal);
        memcpy(story.bg, save.bg, sizeof story.bg);
        memcpy(story.place, save.place, sizeof story.place);
        memcpy(story.title, save.title, sizeof story.title);
        memcpy(story.slot, save.slot, sizeof story.slot);
    }
    screen = S_STORY;
    advance();
    return 1;
}

// ---------------------------------------------------------------- screens: TITLE
static const char *TITLE_CAST[] = { "suzy", "danny", "peppa", "wendy", "gerald" };

static void draw_title_top(void) {
    draw_bg("mall", frame);
    for (int i = 0; i < 5; i++) draw_portrait(TITLE_CAST[i], 40 + i * 80, i != 2, 0.7f);
    C2D_DrawRectangle(0, 0, 0.45f, TOP_W, 90, C2D_Color32(0xFF, 0xFF, 0xFF, 0xE0), C2D_Color32(0xFF, 0xFF, 0xFF, 0xE0),
                      C2D_Color32(0xFF, 0xFF, 0xFF, 0x00), C2D_Color32(0xFF, 0xFF, 0xFF, 0x00));
    text("PEPPA PIG BADDIES", TOP_W / 2 - text_width("PEPPA PIG BADDIES", 0.6f) / 2, 8, 0.6f, COL_PINK, 0, 0);
    text("NEW ROARK", TOP_W / 2 - text_width("NEW ROARK", 1.3f) / 2, 24, 1.3f, COL_INK, 0, 0);
    text("~ choices ~", TOP_W / 2 - text_width("~ choices ~", 0.5f) / 2, 62, 0.5f, COL_PINK, 0, 0);
}

static void draw_title_bottom(void) {
    C2D_DrawRectangle(0, 0, 0.0f, BOT_W, SCR_H, COL_SOFT, COL_SOFT, COL_LILAC, COL_LILAC);
    int y = 60;
    if (save.inStory) {
        panel(40, y, 240, 44, COL_WHITE, COL_PINK);
        text("Continue", 160 - text_width("Continue", 0.7f) / 2, y + 6, 0.7f, COL_INK, 0, 0);
        int ri = route_index(save.route);
        if (ri >= 0) {
            char buf[64]; snprintf(buf, sizeof buf, "%s's story", cast_find(ROUTES[ri].portrait) ? cast_find(ROUTES[ri].portrait)->name : ROUTES[ri].id);
            text(buf, 160 - text_width(buf, 0.42f) / 2, y + 27, 0.42f, COL_GREY, 0, 0);
        }
        y += 60;
    }
    panel(40, y, 240, 44, COL_WHITE, COL_PINK);
    text("Choose a Story", 160 - text_width("Choose a Story", 0.7f) / 2, y + 10, 0.7f, COL_INK, 0, 0);
    y += 60;
    panel(40, y, 240, 44, C2D_Color32(0xFF, 0xE0, 0xF0, 0xFF), COL_LILAC);
    text("Roarstagram", 160 - text_width("Roarstagram", 0.7f) / 2, y + 10, 0.7f, COL_PINK, 0, 0);
    char sbuf[32]; snprintf(sbuf, sizeof sbuf, "%d", save.stars);
    star_icon(18, 18, 8, COL_GOLD);
    text(sbuf, 30, 9, 0.55f, COL_INK, 0, 0);
    text("START: quit", 230, 220, 0.42f, COL_GREY, 0, 0);
}

static void update_title(u32 down, touchPosition tp) {
    int contY = 60, chooseY = save.inStory ? 120 : 60;
    if (down & KEY_TOUCH) {
        if (save.inStory && tp.py >= contY && tp.py < contY + 44 && tp.px >= 40 && tp.px < 280) {
            int ri = route_index(save.route);
            if (ri >= 0) start_route(ri, 1);
            return;
        }
        if (tp.py >= chooseY && tp.py < chooseY + 44 && tp.px >= 40 && tp.px < 280) { screen = S_SELECT; return; }
        if (tp.py >= chooseY + 60 && tp.py < chooseY + 104 && tp.px >= 40 && tp.px < 280) { feedIdx = 0; screen = S_FEED; return; }
    }
    if (down & KEY_A) {
        if (save.inStory) { int ri = route_index(save.route); if (ri >= 0) { start_route(ri, 1); return; } }
        screen = S_SELECT;
    }
    if (down & KEY_X) screen = S_SELECT;
    if (down & KEY_Y) { feedIdx = 0; screen = S_FEED; }
}

// ---------------------------------------------------------------- screens: SELECT
#define SEL_COLS 3
#define SEL_W 100
#define SEL_H 36

static void draw_select_top(void) {
    const Route *r = &ROUTES[selCursor];
    draw_bg("bedroom", frame);
    draw_portrait(r->portrait, 300, 0, 1.0f);
    const CastMember *c = cast_find(r->portrait);
    panel(14, 40, 210, 150, COL_SOFT, rgb(c ? c->color : 0xFF5FB0, 255));
    text(c ? c->name : r->id, 24, 48, 0.85f, rgb(c ? c->color : 0x2B1E3A, 255), 0, 0);
    char ch[80]; snprintf(ch, sizeof ch, "Chapter 1: %s", r->chapter);
    text(ch, 24, 78, 0.48f, COL_INK, 0, 0);
    text(r->blurb, 24, 100, 0.45f, COL_INK, C2D_WordWrap, 194);
    if (selCursor < MAX_ROUTES && save.done[selCursor]) { pill(24, 196, 80, 18, COL_GOLD); text("played", 40, 197, 0.45f, COL_INK, 0, 0); }
}

static void draw_select_bottom(void) {
    C2D_DrawRectangle(0, 0, 0.0f, BOT_W, SCR_H, COL_SOFT, COL_SOFT, COL_LILAC, COL_LILAC);
    text("Who's story?", 8, 4, 0.5f, COL_INK, 0, 0);
    for (int i = 0; i < ROUTE_COUNT; i++) {
        float x = 8 + (i % SEL_COLS) * (SEL_W + 4), y = 24 + (i / SEL_COLS) * (SEL_H + 4);
        const CastMember *c = cast_find(ROUTES[i].portrait);
        panel(x, y, SEL_W, SEL_H, i == selCursor ? rgb(c ? c->color : 0xFF5FB0, 255) : COL_WHITE, i == selCursor ? COL_INK : COL_GREY);
        const char *nm = c ? c->name : ROUTES[i].id;
        text(nm, x + SEL_W / 2 - text_width(nm, 0.5f) / 2, y + 9, 0.5f, i == selCursor ? COL_WHITE : COL_INK, 0, 0);
        if (i < MAX_ROUTES && save.done[i]) star_icon(x + SEL_W - 8, y + 8, 5, COL_GOLD);
    }
    text("A: play   B: back", 200, 224, 0.42f, COL_GREY, 0, 0);
}

static void update_select(u32 down, touchPosition tp) {
    if (down & KEY_DRIGHT) selCursor = (selCursor + 1) % ROUTE_COUNT;
    if (down & KEY_DLEFT)  selCursor = (selCursor + ROUTE_COUNT - 1) % ROUTE_COUNT;
    if (down & KEY_DDOWN)  selCursor = (selCursor + SEL_COLS) % ROUTE_COUNT;
    if (down & KEY_DUP)    selCursor = (selCursor + ROUTE_COUNT - SEL_COLS) % ROUTE_COUNT;
    if (down & KEY_B) { screen = S_TITLE; return; }
    if (down & KEY_TOUCH) {
        for (int i = 0; i < ROUTE_COUNT; i++) {
            float x = 8 + (i % SEL_COLS) * (SEL_W + 4), y = 24 + (i / SEL_COLS) * (SEL_H + 4);
            if (tp.px >= x && tp.px < x + SEL_W && tp.py >= y && tp.py < y + SEL_H) {
                if (selCursor == i) { start_route(i, 0); return; } // second tap plays
                selCursor = i;
            }
        }
    }
    if (down & KEY_A) start_route(selCursor, 0);
}

// ---------------------------------------------------------------- screens: STORY
static const float SLOT_X[ST_SLOTS] = { 82, 200, 318 };

// "rebecca_b" (an outfit variant) still counts as Rebecca talking
static int is_speaker(const char *slotId, const char *speaker) {
    size_t n = strlen(speaker);
    return !strncmp(slotId, speaker, n) && (slotId[n] == 0 || slotId[n] == '_');
}

// ---------------------------------------------------------------- Roarstagram post (phone on top screen)
static void draw_post_top(const char *user, const char *img, const char *likes, int liked, const char *caption) {
    C2D_DrawRectangle(0, 0, 0.0f, TOP_W, SCR_H, C2D_Color32(0xFD, 0xE6, 0xF3, 0xFF), C2D_Color32(0xFD, 0xE6, 0xF3, 0xFF),
                      C2D_Color32(0xC9, 0xA8, 0xFF, 0xFF), C2D_Color32(0xC9, 0xA8, 0xFF, 0xFF));
    // phone body
    panel(96, 2, 208, 236, COL_WHITE, COL_INK);
    // header: avatar dot + handle
    const CastMember *c = cast_find(user);
    C2D_DrawCircleSolid(112, 13, 0.4f, 7, rgb(c ? c->color : 0xFF5FB0, 255));
    text(feed_handle(user), 124, 5, 0.42f, COL_INK, 0, 0);
    text("Roarstagram", 300 - text_width("Roarstagram", 0.38f) - 4, 5, 0.38f, COL_PINK, 0, 0);
    C2D_Image pic;
    if (portrait_get(img, &pic)) C2D_DrawImageAt(pic, 117, 24, 0.3f, NULL, 0.86f, 0.86f);   // 165x206, keeps the 4:5 shape
    else {
        // text-only post ("post <who> none <caption>"): big caption on a gradient card
        C2D_DrawRectangle(104, 24, 0.3f, 192, 206, COL_PINK, COL_LILAC, COL_LILAC, COL_PINK);
        if (caption) text(caption, 112, 40, 0.55f, COL_WHITE, C2D_WordWrap, 176);
    }
    // like heart (two circles + triangle) and count
    u32 hc = liked ? C2D_Color32(0xFF, 0x3D, 0x7F, 0xFF) : COL_GREY;
    C2D_DrawCircleSolid(108, 222, 0.6f, 4, hc); C2D_DrawCircleSolid(115, 222, 0.6f, 4, hc);
    C2D_DrawTriangle(104, 223, hc, 119, 223, hc, 111.5f, 231, hc, 0.6f);
    if (likes) text(likes, 124, 216, 0.4f, COL_WHITE, 0, 0);
}

static void draw_post_bottom(const char *user, const char *caption, const char *c1u, const char *c1, const char *c2u, const char *c2) {
    C2D_DrawRectangle(0, 0, 0.0f, BOT_W, SCR_H, COL_SOFT, COL_SOFT, COL_LILAC, COL_LILAC);
    char b[600];
    snprintf(b, sizeof b, "%s  %s", feed_handle(user), caption);
    panel(8, 8, 304, 76, COL_WHITE, COL_PINK);
    text(b, 14, 12, 0.45f, COL_INK, C2D_WordWrap, 292);
    if (c1) { snprintf(b, sizeof b, "%s  %s", feed_handle(c1u), c1); text(b, 14, 96, 0.42f, COL_INK, C2D_WordWrap, 292); }
    if (c2) { snprintf(b, sizeof b, "%s  %s", feed_handle(c2u), c2); text(b, 14, 124, 0.42f, COL_INK, C2D_WordWrap, 292); }
}

static void draw_story_top(void) {
    if (ev == EV_POST) { draw_post_top(story.speaker, story.postImg, NULL, 1, story.text); return; }
    draw_bg(story.bg, frame);
    int talking = -1;
    if (ev == EV_SAY)
        for (int i = 0; i < ST_SLOTS; i++) if (story.slot[i][0] && is_speaker(story.slot[i], story.speaker)) talking = i;
    for (int i = 0; i < ST_SLOTS; i++)
        if (story.slot[i][0] && i != talking) draw_portrait(story.slot[i], SLOT_X[i], ev == EV_SAY && strcmp(story.speaker, "narr"), 1.0f);
    if (talking >= 0) draw_portrait(story.slot[talking], SLOT_X[talking], 0, 1.0f);

    if (story.place[0]) {
        float w = text_width(story.place, 0.42f) + 16;
        pill(6, 5, w, 16, C2D_Color32(0x2B, 0x1E, 0x3A, 0xB0));
        text(story.place, 14, 6, 0.42f, COL_WHITE, 0, 0);
    }

    if (ev == EV_SAY) {
        int narr = !strcmp(story.speaker, "narr");
        const CastMember *c = cast_find(story.speaker);
        u32 accent = narr ? COL_LILAC : rgb(c ? c->color : 0xFF5FB0, 255);
        panel(10, 152, 380, 82, narr ? C2D_Color32(0x2B, 0x1E, 0x3A, 0xE6) : COL_SOFT, accent);
        if (!narr) {
            const char *nm = (c && c->name[0]) ? c->name : story.speaker;
            float w = text_width(nm, 0.55f) + 20;
            pill(18, 140, w, 20, accent);
            text(nm, 28, 141, 0.55f, COL_WHITE, 0, 0);
        }
        char shown[512];
        int n = utf8_cut(story.text, reveal);
        memcpy(shown, story.text, n); shown[n] = 0;
        text(shown, 20, 162, 0.5f, narr ? COL_WHITE : COL_INK, C2D_WordWrap, 360);
        if (n >= (int)strlen(story.text) && (frame / 20) % 2)
            C2D_DrawTriangle(374, 222, accent, 384, 222, accent, 379, 229, accent, 0.6f);
    } else if (ev == EV_CHOICE) {
        panel(10, 196, 380, 36, C2D_Color32(0x2B, 0x1E, 0x3A, 0xE6), COL_PINK);
        text("What do you do?", 200 - text_width("What do you do?", 0.6f) / 2, 203, 0.6f, COL_WHITE, 0, 0);
    }
}

static float choice_y(int i) { return 30 + i * 50; }

static void draw_story_bottom(void) {
    C2D_DrawRectangle(0, 0, 0.0f, BOT_W, SCR_H, COL_SOFT, COL_SOFT, COL_LILAC, COL_LILAC);
    char sbuf[24]; snprintf(sbuf, sizeof sbuf, "%d", save.stars);
    star_icon(16, 13, 7, COL_GOLD);
    text(sbuf, 27, 4, 0.5f, COL_INK, 0, 0);
    if (story.title[0]) text(story.title, 70, 5, 0.38f, COL_GREY, 0, 0);

    if (ev == EV_POST) {
        draw_post_bottom(story.speaker, story.text, NULL, NULL, NULL, NULL);
        text("A: keep scrolling", 200, 224, 0.4f, COL_GREY, 0, 0);
        return;
    }
    if (ev == EV_CHOICE) {
        for (int i = 0; i < story.nopts; i++) {
            ChoiceOpt *o = &story.opts[i];
            int afford = save.stars >= o->cost;
            float y = choice_y(i);
            u32 fill = i == choiceSel ? COL_PINK : COL_WHITE;
            if (!afford) fill = C2D_Color32(0xE6, 0xE0, 0xEA, 0xFF);
            panel(12, y, 296, 42, fill, o->cost ? COL_GOLD : COL_INK);
            float tx = 22;
            if (o->cost) {
                pill(18, y + 11, 40, 20, COL_GOLD);
                star_icon(30, y + 21, 6, COL_WHITE);
                char cb[8]; snprintf(cb, sizeof cb, "%d", o->cost);
                text(cb, 40, y + 12, 0.48f, COL_INK, 0, 0);
                tx = 66;
            }
            text(o->text, tx, y + 5, 0.45f, i == choiceSel ? COL_WHITE : (afford ? COL_INK : COL_GREY), C2D_WordWrap, 300 - tx);
        }
    } else {
        panel(60, 90, 200, 50, COL_WHITE, COL_PINK);
        text("Tap or press A", 160 - text_width("Tap or press A", 0.6f) / 2, 104, 0.6f, COL_INK, 0, 0);
    }
    text("SELECT: menu  START: save & quit", 8, 224, 0.4f, COL_GREY, 0, 0);
}

static void pick_choice(int i) {
    ChoiceOpt *o = &story.opts[i];
    if (save.stars < o->cost) { snprintf(toast, sizeof toast, "Not enough stars! Finish chapters to earn more."); toastTimer = 120; return; }
    save.stars -= o->cost;
    story_choose(&story, i);
    advance();
}

static void update_story(u32 down, touchPosition tp) {
    if (down & KEY_SELECT) { save_write(); screen = S_TITLE; portraits_flush(); return; }
    if (ev == EV_SAY) {
        int full = reveal >= (int)strlen(story.text);
        if (down & (KEY_A | KEY_TOUCH)) { if (full) advance(); else reveal = strlen(story.text); }
        else if (down & KEY_B) reveal = strlen(story.text);
        else reveal += 2;
    } else if (ev == EV_POST) {
        if (down & (KEY_A | KEY_TOUCH)) advance();
    } else if (ev == EV_CHOICE) {
        if (down & KEY_DDOWN) choiceSel = (choiceSel + 1) % story.nopts;
        if (down & KEY_DUP) choiceSel = (choiceSel + story.nopts - 1) % story.nopts;
        if (down & KEY_A) { pick_choice(choiceSel); return; }
        if (down & KEY_TOUCH) {
            for (int i = 0; i < story.nopts; i++) {
                float y = choice_y(i);
                if (tp.px >= 12 && tp.px < 308 && tp.py >= y && tp.py < y + 42) { choiceSel = i; pick_choice(i); return; }
            }
        }
    }
}

// ---------------------------------------------------------------- screens: END
static void draw_end_top(void) {
    draw_bg("mall", frame);
    draw_portrait(ROUTES[routeIdx].portrait, 300, 0, 1.0f);
    panel(14, 50, 220, 130, COL_SOFT, COL_PINK);
    if (ev == EV_ERROR) {
        text("Story error", 26, 60, 0.7f, COL_PINK, 0, 0);
        text(story.err, 26, 90, 0.45f, COL_INK, C2D_WordWrap, 196);
        return;
    }
    text("Chapter Complete!", 26, 60, 0.7f, COL_PINK, 0, 0);
    text(story.title, 26, 88, 0.42f, COL_INK, C2D_WordWrap, 200);
    char b[48]; snprintf(b, sizeof b, "+%d", chapterStars);
    star_icon(40, 150, 10, COL_GOLD);
    text(b, 56, 138, 0.8f, COL_INK, 0, 0);
}

static void draw_end_bottom(void) {
    C2D_DrawRectangle(0, 0, 0.0f, BOT_W, SCR_H, COL_SOFT, COL_SOFT, COL_LILAC, COL_LILAC);
    text("Your choices shaped...", 12, 8, 0.5f, COL_INK, 0, 0);
    for (int i = 0; i < story.nvars && i < 8; i++) {
        char b[64]; snprintf(b, sizeof b, "%s: %d", story.varName[i], story.varVal[i]);
        text(b, 24, 34 + i * 20, 0.5f, COL_INK, 0, 0);
    }
    text("A: back to stories", 180, 222, 0.42f, COL_GREY, 0, 0);
}

static void update_end(u32 down) {
    if (down & (KEY_A | KEY_TOUCH | KEY_B)) { screen = S_SELECT; portraits_flush(); }
}

// ---------------------------------------------------------------- screens: FEED (Roarstagram app)
static void draw_feed_top(void) { const FeedPost *p = &FEED[feedIdx]; draw_post_top(p->user, p->img, p->likes, 1, p->caption); }
static void draw_feed_bottom(void) {
    const FeedPost *p = &FEED[feedIdx];
    draw_post_bottom(p->user, p->caption, p->c1user, p->c1, p->c2user, p->c2);
    char b[32]; snprintf(b, sizeof b, "%d / %d", feedIdx + 1, FEED_COUNT);
    text(b, 14, 200, 0.42f, COL_GREY, 0, 0);
    text("< > scroll   B: close app", 150, 222, 0.42f, COL_GREY, 0, 0);
}
static void update_feed(u32 down, touchPosition tp) {
    if (down & (KEY_DRIGHT | KEY_DDOWN | KEY_A)) feedIdx = (feedIdx + 1) % FEED_COUNT;
    if (down & (KEY_DLEFT | KEY_DUP)) feedIdx = (feedIdx + FEED_COUNT - 1) % FEED_COUNT;
    if (down & KEY_TOUCH) { if (tp.px > 160) feedIdx = (feedIdx + 1) % FEED_COUNT; else feedIdx = (feedIdx + FEED_COUNT - 1) % FEED_COUNT; }
    if (down & KEY_B) { screen = S_TITLE; portraits_flush(); }
}

// ---------------------------------------------------------------- main
int main(void) {
    romfsInit();
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();
    C3D_RenderTarget *top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    C3D_RenderTarget *bot = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
    tbuf = C2D_TextBufNew(8192);
    save_read();

    while (aptMainLoop()) {
        hidScanInput();
        u32 down = hidKeysDown();
        touchPosition tp; hidTouchRead(&tp);
        frame++;

        if (down & KEY_START) {
            if (screen == S_STORY) save_write();
            break;
        }
        switch (screen) {
            case S_TITLE:  update_title(down, tp); break;
            case S_SELECT: update_select(down, tp); break;
            case S_STORY:  update_story(down, tp); break;
            case S_END:    update_end(down); break;
            case S_FEED:   update_feed(down, tp); break;
        }
        if (toastTimer > 0) toastTimer--;

        C2D_TextBufClear(tbuf);
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

        C2D_TargetClear(top, COL_WHITE);
        C2D_SceneBegin(top);
        switch (screen) {
            case S_TITLE:  draw_title_top(); break;
            case S_SELECT: draw_select_top(); break;
            case S_STORY:  draw_story_top(); break;
            case S_END:    draw_end_top(); break;
            case S_FEED:   draw_feed_top(); break;
        }

        C2D_TargetClear(bot, COL_WHITE);
        C2D_SceneBegin(bot);
        switch (screen) {
            case S_TITLE:  draw_title_bottom(); break;
            case S_SELECT: draw_select_bottom(); break;
            case S_STORY:  draw_story_bottom(); break;
            case S_END:    draw_end_bottom(); break;
            case S_FEED:   draw_feed_bottom(); break;
        }
        if (toastTimer > 0) {
            panel(10, 196, 300, 22, COL_INK, COL_PINK);
            text(toast, 16, 199, 0.42f, COL_WHITE, 0, 0);
        }
        C3D_FrameEnd(0);
    }

    portraits_flush();
    story_free(&story);
    C2D_TextBufDelete(tbuf);
    C2D_Fini();
    C3D_Fini();
    gfxExit();
    romfsExit();
    return 0;
}
