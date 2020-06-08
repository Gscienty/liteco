#ifndef __LITECO_LINK_H__
#define __LITECO_LINK_H__

#include "liteco.h"

int liteco_link_init(liteco_link_t *const link);
int liteco_link_push(liteco_link_t *const link, liteco_link_node_t *const node);
int liteco_link_pop(liteco_link_node_t **const node, liteco_link_t *const link);
liteco_boolean_t liteco_link_empty(liteco_link_t *const link);

#endif
