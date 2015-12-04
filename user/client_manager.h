#ifndef CLIENT_MANAGER_H
#define CLIENT_MANAGER_H

#include "ets_sys.h"
#include "osapi.h"
#include "c_types.h"
#include "mem.h"
#include "ip_addr.h"
#include "espconn.h"

typedef struct _clientListNode clientListNode;
typedef struct _clientListHead clientListHead;

struct _clientListNode {
    clientListNode *next;
    uint32_t remote_ip;
    int remote_port;
    struct espconn *esp_conn;
};

struct _clientListHead {
    clientListNode *next;
    clientListNode *active_conn;
};

clientListHead* cm_create_clientList();
void cm_add_connection(clientListHead *head, struct espconn *conn);
bool cm_set_active_connection(clientListHead *head, struct espconn *conn);
clientListNode *cm_get_active_connection(clientListHead *head);
bool cm_remove_connection(clientListHead *head, struct espconn *conn);

#endif
