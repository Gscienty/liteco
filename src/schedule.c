#include "liteco.h"
#include <malloc.h>

extern int liteco_link_init(liteco_link_t *const link);
extern int liteco_link_push(liteco_link_t *const link, void *const value);
extern int liteco_link_pop(void **const value, liteco_link_t *const link);
extern int liteco_link_empty(liteco_link_t *const link);
extern int liteco_link_remove_spec(liteco_link_t *const link, void *const value);

int liteco_schedule_init(liteco_schedule_t *const sche) {
    if (sche == NULL) {
        return LITECO_PARAMETER_UNEXCEPTION;
    }
    liteco_link_init(&sche->link);
    pthread_mutex_init(&sche->mutex, NULL);

    return LITECO_SUCCESS;
}

int liteco_schedule_join(liteco_schedule_t *const sche, liteco_coroutine_t *const co) {
    if (sche == NULL || co == NULL || co->status == LITECO_TERMINATE) {
        return LITECO_PARAMETER_UNEXCEPTION;
    }
    pthread_mutex_lock(&co->mutex);
    if (co->sche != NULL) {
        pthread_mutex_unlock(&co->mutex);
        return LITECO_COROUTINE_JOINED;
    }
    co->sche = sche;
    co->ref_count++;
    if (co->status != LITECO_WAITING && co->status != LITECO_TERMINATE) {
        co->status = LITECO_READYING;
    }
    if (co->status == LITECO_TERMINATE) {
        pthread_mutex_unlock(&co->mutex);
        return LITECO_CLOSED;
    }
    pthread_mutex_unlock(&co->mutex);

    pthread_mutex_lock(&sche->mutex);
    int result = liteco_link_push(&sche->link, co);
    pthread_mutex_unlock(&sche->mutex);

    return result;
}

int liteco_schedule_pop(liteco_coroutine_t **const co, liteco_schedule_t *const sche) {
    if (co == NULL || sche == NULL) {
        return LITECO_PARAMETER_UNEXCEPTION;
    }
    pthread_mutex_lock(&sche->mutex);
    int result = liteco_link_pop((void **) co, &sche->link);
    if (*co == NULL) {
        pthread_mutex_unlock(&sche->mutex);
        return result;
    }
    (*co)->sche = NULL;
    pthread_mutex_unlock(&sche->mutex);

    return result;
}

liteco_boolean_t liteco_schedule_empty(liteco_schedule_t *const sche) {
    if (sche == NULL) {
        return LITECO_TRUE;
    }
    pthread_mutex_lock(&sche->mutex);
    liteco_boolean_t result = liteco_link_empty(&sche->link);
    pthread_mutex_unlock(&sche->mutex);

    return result;
}

int liteco_schedule_remove_spec(liteco_schedule_t *const sche, liteco_coroutine_t *const co) {
    if (sche == NULL || co == NULL) {
        return LITECO_PARAMETER_UNEXCEPTION;
    }
    pthread_mutex_lock(&sche->mutex);
    co->ref_count -= liteco_link_remove_spec(&sche->link, co);
    pthread_mutex_unlock(&sche->mutex);

    return LITECO_SUCCESS;
}
