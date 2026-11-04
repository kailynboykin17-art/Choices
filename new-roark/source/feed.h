#ifndef FEED_H
#define FEED_H
typedef struct {
    const char *user, *img, *caption, *likes;
    const char *c1user, *c1, *c2user, *c2;   // two comments
} FeedPost;
extern const FeedPost FEED[];
extern const int FEED_COUNT;
const char *feed_handle(const char *id);
#endif
