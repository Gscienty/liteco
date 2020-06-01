#include "liteco.h"
#include <malloc.h>
#include <pthread.h>

static liteco_schedule_t __LITECO_SCHE_ST__;
static liteco_boolean_t __LITECO_SCHE_ST_INITED__ = LITECO_FALSE;
static size_t __LITECO_DEFAULT_ST_SIZE__ = 128 * 1024;
static pthread_cond_t __LITECO_SCHE_COND_ST__;

static int liteco_g_coroutine_release(void *const stack, const size_t st_size);

extern int liteco_coroutine_set_status(liteco_coroutine_t *const co, int status);

static inline int __liteco_g_init__() {
    if (__LITECO_SCHE_ST_INITED__ == LITECO_FALSE) {
        liteco_schedule_init(&__LITECO_SCHE_ST__);
        __LITECO_SCHE_ST_INITED__ = LITECO_TRUE;
        pthread_cond_init(&__LITECO_SCHE_COND_ST__, NULL);
    }
    return LITECO_SUCCESS;
}

int liteco_g_create_st(liteco_coroutine_t **const result_co, int (*fn) (liteco_coroutine_t *const, void *const), void *const args) {
    __liteco_g_init__();

    liteco_coroutine_t *co = NULL;
    void *stack = NULL;

    if (fn == NULL) {
        return LITECO_PARAMETER_UNEXCEPTION;
    }
    if ((co = malloc(sizeof(*co))) == NULL) {
        return LITECO_INTERNAL_ERROR;
    }
    if ((stack = malloc(__LITECO_DEFAULT_ST_SIZE__)) == NULL) {
        free(co);
        return LITECO_INTERNAL_ERROR;
    }
    liteco_create(co, stack, __LITECO_DEFAULT_ST_SIZE__, fn, args, liteco_g_coroutine_release);
    if (result_co != NULL) {
        *result_co = co;
    }

    liteco_schedule_join(&__LITECO_SCHE_ST__, co);
    pthread_cond_signal(&__LITECO_SCHE_COND_ST__);

    return LITECO_SUCCESS;
}

static int liteco_g_coroutine_release(void *const stack, const size_t st_size) {
    if (stack == NULL) {
        return LITECO_INTERNAL_ERROR;
    }
    (void) st_size;
    free(stack);

    return LITECO_SUCCESS;
}

int liteco_g_publish_st(liteco_channel_t *const channel, void *const event) {
    __liteco_g_init__();

    liteco_coroutine_t *co = NULL;

    if (channel == NULL || event == NULL) {
        return LITECO_PARAMETER_UNEXCEPTION;
    }

    liteco_channel_publish(channel, event);
    liteco_channel_pop(&co, channel);
    if (co != NULL) {
        pthread_mutex_lock(&co->mutex);
        liteco_coroutine_set_status(co, LITECO_READYING);
        pthread_mutex_unlock(&co->mutex);
        liteco_schedule_join(&__LITECO_SCHE_ST__, co);
        pthread_cond_signal(&__LITECO_SCHE_COND_ST__);
    }
    return LITECO_SUCCESS;
}

int liteco_g_resume_st() {
    __liteco_g_init__();

    liteco_coroutine_t *co = NULL;
    liteco_schedule_pop(&co, &__LITECO_SCHE_ST__);

    if (co == NULL) {
        return LITECO_EMPTY;
    }

    liteco_resume(co);

    switch (co->status) {
    case LITECO_TERMINATE:
        liteco_release(co);
        break;
    case LITECO_RUNNING:
        liteco_coroutine_set_status(co, LITECO_READYING);
        liteco_schedule_join(&__LITECO_SCHE_ST__, co);
        pthread_cond_signal(&__LITECO_SCHE_COND_ST__);
        break;
    }

    return LITECO_SUCCESS;
}

int liteco_g_resume_until_terminate_st(liteco_coroutine_t *const target_co) {
    __liteco_g_init__();

    pthread_mutex_t mutex;
    liteco_coroutine_t *co = NULL;

    pthread_mutex_init(&mutex, NULL);

    for ( ;; ) {
        co = NULL;
        liteco_schedule_pop(&co, &__LITECO_SCHE_ST__);
        if (co == NULL) {
            pthread_mutex_lock(&mutex);
            pthread_cond_wait(&__LITECO_SCHE_COND_ST__, &mutex);
            pthread_mutex_unlock(&mutex);
            continue;
        }

        liteco_resume(co);

        switch (co->status) {
        case LITECO_TERMINATE:
            liteco_release(co);
            if (co == target_co) {
                return LITECO_SUCCESS;
            }
            break;
        case LITECO_STARTING:
        case LITECO_READYING:
        case LITECO_RUNNING:
            liteco_coroutine_set_status(co, LITECO_READYING);
            liteco_schedule_join(&__LITECO_SCHE_ST__, co);
            pthread_cond_signal(&__LITECO_SCHE_COND_ST__);
            break;
        }
    }

    pthread_mutex_destroy(&mutex);

    return LITECO_SUCCESS;
}

int liteco_g_yield(liteco_coroutine_t *const co) {
    if (co == NULL) {
        return LITECO_PARAMETER_UNEXCEPTION;
    }

    liteco_coroutine_set_status(co, LITECO_READYING);
    liteco_schedule_join(&__LITECO_SCHE_ST__, co);
    pthread_cond_signal(&__LITECO_SCHE_COND_ST__);

    liteco_yield(co);

    return LITECO_SUCCESS;
}
