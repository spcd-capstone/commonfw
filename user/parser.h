#ifndef PARSER_H

#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "ip_addr.h"
#include "espconn.h"
#include "user_interface.h"
#include "user_config.h"

#include "ringbuffer.h"

#define PARSER_BUFFER_SIZE 512

typedef enum {
    INT_VALUE,
    STRING_VALUE
} parser_value_type;

typedef enum {
    PS_READY_FOR_COMMAND,
    PS_READY_FOR_SKEY,
    PS_READY_FOR_GKEY,
    PS_PARTIAL_KEY,
    PS_READY_FOR_VALUE,
    PS_PARTIAL_VALUE
} parser_state;

typedef enum {
    PR_NONE,
    PR_INCOMPLETE,          // mid parse, nothing to report yet, waiting for more data
    PR_GET_COMMAND,
    PR_SET_COMMAND,
    PR_STATUS_COMMAND,
    PR_KEY,
    PR_VALUE_STRING,
    PR_VALUE_INT,
    PR_ERROR_BUFFTOOSMALL, // supplied buffer is too small
    PR_ERROR_INVALIDDATA,
    PR_ERROR_INVALIDCOMMAND
} parser_result;

typedef struct _parser {
    ring_buffer *buffer;
    parser_state state;
    int output_pos;
    int partial_int;
} parser;

parser* parser_create();
void parser_destroy(parser*);
sint8 parser_feed(parser*, char*, int len);
parser_result parser_process(parser*, int*, char*, int maxlen);
parser_state get_state(parser*);
void parser_reset(parser*);

#endif

