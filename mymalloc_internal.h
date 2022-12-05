#ifndef MYMALLOC_INTERNAL_HEADER
#define MYMALLOC_INTERNAL_HEADER

#include "dllist.h"

#define MEM_ALIGN 8

#define PAGE_MIN_SIZE 4096
#define PAGE_DIV 4096

#define HEAP_HEADER_SIZE                                                       \
  (MEM_ALIGN * ((sizeof(HeapHeader) + MEM_ALIGN - 1) / MEM_ALIGN))

#define BLOCK_SIZE                                                             \
  (MEM_ALIGN * ((sizeof(BlockHeader) + MEM_ALIGN - 1) / MEM_ALIGN))

enum { MY_BLOCK_OCCUPIED = 1 } MyBlockFlag;

typedef DLLElement HeapHeader;
typedef DLLElement BlockHeader;

#endif /*MYMALLOC_INTERNAL_HEADER*/