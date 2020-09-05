/*
 * Copyright (c) 2020 Gscienty <gaoxiaochuan@hotmail.com>
 *
 * Distributed under the MIT software license, see the accompanying
 * file LICENSE or https://www.opensource.org/licenses/mit-license.php .
 *
 * 本文件功能说明：
 *
 * 链表实现
 *
 * 1. liteco_link_init
 *      初始化链表
 * 2. liteco_link_push
 *      添加元素到链表末端
 * 3. liteco_link_pop
 *      删除链表头部的元素
 * 4. liteco_link_empty
 *      判断链表是否为空
 *
 */

#include "liteco.h"

int liteco_link_init(liteco_link_t *const link) {
    if (link == NULL) {
        return LITECO_PARAMETER_UNEXCEPTION;
    }
    link->head.next = &link->head;
    link->q_tail = &link->head;

    return LITECO_SUCCESS;
}

int liteco_link_push(liteco_link_t *const link, liteco_link_node_t *const node) {
    if (link == NULL) {
        return LITECO_PARAMETER_UNEXCEPTION;
    }
    node->next = &link->head;

    link->q_tail->next = node;
    link->q_tail = node;

    return LITECO_SUCCESS;
}

int liteco_link_pop(liteco_link_node_t **const node, liteco_link_t *const link) {
    if (link == NULL) {
        return LITECO_PARAMETER_UNEXCEPTION;
    }
    *node = NULL;
    if (link->q_tail == &link->head) {
        return LITECO_EMPTY;
    }

    *node = link->head.next;
    link->head.next = (*node)->next;
    if (*node == link->q_tail) {
        link->q_tail = &link->head;
    }

    return LITECO_SUCCESS;
}

liteco_boolean_t liteco_link_empty(liteco_link_t *const link) {
    if (link == NULL) {
        return LITECO_TRUE;
    }
    
    return &link->head == link->q_tail;
}

