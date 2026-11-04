// story.c - script engine. See STORY_FORMAT.md for the script language.
#include "story.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static char *trim(char *p) {
    while (*p && isspace((unsigned char)*p)) p++;
    char *e = p + strlen(p);
    while (e > p && isspace((unsigned char)e[-1])) *--e = 0;
    return p;
}

static void copyn(char *dst, const char *src, size_t n) {
    strncpy(dst, src, n - 1);
    dst[n - 1] = 0;
}

static int starts_word(const char *line, const char *w) {
    size_t n = strlen(w);
    return strncmp(line, w, n) == 0 && (line[n] == 0 || line[n] == ' ' || line[n] == '\t');
}

int story_find_label(const Story *s, const char *name) {
    for (int i = 0; i < s->nlabels; i++)
        if (strcmp(s->labelName[i], name) == 0) return s->labelLine[i];
    return -1;
}

int story_get(const Story *s, const char *var) {
    for (int i = 0; i < s->nvars; i++)
        if (strcmp(s->varName[i], var) == 0) return s->varVal[i];
    return 0;
}

void story_set(Story *s, const char *var, int val) {
    for (int i = 0; i < s->nvars; i++)
        if (strcmp(s->varName[i], var) == 0) { s->varVal[i] = val; return; }
    if (s->nvars < ST_MAX_VARS) {
        copyn(s->varName[s->nvars], var, sizeof s->varName[0]);
        s->varVal[s->nvars++] = val;
    }
}

void story_free(Story *s) {
    free(s->buf);
    memset(s, 0, sizeof *s);
}

int story_load(Story *s, const char *src) {
    memset(s, 0, sizeof *s);
    size_t n = strlen(src);
    s->buf = malloc(n + 1);
    if (!s->buf) { strcpy(s->err, "out of memory"); return 0; }
    memcpy(s->buf, src, n + 1);
    char *p = s->buf;
    while (p && *p) {
        char *nl = strchr(p, '\n');
        if (nl) *nl = 0;
        char *t = trim(p);
        if (*t && *t != '#') {
            if (s->nlines >= ST_MAX_LINES) { strcpy(s->err, "script too long"); return 0; }
            if (starts_word(t, "label") && s->nlabels < ST_MAX_LABELS) {
                copyn(s->labelName[s->nlabels], trim(t + 5), sizeof s->labelName[0]);
                s->labelLine[s->nlabels++] = s->nlines;
            }
            s->lines[s->nlines++] = t;
        }
        p = nl ? nl + 1 : NULL;
    }
    return 1;
}

// "trust +1", "trust -2", "trust 3"
static void apply_set(Story *s, char *arg) {
    char name[24]; char val[16] = "";
    if (sscanf(arg, "%23s %15s", name, val) < 1) return;
    if (val[0] == '+' || val[0] == '-') story_set(s, name, story_get(s, name) + atoi(val));
    else story_set(s, name, atoi(val));
}

static int eval_if(Story *s, const char *arg, char *target, size_t tn) {
    char var[24], op[4], kw[8], lab[48]; int v;
    if (sscanf(arg, "%23s %3s %d %7s %47s", var, op, &v, kw, lab) != 5 || strcmp(kw, "goto")) {
        snprintf(s->err, sizeof s->err, "bad if: %.120s", arg);
        return -1;
    }
    copyn(target, lab, tn);
    int x = story_get(s, var);
    if (!strcmp(op, ">=")) return x >= v;
    if (!strcmp(op, "<=")) return x <= v;
    if (!strcmp(op, ">"))  return x > v;
    if (!strcmp(op, "<"))  return x < v;
    if (!strcmp(op, "==")) return x == v;
    if (!strcmp(op, "!=")) return x != v;
    snprintf(s->err, sizeof s->err, "bad if operator: %s", op);
    return -1;
}

static int jump(Story *s, const char *label) {
    int l = story_find_label(s, label);
    if (l < 0) { snprintf(s->err, sizeof s->err, "missing label: %s", label); return 0; }
    s->pc = l + 1;
    return 1;
}

// "> [3] Text here | label | trust +1"
static void parse_opt(ChoiceOpt *o, char *line) {
    memset(o, 0, sizeof *o);
    char *t = trim(line + 1);
    if (*t == '[') { o->cost = atoi(t + 1); char *e = strchr(t, ']'); if (e) t = trim(e + 1); }
    char *f[3] = { t, NULL, NULL };
    for (int i = 1; i < 3; i++) {
        char *bar = f[i - 1] ? strchr(f[i - 1], '|') : NULL;
        if (bar) { *bar = 0; f[i] = bar + 1; }
    }
    copyn(o->text, trim(f[0]), sizeof o->text);
    if (f[1]) copyn(o->label, trim(f[1]), sizeof o->label);
    if (f[2]) {
        char var[24], val[16] = "+1";
        if (sscanf(trim(f[2]), "%23s %15s", var, val) >= 1) {
            copyn(o->var, var, sizeof o->var);
            o->delta = atoi(val);
        }
    }
}

