#ifndef RESPONSE_PARSER_H
#define RESPONSE_PARSER_H

#include "ets_sys.h"
#include "osapi.h"
#include "c_types.h"
#include "mem.h"

#define RPARSER_BUFF_SIZE 512

typedef enum {
    RPR_ERROR,
    RPR_PARTIAL,
    RPR_READY
} rparser_result;

typedef enum {
    RPS_NONE,
    RPS_GOT_HEADER,
    RPS_PARSING_INT,
    RPS_PARSING_STRING_LEN,
    RPS_PARSING_STRING,
    RPS_READY_TO_SEND
} rparser_state;

typedef struct _rparser {
    char buffer[RPARSER_BUFF_SIZE];
    rparser_state state;
    int partial_int;
    int buffer_pos;
} rparser;

rparser *rparser_create();
rparser_result rparser_parse_char(rparser *, char);
char *rparser_get_msg(rparser *);
void rparser_reset(rparser *);

#endif

