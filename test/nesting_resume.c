#include "liteco.h"
#include <stdio.h>
char co1_stack[2 * 1024];
char co2_stack[2 * 1024];
char co3_stack[2 * 1024];
char co4_stack[2 * 1024];
liteco_coroutine_t co1;
liteco_coroutine_t co2;
liteco_coroutine_t co3;
liteco_coroutine_t co4;

int co1_fn(void *args) {
    (void) args;

    printf("co1 ->\n");
    liteco_resume(&co2);
    printf("co1 <-\n");

    return 0;
}

int co2_fn(void *args) {
    (void) args;

    printf("co2 ->\n");
    liteco_resume(&co3);
    liteco_resume(&co4);
    liteco_resume(&co3);
    liteco_resume(&co3);
    liteco_resume(&co3);
    liteco_resume(&co3);
    liteco_resume(&co3);
    liteco_resume(&co3);
    printf("co2 <-\n");

    return 0;
}

int co3_fn(void *args) {
    (void) args;

    printf("co3 1\n");
    liteco_yield();
    printf("co3 2\n");
    liteco_yield();
    printf("co3 3\n");

    return 0;
}

int co4_fn(void *args) {
    (void) args;

    printf("co4\n");

    return 0;
}

int main() {
    liteco_create(&co1, co1_stack, 2 * 1024, co1_fn, NULL, NULL);
    liteco_create(&co2, co2_stack, 2 * 1024, co2_fn, NULL, NULL);
    liteco_create(&co3, co3_stack, 2 * 1024, co3_fn, NULL, NULL);
    liteco_create(&co4, co4_stack, 2 * 1024, co4_fn, NULL, NULL);

    printf("main ->\n");
    liteco_resume(&co1);
    printf("main <-\n");

    return 0;
}
