// ============================================================
//  CHOICES  -  a 2004 teen-drama story collection for Nintendo 3DS
//  "Maple Valley" visual identity: dusty rose, cream, cocoa,
//  champagne gold, sunset light, teen-magazine layouts.
//  citro2d + libctru.  Stories live in romfs:/story.txt + romfs:/stories/
// ============================================================
#include <3ds.h>
#include <citro2d.h>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <sstream>
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <dirent.h>
#include <sys/stat.h>

using std::string;
using std::vector;

// =============== PALETTE (Maple Valley) ===============
#define RGBA(r,g,b,a) C2D_Color32(r,g,b,a)
static const u32 ROSE     = RGBA(0xC9,0x8B,0x93,0xFF); // dusty rose
static const u32 BLUSH    = RGBA(0xE9,0xC9,0xC6,0xFF); // muted blush
static const u32 CREAM    = RGBA(0xF4,0xEA,0xDC,0xFF); // cream
static const u32 IVORY    = RGBA(0xFB,0xF6,0xEE,0xFF); // warm ivory
static const u32 COCOA    = RGBA(0x5B,0x3A,0x2E,0xFF); // cocoa brown
static const u32 MAUVE    = RGBA(0xA5,0x7C,0x8A,0xFF); // mauve
static const u32 PLUM     = RGBA(0x6E,0x4A,0x5E,0xFF); // soft plum
static const u32 GOLD     = RGBA(0xC9,0xA9,0x6E,0xFF); // champagne gold
static const u32 GOLD_LT  = RGBA(0xE6,0xD2,0xA8,0xFF);
static const u32 INK      = RGBA(0x3A,0x26,0x20,0xFF); // text cocoa-black
static const u32 SHADOW   = RGBA(0x3A,0x26,0x20,0x38);
static const u32 SUNSET_T = RGBA(0xE8,0xB8,0xA8,0xFF); // peach-rose sky
static const u32 SUNSET_B = RGBA(0xF6,0xE2,0xC8,0xFF); // golden-hour horizon

static u32 withA(u32 c, u8 a) { return (c & 0x00FFFFFF) | ((u32)a << 24); }
static u8 cr(u32 c) { return c & 0xFF; }
static u8 cg(u32 c) { return (c >> 8) & 0xFF; }
static u8 cb(u32 c) { return (c >> 16) & 0xFF; }
static u32 mix(u32 a, u32 b, float t) {
    return RGBA((u8)(cr(a) + (cr(b) - cr(a)) * t), (u8)(cg(a) + (cg(b) - cg(a)) * t),
                (u8)(cb(a) + (cb(b) - cb(a)) * t), 0xFF);
}
static u32 rawHex(const string& h) {
    unsigned v = strtoul(h.c_str(), nullptr, 16);
    return RGBA((v>>16)&0xFF, (v>>8)&0xFF, v&0xFF, 0xFF);
}
// "Film grade": pulls any bright script colour into the dusty, warm palette
static u32 grade(u32 c) {
    u8 g = (cr(c) + cg(c) + cb(c)) / 3;
    u32 gray = RGBA(g, g, g, 0xFF);
    return mix(mix(c, gray, 0.35f), RGBA(0xB8,0x92,0x8C,0xFF), 0.22f);
}
static u32 hexColor(const string& h) { return grade(rawHex(h)); }

// =============== FONTS ===============
// romfs:/fonts/serif.bcfnt + italic.bcfnt are built from fonts/*.ttf by the Makefile.
// If they're missing the game falls back to the 3DS system font.
enum FontId { F_SYS = 0, F_SERIF = 1, F_ITALIC = 2 };
static C2D_Font gFonts[3] = { nullptr, nullptr, nullptr };
static float gFontFix[3] = { 1, 1, 1 };
static C2D_TextBuf gBuf;

static void fontsInit() {
    gFonts[F_SERIF]  = C2D_FontLoad("romfs:/fonts/serif.bcfnt");
    gFonts[F_ITALIC] = C2D_FontLoad("romfs:/fonts/italic.bcfnt");
    float sys = C2D_FontGetInfo(nullptr)->lineFeed;
    for (int i = 1; i < 3; i++) if (gFonts[i]) gFontFix[i] = sys / (float)C2D_FontGetInfo(gFonts[i])->lineFeed;
}
static void parseText(C2D_Text* t, const string& s, int f) {
    if (f != F_SYS && gFonts[f]) C2D_TextFontParse(t, gFonts[f], gBuf, s.c_str());
    else C2D_TextParse(t, gBuf, s.c_str());
    C2D_TextOptimize(t);
}
enum { AL_LEFT = 0, AL_CENTER = 1, AL_RIGHT = 2 };
static void drawText(const string& s, float x, float y, float sc, u32 col, float wrap = 0, int align = AL_LEFT, int f = F_SERIF) {
    C2D_Text t; parseText(&t, s, f);
    sc *= gFontFix[f];
    u32 flags = C2D_WithColor;
    if (wrap > 0) flags |= C2D_WordWrap;
    if (align == AL_CENTER) flags |= C2D_AlignCenter;
    if (align == AL_RIGHT)  flags |= C2D_AlignRight;
    C2D_DrawText(&t, flags, x, y, 0.9f, sc, sc, col, wrap);
}
static float textW(const string& s, float sc, int f = F_SERIF) {
    C2D_Text t; parseText(&t, s, f); float w, h;
    C2D_TextGetDimensions(&t, sc * gFontFix[f], sc * gFontFix[f], &w, &h); return w;
}
static string upper(string s) { for (auto& c : s) c = toupper(c); return s; }
static string spaced(const string& s) { string o; for (size_t i = 0; i < s.size(); i++) { o += s[i]; if (i + 1 < s.size() && s[i] != ' ') o += ' '; } return o; }
static string pretty(string id) { for (auto& c : id) if (c == '_') c = ' '; bool up = true;
    for (auto& c : id) { if (up && c >= 'a' && c <= 'z') c -= 32; up = c == ' '; } return id; }

// =============== AUDIO (unchanged engine) ===============
static bool gAudio = false;
static s16* gBlip = nullptr;
static ndspWaveBuf gWave[2];
static std::map<string, std::pair<s16*, u32>> gClips;

static void audioInit() {
    if (R_FAILED(ndspInit())) return;
    gAudio = true;
    ndspSetOutputMode(NDSP_OUTPUT_STEREO);
    for (int c = 0; c < 2; c++) {
        ndspChnReset(c); ndspChnSetInterp(c, NDSP_INTERP_LINEAR);
        ndspChnSetFormat(c, NDSP_FORMAT_MONO_PCM16);
        float m[12] = {0}; m[0] = m[1] = 0.6f; ndspChnSetMix(c, m);
    }
    gBlip = (s16*)linearAlloc(1600 * sizeof(s16));
}
// softer, breathier "voice" blip than before (sine + gentle 2nd harmonic)
static void playBlip(int pitch) {
    if (!gAudio || pitch <= 0) return;
    const int rate = 22050, n = 1300; float ph = 0;
    for (int i = 0; i < n; i++) {
        float t = (float)i / n, env = (t < 0.1f ? t * 10 : 1.0f) * (1 - t);
        ph += 2 * M_PI * (pitch + 40 * sinf(i * 0.012f)) / rate;
        gBlip[i] = (s16)((sinf(ph) * 0.7f + sinf(ph * 2) * 0.12f) * env * 7000);
    }
    DSP_FlushDataCache(gBlip, n * sizeof(s16));
    ndspChnWaveBufClear(0); ndspChnSetRate(0, rate);
    memset(&gWave[0], 0, sizeof(ndspWaveBuf));
    gWave[0].data_vaddr = gBlip; gWave[0].nsamples = n;
    ndspChnWaveBufAdd(0, &gWave[0]);
}
static void playClip(const string& name) {
    if (!gAudio) return;
    auto it = gClips.find(name);
    if (it == gClips.end()) {
        FILE* f = fopen(("romfs:/voice/" + name + ".raw").c_str(), "rb");
        if (!f) { gClips[name] = {nullptr, 0}; return; }
        fseek(f, 0, SEEK_END); long sz = ftell(f); fseek(f, 0, SEEK_SET);
        s16* d = (s16*)linearAlloc(sz); fread(d, 1, sz, f); fclose(f);
        DSP_FlushDataCache(d, sz);
        gClips[name] = {d, (u32)(sz / 2)}; it = gClips.find(name);
    }
    if (!it->second.first) return;
    ndspChnWaveBufClear(1); ndspChnSetRate(1, 22050);
    memset(&gWave[1], 0, sizeof(ndspWaveBuf));
    gWave[1].data_vaddr = it->second.first; gWave[1].nsamples = it->second.second;
    ndspChnWaveBufAdd(1, &gWave[1]);
}

// =============== LOOKS / WARDROBE ===============
// style: 0 long layers, 1 bob, 2 high pony, 3 curls, 4 sleek middle part
// outfit: 0 silk cami, 1 baby tee, 2 velour tracksuit, 3 sundress, 4 blazer, 5 cardigan+pearls
struct Look { u32 skin, hair, shirt, clip; bool glasses; int style; int outfit = 0; bool hoops = true; };
struct Character { string id; string name; Look look; int pitch; };
static std::map<string, Character> gChars;

static const u32 SKINS[] = { RGBA(0x4E,0x2F,0x22,0xFF), RGBA(0x6E,0x45,0x30,0xFF), RGBA(0x8D,0x5A,0x3B,0xFF),
                             RGBA(0xB5,0x7F,0x5A,0xFF), RGBA(0xD8,0xA8,0x82,0xFF), RGBA(0xF1,0xD0,0xB4,0xFF) };
static const char* SKIN_N[] = {"Espresso","Cocoa","Caramel","Honey","Almond","Porcelain"};
static const u32 HAIRS[] = { RGBA(0x1C,0x14,0x12,0xFF), RGBA(0x3E,0x26,0x1C,0xFF), RGBA(0x6B,0x40,0x2C,0xFF),
                             RGBA(0xC9,0xA0,0x6A,0xFF), RGBA(0x8A,0x3E,0x2E,0xFF), RGBA(0x9C,0x6E,0x7C,0xFF) };
static const char* HAIR_N[] = {"Jet","Espresso","Chestnut","Honey Blonde","Auburn","Mauve Rose"};
static const char* STYLE_N[] = {"Long Layers","Chic Bob","High Pony","Soft Curls","Sleek Part"};
static const char* OUTFIT_N[] = {"Silk Cami","Baby Tee","Velour Tracksuit","Sundress","Cropped Blazer","Cardigan & Pearls"};
static const u32 OUTFIT_C[] = { ROSE, IVORY, PLUM, BLUSH, COCOA, CREAM };

