#ifndef CLIENT_MANAGER_H

#include "ets_sys.h"
#include "osapi.h"
#include "c_types.h"
#include "mem.h"
#include "ip_addr.h"
#include "espconn.h"

typedef struct _clientList clientList;
typedef struct _clientListHead clientListHead;

struct _clientList {
    clientList *next;
    struct espconn esp_conn;
};

struct _clientListHead {
    clientList *next;
};

clientListHead* cm_create_clientList();
void cm_add_connection(clientListHead *head, struct espconn *conn);
bool cm_remove_connection(clientListHead *head, struct espconn *conn);

#endif
