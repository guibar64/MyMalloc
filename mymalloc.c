#include "dllist.h"
#include "lock.h"
#include "page_alloc.h"
#include <stdio.h>
#include <stdlib.h>

#include "mymalloc.h"
#include "mymalloc_internal.h"

typedef struct {
  DLList heap;
  DLList free_list;
  Lock lock;
} Heap;

#define NB_HEAPS 8

static Heap heaps[NB_HEAPS] = {
    {LIST_INITIALIZER, LIST_INITIALIZER, LOCK_INITIALIZER},
    {LIST_INITIALIZER, LIST_INITIALIZER, LOCK_INITIALIZER},
    {LIST_INITIALIZER, LIST_INITIALIZER, LOCK_INITIALIZER},
    {LIST_INITIALIZER, LIST_INITIALIZER, LOCK_INITIALIZER},
    {LIST_INITIALIZER, LIST_INITIALIZER, LOCK_INITIALIZER},
    {LIST_INITIALIZER, LIST_INITIALIZER, LOCK_INITIALIZER},
    {LIST_INITIALIZER, LIST_INITIALIZER, LOCK_INITIALIZER},
    {LIST_INITIALIZER, LIST_INITIALIZER, LOCK_INITIALIZER}};

static inline size_t fit_to_memalign(size_t size) {
  return (MEM_ALIGN * ((size + MEM_ALIGN - 1) / MEM_ALIGN));
}

void my_init() {
  Lock defaut = LOCK_INITIALIZER;
  for (int i = 0; i < NB_HEAPS; i++) {
    heaps[i].heap = dllist_new();
    heaps[i].free_list = dllist_new();
    heaps[i].lock = defaut;
  }
}

static char *get_start(BlockHeader *block) {
  return (char *)block + BLOCK_SIZE;
}

void block_init(BlockHeader *block, size_t size) {
  block->size = size;
  block->flags = 0;
  block->previous_in_mem = NULL;
}

void split_block(DLList *free_list, BlockHeader *block, size_t size) {
  size_t old_size = block->size;
  size = fit_to_memalign(size);
  if (old_size > size + BLOCK_SIZE) {
    block->size = size;
    BlockHeader *next_free = (BlockHeader *)((char *)get_start(block) + size);
    block_init(next_free, old_size - size - BLOCK_SIZE);
    next_free->flags = 0;
    next_free->previous_in_mem = block;
    dllist_add_after(free_list, next_free, block);
  }
}

static BlockHeader *get_free_first_fit(DLList *free_list, size_t size) {
  DLLElement *chunk_it;
  for (chunk_it = free_list->head; chunk_it != NULL && chunk_it->size < size;
       chunk_it = chunk_it->next) {
    ;
  }
  return chunk_it;
}

static BlockHeader *find_free_block(DLList *free_list, size_t size) {
  return get_free_first_fit(free_list, size);
}

HeapHeader *get_new_heap_block(Heap *heap, size_t size) {
  size_t min_size =
      ((size + HEAP_HEADER_SIZE + PAGE_DIV - 1) / PAGE_DIV) * PAGE_DIV;
  size_t block_size = min_size < PAGE_MIN_SIZE ? PAGE_MIN_SIZE : min_size;
  HeapHeader *block = (HeapHeader *)page_alloc(block_size);
  if (block != NULL) {
    block->size = block_size - HEAP_HEADER_SIZE;
    block->next = NULL;
    block->previous = NULL;
    dllist_push(&heap->heap, block);
    return block;
  } else {
    return NULL;
  }
}

static void *heap_malloc(Heap *heap, size_t size) {
  BlockHeader *block;
  block = find_free_block(&heap->free_list, size);
  if (block == NULL) {
    HeapHeader *heap_block = get_new_heap_block(heap, size);
    if (heap_block == NULL) {
      return NULL;
    }
    block = (BlockHeader *)((char *)get_start(heap_block));
    block->size = heap_block->size - BLOCK_SIZE;
    block->flags = 0;
    block->previous_in_mem = NULL;
    dllist_push(&heap->free_list, block);
  }
  split_block(&heap->free_list, block, size);
  dllist_remove(&heap->free_list, block);
  block->flags |= MY_BLOCK_OCCUPIED;
  return (void *)get_start(block);
}

void *my_malloc(size_t size) {
  void *mem;
  lock_acquire(heaps[0].lock);
  mem = heap_malloc(heaps + 0, size);
  lock_release(heaps[0].lock);
  return mem;
}

static int is_free(BlockHeader *block) {
  return !(block->flags & MY_BLOCK_OCCUPIED);
}

static BlockHeader *next_block_in_mem(BlockHeader *block) {
  return (BlockHeader *)((char *)block + block->size + BLOCK_SIZE);
}

static void heap_free(Heap *heap, void *pointer) {
  BlockHeader *block = (BlockHeader *)((char *)(pointer)-BLOCK_SIZE);
  /* split_block puts block->previous and block->next */
  BlockHeader *block_next = next_block_in_mem(block);
  if (block->next == block_next && is_free(block_next)) {
    block->size += block_next->size + BLOCK_SIZE;
    dllist_remove(&heap->free_list, block_next);
  }
  if (block->previous_in_mem != NULL) {
    BlockHeader *next = next_block_in_mem(block->previous_in_mem);
    if (next == block && is_free(block_next)) {
      block->previous_in_mem->size += block->size + BLOCK_SIZE;
      return;
    }
  }
  block->flags &= ~MY_BLOCK_OCCUPIED;
  dllist_push_front(&heap->free_list, block);
}

void my_free(void *pointer) {
  lock_acquire(heaps[0].lock);
  heap_free(heaps + 0, pointer);
  lock_release(heaps[0].lock);
}

void *my_realloc(void *pointer, size_t new_size) {
  BlockHeader *block = (BlockHeader *)((char *)(pointer)-BLOCK_SIZE);
  if (block->size >= new_size) {
    return pointer;
  } else {
    my_free(pointer);
    return my_malloc(new_size);
  }
}

void heap_block_free(HeapHeader *heap) {
  size_t true_size = heap->size + HEAP_HEADER_SIZE;
  // printf("Dellaloc page %p \n", heap);
  if (!page_free(heap, true_size)) {
    fprintf(stderr, "ERROR: Cannot free page at %p of size %zd\n", heap,
            true_size);
  }
}

void my_cleanup() {
  for (int i = 0; i < NB_HEAPS; i++) {
    lock_acquire(heaps[i].lock);
    ddlist_clean(&heaps[i].heap, heap_block_free);
    my_init();
    lock_release(heaps[i].lock);
  }
}