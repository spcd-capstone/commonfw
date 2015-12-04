#include "client_manager.h"

clientListHead* cm_create_clientList() {
    // allocate head of list and zero it. A zeroed head will signal end of list
    clientListHead* r = (clientListHead*)os_malloc(sizeof(clientListHead));
    r->next = 0;

    r->active_conn = 0;
    return r;
}

void cm_add_connection(clientListHead *head, struct espconn *conn) {
    // allocate new list item
    clientListNode *newNode = (clientListNode*)os_malloc(sizeof(clientListNode));

    // copy all connection data into new node
    os_memcpy(&newNode->remote_ip, conn->proto.tcp->remote_ip, 4);
    newNode->remote_port = conn->proto.tcp->remote_port;
    newNode->esp_conn = conn;

    // insert new node at begining of list
    newNode->next = head->next;
    head->next = newNode;
}

bool cm_set_active_connection(clientListHead *head, struct espconn *conn) {
    int port = conn->proto.tcp->remote_port;
    uint32_t ip = 0;
    os_memcpy(&ip, conn->proto.tcp->remote_ip, 4);

    // find node
    clientListNode *curr = head->next;
    while (curr) {
        if (curr->remote_port == port
                && curr->remote_ip == ip) {
            head->active_conn = curr;
            return true;
        }
        curr = curr->next;
    }
    // node not found
    return false;
}

clientListNode *cm_get_active_connection(clientListHead *head) {
    return head->active_conn;
}

bool cm_remove_connection(clientListHead *head, struct espconn *conn) {
    int port = conn->proto.tcp->remote_port;
    uint32_t ip = 0;
    os_memcpy(&ip, conn->proto.tcp->remote_ip, 4);

    // find node
    clientListNode *prev = 0;
    clientListNode *curr = head->next;
    while (curr) {
        if (curr->remote_port == port
                && curr->remote_ip == ip) {
            if (prev == 0) {
                head->next = curr->next;
            }
            else {
                prev->next = curr->next;
            }

            os_free(curr);
            return true;
        }
        prev = curr;
        curr = curr->next;
    }
    return false;
}



