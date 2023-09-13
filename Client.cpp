/*******************************************************************************
 * Author : yongheng
 * Data   : 2023/09/10 23:19
*******************************************************************************/


#include "Client.h"
void Client::attach() {
    next = mon->clients;
    mon->clients = this;
}

void Client::detach() {
    Client **tc;

    for (tc = &mon->clients; *tc && *tc != this; tc = &(*tc)->next);
    *tc = next;
}

void Client::detachstack() {
    Client **tc;
    for (tc = &mon->stack; *tc && *tc != this; tc = &(*tc)->snext)
        ;
    *tc = snext;

    Client *t;
    if (this == mon->sel) {
        for (t = mon->stack; t && !t->IsVisible(); t = t->snext)
            ;
        mon->sel = t;
    }
}