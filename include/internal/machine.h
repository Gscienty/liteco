#ifndef __LITECO_MACHINE_H__
#define __LITECO_MACHINE_H__

#include "liteco.h"
#include <sys/types.h>
#include <sys/time.h>

typedef struct liteco_timer_coroutine_s liteco_timer_coroutine_t;
struct liteco_timer_coroutine_s {
    liteco_link_node_t node;
    u_int64_t timeout;
    liteco_coroutine_t *co;
};

int liteco_timer_join(liteco_link_t *const q_timer, liteco_coroutine_t *const co, const u_int64_t timeout);
u_int64_t liteco_timer_last(liteco_link_t *const q_timer);
int liteco_timer_pop(liteco_coroutine_t **const co, liteco_link_t *const q_timer);
int liteco_timer_remove_spec(liteco_link_t *const q_timer, liteco_coroutine_t *const co);

typedef struct liteco_wait_coroutine_s liteco_wait_coroutine_t;
struct liteco_wait_coroutine_s {
    liteco_link_node_t node;
    liteco_coroutine_t *co;
    liteco_channel_t *const *channels;
};

int liteco_wait_join(liteco_link_t *const q_wait, liteco_coroutine_t *const co, liteco_channel_t *const channels[]);
int liteco_wait_pop_spec(liteco_coroutine_t **const co, liteco_link_t *const q_wait, liteco_channel_t *const channel);
int liteco_wait_remove_spec(liteco_link_t *const q_wait, liteco_coroutine_t *const co);

typedef struct liteco_ready_coroutine_s liteco_ready_coroutine_t;
struct liteco_ready_coroutine_s {
    liteco_link_node_t node;
    liteco_coroutine_t *co;
};

int liteco_ready_join(liteco_link_t *const q_ready, liteco_coroutine_t *const co);
int liteco_ready_pop(liteco_coroutine_t **const co, liteco_link_t *const q_ready);

#endif
