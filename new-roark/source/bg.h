#ifndef BG_H
#define BG_H
typedef struct { const char *key; unsigned int top, bottom, accent; } BgStyle;
extern const BgStyle BGS[];
extern const int BG_COUNT;
const BgStyle *bg_find(const char *key);
#endif
