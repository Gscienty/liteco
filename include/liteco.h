#ifndef __LITECO_H__
#define __LITECO_H__

#include <stddef.h>

#define __co__ liteco_coroutine_t *const __lite_co__
#define __yield__ liteco_yield(__lite_co__)

typedef struct liteco_internal_context_s liteco_internal_context_t;
struct liteco_internal_context_s {
    char regs[256];
} __attribute__((packed));

typedef struct liteco_coroutine_s liteco_coroutine_t;
struct liteco_coroutine_s {
    liteco_internal_context_t context;
    int status;
    liteco_internal_context_t **swap_link;

    int (*fn) (__co__, void *const);
    void *args;
};

#define LITECO_PARAMETER_UNEXCEPTION -1
#define LITECO_SUCCESS 0

#define LITECO_UNKNOW       0x00
#define LITECO_STARTING     0x01
#define LITECO_READYING     0x02
#define LITECO_RUNNING      0x03
#define LITECO_WAITING      0x04
#define LITECO_TERMINATE    0x05


int liteco_status(const liteco_coroutine_t *const co);
int liteco_create(liteco_coroutine_t *const co, void *const stack, size_t st_size, int (*fn) (__co__, void *const), void *const args);
int liteco_resume(liteco_coroutine_t *const co);
int liteco_yield(liteco_coroutine_t *const co);


#endif
