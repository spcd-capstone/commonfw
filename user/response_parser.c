#include "response_parser.h"

rparser *rparser_create() {
    rparser *r = (rparser*)os_malloc(sizeof(rparser));
    rparser_reset(r);
    return r;
}

rparser_result rparser_parse_char(rparser *p, char c) {
    p->buffer[p->buffer_pos++] = c;
    if (p->state == RPS_NONE) {
        if (c == '+' || c == '-') {
            p->state = RPS_GOT_HEADER;
            return RPR_PARTIAL;
        }
        else {
            rparser_reset(p);
            return RPR_ERROR;
        }
    }
    else if (p->state == RPS_GOT_HEADER) {
        if (c == 'i') {
            p->state = RPS_PARSING_INT;
            return RPR_PARTIAL;
        }
        else if (c >= '0' && c <= '9') {
            p->state = RPS_PARSING_STRING_LEN;
            p->partial_int = c - '0';
            return RPR_PARTIAL;
        }
        else {
            rparser_reset(p);
            return RPR_ERROR;
        }
    }
    else if (p->state == RPS_PARSING_INT) {
        if (c == 'e') {
            p->state = RPS_READY_TO_SEND;
            return RPR_READY;
        }
        else if (c >= '0' && c <= '9') {
            return RPR_PARTIAL;
        }
        else {
            rparser_reset(p);
            return RPR_ERROR;
        }
    }
    else if (p->state == RPS_PARSING_STRING_LEN) {
        if (c == ':') {
            if (p->partial_int == 0) {
                p->state = RPS_READY_TO_SEND;
                return RPR_READY;
            }
            else {
                p->state = RPS_PARSING_STRING;
                return RPR_PARTIAL;
            }
        }
        else if (c >= '0' && c <= '9') {
            p->partial_int *= 10;
            p->partial_int += c - '0';
            return RPR_PARTIAL;
        }
        else {
            rparser_reset(p);
            return RPR_ERROR;
        }
    }
    else if (p->state == RPS_PARSING_STRING) {
        p->partial_int -= 1;
        if (p->partial_int == 0) {
            p->state = RPS_READY_TO_SEND;
            return RPR_READY;
        }
    }
    else if (p->state == RPS_READY_TO_SEND) {
        rparser_reset(p); // this is actually an error. must manually reset after success
        return RPR_ERROR;
    }
}

char *rparser_get_msg(rparser *p) {
    return p->buffer;
}

void rparser_reset(rparser *p) {
    os_memset(p->buffer, 0, RPARSER_BUFF_SIZE);
    p->buffer_pos = 0;
    p->partial_int = 0;
    p->state = RPS_NONE;
}