// ---- tiny shape helpers ----
static void rect(float x, float y, float w, float h, u32 c, float z = 0.5f) { C2D_DrawRectSolid(x, y, z, w, h, c); }
static void ell(float x, float y, float w, float h, u32 c, float z = 0.5f) { C2D_DrawEllipseSolid(x, y, z, w, h, c); }
static void circ(float x, float y, float r, u32 c, float z = 0.5f) { C2D_DrawCircleSolid(x, y, z, r, c); }
static void line(float x0, float y0, float x1, float y1, u32 c, float t = 1, float z = 0.5f) { C2D_DrawLine(x0, y0, c, x1, y1, c, t, z); }
static void tri(float x0, float y0, float x1, float y1, float x2, float y2, u32 c, float z = 0.5f) { C2D_DrawTriangle(x0, y0, c, x1, y1, c, x2, y2, c, z); }
static void vgrad(float x, float y, float w, float h, u32 top, u32 bot, float z = 0.1f) { C2D_DrawRectangle(x, y, z, w, h, top, top, bot, bot); }
static void frameRect(float x, float y, float w, float h, u32 c, float t = 1, float z = 0.5f) {
    rect(x, y, w, t, c, z); rect(x, y + h - t, w, t, c, z); rect(x, y, t, h, c, z); rect(x + w - t, y, t, h, c, z);
}
// line-art heart (doodle style, not a cartoon blob)
static void heartLine(float cx, float cy, float s, u32 c, float th = 1.2f) {
    float px = 0, py = 0;
    for (int i = 0; i <= 24; i++) {
        float t = i / 24.0f * 2 * M_PI;
        float x = cx + s * 16 * powf(sinf(t), 3) / 16.0f;
        float y = cy - s * (13 * cosf(t) - 5 * cosf(2*t) - 2 * cosf(3*t) - cosf(4*t)) / 16.0f;
        if (i) line(px, py, x, y, c, th, 0.8f);
        px = x; py = y;
    }
}
static void heartSolid(float cx, float cy, float s, u32 c, float z = 0.8f) {
    circ(cx - s * 0.5f, cy - s * 0.2f, s * 0.55f, c, z); circ(cx + s * 0.5f, cy - s * 0.2f, s * 0.55f, c, z);
    tri(cx - s * 1.03f, cy, cx + s * 1.03f, cy, cx, cy + s * 1.1f, c, z);
}
static void sparkle(float x, float y, float s, u32 c) {
    tri(x - s*0.25f, y, x + s*0.25f, y, x, y - s, c, 0.8f); tri(x - s*0.25f, y, x + s*0.25f, y, x, y + s, c, 0.8f);
    tri(x, y - s*0.25f, x, y + s*0.25f, x - s, y, c, 0.8f); tri(x, y - s*0.25f, x, y + s*0.25f, x + s, y, c, 0.8f);
}
static void diamond(float x, float y, float s, u32 c, float z = 0.8f) { tri(x - s, y, x + s, y, x, y - s, c, z); tri(x - s, y, x + s, y, x, y + s, c, z); }
static void goldRule(float x, float y, float w) {   // ———— ◆ ————
    rect(x, y, w / 2 - 6, 0.8f, GOLD, 0.7f); rect(x + w / 2 + 6, y, w / 2 - 6, 0.8f, GOLD, 0.7f); diamond(x + w / 2, y + 0.4f, 2.5f, GOLD);
}
static void tape(float x, float y, float w, float h) { rect(x, y, w, h, RGBA(0xF4,0xEA,0xDC,0xB0), 0.85f); }

// =============== PORTRAITS (teen-drama style) ===============
static void drawBackHair(float cx, float cy, float s, const Look& L) {
    u32 h = L.hair, hi = mix(L.hair, GOLD_LT, 0.25f);
    switch (L.style) {
    case 0: ell(cx-46*s, cy-46*s, 92*s, 128*s, h); ell(cx-50*s, cy+20*s, 26*s, 70*s, h); ell(cx+24*s, cy+20*s, 26*s, 70*s, h);
            ell(cx+28*s, cy+30*s, 8*s, 40*s, withA(hi, 0x90)); break;
    case 1: ell(cx-44*s, cy-46*s, 88*s, 86*s, h); rect(cx-44*s, cy, 88*s, 28*s, h); break;
    case 2: circ(cx, cy-56*s, 14*s, h); ell(cx+6*s, cy-62*s, 22*s, 78*s, h); ell(cx-36*s, cy-44*s, 72*s, 50*s, h);
            circ(cx, cy-46*s, 4*s, GOLD); break;
    case 3: for (int i = 0; i < 14; i++) { float a = M_PI * (i / 13.0f) * 1.3f - 0.15f;
                circ(cx + cosf(a + M_PI) * 42*s, cy - 6*s + sinf(a + M_PI) * -40*s + (i > 6 ? 0 : 0), 15*s, h); }
            for (int i = 0; i < 6; i++) { circ(cx - 46*s + (i%2)*4*s, cy + i*14*s, 13*s, h); circ(cx + 46*s - (i%2)*4*s, cy + i*14*s, 13*s, h); }
            break;
    default: ell(cx-44*s, cy-46*s, 88*s, 60*s, h); rect(cx-44*s, cy-20*s, 88*s, 108*s, h);
            rect(cx+30*s, cy-10*s, 4*s, 90*s, withA(hi, 0x80)); break;
    }
}
static void drawOutfit(float cx, float cy, float s, const Look& L) {
    u32 c = L.shirt, dk = mix(c, COCOA, 0.25f);
    float top = cy + 46*s;
    ell(cx-46*s, top - 4*s, 92*s, 70*s, L.skin);          // shoulders
    switch (L.outfit) {
    case 0: // silk cami with lace trim + thin straps
        ell(cx-38*s, top + 10*s, 76*s, 60*s, c); rect(cx-32*s, top + 12*s, 64*s, 3*s, withA(IVORY, 0xC0));
        line(cx-22*s, top + 13*s, cx-24*s, top - 2*s, c, 1.4f*s); line(cx+22*s, top + 13*s, cx+24*s, top - 2*s, c, 1.4f*s); break;
    case 1: // fitted baby tee
        ell(cx-46*s, top - 2*s, 92*s, 70*s, c); ell(cx-12*s, top - 6*s, 24*s, 14*s, L.skin);
        drawText("2004", cx, top + 20*s, 0.32f*s, MAUVE, 0, AL_CENTER, F_ITALIC); break;
    case 2: // velour tracksuit: hood collar + gold zip
        ell(cx-48*s, top - 4*s, 96*s, 72*s, c); ell(cx-24*s, top - 8*s, 48*s, 16*s, dk);
        ell(cx-10*s, top - 6*s, 20*s, 10*s, L.skin); line(cx, top + 2*s, cx, top + 60*s, GOLD, 1.2f*s);
        circ(cx, top + 8*s, 2*s, GOLD); break;
    case 3: // sweetheart sundress
        rect(cx-34*s, top + 16*s, 68*s, 50*s, c); circ(cx-16*s, top + 16*s, 18*s, c); circ(cx+16*s, top + 16*s, 18*s, c);
        rect(cx-34*s, top + 16*s, 68*s, 40*s, c); line(cx-20*s, top + 4*s, cx-26*s, top - 4*s, c, 2*s); line(cx+20*s, top + 4*s, cx+26*s, top - 4*s, c, 2*s); break;
    case 4: // cropped blazer over white tee
        ell(cx-48*s, top - 4*s, 96*s, 72*s, c); tri(cx-14*s, top - 2*s, cx+14*s, top - 2*s, cx, top + 40*s, IVORY);
        tri(cx-14*s, top - 2*s, cx-4*s, top + 30*s, cx-24*s, top + 12*s, dk); tri(cx+14*s, top - 2*s, cx+4*s, top + 30*s, cx+24*s, top + 12*s, dk); break;
    default: // cardigan + pearls
        ell(cx-48*s, top - 4*s, 96*s, 72*s, c); tri(cx-16*s, top - 2*s, cx+16*s, top - 2*s, cx, top + 30*s, L.skin);
        for (int i = 0; i < 9; i++) { float a = M_PI * (0.15f + 0.7f * i / 8.0f); circ(cx + cosf(a) * 14*s, top - 2*s + sinf(a) * 12*s, 1.6f*s, IVORY); }
        for (int i = 0; i < 3; i++) circ(cx, top + 34*s + i*8*s, 1.4f*s, GOLD); break;
    }
    if (L.outfit != 5) { // delicate gold chain
        for (int i = 0; i < 11; i++) { float a = M_PI * (0.12f + 0.76f * i / 10.0f); circ(cx + cosf(a) * 11*s, top - 6*s + sinf(a) * 9*s, 0.7f*s, GOLD); }
        circ(cx, top + 3*s, 1.6f*s, GOLD);
    }
}
static void drawFrontHair(float cx, float cy, float s, const Look& L) {
    u32 h = L.hair, hi = withA(mix(L.hair, GOLD_LT, 0.35f), 0xA0);
    switch (L.style) {
    case 0: ell(cx-38*s, cy-44*s, 52*s, 28*s, h); ell(cx-2*s, cy-46*s, 40*s, 22*s, h); ell(cx-36*s, cy-30*s, 12*s, 46*s, h);
            ell(cx-20*s, cy-40*s, 18*s, 5*s, hi); break;
    case 1: ell(cx-36*s, cy-44*s, 72*s, 28*s, h); rect(cx-36*s, cy-30*s, 72*s, 8*s, h); ell(cx-34*s, cy-28*s, 10*s, 52*s, h);
            ell(cx+24*s, cy-28*s, 10*s, 52*s, h); ell(cx-10*s, cy-40*s, 20*s, 4*s, hi); break;
    case 2: ell(cx-34*s, cy-42*s, 68*s, 22*s, h); ell(cx-8*s, cy-40*s, 22*s, 4*s, hi); break;
    case 3: for (int i = 0; i < 6; i++) circ(cx - 28*s + i * 11*s, cy - 34*s - (i % 2) * 3*s, 9*s, h);
            circ(cx - 34*s, cy - 18*s, 8*s, h); circ(cx + 34*s, cy - 18*s, 8*s, h); break;
    default: ell(cx-36*s, cy-44*s, 36*s, 30*s, h); ell(cx, cy-44*s, 36*s, 30*s, h); ell(cx-36*s, cy-28*s, 8*s, 60*s, h);
             ell(cx+28*s, cy-28*s, 8*s, 60*s, h); ell(cx+6*s, cy-40*s, 16*s, 4*s, hi); break;
    }
}
// talk: 0 closed, 1 open.  blink: eyes closed
static void drawPortrait(float cx, float cy, float s, const Look& L, int talk = 0, bool blink = false) {
    u32 skinSh = mix(L.skin, COCOA, 0.22f), skinHi = mix(L.skin, IVORY, 0.18f);
    drawBackHair(cx, cy, s, L);
    drawOutfit(cx, cy, s, L);
    rect(cx-7*s, cy+24*s, 14*s, 24*s, L.skin); ell(cx-9*s, cy+26*s, 18*s, 8*s, skinSh);    // neck + chin shadow
    ell(cx-36*s, cy-4*s, 10*s, 16*s, L.skin); ell(cx+26*s, cy-4*s, 10*s, 16*s, L.skin);    // ears
    ell(cx-32*s, cy-36*s, 64*s, 66*s, L.skin);                                              // face
    ell(cx-26*s, cy+4*s, 52*s, 30*s, L.skin);                                               // softer jaw
    ell(cx-20*s, cy-30*s, 26*s, 10*s, withA(skinHi, 0x80));                                 // forehead glow
    if (L.hoops) { // gold hoops
        circ(cx-31*s, cy+14*s, 5*s, GOLD); circ(cx-31*s, cy+14*s, 3.6f*s, L.hair);
        circ(cx+31*s, cy+14*s, 5*s, GOLD); circ(cx+31*s, cy+14*s, 3.6f*s, L.hair);
    }
    // brows
    line(cx-22*s, cy-15*s, cx-8*s, cy-17*s, mix(L.hair, COCOA, 0.3f), 2*s);
    line(cx+8*s, cy-17*s, cx+22*s, cy-15*s, mix(L.hair, COCOA, 0.3f), 2*s);
    // eyes: almond, lashes, winged liner, catchlights
    for (int side = -1; side <= 1; side += 2) {
        float ex = cx + side * 13*s, ey = cy - 4*s;
        if (blink) { line(ex - 7*s, ey + 1*s, ex + 7*s, ey + 1*s, INK, 1.6f*s); continue; }
        ell(ex - 7*s, ey - 4*s, 14*s, 9*s, IVORY);
        ell(ex - 4*s, ey - 4.5f*s, 8*s, 9*s, RGBA(0x4A,0x2E,0x24,0xFF));
        circ(ex, ey, 2.2f*s, INK);
        circ(ex + 1.6f*s, ey - 2*s, 1.3f*s, IVORY);
        line(ex - 8*s, ey - 4*s, ex + 8*s, ey - 4.5f*s, INK, 1.8f*s);
        line(ex + side * 8*s, ey - 4.5f*s, ex + side * 11*s, ey - 7*s, INK, 1.4f*s);           // wing
        rect(ex - 6*s, ey - 7.5f*s, 12*s, 1.4f*s, withA(ROSE, 0x70));                          // soft lid shimmer
    }
    // nose
    line(cx + 1*s, cy + 2*s, cx + 2.5f*s, cy + 8*s, withA(skinSh, 0xC0), 1.2f*s);
    circ(cx - 0.5f*s, cy + 4*s, 1.2f*s, withA(skinHi, 0xC0));
    // blush
    ell(cx-26*s, cy+6*s, 13*s, 6*s, withA(ROSE, 0x60)); ell(cx+13*s, cy+6*s, 13*s, 6*s, withA(ROSE, 0x60));
    // glossy lips
    u32 lip = mix(ROSE, PLUM, 0.25f);
    if (talk) { ell(cx-6*s, cy+13*s, 12*s, 7*s, mix(PLUM, INK, 0.4f)); ell(cx-6*s, cy+12*s, 12*s, 3*s, lip); ell(cx-6*s, cy+18*s, 12*s, 3*s, lip); }
    else { ell(cx-7*s, cy+13*s, 7*s, 3.5f*s, lip); ell(cx, cy+13*s, 7*s, 3.5f*s, lip); ell(cx-7*s, cy+15*s, 14*s, 5*s, mix(ROSE, IVORY, 0.1f));
           ell(cx-3*s, cy+16*s, 5*s, 1.6f*s, withA(IVORY, 0xC0)); }
    drawFrontHair(cx, cy, s, L);
    // barrette in accent colour
    rect(cx+10*s, cy-30*s, 14*s, 3*s, L.clip); rect(cx+10*s, cy-30*s, 14*s, 1*s, GOLD_LT);
    if (L.glasses) for (int side = -1; side <= 1; side += 2) frameRect(cx + side*13*s - 10*s, cy - 10*s, 20*s, 13*s, GOLD, 1.1f*s);
}

