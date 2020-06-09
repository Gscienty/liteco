#include "liteco.h"

int liteco_link_init(liteco_link_t *const link) {
    if (link == NULL) {
        return LITECO_PARAMETER_UNEXCEPTION;
    }
    link->head.next = &link->head;
    link->q_tail = &link->head;

    return LITECO_SUCCESS;
}

int liteco_link_push(liteco_link_t *const link, liteco_link_node_t *const node) {
    if (link == NULL) {
        return LITECO_PARAMETER_UNEXCEPTION;
    }
    node->next = &link->head;

    link->q_tail->next = node;
    link->q_tail = node;

    return LITECO_SUCCESS;
}

int liteco_link_pop(liteco_link_node_t **const node, liteco_link_t *const link) {
    if (link == NULL) {
        return LITECO_PARAMETER_UNEXCEPTION;
    }
    *node = NULL;
    if (link->q_tail == &link->head) {
        return LITECO_EMPTY;
    }

    *node = link->head.next;
    link->head.next = (*node)->next;
    if (*node == link->q_tail) {
        link->q_tail = &link->head;
    }

    return LITECO_SUCCESS;
}

liteco_boolean_t liteco_link_empty(liteco_link_t *const link) {
    if (link == NULL) {
        return LITECO_TRUE;
    }
    
    return &link->head == link->q_tail;
}

