/*
 * Copyright (c) 2020 Gscienty <gaoxiaochuan@hotmail.com>
 *
 * Distributed under the MIT software license, see the accompanying
 * file LICENSE or https://www.opensource.org/licenses/mit-license.php .
 *
 * 本文件功能说明：
 *
 * 协程运行时
 *
 */

#include "liteco.h"
#include "internal/link.h"
#include "internal/runtime.h"
#include "internal/couroutine.h"
#include "internal/malloc.h"

int liteco_runtime_init(liteco_runtime_t *const runtime) {
    if (runtime == NULL) {
        return LITECO_PARAMETER_UNEXCEPTION;
    }

    liteco_link_init(&runtime->q_wait);
    liteco_link_init(&runtime->q_ready);
    liteco_link_init(&runtime->q_timer);

    pthread_cond_init(&runtime->cond, NULL);
    pthread_mutex_init(&runtime->lock, NULL);

    return LITECO_SUCCESS;
}

int liteco_timer_join(liteco_link_t *const q_timer, liteco_coroutine_t *const co, const u_int64_t timeout) {
    liteco_link_node_t *prev = NULL;
    liteco_link_node_t *node = NULL;
    liteco_timer_coroutine_t *timer_node = NULL;
    if (q_timer == NULL || co == NULL) {
        return LITECO_PARAMETER_UNEXCEPTION;
    }
    if (co->status == LITECO_TERMINATE) {
        return LITECO_CLOSED;
    }

    liteco_timer_remove_spec(q_timer, co);

    if ((timer_node = liteco_malloc(sizeof(*timer_node))) == NULL) {
        return LITECO_INTERNAL_ERROR;
    }
    timer_node->co = co;
    timer_node->timeout = timeout;
    timer_node->node.next = NULL;

    prev = &q_timer->head;
    node = q_timer->head.next;
    while (node != &q_timer->head) {
        if (timeout < liteco_container_of(liteco_timer_coroutine_t, node, node)->timeout) {
            timer_node->node.next = node;
            prev->next = &timer_node->node;

            return LITECO_SUCCESS;
        }

        prev = node;
        node = node->next;
    }

    if (timer_node->node.next == NULL) {
        liteco_link_push(q_timer, &timer_node->node);
    }

    return LITECO_SUCCESS;
}

u_int64_t liteco_timer_last(liteco_link_t *const q_timer) {
    liteco_timer_coroutine_t *timer_node = NULL;
    if (q_timer == NULL) {
        return 0;
    }
    timer_node = liteco_container_of(liteco_timer_coroutine_t, node, q_timer->head.next);
    return timer_node->timeout;
}

int liteco_timer_pop(liteco_coroutine_t **const co, liteco_link_t *const q_timer) {
    liteco_link_node_t *node = NULL;
    if (co == NULL || q_timer == NULL) {
        return LITECO_PARAMETER_UNEXCEPTION;
    }
    liteco_link_pop(&node, q_timer);
    if (node == NULL) {
        return LITECO_EMPTY;
    }
    *co = liteco_container_of(liteco_timer_coroutine_t, node, node)->co;
    liteco_free(liteco_container_of(liteco_timer_coroutine_t, node, node));

    return LITECO_SUCCESS;
}

int liteco_timer_remove_spec(liteco_link_t *const q_timer, liteco_coroutine_t *const co) {
    liteco_link_node_t *prev = NULL;
    liteco_link_node_t *node = NULL;
    if (q_timer == NULL || co == NULL) {
        return LITECO_PARAMETER_UNEXCEPTION;
    }
    prev = &q_timer->head;
    node = q_timer->head.next;
    while (node != &q_timer->head) {
        if (liteco_container_of(liteco_timer_coroutine_t, node, node)->co == co) {
            if (node == q_timer->q_tail) {
                q_timer->q_tail = prev;
            }
            prev->next = node->next;
            liteco_free(liteco_container_of(liteco_timer_coroutine_t, node, node));
            node = prev->next;
        }
        else {
            prev = node;
            node = node->next;
        }
    }

    return LITECO_SUCCESS;
}

bool liteco_timer_exist(liteco_link_t *const q_timer, liteco_coroutine_t *const co) {
    liteco_link_node_t *prev = NULL;
    liteco_link_node_t *node = NULL;
    if (q_timer == NULL || co == NULL) {
        return false;
    }

    prev = &q_timer->head;
    node = q_timer->head.next;
    while (node != &q_timer->head) {
        if (liteco_container_of(liteco_timer_coroutine_t, node, node)->co == co) {
            return true;
        }
        prev = node;
        node = node->next;
    }

    return false;
}

