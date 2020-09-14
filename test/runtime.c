#include "liteco.h"
#include <stdio.h>

int co_fn(void *args) {
    (void) args;

    printf("Hello\n");

    liteco_yield();

    printf("World\n");

    return 0;
}

int main() {
    liteco_runtime_t runtime;

    liteco_runtime_init(&runtime);

    liteco_coroutine_t co;
    char stack[2 * 1024];

    liteco_create(&co, stack, 2 * 1024, co_fn, NULL, NULL);

    liteco_runtime_join(&runtime, &co);
    liteco_runtime_schedule(&runtime);
    liteco_runtime_schedule(&runtime);

    return 0;
}
