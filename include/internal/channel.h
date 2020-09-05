/*
 * Copyright (c) 2020 Gscienty <gaoxiaochuan@hotmail.com>
 *
 * Distributed under the MIT software license, see the accompanying
 * file LICENSE or https://www.opensource.org/licenses/mit-license.php .
 *
 */

#ifndef __LITECO_CHANNEL_H__
#define __LITECO_CHANNEL_H__

#include "liteco.h"

typedef struct liteco_channel_link_node_s liteco_channel_link_node_t;
struct liteco_channel_link_node_s {
    liteco_link_node_t node;
    liteco_runtime_t *runtime;
};

typedef struct liteco_channel_element_link_node_s liteco_channel_element_link_node_t;
struct liteco_channel_element_link_node_s {
    liteco_link_node_t node;
    const void *element;
};

int liteco_channel_waiting_runtime_join(liteco_link_t *const link, liteco_runtime_t *const runtime);
int liteco_channel_remove_spec(liteco_link_t *const link, liteco_runtime_t *const runtime);

#endif
