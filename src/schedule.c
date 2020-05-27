#include "liteco.h"
#include <malloc.h>

int liteco_schedule_init(liteco_schedule_t *const sche) {
    if (sche == NULL) {
        return LITECO_PARAMETER_UNEXCEPTION;
    }
    sche->queue.co = NULL;
    sche->queue.next = &sche->queue;
    sche->q_tail = &sche->queue;

    return LITECO_SUCCESS;
}

int liteco_schedule_join(liteco_schedule_t *const sche, liteco_coroutine_t *const co) {
    liteco_link_node_t *node = NULL;
    if (sche == NULL || co == NULL) {
        return LITECO_PARAMETER_UNEXCEPTION;
    }
    if (co->sche != NULL) {
        return LITECO_COROUTINE_JOINED;
    }
    if ((node = malloc(sizeof(*node))) == NULL) {
        return LITECO_INTERNAL_ERROR;
    }
    sche->q_tail->next = node;
    sche->q_tail = node;
    node->next = &sche->queue;
    node->co = co;
    co->sche = sche;

    return LITECO_SUCCESS;
}

int liteco_schedule_pop(liteco_coroutine_t **const co, liteco_schedule_t *const sche) {
    liteco_link_node_t *node = NULL;
    if (co == NULL || sche == NULL) {
        return LITECO_PARAMETER_UNEXCEPTION;
    }
    if (&sche->queue == sche->q_tail) {
        return LITECO_SCHEDULE_EMPTY;
    }
    node = sche->queue.next;
    sche->queue.next = node->next;
    if (node == sche->q_tail) {
        sche->q_tail = &sche->queue;
    }
    *co = node->co;
    free(node);

    return LITECO_SUCCESS;
}

int liteco_schedule_empty(liteco_schedule_t *const sche) {
    if (sche == NULL) {
        return 1;
    }
    return &sche->queue == sche->q_tail;
}