u_int64_t liteco_timer_spec(liteco_link_t *const q_timer, liteco_coroutine_t *const co) {
    liteco_link_node_t *prev = NULL;
    liteco_link_node_t *node = NULL;
    if (q_timer == NULL || co == NULL) {
        return 0;
    }

    prev = &q_timer->head;
    node = q_timer->head.next;
    while (node != &q_timer->head) {
        if (liteco_container_of(liteco_timer_coroutine_t, node, node)->co == co) {
            return liteco_container_of(liteco_timer_coroutine_t, node, node)->timeout;
        }
        prev = node;
        node = node->next;
    }

    return 0;
}

int liteco_wait_join(liteco_link_t *const q_wait, liteco_coroutine_t *const co, liteco_channel_t *const channels[]) {
    liteco_wait_coroutine_t *node = NULL;
    if (q_wait == NULL || co == NULL || channels == NULL) {
        return LITECO_PARAMETER_UNEXCEPTION;
    }
    liteco_wait_remove_spec(q_wait, co);

    if ((node = liteco_malloc(sizeof(*node))) == NULL) {
        return LITECO_INTERNAL_ERROR;
    }
    node->co = co;
    node->channels = channels;

    liteco_link_push(q_wait, &node->node);

    return LITECO_SUCCESS;
}

int liteco_wait_pop_spec(liteco_coroutine_t **const co, liteco_link_t *const q_wait, liteco_channel_t *const channel) {
    liteco_link_node_t *prev = NULL;
    liteco_link_node_t *node = NULL;
    liteco_channel_t *const *spec_channel = NULL;
    if (co == NULL || q_wait == NULL || channel == NULL) {
        return LITECO_PARAMETER_UNEXCEPTION;
    }

    *co = NULL;
    prev = &q_wait->head;
    node = q_wait->head.next;
    while (node != &q_wait->head) {
        for (spec_channel = liteco_container_of(liteco_wait_coroutine_t, node, node)->channels; *spec_channel != NULL; spec_channel++) {
            if (*spec_channel == channel) {
                *co = liteco_container_of(liteco_wait_coroutine_t, node, node)->co;

                if (node == q_wait->q_tail) {
                    q_wait->q_tail = prev;
                }
                prev->next = node->next;

                liteco_free(liteco_container_of(liteco_wait_coroutine_t, node, node));
                return LITECO_SUCCESS;
            }
        }

        prev = node;
        node = node->next;
    }

    return LITECO_SUCCESS;
}

int liteco_wait_remove_spec(liteco_link_t *const q_wait, liteco_coroutine_t *const co) {
    liteco_link_node_t *prev = NULL;
    liteco_link_node_t *node = NULL;
    if (q_wait == NULL || co == NULL) {
        return LITECO_PARAMETER_UNEXCEPTION;
    }
    prev = &q_wait->head;
    node = q_wait->head.next;
    while (node != &q_wait->head) {
        if (liteco_container_of(liteco_wait_coroutine_t, node, node)->co == co) {
            if (node == q_wait->q_tail) {
                q_wait->q_tail = prev;
            }
            prev->next = node->next;
            liteco_free(liteco_container_of(liteco_wait_coroutine_t, node, node));
            node = prev->next;
        }
        else {
            prev = node;
            node = node->next;
        }
    }

    return LITECO_SUCCESS;
}

int liteco_ready_join(liteco_link_t *const q_ready, liteco_coroutine_t *const co) {
    liteco_ready_coroutine_t *node = NULL;
    if (q_ready == NULL || co == NULL) {
        return LITECO_PARAMETER_UNEXCEPTION;
    }
    if ((node = liteco_malloc(sizeof(*node))) == NULL) {
        return LITECO_INTERNAL_ERROR;
    }
    node->co = co;

    liteco_link_push(q_ready, &node->node);

    return LITECO_SUCCESS;
}

int liteco_ready_pop(liteco_coroutine_t **const co, liteco_link_t *const q_ready) {
    liteco_link_node_t *node;
    if (co == NULL || q_ready == NULL) {
        return LITECO_PARAMETER_UNEXCEPTION;
    }
    *co = NULL;
    liteco_link_pop(&node, q_ready);
    if (node == NULL) {
        return LITECO_EMPTY;
    }
    *co = liteco_container_of(liteco_ready_coroutine_t, node, node)->co;
    liteco_free(liteco_container_of(liteco_ready_coroutine_t, node, node));

    return LITECO_SUCCESS;
}

