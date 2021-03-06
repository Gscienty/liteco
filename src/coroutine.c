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
#include "internal/couroutine.h"
#include <stddef.h>
#include <sys/types.h>
#include <sys/time.h>
#include <string.h>

__thread liteco_coroutine_t *__CURR_CO__ = NULL;

static int liteco_callback(void *const);

extern int liteco_internal_context_swap(liteco_internal_context_t const from, liteco_internal_context_t const to);
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
    co->finished_fn = finished_fn;
    co->st_size = st_size;
    co->stack = stack;
    memset(co->context, 0, sizeof(co->context));
    pthread_mutex_init(&co->mutex, NULL);
    liteco_internal_context_make(&co->context, stack, st_size, liteco_callback, co);
    co->result = 0;

    return LITECO_SUCCESS;
}

int liteco_resume(liteco_coroutine_t *const co) {
    liteco_coroutine_t *remember_current_co = NULL;
    static __thread liteco_internal_context_t thread_context;
    if (co == NULL) {
        return LITECO_PARAMETER_UNEXCEPTION;
    }
    pthread_mutex_lock(&co->mutex);
    if (co->status == LITECO_TERMINATE) {
        pthread_mutex_unlock(&co->mutex);
        return LITECO_TERMINATE;
    }
    if (co->status == LITECO_STARTING) {
        co->status = LITECO_READYING;
    }

    remember_current_co = __CURR_CO__;

    co->link = remember_current_co == NULL ? &thread_context : &remember_current_co->context;
    pthread_mutex_unlock(&co->mutex);

    __CURR_CO__ = co;
    liteco_status_cas(co, LITECO_READYING, LITECO_RUNNING);

    liteco_internal_context_swap(*co->link, co->context);

    __CURR_CO__ = remember_current_co;

    if (co->status == LITECO_TERMINATE) {
        if (co->finished_fn != NULL) {
            co->finished_fn(co);
        }
        return LITECO_TERMINATE;
    }

    return co->status;
}

int liteco_yield() {
    liteco_coroutine_t *co = __CURR_CO__;
    if (co == NULL) {
        return LITECO_PARAMETER_UNEXCEPTION;
    }

    if (co->status == LITECO_RUNNING) {
        co->status = LITECO_READYING;
    }
    __CURR_CO__ = NULL;
    liteco_internal_context_swap(co->context, *co->link);

    return LITECO_SUCCESS;
}

static int liteco_callback(void *const co_) {
    liteco_coroutine_t *const co = co_;
    if (co_ == NULL) {
        return LITECO_PARAMETER_UNEXCEPTION;
    }
    co->result = co->fn(co->args);
    liteco_status_cas(co, LITECO_RUNNING, LITECO_TERMINATE);
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

