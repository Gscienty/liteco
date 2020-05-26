#include <stdio.h>
#include <malloc.h>
#include "internal/context.h"

struct liteco_internal_context curr_ctx;
struct liteco_internal_context sub_ctx;

int func(void *const _) {
    (void) _;

    printf("2\n");

    return 0;
}


int main() {
    void *fuck = malloc(128 * 1024);
    sub_ctx.link = &curr_ctx;
    liteco_internal_context_make(&sub_ctx, fuck, 128 * 1024, func, NULL);
    liteco_internal_context_swap(&curr_ctx, &sub_ctx);
    printf("3\n");
    return 0;
}
