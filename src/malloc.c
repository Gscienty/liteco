#include "internal/malloc.h"
#include <malloc.h>

void *liteco_malloc(const size_t size) {
    void *result = malloc(size);

#if DEBUG
    printf("malloc %ld\n", (size_t) result);
#endif

    return result;
}

int liteco_free(void *const ptr) {
#if DEBUG
    printf("free %ld\n", (size_t) ptr);
#endif

    free(ptr);
    return 0;
}
