/*
 * Copyright (c) 2020 Gscienty <gaoxiaochuan@hotmail.com>
 *
 * Distributed under the MIT software license, see the accompanying
 * file LICENSE or https://www.opensource.org/licenses/mit-license.php .
 *
 */

#ifndef __LITECO_COROUTINE_H__
#define __LITECO_COROUTINE_H__

#include "liteco.h"

typedef struct liteco_coroutine_link_node_s liteco_coroutine_link_node_t;
struct liteco_coroutine_link_node_s {
    liteco_link_node_t node;
    liteco_coroutine_t *co;
};

int liteco_status_cas(liteco_coroutine_t *const co, int old, int nex);

#endif
