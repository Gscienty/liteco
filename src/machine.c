#include "liteco.h"
#include "internal/link.h"
#include "internal/machine.h"
#include "internal/couroutine.h"
#include "internal/malloc.h"


int liteco_machine_init(liteco_machine_t *const machine) {
    if (machine == NULL) {
        return LITECO_PARAMETER_UNEXCEPTION;
    }

    liteco_link_init(&machine->q_wait);
    liteco_link_init(&machine->q_ready);
    liteco_link_init(&machine->q_timer);

    pthread_cond_init(&machine->cond, NULL);
    pthread_mutex_init(&machine->lock, NULL);

    return LITECO_SUCCESS;
}

int liteco_timer_join(liteco_link_t *const q_timer, liteco_coroutine_t *const co, const u_int64_t timeout) {
    liteco_link_node_t *prev = NULL;
    liteco_link_node_t *node = NULL;
    liteco_timer_coroutine_t *timer_node = NULL;
    if (q_timer == NULL || co == NULL) {
        return LITECO_PARAMETER_UNEXCEPTION;
    }

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
            break;
        }

        prev = node;
        node = node->next;
    }

    if (timer_node->node.next == NULL) {
        q_timer->q_tail->next = &timer_node->node;
        timer_node->node.next = &q_timer->head;
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

int liteco_wait_join(liteco_link_t *const q_wait, liteco_coroutine_t *const co, liteco_channel_t *const channels[]) {
    liteco_wait_coroutine_t *node = NULL;
    if (q_wait == NULL || co == NULL || channels == NULL) {
        return LITECO_PARAMETER_UNEXCEPTION;
    }
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

int liteco_machine_wait(liteco_machine_t *const machine, liteco_coroutine_t *const co, liteco_channel_t *const channels[], const u_int64_t timeout) {
    if (machine == NULL || co == NULL) {
        return LITECO_PARAMETER_UNEXCEPTION;
    }
    co->machine = machine;
    co->active_channel = NULL;
    pthread_mutex_lock(&machine->lock);
    if (channels != NULL) {
        liteco_wait_join(&machine->q_wait, co, channels);
    }
    if (timeout != 0) {
        liteco_timer_join(&machine->q_timer, co, timeout);
    }
    pthread_mutex_unlock(&machine->lock);
    pthread_cond_signal(&machine->cond);

    liteco_status_cas(co, LITECO_RUNNING, LITECO_WAITING);
    liteco_yield(co);
    return LITECO_SUCCESS;
}

int liteco_machine_channel_notify(liteco_machine_t *const machine, liteco_channel_t *const channel) {
    liteco_coroutine_t *co = NULL;
    if (machine == NULL || channel == NULL) {
        return LITECO_PARAMETER_UNEXCEPTION;
    }
    pthread_mutex_lock(&machine->lock);
    liteco_wait_pop_spec(&co, &machine->q_wait, channel);
    if (co != NULL) {
        liteco_timer_remove_spec(&machine->q_timer, co);
        liteco_status_cas(co, LITECO_WAITING, LITECO_READYING);
        co->active_channel = channel;
        liteco_ready_join(&machine->q_ready, co);
        pthread_mutex_unlock(&machine->lock);
        pthread_cond_signal(&machine->cond);
        return LITECO_SUCCESS;
    }
    pthread_mutex_unlock(&machine->lock);
    pthread_cond_signal(&machine->cond);

    return LITECO_EMPTY;
}

static inline u_int64_t __now__() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000 * 1000 + tv.tv_usec;
}

int liteco_machine_schedule(liteco_machine_t *const machine) {
    u_int64_t timeout = 0;
    liteco_coroutine_t *co = NULL;
    if (machine == NULL) {
        return LITECO_PARAMETER_UNEXCEPTION;
    }
    u_int64_t now = __now__();
    pthread_mutex_lock(&machine->lock);
    while (!liteco_link_empty(&machine->q_timer) && liteco_timer_last(&machine->q_timer) < now) {
        liteco_timer_pop(&co, &machine->q_timer);
        liteco_status_cas(co, LITECO_WAITING, LITECO_READYING);
        liteco_ready_join(&machine->q_ready, co);
    }

    co = NULL;
    while (liteco_link_empty(&machine->q_ready)) {
        if (liteco_link_empty(&machine->q_timer)) {
            pthread_cond_wait(&machine->cond, &machine->lock);
        }
        else {
            timeout = liteco_timer_last(&machine->q_timer);
            if (timeout < __now__()) {
                liteco_timer_pop(&co, &machine->q_timer);
                liteco_status_cas(co, LITECO_WAITING, LITECO_READYING);
                liteco_ready_join(&machine->q_ready, co);
            }
            else {
                struct timespec spec = { timeout / (1000 * 1000), timeout % (1000 * 1000) * 1000 };
                pthread_cond_timedwait(&machine->cond, &machine->lock, &spec);
            }
        }
    }
    liteco_ready_pop(&co, &machine->q_ready);
    pthread_mutex_unlock(&machine->lock);

    liteco_status_cas(co, LITECO_READYING, LITECO_RUNNING);
    liteco_resume(co);

    if (co->status == LITECO_TERMINATE) {
        if (co->finished_fn != NULL) {
            co->finished_fn(co);
        }
    }
    else if (co->status == LITECO_RUNNING) {
        liteco_status_cas(co, LITECO_RUNNING, LITECO_READYING);
        liteco_ready_join(&machine->q_ready, co);
    }

    return LITECO_SUCCESS;
}

int liteco_machine_join(liteco_machine_t *const machine, liteco_coroutine_t *const co) {
    if (machine == NULL || co == NULL) {
        return LITECO_PARAMETER_UNEXCEPTION;
    }
    co->machine = machine;
    liteco_status_cas(co, LITECO_STARTING, LITECO_READYING);

    pthread_mutex_lock(&machine->lock);
    liteco_ready_join(&machine->q_ready, co);
    pthread_mutex_unlock(&machine->lock);

    return LITECO_SUCCESS;
}
