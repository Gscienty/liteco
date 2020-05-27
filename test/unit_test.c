#include "liteco.h"
#include <stdio.h>
#include <malloc.h>

int func(__co__, void *const _) {
    (void) _;

    printf("2\n");
    __yield__;
    printf("4\n");

    return 0;
}

char stack[128 * 1024];
int main() {
    liteco_coroutine_t co;
    liteco_create(&co, stack, 128 * 1024, func, NULL);
    printf("1\n");
    liteco_resume(&co);
    printf("3\n");
    liteco_resume(&co);
    return 0;
}
