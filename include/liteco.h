#ifndef __LITECO_H__
#define __LITECO_H__

#include <stddef.h>
#include <pthread.h>

typedef char liteco_internal_context_t[256];
typedef int liteco_boolean_t;

typedef struct liteco_link_node_s liteco_link_node_t;
typedef struct liteco_link_s liteco_link_t;
typedef struct liteco_schedule_s liteco_schedule_t;
typedef struct liteco_coroutine_s liteco_coroutine_t;
typedef struct liteco_channel_s liteco_channel_t;

struct liteco_coroutine_s {
    liteco_internal_context_t context;
    int status;
    liteco_internal_context_t **link;

    int (*fn) (liteco_coroutine_t *const, void *const);
    void *args;
    int (*release) (void *const, const size_t);
    size_t st_size;
    void *stack;

    liteco_schedule_t *sche;

    pthread_mutex_t mutex;

    int ref_count;
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

#define LITECO_TRUE     1
#define LITECO_FALSE    0


int liteco_create(liteco_coroutine_t *const co,
                  void *const stack, const size_t st_size,
                  int (*fn) (liteco_coroutine_t *const, void *const), void *const args,
                  int (*release) (void *const, const size_t));
int liteco_resume(liteco_coroutine_t *const co);
int liteco_yield(liteco_coroutine_t *const co);
int liteco_kill(liteco_coroutine_t *const co);

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

    pthread_mutex_t mutex;
};

int liteco_schedule_init(liteco_schedule_t *const sche);
int liteco_schedule_join(liteco_schedule_t *const sche, liteco_coroutine_t *const co);
int liteco_schedule_pop(liteco_coroutine_t **const co, liteco_schedule_t *const sche);
int liteco_schedule_remove_spec(liteco_schedule_t *const sche, liteco_coroutine_t *const co);
liteco_boolean_t liteco_schedule_empty(liteco_schedule_t *const sche);

struct liteco_channel_s {
    liteco_boolean_t closed;
    liteco_link_t events;
    liteco_schedule_t waiting_co;

    pthread_mutex_t mutex;
};

int liteco_channel_init(liteco_channel_t *const channel);
int liteco_channel_subscribe(void **const event, liteco_channel_t **const channel, liteco_coroutine_t *const co, ...);
int liteco_channel_publish(liteco_channel_t *const channel, void *const event);
int liteco_channel_pop(liteco_coroutine_t **const co, liteco_channel_t *const channel);

extern liteco_channel_t __LITECO_CLOSED_CHANNEL__;

#endif