// =============== ILLUSTRATED PORTRAITS ===============
// Painted comic-book portraits: art/portraits/<charId>.png  ->  romfs:/gfx/<charId>.t3x (built by Makefile).
// Optional talking frame: <charId>_talk.png.  Player: me_<outfit 0-5>.png (falls back to me.png).
// If no art exists for a character, a placeholder bust is drawn so the game still runs.
static std::map<string, C2D_SpriteSheet> gArt;
static C2D_SpriteSheet artFor(const string& id) {
    if (id.empty()) return nullptr;
    auto it = gArt.find(id);
    if (it != gArt.end()) return it->second;
    C2D_SpriteSheet sh = C2D_SpriteSheetLoad(("romfs:/gfx/" + id + ".t3x").c_str());
    gArt[id] = sh; return sh;
}
// draws art fitted to a box (cover-crop from the top). returns false if no art.
static bool drawArt(const string& id, bool talk, float x, float y, float w, float h, float z) {
    C2D_SpriteSheet sh = talk ? artFor(id + "_talk") : nullptr;
    if (!sh) sh = artFor(id);
    if (!sh) return false;
    C2D_Image img = C2D_SpriteSheetGetImage(sh, 0);
    float iw = img.subtex->width, ih = img.subtex->height;
    float sc = std::max(w / iw, h / ih);
    C2D_DrawImageAt(img, x + (w - iw * sc) / 2, y, z, nullptr, sc, sc);
    return true;
}

// =============== GAME STATE / SAVE ===============
struct Game {
    std::map<string,int> vars;
    std::set<string> stickers, done;
    Look me;
    string myName = "You";
    vector<string> owned;
    int hero = -1;          // -1 = my Style File look, 0-5 = illustrated heroine
};
static Game G;
static const char* SAVE_DIR  = "sdmc:/3ds/NannyDiaries";          // kept: old saves still load
static const char* SAVE_PATH = "sdmc:/3ds/NannyDiaries/save.txt";
static int& V(const string& k) { return G.vars[k]; }
static string gSavedAt = "";

static void saveGame() {
    mkdir("sdmc:/3ds", 0777); mkdir(SAVE_DIR, 0777);
    FILE* f = fopen(SAVE_PATH, "w"); if (!f) return;
    fprintf(f, "hearts %d\n", V("hearts"));
    fprintf(f, "look %lu %lu %lu %lu %d %d\n", G.me.skin, G.me.hair, G.me.shirt, G.me.clip, G.me.glasses, G.me.style);
    fprintf(f, "wardrobe %d %d %d\n", G.me.outfit, G.me.hoops ? 1 : 0, V("redo"));
    string nm = G.myName; for (auto& c : nm) if (c == ' ') c = '_';
    fprintf(f, "name %s\n", nm.c_str());
    fprintf(f, "hero %d\n", G.hero);
    for (auto& s : G.stickers) fprintf(f, "sticker %s\n", s.c_str());
    for (auto& s : G.done)     fprintf(f, "done %s\n", s.c_str());
    for (auto& o : G.owned)    fprintf(f, "own %s\n", o.c_str());
    fclose(f);
    gSavedAt = "just now";
}
static bool loadGame() {
    std::ifstream in(SAVE_PATH); if (!in) return false;
    G.stickers.clear(); G.done.clear(); G.owned.clear();
    string k;
    while (in >> k) {
        if (k == "hearts") in >> V("hearts");
        else if (k == "look") { unsigned long a,b,c,d; int g,st; in >> a >> b >> c >> d >> g >> st;
            G.me.skin = a; G.me.hair = b; G.me.shirt = c; G.me.clip = d; G.me.glasses = g != 0; G.me.style = st; }
        else if (k == "wardrobe") { int o, h, r; in >> o >> h >> r; G.me.outfit = o; G.me.hoops = h; V("redo") = r;
                                    G.me.shirt = OUTFIT_C[o % 6]; }
        else if (k == "name") { in >> G.myName; for (auto& c : G.myName) if (c == '_') c = ' '; }
        else if (k == "hero") in >> G.hero;
        else if (k == "sticker") { string s; in >> s; G.stickers.insert(s); }
        else if (k == "done") { string s; in >> s; G.done.insert(s); }
        else if (k == "own") { string s; in >> s; G.owned.push_back(s); }
        else { string rest; std::getline(in, rest); }   // unknown key: skip line
    }
    gSavedAt = "on SD card";
    return true;
}
static void wearOutfit(int o) { G.me.outfit = o; G.me.shirt = OUTFIT_C[o % 6]; }

// =============== SCRIPT (engine unchanged) ===============
struct Line { string cmd, a, b, c; };
static vector<Line> gScript;
static std::map<string,int> gLabels;
static string trim(const string& s) {
    size_t a = s.find_first_not_of(" \t\r"), b = s.find_last_not_of(" \t\r");
    return a == string::npos ? "" : s.substr(a, b - a + 1);
}
static vector<string> split(const string& s, char d) {
    vector<string> out; string cur; std::stringstream ss(s);
    while (std::getline(ss, cur, d)) out.push_back(trim(cur));
    return out;
}
static std::set<string> gAllStickers;
static int gCharSeed = 0;
static void loadFile(const string& path) {
    std::ifstream in(path); string raw;
    while (std::getline(in, raw)) {
        string l = trim(raw);
        if (l.empty() || l[0] == ';') continue;
        if (l[0] == '#') { gLabels[l.substr(1)] = gScript.size(); continue; }
        if (l[0] == '>') { auto p = split(l.substr(1), '|'); while (p.size() < 3) p.push_back("");
                           gScript.push_back({">", p[0], p[1], p[2]}); continue; }
        size_t sp = l.find(' ');
        string cmd = l.substr(0, sp), rest = sp == string::npos ? "" : trim(l.substr(sp+1));
        auto p = split(rest, '|'); while (p.size() < 3) p.push_back("");
        if (cmd == "char") {
            auto q = split(p[2], ',');
            if (q.size() >= 7) {
                Character c; c.id = p[0]; c.name = p[1];
                c.look.skin = grade(rawHex(q[0])) ; c.look.skin = mix(rawHex(q[0]), c.look.skin, 0.25f); // skin: barely graded
                c.look.hair = mix(rawHex(q[1]), grade(rawHex(q[1])), 0.6f);
                c.look.shirt = hexColor(q[2]); c.look.clip = hexColor(q[3]);
                c.look.glasses = q[4] == "1"; c.look.style = atoi(q[5].c_str());
                if (c.look.style < 2 && (gCharSeed % 3) == 0) c.look.style = 3 + (gCharSeed / 3) % 2; // more hair variety
                c.look.outfit = gCharSeed % 6; c.look.hoops = (gCharSeed % 2) == 0;
                c.pitch = atoi(q[6].c_str());
                gChars[p[0]] = c; gCharSeed++;
            }
            continue;
        }
        gScript.push_back({cmd, p[0], p[1], p[2]});
        if (cmd == "sticker") gAllStickers.insert(p[0]);
    }
}
static void loadScript() {
    loadFile("romfs:/story.txt");
    DIR* d = opendir("romfs:/stories"); if (!d) return;
    vector<string> files; struct dirent* e;
    while ((e = readdir(d))) { string n = e->d_name; if (n.size() > 4 && n.substr(n.size()-4) == ".txt") files.push_back(n); }
    closedir(d); std::sort(files.begin(), files.end());
    for (auto& f : files) loadFile("romfs:/stories/" + f);
}
static string subst(string s) {
    size_t i; while ((i = s.find("{name}")) != string::npos) s.replace(i, 6, G.myName); return s;
}
static void applyEffects(const string& fx) {
    std::stringstream ss(fx); string e;
    while (ss >> e) {
        size_t op = e.find_first_of("+-=");
        if (op == string::npos) continue;
        string k = e.substr(0, op); int n = atoi(e.c_str() + op + 1);
        if (k == "outfit") { wearOutfit(n); continue; }               // outfit=N changes your look
        if (e[op] == '+') V(k) += n; else if (e[op] == '-') V(k) -= n; else V(k) = n;
        if (k == "happy" || k == "tired" || k == "hungry") { if (V(k) < 0) V(k) = 0; if (V(k) > 5) V(k) = 5; }
        if (V("hearts") < 0) V("hearts") = 0;
    }
}
static int fxOutfit(const string& fx) { size_t i = fx.find("outfit="); return i == string::npos ? -1 : atoi(fx.c_str() + i + 7); }
static bool evalCond(const string& c) {
    if (c.rfind("has:", 0) == 0) return G.stickers.count(c.substr(4));
    const char* ops[] = {">=","<=","==",">","<"};
    for (auto op : ops) {
        size_t i = c.find(op); if (i == string::npos) continue;
        int l = V(c.substr(0, i)), r = atoi(c.c_str() + i + strlen(op)); string o = op;
        if (o == ">=") return l >= r; if (o == "<=") return l <= r;
        if (o == "==") return l == r; if (o == ">") return l > r; return l < r;
    }
    return false;
}

