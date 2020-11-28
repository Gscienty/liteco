/*
 * Copyright (c) 2020 Gscienty <gaoxiaochuan@hotmail.com>
 *
 * Distributed under the MIT software license, see the accompanying
 * file LICENSE or https://www.opensource.org/licenses/mit-license.php .
 *
 * 本文件功能说明：
 *
 * 本文件主要是对等待通道的实现
 *
 * 1. liteco_channel_init
 *      初始化等待通道
 * 2. liteco_channel_send
 *      向等待通道发送一个消息事件
 * 3. liteco_channel_recv
 *      等待一个消息事件
 * 4. liteco_channel_close
 *      关闭等待通道
 */

#include "liteco.h"
#include "internal/link.h"
#include "internal/channel.h"
#include "internal/malloc.h"
#include <sys/time.h>

liteco_channel_t __CLOSED_CHAN__ = { .closed = LITECO_TRUE };

int liteco_channel_init(liteco_channel_t *const channel) {
    if (channel == NULL) {
        return LITECO_PARAMETER_UNEXCEPTION;
    }
    liteco_link_init(&channel->q_ele);
    liteco_link_init(&channel->q_runtimes);
    channel->closed = LITECO_FALSE;

    pthread_mutex_init(&channel->lock, NULL);

    return LITECO_SUCCESS;
}

static inline u_int64_t __now__() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000 * 1000 + tv.tv_usec;
}


// liteco_channel_recv是挂起当前运行的协程到运行时的等待队列中，
// 等待指定的等待通道发来的消息来唤起当前协程。
//
// 判断当前线程是否处于协程当中的依据是 __CURR_CO__不为NULL，
// 因此在执行liteco_channel_recv时，__CURR_CO__必须不为NULL，
// 即当前线程处于协程状态中。
int liteco_channel_recv(const void **const ele, const liteco_channel_t **const channel,
                        liteco_runtime_t *const runtime, liteco_channel_t *const channels[], const u_int64_t timeout) {

    bool finished = false;
    int result = LITECO_SUCCESS;
    liteco_link_node_t *node = NULL;
    liteco_channel_t *const *eachor_channel;

    if (__CURR_CO__ == NULL || runtime == NULL) {
        return LITECO_PARAMETER_UNEXCEPTION;
    }
    for ( ;; ) {
        // 当已经超过等待时间时，返回LITECO_TIMEOUT
        if (timeout != 0 && timeout <= __now__()) {
            return LITECO_TIMEOUT;
        }

        finished = false;
        for (eachor_channel = channels; *eachor_channel != NULL; eachor_channel++) {
            pthread_mutex_lock(&(*eachor_channel)->lock);
            // 当发现有等待队列已经关闭时，发回LITECO_CLOSED
            if ((*eachor_channel)->closed) {
                if (channel != NULL) {
                    *channel = *eachor_channel;
                }

                pthread_mutex_unlock(&(*eachor_channel)->lock);
                finished = true;
                result = LITECO_CLOSED;
                break;
            }

            // 当等待队列中的消息事件不为空时，获取消息事件队列中的队首事件，并返回LITECO_SUCCESS
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
                finished = true;
                result = LITECO_SUCCESS;
                break;
            }

            // 如果判断需要等待消息事件时
            // 将当前协程添加到当前的运行时中，并将运行时添加到每个等待通道的通知队列中
            // 并保证当前运行时等待通道的通知队列中唯一
            liteco_channel_waiting_runtime_join(&(*eachor_channel)->q_runtimes, runtime);
            pthread_mutex_unlock(&(*eachor_channel)->lock);
        }

        u_int64_t now = __now__();
        if (!finished && (timeout == 0 || timeout > now)) {
            u_int64_t ensure_timeout = __now__() + LITECO_TIMEOUT_MAX;
            if (timeout != 0 && timeout < ensure_timeout) {
                ensure_timeout = timeout;
            }
            liteco_runtime_wait(runtime, __CURR_CO__, channels, ensure_timeout);
        }

        for (eachor_channel = channels; *eachor_channel != NULL; eachor_channel++) {
            pthread_mutex_lock(&(*eachor_channel)->lock);
            liteco_channel_remove_spec(&(*eachor_channel)->q_runtimes, runtime);
            pthread_mutex_unlock(&(*eachor_channel)->lock);
        }

        if (finished) {
            return result;
        }
    }
}

int liteco_channel_waiting_runtime_join(liteco_link_t *const link, liteco_runtime_t *const runtime) {
    liteco_channel_link_node_t *node = NULL;
    if (link == NULL || runtime == NULL) {
        return LITECO_PARAMETER_UNEXCEPTION;
    }
    if ((node = liteco_malloc(sizeof(*node))) == NULL) {
        return LITECO_INTERNAL_ERROR;
    }
    node->runtime = runtime;
    liteco_link_push(link, &node->node);

    return LITECO_SUCCESS;
}

int liteco_channel_remove_spec(liteco_link_t *const link, liteco_runtime_t *const runtime) {
    liteco_link_node_t *prev = NULL;
    liteco_link_node_t *node = NULL;
    if (link == NULL || runtime == NULL) {
        return LITECO_PARAMETER_UNEXCEPTION;
    }
    prev = &link->head;
    node = link->head.next;
    while (node != &link->head) {
        if (liteco_container_of(liteco_channel_link_node_t, node, node)->runtime == runtime) {
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

int liteco_channel_send(liteco_channel_t *const channel, const void *const element) {
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
    for (node = channel->q_runtimes.head.next; node != &channel->q_runtimes.head; node = node->next) {
        if (liteco_runtime_channel_notify(liteco_container_of(liteco_channel_link_node_t, node, node)->runtime, channel) == LITECO_SUCCESS) {
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
    for (node = channel->q_runtimes.head.next; node != &channel->q_runtimes.head; node = node->next) {
        while (liteco_runtime_channel_notify(liteco_container_of(liteco_channel_link_node_t, node, node)->runtime, channel) == LITECO_SUCCESS);
    }
    pthread_mutex_unlock(&channel->lock);

    return LITECO_SUCCESS;
}
