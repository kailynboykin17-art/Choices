// story.h - tiny script engine for New Roark (no 3DS code in here, so it can be tested on a PC)
#ifndef STORY_H
#define STORY_H

#define ST_MAX_LINES  3000
#define ST_MAX_VARS   32
#define ST_MAX_OPTS   4
#define ST_MAX_LABELS 128
#define ST_SLOTS      3   // 0 = left, 1 = center, 2 = right

typedef enum { EV_NONE, EV_SAY, EV_CHOICE, EV_END, EV_ERROR, EV_POST } EvType;

typedef struct {
    char text[160];   // what the button says
    char label[48];   // where to jump ("" = keep going)
    char var[24];     // stat to change ("" = none)
    int  delta;       // how much to change it
    int  cost;        // stars needed (0 = free)
} ChoiceOpt;

typedef struct {
    // script
    char *buf;
    char *lines[ST_MAX_LINES];
    int   nlines;
    int   pc;          // next line to run
    int   sayPc;       // line of the current SAY/CHOICE (for saving)
    char  labelName[ST_MAX_LABELS][48];
    int   labelLine[ST_MAX_LABELS];
    int   nlabels;
    // stats
    char  varName[ST_MAX_VARS][24];
    int   varVal[ST_MAX_VARS];
    int   nvars;
    // scene
    char  title[96];
    char  bg[32];
    char  place[64];
    char  slot[ST_SLOTS][24];
    // current event
    char  speaker[24];  // cast id, "narr" for narration
    char  text[512];
    char  postImg[40];  // EV_POST: picture id (gfx/<id>.png); speaker = who posted, text = caption
    ChoiceOpt opts[ST_MAX_OPTS];
    int   nopts;
    int   stars;        // stars earned by "reward" lines since last check
    char  err[160];
} Story;

int    story_load(Story *s, const char *src);   // 1 = ok
void   story_free(Story *s);
EvType story_next(Story *s);
int    story_choose(Story *s, int idx);           // 1 = ok
int    story_get(const Story *s, const char *var);
void   story_set(Story *s, const char *var, int val);
int    story_find_label(const Story *s, const char *name); // -1 if missing

// cast table (cast.c)
typedef struct { const char *id; const char *name; unsigned int color; } CastMember;
const CastMember *cast_find(const char *id);
extern const CastMember CAST[];
extern const int CAST_COUNT;

#endif