// =============== INPUT ===============
static u32 kDown; static touchPosition touch; static bool touched;
static void pollInput() { hidScanInput(); kDown = hidKeysDown(); touched = kDown & KEY_TOUCH; if (hidKeysHeld() & KEY_TOUCH) hidTouchRead(&touch); }
static bool tapIn(float x, float y, float w, float h) { return touched && touch.px >= x && touch.px <= x+w && touch.py >= y && touch.py <= y+h; }

// =============== SCENE CHROME ===============
static C3D_RenderTarget *gTop, *gBot;
static u32 gBg = ROSE;                 // per-story tint (from "bg")
static int gFrame = 0;
static string gStoryTitle = "", gStoryKey = "", gBondVar = "", gBondLabel = "";

static void beginFrame() {
    C2D_TextBufClear(gBuf);
    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
    C2D_TargetClear(gTop, CREAM); C2D_TargetClear(gBot, IVORY);
}
// Golden-hour sky + Tuscan campus arcade silhouette + soft sun flare
static void drawSunset(float W, float H, u32 tint, bool arches = true) {
    u32 top = mix(SUNSET_T, tint, 0.35f), bot = mix(SUNSET_B, tint, 0.12f);
    vgrad(0, 0, W, H * 0.72f, top, bot);
    vgrad(0, H * 0.72f, W, H * 0.28f, bot, mix(bot, CREAM, 0.6f));
    float sx = W * 0.72f, sy = H * 0.58f;                                // sun + flare
    for (int i = 5; i > 0; i--) circ(sx, sy, 10 + i * 12, withA(GOLD_LT, 0x12 + (5 - i) * 4), 0.11f);
    circ(sx, sy, 10, withA(IVORY, 0xC0), 0.12f);
    if (arches) {
        u32 a = withA(mix(COCOA, tint, 0.2f), 0x34);
        float base = H * 0.80f;
        rect(0, base - 34, W, 6, a, 0.13f);                              // cornice
        for (float x = 6; x < W; x += 34) {                              // arcade columns + arches
            rect(x, base - 28, 6, 28, a, 0.13f);
            for (int k = 0; k < 10; k++) { float t = k / 9.0f * M_PI;
                rect(x + 6 + 14 - cosf(t) * 14, base - 28 + 8 - sinf(t) * 8, 2, 2, a, 0.13f); }
        }
        rect(W * 0.12f, base - 70, 18, 42, a, 0.13f);                    // bell tower
        tri(W * 0.12f - 2, base - 70, W * 0.12f + 20, base - 70, W * 0.12f + 9, base - 86, a, 0.13f);
        rect(0, base, W, H - base, withA(COCOA, 0x10), 0.13f);
    }
    for (int i = 0; i < 26; i++) {                                       // film grain / dust motes
        float x = (i * 97 + gFrame / 4) % (int)W, y = (i * 61) % (int)H;
        circ(x, y, 0.8f, withA(IVORY, 0x50), 0.14f);
    }
}
// cream paper with faint ruled lines + margin, like a diary
static void drawPaper(float x, float y, float w, float h, bool lines = true, float z = 0.3f) {
    rect(x + 2, y + 3, w, h, SHADOW, z);
    rect(x, y, w, h, IVORY, z + 0.01f);
    if (lines) { for (float ly = y + 18; ly < y + h - 4; ly += 15) rect(x + 6, ly, w - 12, 0.6f, withA(MAUVE, 0x30), z + 0.02f);
                 rect(x + 16, y, 0.8f, h, withA(ROSE, 0x60), z + 0.02f); }
    frameRect(x + 3, y + 3, w - 6, h - 6, withA(GOLD, 0x90), 0.6f, z + 0.02f);
}
static void kicker(const string& s, float x, float y, u32 c = GOLD, int al = AL_LEFT) { drawText(spaced(upper(s)), x, y, 0.36f, c, 0, al, F_SERIF); }

// polaroid with a portrait inside; overflow is hidden by the bottom strip
static void drawPolaroid(float x, float y, float w, float h, const Look& L, const string& caption, int talk, bool blink, u32 tint, const string& artId = "") {
    rect(x + 3, y + 4, w, h, SHADOW, 0.3f);
    rect(x, y, w, h, IVORY, 0.31f);
    float px = x + 7, py = y + 7, pw = w - 14, ph = h - 34;
    C2D_DrawRectangle(px, py, 0.32f, pw, ph, mix(SUNSET_T, tint, 0.3f), mix(SUNSET_T, tint, 0.3f), SUNSET_B, SUNSET_B);
    circ(px + pw * 0.8f, py + ph * 0.75f, 18, withA(GOLD_LT, 0x50), 0.33f);
    bool art = false;
    if (!artId.empty()) {
        if (artId == "me" && G.hero >= 0) art = drawArt("hero_" + std::to_string(G.hero), talk, px, py, pw, ph, 0.34f);
        if (art) {}
        else if (artId == "me") art = drawArt("me_" + std::to_string(L.outfit), talk, px, py, pw, ph, 0.34f) || drawArt("me", talk, px, py, pw, ph, 0.34f);
        else art = drawArt(artId, talk, px, py, pw, ph, 0.34f);
    }
    if (art) { vgrad(px, py + ph - 24, pw, 24, withA(COCOA, 0), withA(COCOA, 0x40), 0.35f); }   // soft cinematic vignette
    else { float s = pw / 112.0f; drawPortrait(px + pw / 2, py + ph * 0.46f, s, L, talk, blink); }
    // cover overflow: frame sides + bottom strip on top of portrait
    rect(x, y, w, 7, IVORY, 0.7f); rect(x, y, 7, h, IVORY, 0.7f); rect(x + w - 7, y, 7, h, IVORY, 0.7f);
    rect(x, py + ph, w, h - (py + ph - y), IVORY, 0.7f);
    drawText(caption, x + w / 2, py + ph + 5, 0.5f, PLUM, 0, AL_CENTER, F_ITALIC);
    tape(x + w / 2 - 16, y - 5, 32, 11);
}

// --- HUD: hearts (gold), mood/energy/appetite bars, relationship bond ---
static void drawHUD(bool showMeters = true) {
    rect(0, 0, 320, 24, IVORY, 0.6f); rect(0, 24, 320, 0.8f, GOLD, 0.61f); rect(0, 26, 320, 0.4f, withA(GOLD, 0x80), 0.61f);
    heartSolid(13, 11, 5.5f, ROSE, 0.62f);
    drawText(std::to_string(V("hearts")), 22, 4, 0.5f, COCOA, 0, AL_LEFT, F_SERIF);
    if (showMeters) {
        const char* nm[] = {"mood","energy","appetite"};
        int val[] = { V("happy"), 5 - V("tired"), 5 - V("hungry") };
        u32 cl[] = { ROSE, MAUVE, GOLD };
        for (int i = 0; i < 3; i++) {
            float x = 56 + i * 56;
            drawText(nm[i], x, 2, 0.3f, PLUM, 0, AL_LEFT, F_ITALIC);
            rect(x, 15, 44, 3, withA(MAUVE, 0x30), 0.62f);
            rect(x, 15, 44 * std::max(0, std::min(5, val[i])) / 5.0f, 3, cl[i], 0.63f);
        }
    }
    if (!gBondVar.empty()) {          // relationship indicator: 5 line-art hearts that fill in
        int b = std::max(0, std::min(5, V(gBondVar)));
        drawText(gBondLabel, 314, 1, 0.3f, PLUM, 0, AL_RIGHT, F_ITALIC);
        for (int i = 0; i < 5; i++) {
            float hx = 262 + i * 11, hy = 17;
            if (i < b) heartSolid(hx, hy - 1, 3.6f, ROSE, 0.63f); else heartLine(hx, hy - 1, 3.6f, withA(MAUVE, 0xA0), 0.8f);
        }
    }
}
// option button: ivory card, gold hairline, selected = dusty rose with gold diamond
static void drawButton(float x, float y, float w, float h, const string& label, bool hi, const string& tag = "") {
    rect(x + 2, y + 2, w, h, SHADOW, 0.4f);
    rect(x, y, w, h, hi ? ROSE : IVORY, 0.41f);
    frameRect(x + 2, y + 2, w - 4, h - 4, hi ? withA(IVORY, 0x90) : withA(GOLD, 0xA0), 0.6f, 0.42f);
    float tx = x + 12;
    if (!tag.empty()) { drawText(tag, x + 13, y + h / 2 - 8, 0.5f, hi ? IVORY : GOLD, 0, AL_CENTER, F_ITALIC); tx = x + 26; }
    if (hi) diamond(x + w - 12, y + h / 2, 3, GOLD_LT, 0.45f);
    drawText(label, tx, y + h / 2 - 7.5f, 0.46f, hi ? IVORY : INK, w - (tx - x) - 20, AL_LEFT, F_SERIF);
}
static void footer(const string& left, const string& right) {
    rect(0, 222, 320, 0.6f, withA(GOLD, 0xA0), 0.6f);
    drawText(left, 8, 225, 0.38f, MAUVE, 0, AL_LEFT, F_ITALIC);
    drawText(right, 312, 225, 0.38f, MAUVE, 0, AL_RIGHT, F_ITALIC);
}

