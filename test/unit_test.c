#include "liteco.h"
#include <stdio.h>
#include <malloc.h>

char consumer_stack[128 * 1024];
char producer_stack[128 * 1024];

int consumer_finished = LITECO_FALSE;

liteco_channel_t chan;
liteco_machine_t machine;

int consumer_finished_fn(liteco_coroutine_t *const co) {
    (void) co;
    consumer_finished = LITECO_TRUE;
    return LITECO_SUCCESS;
}

int consumer_fn(void *const args) {
    (void) args;
    int *ele;
    liteco_channel_t *const recv_channels[] = { &chan, NULL };

    int i = 0;
    for (i = 0; i < 10; i++) {
        liteco_channel_recv((const void **) &ele, NULL, &machine, recv_channels, 0);
        printf("consumer recv %d\n", *ele);
        free(ele);
    }

    return LITECO_SUCCESS;
}

int producer_fn(void *const args) {
    (void) args;
    printf("producer\n");

    int i = 0;
    for (i = 0; i < 20; i++) {
        int *ele = malloc(sizeof(*ele));
        *ele = i;
        liteco_channel_send(&chan, ele);
        printf("producer send\n");
        liteco_yield();
    }


    return LITECO_SUCCESS;
}

int main() {
    liteco_coroutine_t consumer;
    liteco_create(&consumer, consumer_stack, 128 * 1024, consumer_fn, NULL, consumer_finished_fn);

    liteco_coroutine_t producer;
    liteco_create(&producer, producer_stack, 128 * 1024, producer_fn, NULL, NULL);

    liteco_channel_init(&chan);

    liteco_machine_init(&machine);

    liteco_machine_join(&machine, &consumer);
    liteco_machine_join(&machine, &producer);

    while (consumer_finished == LITECO_FALSE) {
        liteco_machine_schedule(&machine);
    }

    return 0;
}
