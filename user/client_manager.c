#include "client_manager.h"

clientListHead* cm_create_clientList() {
    // allocate head of list and zero it. A zeroed head will signal end of list
    clientListHead* r = (clientListHead*)os_malloc(sizeof(clientListHead));
    r->next = 0;

    return r;
}

void cm_add_connection(clientListHead *head, struct espconn *conn) {
    // allocate new list item
    clientList *newNode = (clientList*)os_malloc(sizeof(clientList));

    // copy all connection data into new node
    os_memcpy(&(newNode->esp_conn), conn, sizeof(struct espconn));

    // insert new node at begining of list
    newNode->next = head->next;
    head->next = newNode;
}

bool compare_connections(struct espconn *fst, struct espconn *snd) {
    if (fst == snd)
        return true;
    if (fst == 0 || snd == 0)
        return false;
    if (fst->type == snd->type && fst->state == snd->state) {
        if (fst->type == ESPCONN_TCP) {
            esp_tcp *c1 = fst->proto.tcp;
            esp_tcp *c2 = snd->proto.tcp;
            if (c1->remote_port == c2->remote_port
                    && c1->local_port == c2->local_port
                    && c1->remote_ip[0] == c2->remote_ip[0]
                    && c1->remote_ip[1] == c2->remote_ip[1]
                    && c1->remote_ip[2] == c2->remote_ip[2]
                    && c1->remote_ip[3] == c2->remote_ip[3])
                return true;
        }
        else if (fst->type == ESPCONN_UDP) {
            esp_udp *c1 = fst->proto.udp;
            esp_udp *c2 = snd->proto.udp;
            if (c1->remote_port == c2->remote_port
                    && c1->local_port == c2->local_port
                    && c1->remote_ip[0] == c2->remote_ip[0]
                    && c1->remote_ip[1] == c2->remote_ip[1]
                    && c1->remote_ip[2] == c2->remote_ip[2]
                    && c1->remote_ip[3] == c2->remote_ip[3])
                return true;
        }
    }
    return false;
}

bool cm_remove_connection(clientListHead *head, struct espconn *conn) {
    // find node
    clientList *prev = 0;
    clientList *curr = head->next;
    while (head) {
        if (compare_connections(&(curr->esp_conn), conn)) {
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



