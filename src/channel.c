#include "liteco.h"
#include <stdarg.h>

extern int liteco_link_init(liteco_link_t *const link);
extern int liteco_link_push(liteco_link_t *const link, void *const value);
extern int liteco_link_pop(void **const value, liteco_link_t *const link);
extern int liteco_link_empty(liteco_link_t *const link);
extern int liteco_link_remove_spec(liteco_link_t *const link, void *const value);

liteco_channel_t __LITECO_CLOSED_CHANNEL__ = { .closed = LITECO_TRUE };

int liteco_channel_init(liteco_channel_t *const channel) {
    if (channel == NULL) {
        return LITECO_PARAMETER_UNEXCEPTION;
    }
    channel->closed = LITECO_FALSE;
    liteco_link_init(&channel->waiting_co);
    liteco_link_init(&channel->events);

    return LITECO_SUCCESS;
}

int liteco_channel_subscribe(void **const event, liteco_channel_t **const channel, liteco_coroutine_t *const co, ...) {
    liteco_channel_t *cand_channel = NULL;
    va_list channels;
    if (event == NULL || channel == NULL || co == NULL) {
        return LITECO_PARAMETER_UNEXCEPTION;
    }
    *event = NULL;
    *channel = NULL;

    for ( ;; ) {
        va_start(channels, co);
        while ((cand_channel = va_arg(channels, liteco_channel_t *)) != NULL) {
            if (cand_channel->closed) {
                *channel = cand_channel;
                break;
            }
            if (!liteco_link_empty(&cand_channel->events)) {
                liteco_link_pop((void **) event, &cand_channel->events);
                *channel = cand_channel;
                break;
            }

            liteco_link_push(&cand_channel->waiting_co, co);
        }
        va_end(channels);

        if (*channel == NULL) {
            liteco_yield(co);
        }

        va_start(channels, co);
        while ((cand_channel = va_arg(channels, liteco_channel_t *)) != NULL) {
            liteco_link_remove_spec(&cand_channel->waiting_co, co);
        }
        va_end(channels);

        if (*channel != NULL) {
            break;
        }
    }

    return LITECO_SUCCESS;
}

int liteco_channel_publish(liteco_coroutine_t **const co, liteco_channel_t *const channel, void *const event) {
    if (co == NULL || channel == NULL || event == NULL) {
        return LITECO_PARAMETER_UNEXCEPTION;
    }
    if (channel->closed) {
        return LITECO_CLOSED;
    }
    liteco_link_push(&channel->events, event);
    if (liteco_link_empty(&channel->waiting_co)) {
        return LITECO_SUCCESS;
    }
    return liteco_link_pop((void **) co, &channel->waiting_co);
}
