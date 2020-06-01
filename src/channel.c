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
    liteco_schedule_init(&channel->waiting_co);
    liteco_link_init(&channel->events);
    pthread_mutex_init(&channel->mutex, NULL);

    return LITECO_SUCCESS;
}

int liteco_channel_subscribe(void **const event, liteco_channel_t **const channel, liteco_coroutine_t *const co, ...) {
    liteco_channel_t *cand_channel = NULL;
    va_list channels;
    if (event == NULL || channel == NULL || co == NULL || co->status == LITECO_TERMINATE) {
        return LITECO_PARAMETER_UNEXCEPTION;
    }
    *event = NULL;
    *channel = NULL;

    for ( ;; ) {
        pthread_mutex_lock(&co->mutex);
        if (co->status == LITECO_TERMINATE) {
            pthread_mutex_unlock(&co->mutex);
            return LITECO_SUCCESS;
        }
        va_start(channels, co);
        while ((cand_channel = va_arg(channels, liteco_channel_t *)) != NULL) {
            pthread_mutex_lock(&cand_channel->mutex);
            if (cand_channel->closed) {
                pthread_mutex_unlock(&cand_channel->mutex);
                *channel = cand_channel;
                break;
            }

            if (!liteco_link_empty(&cand_channel->events)) {
                liteco_link_pop(event, &cand_channel->events);
                pthread_mutex_unlock(&cand_channel->mutex);
                *channel = cand_channel;
                break;
            }

            co->status = LITECO_WAITING;

            liteco_schedule_join(&cand_channel->waiting_co, co);
            pthread_mutex_unlock(&cand_channel->mutex);
        }
        va_end(channels);
        pthread_mutex_unlock(&co->mutex);

        if (*channel == NULL) {
            liteco_yield(co);
        }

        pthread_mutex_lock(&co->mutex);
        va_start(channels, co);
        while ((cand_channel = va_arg(channels, liteco_channel_t *)) != NULL) {
            pthread_mutex_lock(&cand_channel->mutex);
            if (cand_channel->closed) {
                pthread_mutex_unlock(&cand_channel->mutex);
                continue;
            }
            liteco_schedule_remove_spec(&cand_channel->waiting_co, co);
            pthread_mutex_unlock(&cand_channel->mutex);
        }
        va_end(channels);
        pthread_mutex_unlock(&co->mutex);

        if (*channel != NULL) {
            break;
        }
    }
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
    if (co == NULL || channel == NULL) {
        return LITECO_PARAMETER_UNEXCEPTION;
    }

    return liteco_schedule_pop(co, &channel->waiting_co);
}
