// feed.c - Roarstagram, the in-game Instagram. Newest post first.
// To add a post: put a 192x240 picture at gfx/<img>.png (+ a .t3s like the others) and add a line here.
// In a story, show a post with:   post <who> <img> <caption>
#include "feed.h"
#include <string.h>

const FeedPost FEED[] = {
    {"candy", "post_candy4", "night out w my fav person. dont call us",
     "2,481", "simon", "best night ever fr", "rebecca", "cute"},
    {"candy", "post_candy3", "sunday walk. he carried my bag the whole time (he complained the whole time)",
     "1,907", "molly", "THE BAG LORE", "wendy", "ok this is cute i'll allow it"},
    {"candy", "post_candy2", "home is a person",
     "3,118", "suzy", "STOPPP", "simon", "always"},
    {"candy", "post_candy1", "him >>>",
     "2,760", "peppa", "who let yall be this cute", "gerald", "certified couple of the year"},
};
const int FEED_COUNT = sizeof FEED / sizeof FEED[0];

static const struct { const char *id; const char *handle; } HANDLES[] = {
    {"candy", "@candycat.xo"}, {"simon", "@simon.iora"}, {"rebecca", "@becca.bunny"}, {"molly", "@mollyreads"},
    {"wendy", "@wendy.wlf"}, {"suzy", "@suzysheep"}, {"peppa", "@peppa.pig"}, {"gerald", "@gerald.k"},
    {"danny", "@dnny"}, {"pedro", "@pedropony"}, {"emily", "@em.elephant"}, {"karter", "@karter.k"},
    {"freddy", "@freddy.fox"}, {"kylie", "@kyliekangaroo"}, {"peggi", "@peggi.panda"}, {"george", "@george.biz"},
};

const char *feed_handle(const char *id) {
    for (unsigned i = 0; i < sizeof HANDLES / sizeof HANDLES[0]; i++)
        if (!strcmp(HANDLES[i].id, id)) return HANDLES[i].handle;
    return id;
}
