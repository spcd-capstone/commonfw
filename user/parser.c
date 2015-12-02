#include "parser.h"

#include "ets_sys.h"
#include "osapi.h"
#include "mem.h"
#include "gpio.h"
#include "os_type.h"
#include "ip_addr.h"
#include "espconn.h"
#include "user_interface.h"
#include "user_config.h"

ICACHE_FLASH_ATTR
parser* parser_create() {
    parser *p;
    p = (parser*)os_malloc(sizeof(parser));
    p->buffer = rb_create(PARSER_BUFFER_SIZE);
    p->state = PS_READY_FOR_COMMAND;
    p->output_pos = 0;
    p->partial_int = 0;

    return p;
}

ICACHE_FLASH_ATTR
void parser_destroy(parser* p) {
    rb_destroy(p->buffer);
    os_free(p);
}

ICACHE_FLASH_ATTR
sint8 parser_feed(parser *p, char *data, int len) {
    if (len > rb_getfreespace(p->buffer)) {
        return -1;
    }
    int i;
    for (i = 0; i < len; i++) {
        rb_putchar(p->buffer, data[i]);
    }
    return 0;
}

ICACHE_FLASH_ATTR
parser_result parser_process(parser *p, int *parsedInt, char *parsedString, int maxlen) {
    char c;
    rb_code get_result;
    if (p->state == PS_READY_FOR_COMMAND) {
        if (rb_getchar(p->buffer, &c) == RB_OK) {
            if (c == 's') {
                p->state = PS_READY_FOR_SKEY;
                return PR_SET_COMMAND;
            }
            else if (c == 'g') {
                p->state = PS_READY_FOR_GKEY;
                return PR_GET_COMMAND;
            }
            else {
                return PR_ERROR_INVALIDCOMMAND;
            }
        }
        return PR_NONE;
    }
    else if (p->state == PS_READY_FOR_SKEY) {
        p->partial_int = 0;
        get_result = rb_getchar(p->buffer, &c);
        while (get_result == RB_OK && (c > '0' && c < '9')) {
            p->partial_int *= 10;
            p->partial_int += c - '0';
            rb_code result = rb_getchar(p->buffer, &c);
        }
        for (p->output_pos = 0; p->output_pos < p->partial_int; p->output_pos += 1) {
            get_result = rb_getchar(p->buffer, &c);
            parsedString[p->output_pos] = c;
        }

        parsedString[p->output_pos] = '\0';

        p->state = PS_READY_FOR_VALUE;
        return PR_KEY;

        /*
        // TODO: finish error checked version
        bool never_looped = true;
        p->partial_int = 0;
        get_result = rb_getchar(p->buffer, &c);
        while (get_result == RB_OK && (c > '0' && c < '9')) {
            never_looped = false;
            p->partial_int *= 10;
            p->partial_int += c - '0';
            rb_code result = rb_getchar(p->buffer, &c);
        }
        if (never_looped || c != ':' || get_result == RB_EMPTY) {
            parser_reset(p);
            return PS_ERROR_INVALIDDATA;
        }
        // begin copying key to supplied buffer
        p->output_pos = 0;
        while (p->partial_int > 0) {
            if (p->output_pos >= maxlen) {
                p->state = PS_PARTIAL_KEY;
                return PR_ERROR_BUFFTOOSMALL;
            }

            get_result = rb_getchar(p->buffer, &c);
            p->partial_int -= 1;

            if (get_result == RB_OK) {
                parsedString[p->output_pos] = c;
                p->output_pos += 1;
            }
            else {

            }


        }
        */
    }
    else if (p->state == PS_READY_FOR_GKEY) {
        p->partial_int = 0;
        get_result = rb_getchar(p->buffer, &c);
        while (get_result == RB_OK && (c > '0' && c < '9')) {
            p->partial_int *= 10;
            p->partial_int += c - '0';
            rb_code result = rb_getchar(p->buffer, &c);
        }
        for (p->output_pos = 0; p->output_pos < p->partial_int; p->output_pos += 1) {
            get_result = rb_getchar(p->buffer, &c);
            parsedString[p->output_pos] = c;
        }
        parsedString[p->output_pos] = '\0';
        p->state = PS_READY_FOR_COMMAND;
        return PR_KEY;
    }
    else if (p->state == PS_PARTIAL_KEY) {
        // TODO: part of error checked version
    }
    else if (p->state == PS_READY_FOR_VALUE) {
        bool isInt = false;
        p->partial_int = 0;
        get_result = rb_getchar(p->buffer, &c);
        if (c == 'i') {
            isInt = true;
            get_result = rb_getchar(p->buffer, &c);
        }
        while (get_result == RB_OK && (c >= '0' && c <= '9')) {
            p->partial_int *= 10;
            p->partial_int += c - '0';
            rb_code result = rb_getchar(p->buffer, &c);
        }
        if (isInt) {
            p->state = PS_READY_FOR_COMMAND;
            *parsedInt = p->partial_int;
            return PR_VALUE_INT;
        }
        for (p->output_pos = 0; p->output_pos < p->partial_int; p->output_pos += 1) {
            get_result = rb_getchar(p->buffer, &c);
            parsedString[p->output_pos] = c;
        }
        parsedString[p->output_pos] = '\0';
        p->state = PS_READY_FOR_COMMAND;
        return PR_VALUE_STRING;
    }
    else if (p->state == PS_PARTIAL_VALUE) {
        // TODO: part of error checked version
    }
}

ICACHE_FLASH_ATTR
parser_state get_state(parser *p) {
    return p->state;
}

ICACHE_FLASH_ATTR
void parser_reset(parser *p) {
    rb_clear(p->buffer);
    p->state = PS_READY_FOR_COMMAND;
}