// =============== DIALOGUE ===============
static Character* findChar(const string& id) {
    if (id == "me") { auto& c = gChars["me"]; c.id = "me"; c.look = G.me; c.name = G.myName; c.pitch = 760; return &c; }
    auto it = gChars.find(id); return it == gChars.end() ? nullptr : &it->second;
}
static void drawFlipPhone(const string& from, const string& msg) {
    rect(148, 22, 104, 196, SHADOW, 0.4f);
    C2D_DrawRectangle(144, 18, 0.41f, 104, 196, mix(PLUM, IVORY, 0.35f), mix(PLUM, IVORY, 0.35f), PLUM, PLUM);
    rect(152, 32, 88, 96, mix(CREAM, GOLD_LT, 0.3f), 0.42f); frameRect(152, 32, 88, 96, GOLD, 0.8f, 0.43f);
    drawText("1 new message", 196, 35, 0.3f, PLUM, 0, AL_CENTER, F_ITALIC);
    drawText(from, 156, 46, 0.38f, COCOA, 80);
    drawText(msg, 156, 62, 0.34f, INK, 80, AL_LEFT, F_SYS);
    for (int i = 0; i < 12; i++) ell(158 + (i % 3) * 28, 140 + (i / 3) * 18, 20, 11, withA(IVORY, 0xC0), 0.42f);
    sparkle(236, 28, 5, GOLD_LT);
}
static void drawDialogueTop(Character* ch, const string& shown, bool phone, const string& from, bool talking) {
    C2D_SceneBegin(gTop);
    drawSunset(400, 240, gBg);
    if (phone) { drawFlipPhone(from, shown); return; }
    bool blink = (gFrame % 200) < 7;
    if (ch) {
        bool isMe = ch->id == "me";
        float px = isMe ? 18 : 262;
        drawPolaroid(px, 10, 120, 150, ch->look, ch->name + ", '04", talking && (gFrame / 5) % 2, blink, gBg, ch->id);
    }
    // dialogue card
    rect(13, 166, 378, 70, SHADOW, 0.6f);
    rect(10, 162, 378, 70, withA(IVORY, 0xF4), 0.61f);
    frameRect(14, 166, 370, 62, withA(GOLD, 0xC0), 0.7f, 0.62f);
    if (ch) {
        rect(22, 154, textW(upper(ch->name), 0.42f) + 24, 17, PLUM, 0.63f);
        drawText(upper(ch->name), 34, 156, 0.42f, IVORY, 0, AL_LEFT, F_SERIF);
        diamond(28, 162.5f, 2.2f, GOLD_LT, 0.64f);
    }
    drawText(shown, 24, 177, 0.5f, INK, 350, AL_LEFT, F_SERIF);
}
static void drawDiaryBottom(const string& hint) {
    C2D_SceneBegin(gBot);
    rect(0, 0, 320, 240, IVORY, 0.1f);
    drawPaper(10, 34, 300, 184, true, 0.2f);
    drawHUD();
    if (!gStoryTitle.empty()) {
        kicker("Now Playing", 32, 44, GOLD);
        drawText(gStoryTitle, 32, 56, 0.75f, COCOA, 260, AL_LEFT, F_SERIF);
        goldRule(32, 84, 160);
    }
    // a little "mirror" of you, wearing your current outfit
    drawPolaroid(206, 96, 86, 104, G.me, G.myName, 0, (gFrame % 220) < 6, gBg, "me");
    heartLine(40, 112, 6, withA(ROSE, 0xB0)); sparkle(60, 104, 4, withA(GOLD, 0xC0));
    drawText("dear diary...", 32, 128, 0.48f, PLUM, 160, AL_LEFT, F_ITALIC);
    if (!hint.empty() && (gFrame / 24) % 2) drawText(hint, 32, 196, 0.4f, MAUVE, 0, AL_LEFT, F_ITALIC);
}
static void runSay(const string& who, const string& text, bool phone = false) {
    Character* ch = phone ? nullptr : findChar(who);
    string full = subst(text);
    size_t shown = 0; int tick = 0;
    while (aptMainLoop()) {
        pollInput();
        bool typing = shown < full.size();
        if (typing) {
            if (++tick % 2 == 0) { shown++; if (shown % 3 == 0 && full[shown-1] != ' ') playBlip(ch ? ch->pitch : 700); }
            if (kDown & (KEY_A | KEY_B) || touched) shown = full.size();
        } else if (kDown & KEY_A || touched) return;
        gFrame++;
        beginFrame();
        drawDialogueTop(ch, full.substr(0, shown), phone, who, typing);
        drawDiaryBottom(typing ? "" : "tap to turn the page  \xE2\x86\x92");
        C3D_FrameEnd(0);
    }
}

// =============== CHOICES (quiz / outfit / Story Land TOC) ===============
static bool gStoryDone(const string& label) {
    size_t u = label.rfind("_start"); return u != string::npos && G.done.count(label.substr(0, u));
}
// opts may be "Title ~ tagline"
static int runChoice(const string& promptIn, const vector<string>& opts, const vector<int>& costs = {}, const vector<string>& fx = {}, const vector<string>& targets = {}) {
    bool toc = !promptIn.empty() && promptIn[0] == '@';
    string prompt = subst(toc ? promptIn.substr(1) : promptIn);
    bool outfitMode = false; for (auto& f : fx) if (fxOutfit(f) >= 0) outfitMode = true;
    int sel = 0, n = opts.size(); const int PER = toc ? 6 : 5;
    while (aptMainLoop()) {
        pollInput();
        int page = sel / PER, pages = (n + PER - 1) / PER;
        if (kDown & KEY_DOWN) sel = (sel + 1) % n;
        if (kDown & KEY_UP)   sel = (sel + n - 1) % n;
        if (kDown & (KEY_RIGHT | KEY_R)) sel = std::min(n - 1, (page + 1) * PER);
        if (kDown & (KEY_LEFT | KEY_L))  sel = std::max(0, (page - 1) * PER);
        page = sel / PER;
        int first = page * PER, cnt = std::min(PER, n - first);
        float bh = toc ? 26 : 30, gap = toc ? 5 : 6, y0 = toc ? 32 : 34;
        for (int i = 0; i < cnt; i++) if (tapIn(16, y0 + i*(bh+gap), 288, bh)) { if (sel == first + i) return sel; sel = first + i; }
        if (pages > 1) { if (tapIn(0, 218, 90, 22)) sel = std::max(0, (page - 1) * PER); if (tapIn(230, 218, 90, 22)) sel = std::min(n - 1, (page + 1) * PER); }
        if (kDown & KEY_A) return sel;
        gFrame++;
        string selTitle = opts[sel], selTag = "";
        size_t tl = selTitle.find('~'); if (tl != string::npos) { selTag = trim(selTitle.substr(tl + 1)); selTitle = trim(selTitle.substr(0, tl)); }

        beginFrame();
        C2D_SceneBegin(gTop);
        if (toc) {
            // ---- STORY LAND: magazine "In This Issue" spread ----
            drawSunset(400, 240, ROSE);
            drawPaper(14, 12, 236, 216, false, 0.3f);
            kicker("Choices Magazine  \xC2\xB7  Fall 2004", 28, 22, GOLD);
            drawText("In This Issue", 28, 32, 1.0f, COCOA, 0, AL_LEFT, F_SERIF);
            goldRule(28, 64, 208);
            bool isStory = sel < (int)targets.size() && targets[sel].find("_start") != string::npos;
            char pg[16]; snprintf(pg, sizeof pg, "p. %02d", sel + 1);
            drawText(isStory ? pg : "", 28, 74, 0.5f, GOLD, 0, AL_LEFT, F_ITALIC);
            drawText(selTitle, 28, 88, 0.85f, PLUM, 210, AL_LEFT, F_SERIF);
            drawText(selTag, 28, 132, 0.48f, INK, 210, AL_LEFT, F_ITALIC);
            if (isStory && gStoryDone(targets[sel])) {
                circ(214, 196, 18, GOLD, 0.6f); circ(214, 196, 15, GOLD_LT, 0.61f);
                drawText("read", 214, 186, 0.36f, COCOA, 0, AL_CENTER, F_ITALIC); diamond(214, 200, 3, COCOA, 0.62f);
            }
            drawText(prompt, 28, 200, 0.42f, MAUVE, 200, AL_LEFT, F_ITALIC);
            drawPolaroid(268, 26, 118, 150, G.me, G.myName, 0, (gFrame % 200) < 6, ROSE, "me");
            sparkle(380, 190, 6, GOLD_LT); heartLine(290, 200, 7, withA(IVORY, 0xD0));
        } else if (outfitMode) {
            // ---- STYLE FILE: outfit selection with live preview ----
            drawSunset(400, 240, gBg, false);
            Look prev = G.me; int o = sel < (int)fx.size() ? fxOutfit(fx[sel]) : -1;
            if (o >= 0) { prev.outfit = o; prev.shirt = OUTFIT_C[o % 6]; }
            drawPolaroid(232, 12, 150, 196, prev, o >= 0 ? OUTFIT_N[o % 6] : G.myName, 0, (gFrame % 200) < 6, gBg, "me");
            drawPaper(16, 30, 198, 170, false, 0.3f);
            kicker("The Style File", 30, 42, GOLD);
            drawText(prompt, 30, 58, 0.62f, COCOA, 176, AL_LEFT, F_SERIF);
            goldRule(30, 140, 170);
            drawText(selTitle, 30, 150, 0.48f, PLUM, 176, AL_LEFT, F_ITALIC);
        } else {
            // ---- THE BIG CHOICE: teen-magazine quiz card ----
            drawSunset(400, 240, gBg);
            drawPaper(46, 28, 308, 176, false, 0.3f);
            kicker("The Big Choice", 200, 44, GOLD, AL_CENTER);
            goldRule(120, 58, 160);
            drawText(prompt, 200, 76, 0.72f, COCOA, 260, AL_CENTER, F_SERIF);
            drawText(gStoryTitle.empty() ? "" : ("from  " + gStoryTitle), 200, 176, 0.42f, MAUVE, 0, AL_CENTER, F_ITALIC);
            tape(60, 22, 34, 12); tape(306, 22, 34, 12);
        }

        C2D_SceneBegin(gBot);
        rect(0, 0, 320, 240, IVORY, 0.1f);
        for (float ly = 30; ly < 240; ly += 15) rect(0, ly, 320, 0.5f, withA(MAUVE, 0x18), 0.11f);
        drawHUD(!toc);
        for (int i = 0; i < cnt; i++) {
            string lab = opts[first + i]; size_t t2 = lab.find('~'); if (t2 != string::npos) lab = trim(lab.substr(0, t2));
            if (first + i < (int)costs.size() && costs[first + i] > 0) lab += "  \xC2\xB7  " + std::to_string(costs[first + i]) + " hearts";
            string tag = toc ? "" : string(1, (char)('A' + first + i));
            if (toc && first + i < (int)targets.size() && gStoryDone(targets[first + i])) lab += "  \xC2\xB7  read";
            drawButton(16, y0 + i*(bh+gap), 288, bh, lab, first + i == sel, tag);
        }
        if (pages > 1) { char b[24]; snprintf(b, sizeof b, "page %d of %d", page + 1, pages); footer("\xE2\x80\xB9 prev", "next \xE2\x80\xBA");
                         drawText(b, 160, 225, 0.38f, PLUM, 0, AL_CENTER, F_ITALIC); }
        C3D_FrameEnd(0);
    }
    return 0;
}

