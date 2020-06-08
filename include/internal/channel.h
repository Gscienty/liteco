#ifndef __LITECO_CHANNEL_H__
#define __LITECO_CHANNEL_H__

#include "liteco.h"

typedef struct liteco_channel_link_node_s liteco_channel_link_node_t;
struct liteco_channel_link_node_s {
    liteco_link_node_t node;
    liteco_machine_t *machine;
};

typedef struct liteco_channel_element_link_node_s liteco_channel_element_link_node_t;
struct liteco_channel_element_link_node_s {
    liteco_link_node_t node;
    void *element;
};

int liteco_channel_waiting_machine_join(liteco_link_t *const link, liteco_machine_t *const machine);
int liteco_channel_remove_spec(liteco_link_t *const link, liteco_machine_t *const machine);

#endif
