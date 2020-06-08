#ifndef __LITECO_MALLOC_H__
#define __LITECO_MALLOC_H__

#include <sys/types.h>

void *liteco_malloc(const size_t size);
int liteco_free(void *const ptr);

#endif
