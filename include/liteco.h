#ifndef __LITECO_H__
#define __LITECO_H__

#include <stddef.h>
#include <pthread.h>
#include <sys/types.h>

typedef char liteco_internal_context_t[256];
typedef int liteco_boolean_t;

typedef struct liteco_link_node_s liteco_link_node_t;
typedef struct liteco_link_s liteco_link_t;

typedef struct liteco_coroutine_s liteco_coroutine_t;
typedef struct liteco_machine_s liteco_machine_t;

typedef struct liteco_channel_s liteco_channel_t;

struct liteco_coroutine_s {
    liteco_internal_context_t context;
    int status;
    liteco_internal_context_t **link;

    int (*fn) (liteco_coroutine_t *const, void *const);
    void *args;
    int (*finished_fn) (liteco_coroutine_t *const);
    size_t st_size;
    void *stack;
    int result;

    pthread_mutex_t mutex;

    liteco_machine_t *machine;
    liteco_channel_t *active_channel;
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

#define liteco_container_of(t, m, p) ((t *) ((char *) (p) - ((size_t) &(((t *) 0)->m))))

int liteco_create(liteco_coroutine_t *const co,
                  void *const stack, const size_t st_size,
                  int (*fn) (liteco_coroutine_t *const, void *const), void *const args,
                  int (*finished_fn) (liteco_coroutine_t *const co));
int liteco_resume(liteco_coroutine_t *const co);
int liteco_yield(liteco_coroutine_t *const co);

struct liteco_link_node_s {
    liteco_link_node_t *next;
};

struct liteco_link_s {
    liteco_link_node_t head;
    liteco_link_node_t *q_tail;
};

struct liteco_machine_s {
    liteco_link_t q_ready;
    liteco_link_t q_timer;
    liteco_link_t q_wait;

    pthread_cond_t cond;
    pthread_mutex_t lock;
};

int liteco_machine_init(liteco_machine_t *const machine);
int liteco_machine_wait(liteco_machine_t *const machine, liteco_coroutine_t *const co, liteco_channel_t *const channels[], const u_int64_t timeout);
int liteco_machine_channel_notify(liteco_machine_t *const machine, liteco_channel_t *const channel);
int liteco_machine_schedule(liteco_machine_t *const machine);
int liteco_machine_join(liteco_machine_t *const machine, liteco_coroutine_t *const co);

struct liteco_channel_s {
    liteco_boolean_t closed;

    liteco_link_t q_ele;
    liteco_link_t q_machines;

    pthread_mutex_t lock;
};

int liteco_channel_init(liteco_channel_t *const channel);
int liteco_channel_send(liteco_channel_t *const channel, const void *const element);
int liteco_channel_recv(const void **const ele, const liteco_channel_t **const channel,
                        liteco_machine_t *const machine,
                        liteco_coroutine_t *const co, liteco_channel_t *const channels[], const u_int64_t timeout);
int liteco_channel_close(liteco_channel_t *const channel);

extern liteco_channel_t __CLOSED_CHAN__;
extern __thread liteco_coroutine_t *__CURR_CO__;

#endif
