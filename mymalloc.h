#ifndef MYMALLOC_HEADER
#define MYMALLOC_HEADER
#include <stddef.h>

/**   @brief allocates a region of memory
                        @param size size of the memory
**/
void *my_malloc(size_t size);

/** @brief free a chunk of memory
                @param pointer points the the
**/
void my_free(void *pointer);

/**   @brief reallocates an allocated region to a new size
                        @param size new size of the memory
                        @param old_pointer points to the region of memory
                        @return address to new allocated region
**/
void *my_realloc(void *old_pointer, size_t newsize);

/** @brief Release all memory and reset state */
void my_cleanup();

#endif /*MYMALLOC_HEADER*/
