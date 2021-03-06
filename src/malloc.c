/*
 * Copyright (c) 2020 Gscienty <gaoxiaochuan@hotmail.com>
 *
 * Distributed under the MIT software license, see the accompanying
 * file LICENSE or https://www.opensource.org/licenses/mit-license.php .
 *
 */

#include "internal/malloc.h"
#include <malloc.h>

void *liteco_malloc(const size_t size) {
    void *result = malloc(size);

#if LITECO_DEBUG
    printf("malloc %ld\n", (size_t) result);
#endif

    return result;
}

int liteco_free(void *const ptr) {
#if LITECO_DEBUG
    printf("free %ld\n", (size_t) ptr);
#endif

    free(ptr);
    return 0;
}
