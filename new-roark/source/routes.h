#ifndef ROUTES_H
#define ROUTES_H
typedef struct { const char *id; const char *portrait; const char *chapter; const char *blurb; } Route;
extern const Route ROUTES[];
extern const int ROUTE_COUNT;
#endif
