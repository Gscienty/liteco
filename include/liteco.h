#ifndef __LITECO_H__
#define __LITECO_H__

#include <stddef.h>

typedef char liteco_internal_context_t[256];

typedef struct liteco_link_node_s liteco_link_node_t;
typedef struct liteco_link_s liteco_link_t;
typedef struct liteco_schedule_s liteco_schedule_t;
typedef struct liteco_coroutine_s liteco_coroutine_t;
typedef struct liteco_channel_s liteco_channel_t;

struct liteco_coroutine_s {
    liteco_internal_context_t context;
    int status;
    liteco_internal_context_t **link;
    liteco_schedule_t *sche;

    int (*fn) (liteco_coroutine_t *const, void *const);
    void *args;
};

#define LITECO_PARAMETER_UNEXCEPTION    -1
#define LITECO_COROUTINE_JOINED         -2
#define LITECO_INTERNAL_ERROR           -3
#define LITECO_EMPTY                    -4
#define LITECO_CLOSED                   -5
#define LITECO_SUCCESS                  0

#define LITECO_UNKNOW       0x00
#define LITECO_STARTING     0x01
#define LITECO_READYING     0x02
#define LITECO_RUNNING      0x03
#define LITECO_WAITING      0x04
#define LITECO_TERMINATE    0x05

typedef int liteco_boolean_t;
#define LITECO_TRUE     1
#define LITECO_FALSE    2


int liteco_status(const liteco_coroutine_t *const co);
int liteco_create(liteco_coroutine_t *const co, void *const stack, size_t st_size, int (*fn) (liteco_coroutine_t *const, void *const), void *const args);
int liteco_resume(liteco_coroutine_t *const co);
int liteco_yield(liteco_coroutine_t *const co);

struct liteco_link_node_s {
    liteco_link_node_t *next;
    void *value;
};

struct liteco_link_s {
    liteco_link_node_t head;
    liteco_link_node_t *q_tail;
};

struct liteco_schedule_s {
    liteco_link_t link;
};

int liteco_schedule_init(liteco_schedule_t *const sche);
int liteco_schedule_join(liteco_schedule_t *const sche, liteco_coroutine_t *const co);
int liteco_schedule_pop(liteco_coroutine_t **const co, liteco_schedule_t *const sche);
liteco_boolean_t liteco_schedule_empty(liteco_schedule_t *const sche);

struct liteco_channel_s {
    liteco_boolean_t closed;
    liteco_link_t events;
    liteco_link_t waiting_co;
};

int liteco_channel_init(liteco_channel_t *const channel);
int liteco_channel_subscribe(void **const event, liteco_channel_t **const channel, liteco_coroutine_t *const co, ...);
int liteco_channel_publish(liteco_coroutine_t **const co, liteco_channel_t *const channel, void *const event);

extern liteco_channel_t __LITECO_CLOSED_CHANNEL__;

#endif
