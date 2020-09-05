/*
 * Copyright (c) 2020 Gscienty <gaoxiaochuan@hotmail.com>
 *
 * Distributed under the MIT software license, see the accompanying
 * file LICENSE or https://www.opensource.org/licenses/mit-license.php .
 *
 * 本文件功能说明：
 *
 * 本文件主要是对协程功能的实现
 *
 * 1. liteco_create
 *      创建一个协程
 * 2. liteco_resume
 *      当前线程运行上下文切换为该协程
 * 3. liteco_yield
 *      将协程切出，将上下文切换回link的运行上下文中
 * 4. liteco_internal_context_swap
 *      切换上下文
 * 5. liteco_internal_context_make
 *      构造协程上下文
 * 6. liteco_internal_atomic_cas
 *      更新协程状态（compare and swap）
 *
 */

#include "liteco.h"
#include <stddef.h>
#include <sys/types.h>
#include <sys/time.h>

__thread liteco_coroutine_t *__CURR_CO__ = NULL;

static int liteco_callback(void *const);

extern int liteco_internal_context_swap(liteco_internal_context_t *const from, liteco_internal_context_t *const to);
extern int liteco_internal_context_make(liteco_internal_context_t *const ctx, void *stack, size_t st_size, int (*fn) (void *const), void *const args);
extern int liteco_internal_atomic_cas(int *const ptr, const int old, const int nex);
extern int liteco_internal_p_yield(const int n);

int liteco_create(liteco_coroutine_t *const co,
                  void *const stack, size_t st_size,
                  int (*fn) (void *const), void *const args, int (*finished_fn) (liteco_coroutine_t *const)) {
    if (co == NULL || stack == NULL || fn == NULL) {
        return LITECO_PARAMETER_UNEXCEPTION;
    }
    co->fn = fn;
    co->args = args;
    co->status = LITECO_STARTING;
    co->link = (liteco_internal_context_t **) ((((unsigned long) (stack + st_size) - 8) & 0xfffffffffffffff0));
    co->finished_fn = finished_fn;
    co->st_size = st_size;
    co->stack = stack;
    pthread_mutex_init(&co->mutex, NULL);
    liteco_internal_context_make(&co->context, stack, st_size, liteco_callback, co);
    *co->link = NULL;
    co->result = 0;

    return LITECO_SUCCESS;
}

int liteco_resume(liteco_coroutine_t *const co) {
    static __thread liteco_internal_context_t this_context;
    if (co == NULL) {
        return LITECO_PARAMETER_UNEXCEPTION;
    }
    pthread_mutex_lock(&co->mutex);
    *co->link = &this_context;
    pthread_mutex_unlock(&co->mutex);

    __CURR_CO__ = co;
    liteco_internal_context_swap(&this_context, &co->context);

    return LITECO_SUCCESS;
}

int liteco_yield() {
    liteco_coroutine_t *co = __CURR_CO__;
    if (__CURR_CO__ == NULL) {
        return LITECO_PARAMETER_UNEXCEPTION;
    }
    __CURR_CO__ = NULL;
    liteco_internal_context_swap(&co->context, *co->link);
    return LITECO_SUCCESS;
}

static int liteco_callback(void *const co_) {
    liteco_coroutine_t *const co = co_;
    if (co_ == NULL) {
        return LITECO_PARAMETER_UNEXCEPTION;
    }
    co->result = co->fn(co->args);
    pthread_mutex_lock(&co->mutex);
    co->status = LITECO_TERMINATE;
    pthread_mutex_unlock(&co->mutex);

    return liteco_yield();
}

static inline u_int64_t __now__() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000 * 1000 + tv.tv_usec;
}

int liteco_status_cas(liteco_coroutine_t *const co, int old, int nex) {
    int i;
    int x;
    u_int64_t next_yield;
    if (co == NULL) {
        return LITECO_PARAMETER_UNEXCEPTION;
    }

    for (i = 0; !liteco_internal_atomic_cas(&co->status, old, nex); i++) {
        if (old == LITECO_WAITING && co->status == LITECO_RUNNING) {
            return LITECO_INTERNAL_ERROR;
        }

        if (i == 0) {
            next_yield = __now__() + 5;
        }

        if (__now__() < next_yield) {
            for (x = 0; x < 10 && co->status != old; x++) {
                liteco_internal_p_yield(1);
            }
        }
        else {
            sched_yield();

            next_yield = __now__() + 2;
        }
    }

    return LITECO_SUCCESS;
}

