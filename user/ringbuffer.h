#ifndef RINGBUFFER_H

#include "ets_sys.h"
#include "osapi.h"
#include "c_types.h"

typedef enum {
    RB_EMPTY,
    RB_FULL,
    RB_OK
} rb_code;

typedef struct _ring_buffer {
    char *data;
    int size;
    int count;
    int rpos;
    int wpos;
} ring_buffer;

ring_buffer* rb_create(int size);
void rb_destroy(ring_buffer*);
rb_code rb_putchar(ring_buffer*, char);
rb_code rb_getchar(ring_buffer*, char*);
rb_code rb_peekchar(ring_buffer*, char*);
int rb_getfreespace(ring_buffer*);
void rb_clear(ring_buffer*);

#endif