// =============== STYLE FILE: character creator ===============
static void runCreator() {
    int skin = 2, hair = 1, style = 0, outfit = 0; bool glasses = false, hoops = true;
    int row = 0; const int ROWS = 7;
    const char* rows[] = {"Skin","Hair","Style","Outfit","Glasses","Gold Hoops","Done"};
    while (aptMainLoop()) {
        pollInput();
        if (kDown & KEY_DOWN) row = (row + 1) % ROWS;
        if (kDown & KEY_UP) row = (row + ROWS - 1) % ROWS;
        int d = (kDown & KEY_RIGHT) ? 1 : (kDown & KEY_LEFT) ? -1 : 0;
        for (int i = 0; i < ROWS; i++) if (tapIn(16, 32 + i*27, 288, 24)) { row = i; d = 1; }
        if (d) {
            if (row == 0) skin = (skin + d + 6) % 6;   if (row == 1) hair = (hair + d + 6) % 6;
            if (row == 2) style = (style + d + 5) % 5; if (row == 3) outfit = (outfit + d + 6) % 6;
            if (row == 4) glasses = !glasses;          if (row == 5) hoops = !hoops;
        }
        if (row == 6 && (d || (kDown & KEY_A))) break;
        G.me.skin = SKINS[skin]; G.me.hair = HAIRS[hair]; G.me.style = style; G.me.glasses = glasses; G.me.hoops = hoops;
        G.me.clip = GOLD; wearOutfit(outfit);
        gFrame++;
        beginFrame();
        C2D_SceneBegin(gTop);
        drawSunset(400, 240, ROSE);
        drawPaper(16, 20, 190, 200, false, 0.3f);
        kicker("Meet the New Girl", 30, 32, GOLD);
        drawText("The Style File", 30, 44, 0.9f, COCOA, 0, AL_LEFT, F_SERIF);
        goldRule(30, 76, 160);
        drawText(string("Hair: ") + HAIR_N[hair] + ", " + STYLE_N[style], 30, 88, 0.42f, INK, 168, AL_LEFT, F_ITALIC);
        drawText(string("Wearing: ") + OUTFIT_N[outfit], 30, 106, 0.42f, INK, 168, AL_LEFT, F_ITALIC);
        drawText(string("Skin: ") + SKIN_N[skin], 30, 124, 0.42f, INK, 168, AL_LEFT, F_ITALIC);
        drawText("\"Every girl has a story.\nThis one's yours.\"", 30, 162, 0.46f, PLUM, 168, AL_LEFT, F_ITALIC);
        drawPolaroid(226, 14, 158, 206, G.me, "the new girl", 0, (gFrame % 200) < 6, ROSE, "me");
        C2D_SceneBegin(gBot);
        rect(0, 0, 320, 240, IVORY, 0.1f);
        string val[] = {SKIN_N[skin], HAIR_N[hair], STYLE_N[style], OUTFIT_N[outfit], glasses ? "On" : "Off", hoops ? "On" : "Off", ""};
        kicker("Customize", 160, 10, GOLD, AL_CENTER);
        for (int i = 0; i < ROWS; i++) {
            string lab = i == 6 ? "That's Me  \xE2\x80\x94  Continue" : string(rows[i]) + ":  \xE2\x80\xB9 " + val[i] + " \xE2\x80\xBA";
            drawButton(16, 32 + i*27, 288, 24, lab, i == row);
        }
        C3D_FrameEnd(0);
    }
    SwkbdState sw; char buf[20] = {0};
    swkbdInit(&sw, SWKBD_TYPE_NORMAL, 2, 16);
    swkbdSetHintText(&sw, "Your name");
    if (swkbdInputText(&sw, buf, sizeof buf) == SWKBD_BUTTON_CONFIRM && buf[0]) G.myName = buf;
}

// =============== THE BOUTIQUE (shop) ===============
struct ShopItem { string id, label, blurb; int cost; int outfit; };
static const vector<ShopItem> SHOP = {
    {"top_cami",     "Silk Cami",          "Dusty rose silk with a lace hem.",            3, 0},
    {"top_baby",     "Baby Tee",           "Cropped, fitted, '2004' in script.",         3, 1},
    {"top_velour",   "Velour Tracksuit",   "Plum velour. Gold zip. Iconic.",             6, 2},
    {"top_sundress", "Sundress",           "For sunset walks on the quad.",              5, 3},
    {"top_blazer",   "Cropped Blazer",     "Cocoa blazer, very editor-in-chief.",        6, 4},
    {"top_cardi",    "Cardigan & Pearls",  "Old money, new girl.",                       5, 5},
    {"acc_hoops",    "Gold Hoops",         "The finishing touch. Toggle on or off.",     2, -1},
    {"acc_glasses",  "Gold Frames",        "Rimless with gold arms. Smart & chic.",      2, -1},
    {"redo",         "Second Chance",      "Undo one bad choice in any story.",          3, -1},
};
static bool owns(const string& id) { return std::find(G.owned.begin(), G.owned.end(), id) != G.owned.end(); }
static void applyOwned(const string&) {}   // wardrobe is saved directly now (kept for old saves)
static void runShop() {
    int sel = 0, n = SHOP.size() + 1; string toast = ""; int toastT = 0;
    while (aptMainLoop()) {
        pollInput();
        const int PER = 6; int page = sel / PER;
        if (kDown & KEY_DOWN) sel = (sel + 1) % n;
        if (kDown & KEY_UP) sel = (sel + n - 1) % n;
        if (kDown & (KEY_RIGHT | KEY_R)) sel = std::min(n - 1, (page + 1) * PER);
        if (kDown & (KEY_LEFT | KEY_L)) sel = std::max(0, (page - 1) * PER);
        if (kDown & KEY_B) return;
        page = sel / PER; int first = page * PER, cnt = std::min(PER, n - first);
        bool act = kDown & KEY_A;
        for (int i = 0; i < cnt; i++) if (tapIn(16, 32 + i*30, 288, 26)) { if (sel == first + i) act = true; sel = first + i; }
        if (tapIn(230, 218, 90, 22)) sel = std::min(n - 1, (page + 1) * PER);
        if (tapIn(0, 218, 90, 22)) sel = std::max(0, (page - 1) * PER);
        if (act) {
            if (sel == (int)SHOP.size()) return;
            auto& it = SHOP[sel];
            bool have = owns(it.id) && it.id != "redo";
            if (!have && V("hearts") < it.cost) { toast = "Not enough hearts yet."; toastT = 90; }
            else {
                if (!have) { V("hearts") -= it.cost; if (it.id == "redo") V("redo")++; else G.owned.push_back(it.id); playClip("yay"); }
                if (it.outfit >= 0) wearOutfit(it.outfit);
                if (it.id == "acc_hoops") G.me.hoops = !G.me.hoops;
                if (it.id == "acc_glasses") G.me.glasses = !G.me.glasses;
                toast = have ? "Wearing it." : "Wrapped in tissue paper. It's yours."; toastT = 90;
                saveGame();
            }
        }
        gFrame++;
        bool leave = sel == (int)SHOP.size();
        beginFrame();
        C2D_SceneBegin(gTop);
        drawSunset(400, 240, BLUSH);
        drawPaper(14, 14, 214, 212, false, 0.3f);
        kicker("Maple Ave.  \xC2\xB7  Est. 2004", 28, 24, GOLD);
        drawText("The Boutique", 28, 36, 1.0f, COCOA, 0, AL_LEFT, F_SERIF);
        goldRule(28, 68, 186);
        Look prev = G.me;
        if (!leave) {
            auto& it = SHOP[sel];
            if (it.outfit >= 0) { prev.outfit = it.outfit; prev.shirt = OUTFIT_C[it.outfit]; }
            if (it.id == "acc_hoops") prev.hoops = true; if (it.id == "acc_glasses") prev.glasses = true;
            drawText(it.label, 28, 80, 0.75f, PLUM, 190, AL_LEFT, F_SERIF);
            drawText(it.blurb, 28, 108, 0.45f, INK, 186, AL_LEFT, F_ITALIC);
            // price tag on string
            line(40, 150, 56, 160, GOLD, 0.8f);
            tri(56, 160, 66, 152, 66, 168, IVORY, 0.5f); rect(66, 152, 70, 16, IVORY, 0.5f); frameRect(66, 152, 70, 16, GOLD, 0.6f, 0.51f);
            circ(62, 160, 1.5f, GOLD, 0.52f);
            string price = owns(it.id) && it.id != "redo" ? "owned" : std::to_string(it.cost) + " hearts";
            drawText(price, 101, 153, 0.4f, COCOA, 0, AL_CENTER, F_ITALIC);
            if (it.id == "redo") { char b[32]; snprintf(b, sizeof b, "You have %d", V("redo")); drawText(b, 28, 180, 0.4f, MAUVE, 0, AL_LEFT, F_ITALIC); }
        } else drawText("Come back soon, babe.", 28, 90, 0.6f, PLUM, 190, AL_LEFT, F_ITALIC);
        if (toastT > 0) { toastT--; drawText(toast, 28, 198, 0.42f, ROSE, 190, AL_LEFT, F_ITALIC); }
        drawPolaroid(244, 22, 144, 190, prev, "fitting room", 0, (gFrame % 200) < 6, BLUSH, "me");
        C2D_SceneBegin(gBot);
        rect(0, 0, 320, 240, IVORY, 0.1f);
        drawHUD(false);
        for (int i = 0; i < cnt; i++) {
            int k = first + i;
            string lab = k == (int)SHOP.size() ? "Leave the Boutique" : SHOP[k].label + (owns(SHOP[k].id) && SHOP[k].id != "redo" ? "  \xC2\xB7  owned" : "  \xC2\xB7  " + std::to_string(SHOP[k].cost));
            drawButton(16, 32 + i*30, 288, 26, lab, k == sel);
        }
        footer("\xE2\x80\xB9 prev", "next \xE2\x80\xBA");
        C3D_FrameEnd(0);
    }
}

// =============== COLLECTED MOMENTS (sticker book) ===============
static void runStickers() {
    vector<string> all;
    for (auto& l : gScript) if (l.cmd == "sticker" && std::find(all.begin(), all.end(), l.a) == all.end()) all.push_back(l.a);
    const int PER = 8; int page = 0, pages = std::max(1, ((int)all.size() + PER - 1) / PER);
    while (aptMainLoop()) {
        pollInput();
        if (kDown & (KEY_RIGHT | KEY_R) || tapIn(220, 190, 100, 50)) page = (page + 1) % pages;
        else if (kDown & (KEY_LEFT | KEY_L)) page = (page + pages - 1) % pages;
        else if (kDown & KEY_B || tapIn(0, 190, 100, 50)) return;
        int have = 0; for (auto& a : all) have += G.stickers.count(a);
        gFrame++;
        beginFrame();
        C2D_SceneBegin(gTop);
        rect(0, 0, 400, 240, mix(CREAM, BLUSH, 0.4f), 0.1f);
        for (int i = 0; i < 40; i++) circ((i * 47) % 400, (i * 29) % 240, 1, withA(GOLD, 0x40), 0.11f);
        drawText("Collected Moments", 200, 4, 0.8f, COCOA, 0, AL_CENTER, F_SERIF);
        goldRule(120, 32, 160);
        for (int i = 0; i < PER && page * PER + i < (int)all.size(); i++) {
            const string& id = all[page * PER + i];
            bool got = G.stickers.count(id);
            float x = 14 + (i % 4) * 96, y = 42 + (i / 4) * 98, w = 84, h = 92;
            float jitter = ((i * 7) % 5) - 2;
            rect(x + 2, y + 3 + jitter, w, h, SHADOW, 0.3f);
            rect(x, y + jitter, w, h, IVORY, 0.31f);
            u32 a = got ? mix(SUNSET_T, ROSE, (i % 3) * 0.2f) : withA(MAUVE, 0x30);
            C2D_DrawRectangle(x + 5, y + 5 + jitter, 0.32f, w - 10, h - 28, a, a, got ? SUNSET_B : a, got ? SUNSET_B : a);
            if (got) { circ(x + w / 2, y + 36 + jitter, 15, GOLD, 0.33f); circ(x + w / 2, y + 36 + jitter, 12, GOLD_LT, 0.34f);
                       heartSolid(x + w / 2, y + 36 + jitter, 6, ROSE, 0.35f); }
            else drawText("?", x + w / 2, y + 26 + jitter, 0.8f, withA(PLUM, 0x80), 0, AL_CENTER, F_ITALIC);
            drawText(got ? pretty(id) : "not yet...", x + w / 2, y + h - 21 + jitter, 0.32f, PLUM, w - 6, AL_CENTER, F_ITALIC);
            tape(x + w / 2 - 12, y - 4 + jitter, 24, 9);
        }
        C2D_SceneBegin(gBot);
        rect(0, 0, 320, 240, IVORY, 0.1f);
        drawPaper(20, 30, 280, 150, true, 0.2f);
        kicker("Scrapbook", 160, 44, GOLD, AL_CENTER);
        char b[48]; snprintf(b, sizeof b, "%d of %d moments", have, (int)all.size());
        drawText(b, 160, 66, 0.8f, COCOA, 0, AL_CENTER, F_SERIF);
        drawText("Finish a story with its best ending\nto paste a new polaroid here.", 160, 104, 0.42f, PLUM, 250, AL_CENTER, F_ITALIC);
        snprintf(b, sizeof b, "page %d of %d", page + 1, pages);
        drawText(b, 160, 152, 0.4f, MAUVE, 0, AL_CENTER, F_ITALIC);
        footer("\xE2\x80\xB9 close", "next page \xE2\x80\xBA");
        C3D_FrameEnd(0);
    }
}

