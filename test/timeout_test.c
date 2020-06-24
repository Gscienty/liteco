#include "liteco.h"
#include <stdio.h>
#include <sys/time.h>

char stack[128 * 1024];

liteco_channel_t chan;
liteco_machine_t machine;

int finished_fn(liteco_coroutine_t *const co) {
    (void) co;
    return 0;
}

static inline u_int64_t __now__() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000 * 1000 + tv.tv_usec;
}

int fn(void *const args) {
    (void) args;

    printf("start\n");

    const void *ele;

    liteco_channel_t *const recv_channels[] = { &chan, NULL };
    liteco_channel_recv(&ele, NULL, &machine, recv_channels, __now__() + 1 * 1000 * 1000);

    printf("1s\n");

    return 0;
}

int main() {
    liteco_coroutine_t co;
    liteco_create(&co, stack, 128 * 1024, fn, NULL, finished_fn);
    liteco_channel_init(&chan);

    liteco_machine_init(&machine);

    liteco_machine_join(&machine, &co);

    liteco_machine_schedule(&machine);
    liteco_machine_schedule(&machine);

    return 0;
}
