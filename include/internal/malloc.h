/*
 * Copyright (c) 2020 Gscienty <gaoxiaochuan@hotmail.com>
 *
 * Distributed under the MIT software license, see the accompanying
 * file LICENSE or https://www.opensource.org/licenses/mit-license.php .
 *
 */

#ifndef __LITECO_MALLOC_H__
#define __LITECO_MALLOC_H__

#include <sys/types.h>

void *liteco_malloc(const size_t size);
int liteco_free(void *const ptr);

#endif
