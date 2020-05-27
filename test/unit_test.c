#include "liteco.h"
#include <stdio.h>
#include <malloc.h>

int func(liteco_coroutine_t *const co, void *const _) {
    (void) _;

    printf("2\n");
    liteco_yield(co);
    printf("4\n");

    return 0;
}

char stack[128 * 1024];
int main() {
    liteco_coroutine_t co;
    liteco_create(&co, stack, 128 * 1024, func, NULL);
    liteco_schedule_t sche;
    liteco_schedule_init(&sche);

    liteco_schedule_join(&sche, &co);

    liteco_coroutine_t *cur;
    liteco_schedule_pop(&cur, &sche);
    liteco_resume(cur);

    liteco_schedule_join(&sche, cur);
    liteco_schedule_pop(&cur, &sche);
    liteco_resume(cur);
    return 0;
}
