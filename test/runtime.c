#include "liteco.h"

int co_fn(void *args) {
    (void) args;

    return 0;
}

int main() {
    liteco_coroutine_t co;
    char stack[2 * 1024];

    liteco_create(&co, stack, 2 * 1024, co_fn, NULL, NULL);

    return 0;
}
