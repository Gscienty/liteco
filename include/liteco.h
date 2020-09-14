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
 * 2. 运行时(liteco_runtime_t)：协程具体执行时依附的运行时平台
 * 3. 等待通道(liteco_channel_t)：该模块用于唤起挂起在运行时的协程
 */

#ifndef __LITECO_H__
#define __LITECO_H__

#include <stddef.h>
#include <pthread.h>
#include <sys/types.h>
#include <stdbool.h>

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
typedef struct liteco_runtime_s liteco_runtime_t;
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
    liteco_runtime_t *runtime;

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
 * 调配执行一个协程，该函数执行成功时，对应的协程将占用当前运行时的计算资源
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
 *
 * @return:
 *      LITECO_SUCCESS 调配成功
 *      LITECO_PARAMETER_UNEXCEPTION 参数错误
 */
int liteco_yield();

struct liteco_link_node_s {
    liteco_link_node_t *next;
};

struct liteco_link_s {
    liteco_link_node_t head;
    liteco_link_node_t *q_tail;
};

// 协程运行时
struct liteco_runtime_s {
    // 运行时就绪队列
    liteco_link_t q_ready;
    // 运行时定时器等待队列
    liteco_link_t q_timer;
    // 运行时阻塞队列
    liteco_link_t q_wait;

    pthread_cond_t cond;
    pthread_mutex_t lock;

};

/*
 * 初始化协程运行时
 *
 * @param runtime: 协程运行时
 *
 * @return:
 *      LITECO_SUCCESS 调配成功
 *      LITECO_PARAMETER_UNEXCEPTION 参数错误
 */
int liteco_runtime_init(liteco_runtime_t *const runtime);

/*
 * 将一个协程添加到协程运行时的等待队列中
 *
 * @param runtime: 协程运行时
 * @param co: 协程
 * @param channels: 等待通道，用于唤起协程的管道
 * @param timeout: 超时唤起时间
 *
 * @return:
 *      LITECO_SUCCESS 调配成功
 *      LITECO_PARAMETER_UNEXCEPTION 参数错误
 */
int liteco_runtime_wait(liteco_runtime_t *const runtime, liteco_coroutine_t *const co, liteco_channel_t *const channels[], const u_int64_t timeout);

/*
 * 通过等待通道唤起运行时中的等待协程
 *
 * @param runtime: 协程运行时
 * @param channel: 等待管道
 *
 * @return:
 *      LITECO_SUCCESS 调配成功
 *      LITECO_PARAMETER_UNEXCEPTION 参数错误
 */
int liteco_runtime_channel_notify(liteco_runtime_t *const runtime, liteco_channel_t *const channel);

/*
 * 运行时调度运行一个协程
 *
 * @param runtime: 运行时
 *
 * @return:
 *      LITECO_SUCCESS 调配成功
 *      LITECO_PARAMETER_UNEXCEPTION 参数错误
 */
int liteco_runtime_schedule(liteco_runtime_t *const runtime);

/*
 * 将协程加入到运行时的就绪队列中
 *
 * @param runtime: 运行时
 * @param co: 协程
 *
 * @return:
 *      LITECO_SUCCESS 调配成功
 *      LITECO_PARAMETER_UNEXCEPTION 参数错误
 */
int liteco_runtime_join(liteco_runtime_t *const runtime, liteco_coroutine_t *const co);

/*
 * 将协程加入到运行时的延时执行队列
 *
 * @param runtime: 运行时
 * @param timeout: 延时时间
 * @param co: 协程
 *
 * @return:
 *      LITECO_SUCCESS 调配成功
 *      LITECO_PARAMETER_UNEXCEPTION 参数错误
 */
int liteco_runtime_delay_join(liteco_runtime_t *const runtime, const u_int64_t timeout, liteco_coroutine_t *const co);

/*
 * 判别当前运行时就绪队列是否为空
 *
 * @param runtime: 运行时
 *
 * @return:
 *      true: 就绪队列为空
 *      false: 就绪队列不为空
 */
bool liteco_runtime_ready_empty(liteco_runtime_t *const runtime);

/*
 * 判别当前运行时等待队列是否为空
 *
 * @param runtime: 运行时
 *
 * @return:
 *      true: 等待队列为空
 *      false: 等待队列不为空
 */
bool liteco_runtime_wait_empty(liteco_runtime_t *const runtime);

/*
 * 判别当前运行时等待时钟队列是否为空
 *
 * @param runtime: 运行时
 *
 * @return:
 *      true: 等待时钟队列为空
 *      false: 等待时钟队列不为空
 */
bool liteco_runtime_timer_empty(liteco_runtime_t *const runtime);

/*
 * 判别当前运行时中就绪队列、等待队列和等待时钟队列是否均为空
 *
 * @param runtime: 运行时
 *
 * @return:
 *      true: 各队列均为空
 *      false: 存在不为空的队列
 */
bool liteco_runtime_empty(liteco_runtime_t *const runtime);

// 等待通道
struct liteco_channel_s {
    // 等待通道是否已经关闭
    liteco_boolean_t closed;

    // 等待通道的消息事件队列
    liteco_link_t q_ele;
    // 等待通道通知的等待运行时
    liteco_link_t q_runtimes;

    pthread_mutex_t lock;
};

/*
 * 初始化等待通道
 *
 * @param channel: 等待通道
 *
 * @return:
 *      LITECO_SUCCESS 调配成功
 *      LITECO_PARAMETER_UNEXCEPTION 参数错误
 */
int liteco_channel_init(liteco_channel_t *const channel);

/*
 * 向等待通道发送一个消息事件
 *
 * @param channel: 等待通道
 * @param element: 消息事件
 *
 * @return:
 *      LITECO_SUCCESS 调配成功
 *      LITECO_PARAMETER_UNEXCEPTION 参数错误
 */
int liteco_channel_send(liteco_channel_t *const channel, const void *const element);

/*
 * 等待一个消息事件
 *
 * @param ele: 等待到的消息事件
 * @param channel: 等待到的消息事件来源的等待通道
 * @param runtime: 协程运行时
 * @param channels: 等待通道
 * @param timeout: 超时
 *
 * @return:
 *      LITECO_SUCCESS 调配成功
 *      LITECO_TIMEOUT 等待消息事件超时
 *      LITECO_CLOSED 某个等待通道被关闭
 *      LITECO_PARAMETER_UNEXCEPTION 参数错误
 */
int liteco_channel_recv(const void **const ele, const liteco_channel_t **const channel,
                        liteco_runtime_t *const runtime, liteco_channel_t *const channels[], const u_int64_t timeout);

/*
 * 关闭等待通道
 *
 * @param channel: 消息通道
 *
 * @return:
 *      LITECO_SUCCESS 调配成功
 *      LITECO_PARAMETER_UNEXCEPTION 参数错误
 */
int liteco_channel_close(liteco_channel_t *const channel);

extern liteco_channel_t __CLOSED_CHAN__;
extern __thread liteco_coroutine_t *__CURR_CO__;

#endif
