#include "liteco.h"
#include <malloc.h>

int liteco_link_init(liteco_link_t *const link) {
    if (link == NULL) {
        return LITECO_PARAMETER_UNEXCEPTION;
    }
    link->head.next = &link->head;
    link->q_tail = &link->head;

    return LITECO_SUCCESS;
}

int liteco_link_push(liteco_link_t *const link, void *const value) {
    liteco_link_node_t *node = NULL;
    if (link == NULL || value == NULL) {
        return LITECO_PARAMETER_UNEXCEPTION;
    }
    if ((node = malloc(sizeof(*node))) == NULL) {
        return LITECO_INTERNAL_ERROR;
    }
    node->value = value;
    node->next = &link->head;
    link->q_tail->next = node;
    link->q_tail = node;

    return LITECO_SUCCESS;
}

int liteco_link_pop(void **const value, liteco_link_t *const link) {
    liteco_link_node_t *node = NULL;
    if (value == NULL || link == NULL) {
        return LITECO_PARAMETER_UNEXCEPTION;
    }
    if (link->q_tail == &link->head) {
        return LITECO_EMPTY;
    }
    node = link->head.next;
    link->head.next = node->next;
    if (node == link->q_tail) {
        link->q_tail = &link->head;
    }
    *value = node->value;
    free(node);

    return LITECO_SUCCESS;
}

int liteco_link_empty(liteco_link_t *const link) {
    if (link == NULL) {
        return LITECO_TRUE;
    }
    return &link->head == link->q_tail;
}

int liteco_link_remove_spec(liteco_link_t *const link, void *const value) {
    liteco_link_node_t *node = NULL;
    liteco_link_node_t *prev = NULL;
    if (link == NULL || value == NULL) {
        return LITECO_PARAMETER_UNEXCEPTION;
    }
    prev = &link->head;
    node = link->head.next;
    while (node != &link->head) {
        if (node->value == value) {
            prev->next = node->next;
            free(node);
        }
        prev = node;
        node = node->next;
    }

    return LITECO_SUCCESS;
}
