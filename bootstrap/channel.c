#include "channel.h"
#include <stdlib.h>
#include <string.h>

/* Global channel registry */
static Channel* g_channels[MAX_CHANNELS];
static int g_channel_count = 0;
static int g_next_channel_id = 1;

Channel* channel_create(int capacity) {
    if (g_channel_count >= MAX_CHANNELS) return NULL;
    if (capacity <= 0) capacity = DEFAULT_CHANNEL_CAPACITY;

    Channel* chan = (Channel*)calloc(1, sizeof(Channel));
    chan->id = g_next_channel_id++;
    chan->capacity = capacity;
    chan->closed = false;
    chan->buffer = (Cell**)calloc(capacity, sizeof(Cell*));
    chan->read_pos = 0;
    chan->write_pos = 0;
    chan->count = 0;

    g_channels[g_channel_count++] = chan;
    return chan;
}

void channel_close(Channel* chan) {
    if (!chan) return;
    chan->closed = true;
}

void channel_destroy(Channel* chan) {
    if (!chan) return;

    /* Release buffered values */
    for (int i = 0; i < chan->count; i++) {
        int pos = (chan->read_pos + i) % chan->capacity;
        if (chan->buffer[pos]) {
            cell_release(chan->buffer[pos]);
            chan->buffer[pos] = NULL;
        }
    }

    free(chan->buffer);
    free(chan);
}

bool channel_try_send(Channel* chan, Cell* value) {
    if (!chan || chan->closed) return false;
    if (chan->count >= chan->capacity) return false;

    cell_retain(value);
    chan->buffer[chan->write_pos % chan->capacity] = value;
    chan->write_pos++;
    chan->count++;
    return true;
}

Cell* channel_try_recv(Channel* chan) {
    if (!chan) return NULL;
    if (chan->count == 0) return NULL;

    Cell* value = chan->buffer[chan->read_pos % chan->capacity];
    chan->buffer[chan->read_pos % chan->capacity] = NULL;
    chan->read_pos++;
    chan->count--;

    return value; /* caller owns the ref */
}

Channel* channel_lookup(int id) {
    for (int i = 0; i < g_channel_count; i++) {
        if (g_channels[i] && g_channels[i]->id == id) {
            return g_channels[i];
        }
    }
    return NULL;
}

void channel_reset_all(void) {
    for (int i = 0; i < g_channel_count; i++) {
        if (g_channels[i]) {
            channel_destroy(g_channels[i]);
            g_channels[i] = NULL;
        }
    }
    g_channel_count = 0;
    g_next_channel_id = 1;
}