int liteco_ready_pop_spec(liteco_link_t *const q_ready, liteco_coroutine_t *const co) {
    liteco_link_node_t *prev = NULL;
    liteco_link_node_t *node = NULL;
    if (q_ready == NULL || co == NULL) {
        return LITECO_PARAMETER_UNEXCEPTION;
    }

    prev = &q_ready->head;
    node = q_ready->head.next;
    while (node != &q_ready->head) {
        if (co == liteco_container_of(liteco_ready_coroutine_t, node, node)->co) {
            if (node == q_ready->q_tail) {
                q_ready->q_tail = prev;
            }
            prev->next = node->next;

            liteco_free(liteco_container_of(liteco_ready_coroutine_t, node, node));
            return LITECO_SUCCESS;
        }

        prev = node;
        node = node->next;
    }

    return LITECO_NOT_FOUND;
}

int liteco_runtime_wait(liteco_runtime_t *const runtime, liteco_coroutine_t *const co, liteco_channel_t *const channels[], const u_int64_t timeout) {
    if (runtime == NULL || co == NULL) {
        return LITECO_PARAMETER_UNEXCEPTION;
    }
    co->runtime = runtime;
    co->active_channel = NULL;
    pthread_mutex_lock(&runtime->lock);
    if (channels != NULL) {
        liteco_wait_join(&runtime->q_wait, co, channels);
    }
    if (timeout != 0) {
        liteco_timer_join(&runtime->q_timer, co, timeout);
    }
    pthread_mutex_unlock(&runtime->lock);
    pthread_cond_signal(&runtime->cond);

    liteco_status_cas(co, LITECO_RUNNING, LITECO_WAITING);
    liteco_yield();
    return LITECO_SUCCESS;
}

int liteco_runtime_channel_notify(liteco_runtime_t *const runtime, liteco_channel_t *const channel) {
    int result = LITECO_SUCCESS;
    liteco_coroutine_t *co = NULL;
    if (runtime == NULL || channel == NULL) {
        return LITECO_PARAMETER_UNEXCEPTION;
    }
    pthread_mutex_lock(&runtime->lock);

    // 当 q_wait队列为空时，co设置为NULL
    liteco_wait_pop_spec(&co, &runtime->q_wait, channel);
    if (co != NULL) {
        liteco_timer_remove_spec(&runtime->q_timer, co);
        liteco_status_cas(co, LITECO_WAITING, LITECO_READYING);
        co->active_channel = channel;
        liteco_ready_join(&runtime->q_ready, co);
    }
    else {
        result = LITECO_EMPTY;
    }
    pthread_mutex_unlock(&runtime->lock);
    pthread_cond_signal(&runtime->cond);

    return result;
}

static inline u_int64_t __now__() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000 * 1000 + tv.tv_usec;
}

int liteco_runtime_schedule(liteco_runtime_t *const runtime) {
    u_int64_t timeout = 0;
    liteco_coroutine_t *co = NULL;
    int co_status = LITECO_UNKNOW;
    if (runtime == NULL) {
        return LITECO_PARAMETER_UNEXCEPTION;
    }
    pthread_mutex_lock(&runtime->lock);
    while (!liteco_link_empty(&runtime->q_timer) && liteco_timer_last(&runtime->q_timer) < __now__()) {
        liteco_timer_pop(&co, &runtime->q_timer);
        liteco_wait_remove_spec(&runtime->q_wait, co);

        liteco_status_cas(co, LITECO_WAITING, LITECO_READYING);
        liteco_ready_join(&runtime->q_ready, co);
    }

    co = NULL;
    while (liteco_link_empty(&runtime->q_ready)) {
        if (liteco_link_empty(&runtime->q_timer)) {
            pthread_cond_wait(&runtime->cond, &runtime->lock);
        }
        else {
            timeout = liteco_timer_last(&runtime->q_timer);
            if (timeout < __now__()) {
                liteco_timer_pop(&co, &runtime->q_timer);
                liteco_wait_remove_spec(&runtime->q_wait, co);

                liteco_status_cas(co, LITECO_WAITING, LITECO_READYING);
                liteco_ready_join(&runtime->q_ready, co);
            }
            else {
                struct timespec spec = { timeout / (1000 * 1000), timeout % (1000 * 1000) * 1000 };
                pthread_cond_timedwait(&runtime->cond, &runtime->lock, &spec);
            }
        }
    }
    liteco_ready_pop(&co, &runtime->q_ready);
    pthread_mutex_unlock(&runtime->lock);

    liteco_status_cas(co, LITECO_READYING, LITECO_RUNNING);
    co_status = liteco_resume(co);

    if (co_status == LITECO_READYING) {
        pthread_mutex_lock(&runtime->lock);
        liteco_ready_join(&runtime->q_ready, co);
        pthread_mutex_unlock(&runtime->lock);
    }

    return LITECO_SUCCESS;
}

