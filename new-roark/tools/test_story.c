// test_story.c - checks every story file on your computer before you build for 3DS.
// Explores EVERY choice path, and checks speakers, portraits, backgrounds and labels.
// Build & run:  make test     (or: cc -I source tools/test_story.c source/story.c source/cast.c source/routes.c source/bg.c -o test_story && ./test_story)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "story.h"
#include "routes.h"
#include "bg.h"

static int errors, paths, saysTotal;
static const char *routeId;
static unsigned char seen[ST_MAX_LINES];  // which lines some path reached

static int file_exists(const char *p) { struct stat st; return stat(p, &st) == 0; }

static void check_scene(Story *s) {
    char p[128];
    if (s->bg[0] && !bg_find(s->bg)) { printf("  [%s] unknown bg '%s' (line %d)\n", routeId, s->bg, s->sayPc); errors++; s->bg[0] = 0; }
    for (int i = 0; i < ST_SLOTS; i++) {
        if (!s->slot[i][0]) continue;
        snprintf(p, sizeof p, "gfx/%s.png", s->slot[i]);
        if (!cast_find(s->slot[i]) || !file_exists(p)) {
            printf("  [%s] 'show %s' has no portrait/cast entry\n", routeId, s->slot[i]); errors++; s->slot[i][0] = 0;
        }
    }
}

static void explore(Story s, int depth, int minStars) {
    for (int steps = 0; steps < 5000; steps++) {
        EvType e = story_next(&s);
        check_scene(&s);
        if (e != EV_ERROR && s.sayPc >= 0 && s.sayPc < ST_MAX_LINES) seen[s.sayPc] = 1;
        if (e == EV_ERROR) { printf("  [%s] ERROR: %s\n", routeId, s.err); errors++; return; }
        if (e == EV_END) {
            paths++;
            if (s.stars < minStars) { printf("  [%s] a path gives only %d stars\n", routeId, s.stars); errors++; }
            return;
        }
        if (e == EV_SAY) {
            saysTotal++;
            if (!cast_find(s.speaker)) { printf("  [%s] unknown speaker '%s' (line: %s)\n", routeId, s.speaker, s.lines[s.sayPc]); errors++; }
            if (strlen(s.text) > 260) { printf("  [%s] line too long for the text box (%zu chars): %.40s...\n", routeId, strlen(s.text), s.text); errors++; }
        }
        if (e == EV_POST) {
            char pp[96]; snprintf(pp, sizeof pp, "gfx/%s.png", s.postImg);
            if (strcmp(s.postImg, "none") && !file_exists(pp)) { printf("  [%s] post picture missing: %s\n", routeId, pp); errors++; }
            if (!cast_find(s.speaker)) { printf("  [%s] post by unknown '%s'\n", routeId, s.speaker); errors++; }
        }
        if (e == EV_CHOICE) {
            for (int i = 0; i < s.nopts; i++) {
                if (strlen(s.opts[i].text) > 64) { printf("  [%s] choice text too long: %s\n", routeId, s.opts[i].text); errors++; }
                if (s.opts[i].label[0] && story_find_label(&s, s.opts[i].label) < 0) { printf("  [%s] choice goes to missing label '%s'\n", routeId, s.opts[i].label); errors++; }
            }
            if (depth > 30) { printf("  [%s] too many nested choices\n", routeId); errors++; return; }
            for (int i = 0; i < s.nopts; i++) {
                Story c = s;
                story_choose(&c, i);
                explore(c, depth + 1, minStars);
            }
            return;
        }
    }
    printf("  [%s] story never ends (loop?)\n", routeId); errors++;
}

static char *read_file(const char *path) {
    FILE *f = fopen(path, "rb"); if (!f) return NULL;
    fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET);
    char *b = malloc(n + 1); size_t got = fread(b, 1, n, f); b[got] = 0; fclose(f); return b;
}

int main(void) {
    for (int r = 0; r < ROUTE_COUNT; r++) {
        routeId = ROUTES[r].id;
        char path[128]; snprintf(path, sizeof path, "romfs/story/%s.txt", routeId);
        char *src = read_file(path);
        if (!src) { printf("[%s] missing %s\n", routeId, path); errors++; continue; }
        char pp[128]; snprintf(pp, sizeof pp, "gfx/%s.png", ROUTES[r].portrait);
        if (!file_exists(pp)) { printf("[%s] missing route portrait %s\n", routeId, pp); errors++; }
        static Story s;
        if (!story_load(&s, src)) { printf("[%s] load failed: %s\n", routeId, s.err); errors++; continue; }
        // labels defined twice?
        for (int i = 0; i < s.nlabels; i++) for (int j = i + 1; j < s.nlabels; j++)
            if (!strcmp(s.labelName[i], s.labelName[j])) { printf("  [%s] label '%s' defined twice\n", routeId, s.labelName[i]); errors++; }
        memset(seen, 0, sizeof seen);
        int before = paths;
        saysTotal = 0;
        explore(s, 0, 5);
        for (int i = 0; i < s.nlines; i++) {
            const char *l = s.lines[i];
            if (seen[i] || l[0] == '>' || !strncmp(l, "label", 5) || !strncmp(l, "goto", 4) || !strncmp(l, "if ", 3) ||
                !strncmp(l, "set ", 4) || !strncmp(l, "bg ", 3) || !strncmp(l, "show ", 5) || !strncmp(l, "hide", 4) ||
                !strncmp(l, "title", 5) || !strncmp(l, "reward", 6) || !strncmp(l, "endchoice", 9)) continue;
            printf("  [%s] no path ever reaches: %.70s\n", routeId, l); errors++;
        }
        printf("[%s] ok-ish: %d paths, %d labels, %d lines total\n", routeId, paths - before, s.nlabels, s.nlines);
        story_free(&s);
        free(src);
    }
    printf("\n%d routes, %d paths checked, %d problem(s)\n", ROUTE_COUNT, paths, errors);
    return errors ? 1 : 0;
}