// =============== THE DIARY (save / load) ===============
static void runCreator();
static void runDiary() {
    vector<string> o = {"Save to Diary ~ Write today's page to the SD card.", "Load Diary ~ Go back to your last saved page.",
                        "Restyle Yourself ~ Visit the Style File again.", "Close the Diary ~ Back to Story Land."};
    while (aptMainLoop()) {
        int sel = 0; string msg = ""; int msgT = 0;
        while (aptMainLoop()) {
            pollInput();
            if (kDown & KEY_DOWN) sel = (sel + 1) % 4;
            if (kDown & KEY_UP) sel = (sel + 3) % 4;
            if (kDown & KEY_B) return;
            int pick = -1;
            for (int i = 0; i < 4; i++) if (tapIn(16, 40 + i*40, 288, 34)) { if (sel == i) pick = i; sel = i; }
            if (kDown & KEY_A) pick = sel;
            if (pick == 0) { saveGame(); msg = "Saved. Your secrets are safe."; msgT = 120; }
            if (pick == 1) { if (loadGame()) { msg = "Diary loaded."; } else msg = "No diary found on the SD card."; msgT = 120; }
            if (pick == 2) { runCreator(); saveGame(); break; }
            if (pick == 3) return;
            gFrame++;
            beginFrame();
            C2D_SceneBegin(gTop);
            drawSunset(400, 240, PLUM);
            // leather diary cover with gold foil
            rect(36, 18, 332, 208, SHADOW, 0.3f);
            rect(30, 14, 332, 208, mix(PLUM, COCOA, 0.3f), 0.31f);
            frameRect(40, 24, 312, 188, GOLD, 1.0f, 0.32f);
            rect(30, 14, 14, 208, mix(PLUM, INK, 0.3f), 0.32f);
            drawText("Dear Diary", 200, 32, 0.95f, GOLD_LT, 0, AL_CENTER, F_ITALIC);
            goldRule(120, 66, 160);
            drawPolaroid(62, 80, 96, 124, G.me, G.myName, 0, (gFrame % 200) < 6, PLUM, "me");
            char b[64];
            drawText(G.myName, 178, 84, 0.7f, IVORY, 160, AL_LEFT, F_SERIF);
            snprintf(b, sizeof b, "%d hearts", V("hearts"));               drawText(b, 178, 114, 0.45f, GOLD_LT, 0, AL_LEFT, F_ITALIC);
            snprintf(b, sizeof b, "%d moments collected", (int)G.stickers.size()); drawText(b, 178, 132, 0.45f, GOLD_LT, 0, AL_LEFT, F_ITALIC);
            snprintf(b, sizeof b, "%d of 21 stories read", (int)G.done.size()); drawText(b, 178, 150, 0.45f, GOLD_LT, 0, AL_LEFT, F_ITALIC);
            drawText("last saved: " + (gSavedAt.empty() ? string("never") : gSavedAt), 178, 176, 0.38f, withA(IVORY, 0xB0), 0, AL_LEFT, F_ITALIC);
            if (msgT > 0) { msgT--; drawText(msg, 200, 200, 0.42f, IVORY, 0, AL_CENTER, F_ITALIC); }
            C2D_SceneBegin(gBot);
            rect(0, 0, 320, 240, IVORY, 0.1f);
            kicker("Save  \xC2\xB7  Load", 160, 14, GOLD, AL_CENTER);
            for (int i = 0; i < 4; i++) {
                string t = o[i].substr(0, o[i].find('~')), sub = trim(o[i].substr(o[i].find('~') + 1));
                drawButton(16, 40 + i*40, 288, 34, trim(t), i == sel);
                drawText(sub, 28, 40 + i*40 + 21, 0.3f, i == sel ? IVORY : MAUVE, 0, AL_LEFT, F_ITALIC);
            }
            C3D_FrameEnd(0);
        }
    }
}

// =============== WHO ARE YOU THIS TIME? (heroine picker) ===============
static const char* HERO_N[] = {"Mirror Selfie","On Set","Study Date","Night Out","Snow Day","Old Money","Kailyn"};
static const char* HERO_T[] = {"Main-character energy and a boyfriend in every pic.","Crossed arms, sharp eyes, zero patience for drama.",
                               "Sweet, curly, and secretly the smartest girl here.","Says exactly what everyone else is thinking.",
                               "Pink fur, soft waves, a little mysterious.","Pearls, slick pony, quiet luxury.",
                               "The main character. Phone in hand, heart on her sleeve."};
static void runHeroPick() {
    const int HN = 7, N = HN + 1; int sel = G.hero < 0 ? HN : G.hero;
    while (aptMainLoop()) {
        pollInput();
        if (kDown & KEY_DOWN) sel = (sel + 1) % N;
        if (kDown & KEY_UP) sel = (sel + N - 1) % N;
        if (kDown & KEY_RIGHT) sel = (sel + 1) % N;
        if (kDown & KEY_LEFT) sel = (sel + N - 1) % N;
        int pick = -1;
        for (int i = 0; i < N; i++) { float x = i == HN ? 16 : 16 + (i % 2) * 148, y = 30 + (i == HN ? (HN + 1) / 2 : i / 2) * 38; float w = i == HN ? 288 : 140;
            if (tapIn(x, y, w, 33)) { if (sel == i) pick = i; sel = i; } }
        if (kDown & KEY_A) pick = sel;
        if (pick >= 0) { G.hero = pick == HN ? -1 : pick; saveGame(); return; }
        gFrame++;
        int was = G.hero; G.hero = sel == HN ? -1 : sel;   // preview
        beginFrame();
        C2D_SceneBegin(gTop);
        drawSunset(400, 240, gBg);
        drawPaper(14, 18, 210, 206, false, 0.3f);
        kicker("Cast Your Lead", 28, 30, GOLD);
        drawText(gStoryTitle.empty() ? "Who are you?" : gStoryTitle, 28, 42, 0.7f, COCOA, 186, AL_LEFT, F_SERIF);
        goldRule(28, 72, 180);
        drawText(sel == HN ? "Your Style File Look" : HERO_N[sel], 28, 84, 0.72f, PLUM, 186, AL_LEFT, F_SERIF);
        drawText(sel == HN ? "The girl you designed, in whatever you're wearing." : HERO_T[sel], 28, 114, 0.46f, INK, 186, AL_LEFT, F_ITALIC);
        drawText("starring " + G.myName, 28, 196, 0.45f, MAUVE, 0, AL_LEFT, F_ITALIC);
        drawPolaroid(240, 16, 148, 196, G.me, sel == HN ? G.myName : HERO_N[sel], 0, false, gBg, "me");
        G.hero = was;
        C2D_SceneBegin(gBot);
        rect(0, 0, 320, 240, IVORY, 0.1f);
        kicker("Who Are You This Time?", 160, 10, GOLD, AL_CENTER);
        for (int i = 0; i < N; i++) {
            float x = i == HN ? 16 : 16 + (i % 2) * 148, y = 30 + (i == HN ? (HN + 1) / 2 : i / 2) * 38; float w = i == HN ? 288 : 140;
            drawButton(x, y, w, 33, i == HN ? "My Style File Look" : HERO_N[i], i == sel);
        }
        C3D_FrameEnd(0);
    }
}

// =============== SEASON FINALE (story endings) ===============
static void runEnding(bool good, const string& title, const string& text, const string& sticker) {
    int t = 0;
    while (aptMainLoop()) {
        pollInput(); t++;
        if (t > 30 && (kDown & KEY_A || touched)) return;
        gFrame++;
        beginFrame();
        C2D_SceneBegin(gTop);
        drawSunset(400, 240, good ? GOLD : MAUVE);
        drawPaper(20, 14, 360, 212, false, 0.3f);
        kicker(good ? "Season Finale" : "To Be Continued...", 200, 26, GOLD, AL_CENTER);
        drawText(gStoryTitle, 200, 40, 0.5f, MAUVE, 0, AL_CENTER, F_ITALIC);
        drawText(title, 200, 58, 0.95f, good ? PLUM : COCOA, 320, AL_CENTER, F_SERIF);
        goldRule(110, 104, 180);
        drawText(subst(text), 200, 116, 0.5f, INK, 300, AL_CENTER, F_ITALIC);
        if (good && !sticker.empty()) {
            float pop = std::min(1.0f, t / 30.0f);
            circ(200, 190, 20 * pop, GOLD, 0.6f); circ(200, 190, 16 * pop, GOLD_LT, 0.61f); heartSolid(200, 190, 7 * pop, ROSE, 0.62f);
            sparkle(228, 176, 5 * pop, GOLD); sparkle(172, 200, 4 * pop, GOLD);
        }
        C2D_SceneBegin(gBot);
        rect(0, 0, 320, 240, IVORY, 0.1f);
        drawHUD(false);
        drawPaper(20, 40, 280, 160, true, 0.2f);
        if (good && !sticker.empty()) {
            kicker("New Moment Unlocked", 160, 56, GOLD, AL_CENTER);
            drawText(pretty(sticker), 160, 76, 0.7f, PLUM, 0, AL_CENTER, F_SERIF);
            drawText("+5 hearts", 160, 108, 0.5f, ROSE, 0, AL_CENTER, F_ITALIC);
        } else {
            kicker("Next Time On Choices", 160, 56, GOLD, AL_CENTER);
            drawText("Different choices,\ndifferent ending.", 160, 80, 0.55f, PLUM, 0, AL_CENTER, F_ITALIC);
        }
        if ((gFrame / 24) % 2) drawText("tap to return to Story Land", 160, 176, 0.4f, MAUVE, 0, AL_CENTER, F_ITALIC);
        C3D_FrameEnd(0);
    }
}

