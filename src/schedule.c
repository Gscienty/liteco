#include "liteco.h"
#include <malloc.h>

extern int liteco_link_init(liteco_link_t *const link);
extern int liteco_link_push(liteco_link_t *const link, void *const value);
extern int liteco_link_pop(void **const value, liteco_link_t *const link);
extern int liteco_link_empty(liteco_link_t *const link);

int liteco_schedule_init(liteco_schedule_t *const sche) {
    if (sche == NULL) {
        return LITECO_PARAMETER_UNEXCEPTION;
    }
    liteco_link_init(&sche->link);

    return LITECO_SUCCESS;
}

int liteco_schedule_join(liteco_schedule_t *const sche, liteco_coroutine_t *const co) {
    if (sche == NULL || co == NULL) {
        return LITECO_PARAMETER_UNEXCEPTION;
    }
    if (co->sche != NULL) {
        return LITECO_COROUTINE_JOINED;
    }
    co->sche = sche;
    return liteco_link_push(&sche->link, co);

    return LITECO_SUCCESS;
}

int liteco_schedule_pop(liteco_coroutine_t **const co, liteco_schedule_t *const sche) {
    if (co == NULL || sche == NULL) {
        return LITECO_PARAMETER_UNEXCEPTION;
    }
    return liteco_link_pop((void **) co, &sche->link);
}

liteco_boolean_t liteco_schedule_empty(liteco_schedule_t *const sche) {
    if (sche == NULL) {
        return LITECO_TRUE;
    }
    return liteco_link_empty(&sche->link);
}
