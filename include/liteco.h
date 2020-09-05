/*
 * Copyright (c) 2020 Gscienty <gaoxiaochuan@hotmail.com>
 *
 * Distributed under the MIT software license, see the accompanying
 * file LICENSE or https://www.opensource.org/licenses/mit-license.php .
 *
 * 本文件功能说明：
 *
 * 本文件声明了实现协程模型的三个元素：协程、运行时、等待通道。
 * 1. 协程(liteco_coroutine_t)：具体可执行的可执行体
 * 2. 运行时(liteco_machine_t)：协程具体执行时依附的运行时平台
 * 3. 等待通道(liteco_channel_t)：该模块用于存储当前挂起的协程所在的运行时
 */

#ifndef __LITECO_H__
#define __LITECO_H__

#include <stddef.h>
#include <pthread.h>
#include <sys/types.h>

/*
 * 协程的上下文，用于暂存各种CPU寄存器的内容。
 * 针对各种种类的CPU，暂存的内容各有不同：
 * x86_64:
 *      偏移量 8   (占64 bits) R8，根据C语言在x86_64的函数编译规则，该寄存器寄存第五个参数
 *      偏移量 16  (占64 bits) R9，该寄存器寄存第六个参数
 *      偏移量 24  (占64 bits) R10
 *      偏移量 32  (占64 bits) R11
 *      偏移量 40  (占64 bits) R12
 *      偏移量 48  (占64 bits) R13
 *      偏移量 56  (占64 bits) R14
 *      偏移量 64  (占64 bits) R15
 *      偏移量 72  (占64 bits) RDI，该寄存器寄存第一个参数
 *      偏移量 80  (占64 bits) RSI，该寄存器寄存第二个参数
 *      偏移量 88  (占64 bits) RBP
 *      偏移量 96  (占64 bits) RBX，基址地址，初始化的值为申请的栈空间栈底的地址
 *      偏移量 104 (占64 bits) RDX，该寄存器寄存第三个参数
 *      偏移量 112 (占64 bits) RAX，返回值
 *      偏移量 120 (占64 bits) RCX，该寄存器寄存第三个参数
 *      偏移量 128 (占64 bits) RSP，栈指针
 *      偏移量 136 (占64 bits) 存储起始函数的地址
 *
 */
typedef char liteco_internal_context_t[256];
typedef int liteco_boolean_t;

typedef struct liteco_link_node_s liteco_link_node_t;
typedef struct liteco_link_s liteco_link_t;

typedef struct liteco_coroutine_s liteco_coroutine_t;
typedef struct liteco_machine_s liteco_machine_t;
typedef struct liteco_channel_s liteco_channel_t;

// 协程
struct liteco_coroutine_s {
    // 协程上下文
    liteco_internal_context_t context;

    // 协程状态
    int status;
    // 关联协程
    liteco_internal_context_t **link;

    // 协程起始函数
    int (*fn) (void *const);
    // 协程起始函数参数
    void *args;

    // 协程结束时执行的收尾函数
    int (*finished_fn) (liteco_coroutine_t *const);

    // 协程栈大小
    size_t st_size;
    // 协程栈
    void *stack;

    // 协程执行最终返回值
    int result;

    // 互斥锁
    pthread_mutex_t mutex;

    // 协程所在的运行时
    liteco_machine_t *machine;

    // 从waiting状态唤起当前协程时，标记是从哪个等待通道唤起
    liteco_channel_t *active_channel;
};

#define LITECO_PARAMETER_UNEXCEPTION    -1
#define LITECO_COROUTINE_JOINED         -2
#define LITECO_INTERNAL_ERROR           -3
#define LITECO_EMPTY                    -4
#define LITECO_CLOSED                   -5
#define LITECO_TIMEOUT                  -6
#define LITECO_SUCCESS                  0

#define LITECO_UNKNOW       0x00
#define LITECO_STARTING     0x01
#define LITECO_READYING     0x02
#define LITECO_RUNNING      0x03
#define LITECO_WAITING      0x04
#define LITECO_TERMINATE    0x05

#define LITECO_TRUE     1
#define LITECO_FALSE    0

#define liteco_container_of(t, m, p) ((t *) ((char *) (p) - ((size_t) &(((t *) 0)->m))))

/*
 * 创建一个协程
 * 
 * @param co: 协程
 * @param stack: 协程栈
 * @param st_size: 协程栈长度
 * @param fn: 协程的起始函数
 * @param args: 起始函数的参数
 * @param finished_fn: 结束协程时执行的收尾函数，可为空
 *
 * @return:
 *      LITECO_SUCCESS 创建成功
 *      LITECO_PARAMETER_UNEXCEPTION 参数错误
 *
 */
int liteco_create(liteco_coroutine_t *const co,
                  void *const stack, const size_t st_size,
                  int (*fn) (void *const), void *const args, int (*finished_fn) (liteco_coroutine_t *const co));

/*
 * 调配执行一个协程，该函数执行成功时，对应的协程将占用当前machine的计算资源
 *
 * @param co: 协程
 *
 * @return:
 *      LITECO_SUCCESS 调配成功
 *      LITECO_PARAMETER_UNEXCEPTION 参数错误
 */
int liteco_resume(liteco_coroutine_t *const co);

/*
 * 当前协程释放对CPU的使用
 */
int liteco_yield();

struct liteco_link_node_s {
    liteco_link_node_t *next;
};

struct liteco_link_s {
    liteco_link_node_t head;
    liteco_link_node_t *q_tail;
};

struct liteco_machine_s {
    liteco_link_t q_ready;
    liteco_link_t q_timer;
    liteco_link_t q_wait;

    pthread_cond_t cond;
    pthread_mutex_t lock;
};

int liteco_machine_init(liteco_machine_t *const machine);
int liteco_machine_wait(liteco_machine_t *const machine, liteco_coroutine_t *const co, liteco_channel_t *const channels[], const u_int64_t timeout);
int liteco_machine_channel_notify(liteco_machine_t *const machine, liteco_channel_t *const channel);
int liteco_machine_schedule(liteco_machine_t *const machine);
int liteco_machine_join(liteco_machine_t *const machine, liteco_coroutine_t *const co);
int liteco_machine_delay_join(liteco_machine_t *const machine, const u_int64_t timeout, liteco_coroutine_t *const co);

struct liteco_channel_s {
    liteco_boolean_t closed;

    liteco_link_t q_ele;
    liteco_link_t q_machines;

    pthread_mutex_t lock;
};

int liteco_channel_init(liteco_channel_t *const channel);
int liteco_channel_send(liteco_channel_t *const channel, const void *const element);
int liteco_channel_recv(const void **const ele, const liteco_channel_t **const channel,
                        liteco_machine_t *const machine, liteco_channel_t *const channels[], const u_int64_t timeout);
int liteco_channel_close(liteco_channel_t *const channel);

extern liteco_channel_t __CLOSED_CHAN__;
extern __thread liteco_coroutine_t *__CURR_CO__;

#endif
