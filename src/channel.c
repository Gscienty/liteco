#include "liteco.h"
#include "internal/link.h"
#include "internal/channel.h"
#include "internal/malloc.h"
#include <stdio.h>

liteco_channel_t __CLOSED_CHAN__ = { .closed = LITECO_TRUE };

int liteco_channel_init(liteco_channel_t *const channel) {
    if (channel == NULL) {
        return LITECO_PARAMETER_UNEXCEPTION;
    }
    liteco_link_init(&channel->q_ele);
    liteco_link_init(&channel->q_machines);
    channel->closed = LITECO_FALSE;

    pthread_mutex_init(&channel->lock, NULL);

    return LITECO_SUCCESS;
}

int liteco_channel_recv(void **const ele, liteco_channel_t **const channel,
                        liteco_machine_t *const machine,
                        liteco_coroutine_t *const co, liteco_channel_t *const channels[], const u_int64_t timeout) {
    liteco_link_node_t *node = NULL;
    liteco_channel_t *const *eachor_channel;
    if (co == NULL || machine == NULL) {
        return LITECO_PARAMETER_UNEXCEPTION;
    }
    for ( ;; ) {
        for (eachor_channel = channels; *eachor_channel != NULL; eachor_channel++) {
            pthread_mutex_lock(&(*eachor_channel)->lock);
            if ((*eachor_channel)->closed) {
                if (channel != NULL) {
                    *channel = *eachor_channel;
                }
                pthread_mutex_unlock(&(*eachor_channel)->lock);
                return LITECO_CLOSED;
            }
            if (!liteco_link_empty(&(*eachor_channel)->q_ele)) {
                if (channel != NULL) {
                    *channel = *eachor_channel;
                }
                liteco_link_pop(&node, &(*eachor_channel)->q_ele);
                if (ele != NULL) {
                    *ele = liteco_container_of(liteco_channel_element_link_node_t, node, node)->element;
                }
                liteco_free(liteco_container_of(liteco_channel_element_link_node_t, node, node));

                pthread_mutex_unlock(&(*eachor_channel)->lock);
                return LITECO_SUCCESS;
            }
            pthread_mutex_unlock(&(*eachor_channel)->lock);
        }

        for (eachor_channel = channels; *eachor_channel != NULL; eachor_channel++) {
            pthread_mutex_lock(&(*eachor_channel)->lock);
            liteco_channel_waiting_machine_join(&(*eachor_channel)->q_machines, machine);
            pthread_mutex_unlock(&(*eachor_channel)->lock);
        }

        liteco_machine_wait(machine, co, channels, timeout);

        for (eachor_channel = channels; *eachor_channel != NULL; eachor_channel++) {
            pthread_mutex_lock(&(*eachor_channel)->lock);
            liteco_channel_remove_spec(&(*eachor_channel)->q_machines, machine);
            pthread_mutex_unlock(&(*eachor_channel)->lock);
        }
    }
}

int liteco_channel_waiting_machine_join(liteco_link_t *const link, liteco_machine_t *const machine) {
    liteco_channel_link_node_t *node = NULL;
    if (link == NULL || machine == NULL) {
        return LITECO_PARAMETER_UNEXCEPTION;
    }
    if ((node = liteco_malloc(sizeof(*node))) == NULL) {
        return LITECO_INTERNAL_ERROR;
    }
    node->machine = machine;
    liteco_link_push(link, &node->node);

    return LITECO_SUCCESS;
}

int liteco_channel_remove_spec(liteco_link_t *const link, liteco_machine_t *const machine) {
    liteco_link_node_t *prev = NULL;
    liteco_link_node_t *node = NULL;
    if (link == NULL || machine == NULL) {
        return LITECO_PARAMETER_UNEXCEPTION;
    }
    prev = &link->head;
    node = link->head.next;
    while (node != &link->head) {
        if (liteco_container_of(liteco_channel_link_node_t, node, node)->machine == machine) {
            if (node == link->q_tail) {
                link->q_tail = prev;
            }
            prev->next = node->next;
            liteco_free(liteco_container_of(liteco_channel_link_node_t, node, node));
            node = prev->next;
        }
        else {
            prev = node;
            node = node->next;
        }
    }

    return LITECO_SUCCESS;
}

int liteco_channel_send(liteco_channel_t *const channel, void *const element) {
    liteco_link_node_t *node = NULL;
    liteco_channel_element_link_node_t *ele_node = NULL;
    if (channel == NULL || element == NULL) {
        return LITECO_PARAMETER_UNEXCEPTION;
    }
    if ((ele_node = liteco_malloc(sizeof(*ele_node))) == NULL) {
        return LITECO_INTERNAL_ERROR;
    }
    ele_node->element = element;
    pthread_mutex_lock(&channel->lock);
    liteco_link_push(&channel->q_ele, &ele_node->node);
    for (node = channel->q_machines.head.next; node != &channel->q_machines.head; node = node->next) {
        if (liteco_machine_channel_notify(liteco_container_of(liteco_channel_link_node_t, node, node)->machine, channel) == LITECO_SUCCESS) {
            break;
        }
    }
    pthread_mutex_unlock(&channel->lock);

    return LITECO_SUCCESS;
}

int liteco_channel_close(liteco_channel_t *const channel) {
    liteco_link_node_t *node = NULL;
    if (channel == NULL) {
        return LITECO_PARAMETER_UNEXCEPTION;
    }
    pthread_mutex_lock(&channel->lock);
    channel->closed = LITECO_TRUE;
    for (node = channel->q_machines.head.next; node != &channel->q_machines.head; node = node->next) {
        while (liteco_machine_channel_notify(liteco_container_of(liteco_channel_link_node_t, node, node)->machine, channel) == LITECO_SUCCESS);
    }
    pthread_mutex_unlock(&channel->lock);

    return LITECO_SUCCESS;
}
