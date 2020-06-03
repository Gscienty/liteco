#include "liteco.h"
#include <stdarg.h>
#include <stdio.h>

extern int liteco_link_init(liteco_link_t *const link);
extern int liteco_link_push(liteco_link_t *const link, void *const value);
extern int liteco_link_pop(void **const value, liteco_link_t *const link);
extern liteco_boolean_t liteco_link_empty(liteco_link_t *const link);
extern int liteco_link_remove_spec(liteco_link_t *const link, void *const value);

liteco_channel_t __LITECO_CLOSED_CHANNEL__ = { .closed = LITECO_TRUE };

int liteco_channel_init(liteco_channel_t *const channel) {
    if (channel == NULL) {
        return LITECO_PARAMETER_UNEXCEPTION;
    }

    channel->closed = LITECO_FALSE;
    liteco_link_init(&channel->events);
    liteco_schedule_init(&channel->waiting_co);
    pthread_mutex_init(&channel->mutex, NULL);

    return LITECO_SUCCESS;
}

int liteco_channel_subscribe(void **const event, liteco_channel_t **const channel, liteco_coroutine_t *const co, ...) {
    liteco_channel_t *cand = NULL;
    liteco_channel_t *hit = NULL;
    va_list args;
    if (event == NULL || co == NULL) {
        return LITECO_PARAMETER_UNEXCEPTION;
    }

    *event = NULL;
    if (channel != NULL) {
        *channel = NULL;
    }
    pthread_mutex_lock(&co->mutex);
    for ( ;; ) {
        va_start(args, co);
        while ((cand = va_arg(args, liteco_channel_t *)) != NULL) {
            pthread_mutex_lock(&cand->mutex);
            if (cand->closed) {
                hit = cand;
                pthread_mutex_unlock(&cand->mutex);
                break;
            }
            if (!liteco_link_empty(&cand->events)) {
                liteco_link_pop(event, &cand->events);
                hit = cand;
                pthread_mutex_unlock(&cand->mutex);
                break;
            }

            co->status = LITECO_WAITING;
            liteco_schedule_join(&cand->waiting_co, co);
            pthread_mutex_unlock(&cand->mutex);
        }
        va_end(args);

        pthread_mutex_unlock(&co->mutex);

        if (hit == NULL) {
            liteco_yield(co);
        }

        pthread_mutex_lock(&co->mutex);
        va_start(args, co);
        while ((cand = va_arg(args, liteco_channel_t *)) != NULL) {
            pthread_mutex_lock(&cand->mutex);
            liteco_schedule_remove_spec(&cand->waiting_co, co);
            pthread_mutex_unlock(&cand->mutex);
        }
        va_end(args);

        if (hit != NULL) {
            break;
        }
    }
    pthread_mutex_unlock(&co->mutex);

    if (channel != NULL) {
        *channel = hit;
    }

    // pop check
    liteco_yield(co);

    return LITECO_SUCCESS;
}

int liteco_channel_publish(liteco_channel_t *const channel, void *const event) {
    if (channel == NULL || event == NULL) {
        return LITECO_PARAMETER_UNEXCEPTION;
    }
    pthread_mutex_lock(&channel->mutex);
    if (channel->closed) {
        pthread_mutex_unlock(&channel->mutex);
        return LITECO_CLOSED;
    }
    liteco_link_push(&channel->events, event);
    pthread_mutex_unlock(&channel->mutex);

    return LITECO_SUCCESS;
}

int liteco_channel_pop(liteco_coroutine_t **const co, liteco_channel_t *const channel) {
    liteco_coroutine_t *flag = NULL;
    if (co == NULL || channel == NULL) {
        return LITECO_PARAMETER_UNEXCEPTION;
    }

    *co = NULL;
    for ( ;; ) {
        liteco_schedule_pop(co, &channel->waiting_co);
        if (flag != NULL && flag == *co) {
            *co = NULL;
            break;
        }

        switch ((*co)->status) {
        case LITECO_TERMINATE:
            liteco_release(*co);
            continue;

        case LITECO_WAITING:
            liteco_resume(*co);
            if ((*co)->status != LITECO_WAITING) {
                return LITECO_SUCCESS;
            }
            break;
        }

        if (flag == NULL) {
            flag = *co;
        }
        liteco_schedule_join(&channel->waiting_co, *co);
    }

    return LITECO_SUCCESS;
}