// =============== MINI-GAMES (restyled, same rules) ===============
static void runClaw() {
    float x = 0, dir = 2.4f; int tries = 3, won = 0;
    while (aptMainLoop() && tries > 0) {
        pollInput();
        x += dir; if (x < 0 || x > 260) dir = -dir;
        if (kDown & KEY_A || touched) { tries--; if (fabsf(x - 130) < 14) { won++; playClip("yay"); } }
        gFrame++;
        beginFrame();
        C2D_SceneBegin(gTop);
        drawSunset(400, 240, MAUVE, false);
        kicker("Maple Valley Mall  \xC2\xB7  Arcade", 200, 8, COCOA, AL_CENTER);
        rect(66, 34, 268, 190, withA(IVORY, 0x70), 0.3f); frameRect(66, 34, 268, 190, GOLD, 2, 0.31f);
        circ(70 + 140, 196, 16, IVORY, 0.4f); ell(70 + 132, 166, 6, 20, IVORY, 0.4f); ell(70 + 142, 166, 6, 20, IVORY, 0.4f);
        circ(70 + 135, 194, 1.5f, INK, 0.41f); circ(70 + 145, 194, 1.5f, INK, 0.41f);
        rect(70 + x + 6, 34, 2, 60, GOLD, 0.5f); rect(70 + x - 4, 94, 22, 4, GOLD, 0.5f);
        line(70 + x - 4, 98, 70 + x - 8, 110, GOLD, 2); line(70 + x + 18, 98, 70 + x + 22, 110, GOLD, 2);
        C2D_SceneBegin(gBot);
        rect(0, 0, 320, 240, IVORY, 0.1f); drawHUD(false);
        char b[60]; snprintf(b, sizeof b, "tries left %d   \xC2\xB7   won %d", tries, won);
        drawText("The Claw", 160, 60, 0.9f, COCOA, 0, AL_CENTER, F_SERIF);
        drawText(b, 160, 100, 0.5f, PLUM, 0, AL_CENTER, F_ITALIC);
        drawText("tap or press A to drop", 160, 140, 0.45f, MAUVE, 0, AL_CENTER, F_ITALIC);
        C3D_FrameEnd(0);
    }
    V("hearts") += won * 2; V("happy") = std::min(5, V("happy") + won);
}
static void runFindCat() {
    float cx = 40 + rand() % 240, cy = 40 + rand() % 160; int taps = 0; bool found = false; float dist = 999;
    while (aptMainLoop() && !found) {
        pollInput();
        if (touched) { taps++; dist = hypotf(touch.px - cx, touch.py - cy); if (fabsf(touch.px - cx) < 22 && fabsf(touch.py - cy) < 22) found = true; }
        gFrame++;
        beginFrame();
        C2D_SceneBegin(gTop);
        drawSunset(400, 240, gBg);
        drawPaper(60, 40, 280, 150, false, 0.3f);
        drawText("Where is Mr. Whiskers?", 200, 56, 0.75f, COCOA, 0, AL_CENTER, F_SERIF);
        drawText(dist < 60 ? "So warm..." : dist < 120 ? "Getting warmer" : "Cold", 200, 110, 0.8f, PLUM, 0, AL_CENTER, F_ITALIC);
        C2D_SceneBegin(gBot);
        vgrad(0, 0, 320, 240, CREAM, BLUSH);
        rect(18, 138, 128, 64, ROSE, 0.2f); rect(18, 130, 128, 14, mix(ROSE, IVORY, 0.2f), 0.21f);    // tufted sofa
        rect(188, 56, 104, 146, mix(COCOA, CREAM, 0.3f), 0.2f); for (int i = 0; i < 3; i++) rect(188, 90 + i * 36, 104, 3, COCOA, 0.21f);
        circ(90, 50, 28, mix(MAUVE, IVORY, 0.4f), 0.2f); rect(84, 76, 12, 30, COCOA, 0.2f);
        C3D_FrameEnd(0);
    }
    playClip("meow");
    V("hearts") += taps < 6 ? 3 : 1;
}

// =============== SCRIPT RUNNER ===============
static void runScript(const string& start) {
    int pc = gLabels.count(start) ? gLabels[start] : 0;
    while (aptMainLoop() && pc < (int)gScript.size()) {
        Line L = gScript[pc++];
        if (L.cmd == "bg")           gBg = hexColor(L.a);
        else if (L.cmd == "say")     { if (!L.c.empty()) playClip(L.c); runSay(L.a, L.b); }
        else if (L.cmd == "text")    { playClip("ring"); runSay(L.a, L.b, true); }
        else if (L.cmd == "set")     applyEffects(L.a);
        else if (L.cmd == "goto")    pc = gLabels[L.a];
        else if (L.cmd == "if")      { if (evalCond(L.a)) pc = gLabels[L.b]; }
        else if (L.cmd == "title")   { gStoryTitle = L.a; gStoryKey = L.b; }
        else if (L.cmd == "bond")    { gBondVar = L.a; gBondLabel = L.b; }
        else if (L.cmd == "sticker") { G.stickers.insert(L.a); saveGame(); }
        else if (L.cmd == "ending")  {   // ending good|Title|text   (sticker: next line, if any)
            bool good = L.a == "good"; string stk = "";
            if (pc < (int)gScript.size() && gScript[pc].cmd == "sticker") stk = gScript[pc].a;
            if (!gStoryKey.empty()) G.done.insert(gStoryKey);
            runEnding(good, L.b, L.c, stk);
        }
        else if (L.cmd == "shop")    runShop();
        else if (L.cmd == "claw")    runClaw();
        else if (L.cmd == "findcat") runFindCat();
        else if (L.cmd == "save")    saveGame();
        else if (L.cmd == "diary")   runDiary();
        else if (L.cmd == "pickhero") runHeroPick();
        else if (L.cmd == "reset")   { int h = V("hearts"), r = V("redo"); G.vars.clear();
                                       V("hearts") = h; V("redo") = r; V("happy") = 3; V("tired") = 1; V("hungry") = 2;
                                       gStoryTitle = ""; gStoryKey = ""; gBondVar = ""; gBondLabel = ""; }
        else if (L.cmd == "stickers") runStickers();
        else if (L.cmd == "end")     { saveGame(); return; }
        else if (L.cmd == "rate") {
            int score = V("happy") * 2 + V("trust") - V("tired") - V("hungry") - V("chaos");
            int stars = score >= 10 ? 5 : score >= 7 ? 4 : score >= 4 ? 3 : score >= 1 ? 2 : 1;
            V("stars") += stars; V("hearts") += stars;
            runSay(L.a, "My review this week: " + std::to_string(stars) + " out of 5 stars.\nYou earned " + std::to_string(stars) + " hearts!");
        }
        else if (L.cmd == "choice") {
            int choiceStart = pc - 1;
            vector<Line> opts;
            while (pc < (int)gScript.size() && gScript[pc].cmd == ">") opts.push_back(gScript[pc++]);
            vector<string> labels, fx, tg; for (auto& o : opts) { labels.push_back(o.a); fx.push_back(o.b); tg.push_back(o.c); }
            int i = runChoice(L.a, labels, {}, fx, tg);
            applyEffects(opts[i].b);
            if (opts[i].b.find("chaos") != string::npos) V("_undo_pc") = choiceStart;
            if (!opts[i].c.empty()) pc = gLabels[opts[i].c];
        }
        else if (L.cmd == "undo") {
            bool canToken = V("redo") > 0, canHearts = V("hearts") >= 3;
            if (!canToken && !canHearts) continue;
            int i = runChoice("Rewind that moment?", { canToken ? "Use a Second Chance" : "Spend 3 hearts to rewind", "No regrets. Keep it." });
            if (i == 0) { if (canToken) V("redo")--; else V("hearts") -= 3; V("chaos")--; pc = V("_undo_pc"); }
        }
    }
}

// =============== TITLE: magazine cover ===============
static int runTitle(bool hasSave) {
    vector<string> o = {"Begin a New Story", hasSave ? "Continue" : "Continue (no diary yet)", "Collected Moments", "Close the Magazine"};
    int sel = hasSave ? 1 : 0;
    while (aptMainLoop()) {
        pollInput();
        if (kDown & KEY_DOWN) sel = (sel + 1) % 4;
        if (kDown & KEY_UP) sel = (sel + 3) % 4;
        int pick = -1;
        for (int i = 0; i < 4; i++) if (tapIn(30, 88 + i*34, 260, 28)) { if (sel == i) pick = i; sel = i; }
        if (kDown & (KEY_A | KEY_START)) pick = sel;
        if (pick == 2) { runStickers(); continue; }
        if (pick >= 0) return pick;
        gFrame++;
        beginFrame();
        C2D_SceneBegin(gTop);
        drawSunset(400, 240, ROSE, false);
        // cover girl
        Look cover = G.me; cover.outfit = 2; cover.shirt = OUTFIT_C[2];
        if (!drawArt("cover", false, 196, 0, 204, 214, 0.3f)) drawPortrait(282, 120, 1.55f, cover, 0, (gFrame % 200) < 6);
        // masthead
        drawText("CHOICES", 16, 2, 1.85f, COCOA, 0, AL_LEFT, F_SERIF);
        rect(18, 60, 236, 1.2f, GOLD, 0.8f); rect(18, 63, 236, 0.5f, GOLD, 0.8f);
        kicker("The Girls' Guide to Twenty Lives", 20, 68, PLUM);
        // cover lines
        drawText("PROM QUEEN", 20, 92, 0.5f, PLUM, 0, AL_LEFT, F_SERIF);
        drawText("secrets she'll never tell", 20, 106, 0.4f, INK, 0, AL_LEFT, F_ITALIC);
        drawText("Fall's Style File", 20, 128, 0.5f, PLUM, 0, AL_LEFT, F_SERIF);
        drawText("velour, pearls & gold hoops", 20, 142, 0.4f, INK, 0, AL_LEFT, F_ITALIC);
        drawText("20 Lives. One Summer.", 20, 164, 0.6f, ROSE, 0, AL_LEFT, F_ITALIC);
        // issue strip + barcode
        rect(0, 214, 400, 26, withA(IVORY, 0xD8), 0.8f); rect(0, 214, 400, 0.8f, GOLD, 0.81f);
        drawText("No. 04  \xC2\xB7  Fall 2004  \xC2\xB7  $3.99", 12, 219, 0.4f, COCOA, 0, AL_LEFT, F_SERIF);
        for (int i = 0; i < 26; i++) rect(340 + i * 2, 217, (i % 3 == 0) ? 1.4f : 0.7f, 18, INK, 0.82f);
        sparkle(372, 40, 7, GOLD_LT); sparkle(230, 186, 4, GOLD_LT);
        C2D_SceneBegin(gBot);
        rect(0, 0, 320, 240, IVORY, 0.1f);
        for (float ly = 30; ly < 240; ly += 15) rect(0, ly, 320, 0.5f, withA(MAUVE, 0x18), 0.11f);
        drawText("a teen drama in twenty stories", 160, 20, 0.5f, PLUM, 0, AL_CENTER, F_ITALIC);
        goldRule(80, 52, 160);
        for (int i = 0; i < 4; i++) drawButton(30, 88 + i*34, 260, 28, o[i], i == sel);
        heartLine(160, 228, 5, withA(ROSE, 0xB0));
        C3D_FrameEnd(0);
    }
    return 3;
}

int main() {
    romfsInit();
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS * 2);
    C2D_Prepare();
    gTop = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    gBot = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
    gBuf = C2D_TextBufNew(8192);
    srand(osGetTime());
    fontsInit();
    audioInit();
    loadScript();

    G.me.skin = SKINS[2]; G.me.hair = HAIRS[1]; G.me.clip = GOLD; G.me.glasses = false; G.me.style = 0; wearOutfit(0);
    bool hasSave = loadGame();

    int t = runTitle(hasSave);
    if (t == 0 || (t == 1 && !hasSave)) {
        int keepHearts = V("hearts"), keepRedo = V("redo");
        G.vars.clear(); V("hearts") = keepHearts; V("redo") = keepRedo; V("happy") = 3; V("tired") = 1; V("hungry") = 2;
        runCreator();
        saveGame();
        runScript("storyland");
    } else if (t == 1) {
        V("happy") = 3; V("tired") = 1; V("hungry") = 2;
        runScript("storyland");
    }

    saveGame();
    C2D_TextBufDelete(gBuf);
    for (int i = 1; i < 3; i++) if (gFonts[i]) C2D_FontFree(gFonts[i]);
    if (gAudio) ndspExit();
    C2D_Fini(); C3D_Fini(); gfxExit(); romfsExit();
    return 0;
}
