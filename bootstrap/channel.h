#ifndef GUAGE_CHANNEL_H
#define GUAGE_CHANNEL_H

#include <stdbool.h>
#include "cell.h"

#define MAX_CHANNELS 256
#define DEFAULT_CHANNEL_CAPACITY 64

/* Channel — bounded ring buffer for inter-actor communication */
typedef struct Channel {
    int id;
    int capacity;
    bool closed;
    Cell** buffer;      /* malloc'd ring buffer */
    int read_pos;
    int write_pos;
    int count;
} Channel;

/* Lifecycle */
Channel* channel_create(int capacity);
void     channel_close(Channel* chan);
void     channel_destroy(Channel* chan);

/* Non-blocking buffer operations (return success/failure) */
bool  channel_try_send(Channel* chan, Cell* value);
Cell* channel_try_recv(Channel* chan);  /* NULL if empty */

/* Registry */
Channel* channel_lookup(int id);
void     channel_reset_all(void);

#endif /* GUAGE_CHANNEL_H */
