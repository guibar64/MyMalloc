#ifndef PAGE_ALLOC_HEADER
#define PAGE_ALLOC_HEADER
#include <stddef.h>

void *page_alloc(size_t size);

int page_free(void *pointer, size_t size);

#endif /*PAGE_ALLOC_HEADER*/
