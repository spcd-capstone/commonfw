#include "ringbuffer.h"

#include "mem.h"

ICACHE_FLASH_ATTR
ring_buffer* rb_create(int size) {
    ring_buffer *rb = (ring_buffer*)os_malloc(sizeof(ring_buffer));
    rb->data = (char*)os_malloc(sizeof(char) * size);
    rb->size = size;
    rb->count = 0;
    rb->rpos = 0;
    rb->wpos = 0;

    return rb;
}

ICACHE_FLASH_ATTR
void rb_destroy(ring_buffer *rb) {
    os_free(rb->data);
    os_free(rb);
}

ICACHE_FLASH_ATTR
rb_code rb_putchar(ring_buffer *rb, char c) {
    if (rb->count >= rb->size) {
        return RB_FULL;
    }
    rb->data[rb->wpos] = c;
    rb->wpos = (rb->wpos + 1) % rb->size;
    rb->count += 1;

    return RB_OK;
}

ICACHE_FLASH_ATTR
rb_code rb_getchar(ring_buffer *rb, char *c) {
    if (rb->count == 0) {
        return RB_EMPTY;
    }
    *c = rb->data[rb->rpos];
    rb->rpos = (rb->rpos + 1) % rb->size;
    rb->count -= 1;

    return RB_OK;
}

ICACHE_FLASH_ATTR
rb_code rb_peekchar(ring_buffer *rb, char *c) {
    if (rb->count == 0) {
        return RB_EMPTY;
    }
    *c = rb->data[rb->rpos];

    return RB_OK;
}

ICACHE_FLASH_ATTR
int rb_getfreespace(ring_buffer *rb) {
    return (rb->size - rb->count);
}

void rb_clear(ring_buffer *rb) {
    rb->count = 0;
    rb->rpos = 0;
    rb->wpos = 0;
}

