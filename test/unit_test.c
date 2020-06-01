#include "liteco.h"
#include <stdio.h>

int consumer(liteco_coroutine_t *const co, void *const args) {
    (void) args;

    printf("consumer 1\n");
    liteco_g_yield(co);
    printf("consumer 2\n");

    return LITECO_SUCCESS;
}

int productor(liteco_coroutine_t *const co, void *const args) {
    (void) args;

    printf("productor 1\n");
    liteco_g_yield(co);
    printf("productor 2\n");

    return LITECO_SUCCESS;
}

int main() {
    liteco_coroutine_t *consumer_co = NULL;
    liteco_g_create_st(&consumer_co, consumer, NULL);
    liteco_g_create_st(NULL, productor, NULL);

    liteco_g_resume_until_terminate_st(consumer_co);

    return 0;
}
