#include "liteco.h"
#include <stdio.h>
#include <malloc.h>

liteco_channel_t channel;

int consumer(liteco_coroutine_t *const co, void *const args) {
    (void) args;
    void *event;
    liteco_channel_t *recv_channel;

    int i = 0;
    for (i = 0; i < 5; i++) {
        liteco_channel_subscribe(&event, &recv_channel, co, &channel, NULL);
        printf("%d\n", *(int *) event);
        free(event);
    }

    return LITECO_SUCCESS;
}

int productor(liteco_coroutine_t *const co, void *const args) {
    (void) args;
    int i = 0;
    for (i = 0; i < 5; i++) {
        int *event = NULL;
        event = malloc(sizeof(*event));
        *event = i;
        liteco_g_publish_st(&channel, event);
        liteco_g_yield(co);
    }

    return LITECO_SUCCESS;
}

int main() {
    liteco_channel_init(&channel);
    liteco_coroutine_t *consumer_co = NULL;
    liteco_g_create_st(&consumer_co, consumer, NULL);
    liteco_g_create_st(NULL, productor, NULL);

    liteco_g_resume_until_terminate_st(consumer_co);

    printf("%d\n", consumer_co->ref_count);

    return 0;
}