int liteco_runtime_execute(liteco_runtime_t *const runtime, liteco_coroutine_t *const co) {
    int co_status = LITECO_UNKNOW;
    if (runtime == NULL || co == NULL) {
        return LITECO_PARAMETER_UNEXCEPTION;
    }

    pthread_mutex_lock(&runtime->lock);
    for ( ;; ) {
        if (liteco_ready_pop_spec(&runtime->q_ready, co) == LITECO_SUCCESS) {
            break;
        }
        else if (liteco_timer_exist(&runtime->q_timer, co)) {
            u_int64_t timeout = liteco_timer_spec(&runtime->q_timer, co);
            if (timeout < __now__()) {
                liteco_wait_remove_spec(&runtime->q_timer, co);
            }
            else {
                struct timespec spec = { timeout / (1000 * 1000), timeout % (1000 * 1000) * 1000 };
                pthread_cond_timedwait(&runtime->cond, &runtime->lock, &spec);

                liteco_status_cas(co, LITECO_WAITING, LITECO_READYING);
                liteco_ready_join(&runtime->q_ready, co);
            }
        }
        else {
            pthread_cond_wait(&runtime->cond, &runtime->lock);
        }
    }
    pthread_mutex_unlock(&runtime->lock);

    liteco_status_cas(co, LITECO_READYING, LITECO_RUNNING);
    co_status = liteco_resume(co);

    if (co_status == LITECO_READYING) {
        pthread_mutex_lock(&runtime->lock);
        liteco_ready_join(&runtime->q_ready, co);
        pthread_mutex_unlock(&runtime->lock);
    }

    return LITECO_SUCCESS;
}

int liteco_runtime_join(liteco_runtime_t *const runtime, liteco_coroutine_t *const co) {
    if (runtime == NULL || co == NULL) {
        return LITECO_PARAMETER_UNEXCEPTION;
    }
    co->runtime = runtime;
    liteco_status_cas(co, LITECO_STARTING, LITECO_READYING);

    pthread_mutex_lock(&runtime->lock);
    liteco_ready_join(&runtime->q_ready, co);
    pthread_mutex_unlock(&runtime->lock);

    return LITECO_SUCCESS;
}

int liteco_runtime_delay_join(liteco_runtime_t *const runtime, const u_int64_t timeout, liteco_coroutine_t *const co) {
    if (runtime == NULL || co == NULL) {
        return LITECO_PARAMETER_UNEXCEPTION;
    }
    co->runtime = runtime;
    liteco_status_cas(co, LITECO_STARTING, LITECO_WAITING);

    pthread_mutex_lock(&runtime->lock);
    liteco_timer_join(&runtime->q_timer, co, timeout);
    pthread_mutex_unlock(&runtime->lock);
    pthread_cond_signal(&runtime->cond);

    return LITECO_SUCCESS;
}

bool liteco_runtime_ready_empty(liteco_runtime_t *const runtime) {
    if (runtime == NULL) {
        return true;
    }

    return liteco_link_empty(&runtime->q_ready);
}

bool liteco_runtime_wait_empty(liteco_runtime_t *const runtime) {
    if (runtime == NULL) {
        return true;
    }

    return liteco_link_empty(&runtime->q_wait);
}

bool liteco_runtime_timer_empty(liteco_runtime_t *const runtime) {
    if (runtime == NULL) {
        return true;
    }

    return liteco_link_empty(&runtime->q_timer);
}

bool liteco_runtime_empty(liteco_runtime_t *const runtime) {
    return liteco_runtime_ready_empty(runtime)
        && liteco_runtime_wait_empty(runtime)
        && liteco_runtime_timer_empty(runtime);
}