EvType story_next(Story *s) {
    s->err[0] = 0;
    int guard = 0;
    while (s->pc < s->nlines) {
        if (++guard > 20000) { strcpy(s->err, "infinite loop?"); return EV_ERROR; }
        int here = s->pc;
        char *line = s->lines[s->pc++];
        // the buffer lines are trimmed already; we may cut them, so work on a copy
        char tmp[600];
        copyn(tmp, line, sizeof tmp);
        char *arg;

        if (starts_word(tmp, "label")) continue;
        if (starts_word(tmp, "title")) { copyn(s->title, trim(tmp + 5), sizeof s->title); continue; }
        if (starts_word(tmp, "bg")) {
            arg = trim(tmp + 2);
            char key[32] = ""; sscanf(arg, "%31s", key);
            copyn(s->bg, key, sizeof s->bg);
            char *rest = trim(arg + strlen(key));
            copyn(s->place, rest, sizeof s->place);
            continue;
        }
        if (starts_word(tmp, "show")) {
            char id[24] = "", pos[16] = "center";
            sscanf(trim(tmp + 4), "%23s %15s", id, pos);
            int sl = !strcmp(pos, "left") ? 0 : !strcmp(pos, "right") ? 2 : 1;
            for (int i = 0; i < ST_SLOTS; i++) if (!strcmp(s->slot[i], id)) s->slot[i][0] = 0;
            copyn(s->slot[sl], id, sizeof s->slot[0]);
            continue;
        }
        if (starts_word(tmp, "hide")) {
            arg = trim(tmp + 4);
            for (int i = 0; i < ST_SLOTS; i++)
                if (!strcmp(arg, "all") || !strcmp(s->slot[i], arg)) s->slot[i][0] = 0;
            continue;
        }
        if (starts_word(tmp, "set")) { apply_set(s, trim(tmp + 3)); continue; }
        if (starts_word(tmp, "reward")) { s->stars += atoi(trim(tmp + 6)); continue; }
        if (starts_word(tmp, "goto")) { if (!jump(s, trim(tmp + 4))) return EV_ERROR; continue; }
        if (starts_word(tmp, "if")) {
            char target[48];
            int r = eval_if(s, trim(tmp + 2), target, sizeof target);
            if (r < 0) return EV_ERROR;
            if (r && !jump(s, target)) return EV_ERROR;
            continue;
        }
        if (starts_word(tmp, "end")) { s->sayPc = here; s->pc = s->nlines; return EV_END; }
        if (starts_word(tmp, "post")) {
            // post <user> <image> <caption>
            char user[24] = "", img[40] = "";
            arg = trim(tmp + 4);
            if (sscanf(arg, "%23s %39s", user, img) != 2) { snprintf(s->err, sizeof s->err, "bad post: %.120s", arg); return EV_ERROR; }
            char *cap = strstr(arg, img) + strlen(img);
            s->sayPc = here;
            copyn(s->speaker, user, sizeof s->speaker);
            copyn(s->postImg, img, sizeof s->postImg);
            copyn(s->text, trim(cap), sizeof s->text);
            return EV_POST;
        }
        if (starts_word(tmp, "choice")) {
            s->sayPc = here;
            s->nopts = 0;
            while (s->pc < s->nlines) {
                char *l = s->lines[s->pc++];
                if (starts_word(l, "endchoice")) break;
                if (l[0] == '>' && s->nopts < ST_MAX_OPTS) {
                    char o[600]; copyn(o, l, sizeof o);
                    parse_opt(&s->opts[s->nopts++], o);
                }
            }
            if (s->nopts == 0) { strcpy(s->err, "choice with no options"); return EV_ERROR; }
            return EV_CHOICE;
        }
        // dialogue: "id: text"  (id has no spaces)   otherwise narration
        s->sayPc = here;
        char *colon = strchr(tmp, ':');
        if (colon && colon - tmp < 24 && !memchr(tmp, ' ', colon - tmp)) {
            *colon = 0;
            copyn(s->speaker, tmp, sizeof s->speaker);
            copyn(s->text, trim(colon + 1), sizeof s->text);
        } else {
            strcpy(s->speaker, "narr");
            copyn(s->text, tmp, sizeof s->text);
        }
        return EV_SAY;
    }
    return EV_END;
}

int story_choose(Story *s, int idx) {
    if (idx < 0 || idx >= s->nopts) return 0;
    ChoiceOpt *o = &s->opts[idx];
    if (o->var[0]) story_set(s, o->var, story_get(s, o->var) + o->delta);
    if (o->label[0]) return jump(s, o->label);
    return 1;
}
