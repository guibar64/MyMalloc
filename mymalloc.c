#include "dllist.h"
#include "lock.h"
#include "page_alloc.h"
#include <stdio.h>
#include <stdlib.h>

#include "mymalloc.h"
#include "mymalloc_internal.h"

static DLList heap = {NULL, NULL};
static DLList free_list = {NULL, NULL};
static Lock free_list_lock = LOCK_INITIALIZER;
static Lock heap_list_lock = LOCK_INITIALIZER;

static inline size_t fit_to_memalign(size_t size) {
  return (MEM_ALIGN * ((size + MEM_ALIGN - 1) / MEM_ALIGN));
}

void my_init() {
  heap = dllist_new();
  free_list = dllist_new();
}

static char *get_start(BlockHeader *block) {
  return (char *)block + BLOCK_SIZE;
}

void block_init(BlockHeader *block, size_t size) {
  block->size = size;
  block->flags = 0;
  block->previous_in_mem = NULL;
}

void split_block(BlockHeader *block, size_t size) {
  size_t old_size = block->size;
  size = fit_to_memalign(size);
  if (old_size > size + BLOCK_SIZE) {
    block->size = size;
    BlockHeader *next_free = (BlockHeader *)((char *)get_start(block) + size);
    block_init(next_free, old_size - size - BLOCK_SIZE);
    next_free->flags = 0;
    next_free->previous_in_mem = block;
    dllist_add_after(&free_list, next_free, block);
  }
}

static BlockHeader *get_free_first_fit(size_t size) {
  DLLElement *chunk_it;
  for (chunk_it = free_list.head; chunk_it != NULL && chunk_it->size < size;
       chunk_it = chunk_it->next) {
    ;
  }
  return chunk_it;
}

static BlockHeader *find_free_block(size_t size) {
  return get_free_first_fit(size);
}

HeapHeader *get_new_heap_block(size_t size) {
  size_t min_size =
      ((size + HEAP_HEADER_SIZE + PAGE_DIV - 1) / PAGE_DIV) * PAGE_DIV;
  size_t block_size = min_size < PAGE_MIN_SIZE ? PAGE_MIN_SIZE : min_size;
  HeapHeader *block = (HeapHeader *)page_alloc(block_size);
  if (block != NULL) {
    block->size = block_size - HEAP_HEADER_SIZE;
    block->next = NULL;
    block->previous = NULL;
    lock_acquire(heap_list_lock);
    dllist_push(&heap, block);
    lock_release(heap_list_lock);
    return block;
  } else {
    return NULL;
  }
}

void *my_malloc(size_t size) {
  BlockHeader *block;
  lock_acquire(free_list_lock);
  block = find_free_block(size);
  if (block == NULL) {
    HeapHeader *heap = get_new_heap_block(size);
    if (heap == NULL) {
      lock_release(free_list_lock);
      return NULL;
    }
    block = (BlockHeader *)((char *)get_start(heap));
    block->size = heap->size - BLOCK_SIZE;
    block->flags = 0;
    block->previous_in_mem = NULL;
    dllist_push(&free_list, block);
  }
  split_block(block, size);
  dllist_remove(&free_list, block);
  block->flags |= MY_BLOCK_OCCUPIED;
  lock_release(free_list_lock);
  return (void *)get_start(block);
}

static int is_free(BlockHeader *block) {
  return !(block->flags & MY_BLOCK_OCCUPIED);
}

static BlockHeader *next_block_in_mem(BlockHeader *block) {
  return (BlockHeader *)((char *)block + block->size + BLOCK_SIZE);
}

void my_free(void *pointer) {
  BlockHeader *block = (BlockHeader *)((char *)(pointer)-BLOCK_SIZE);
  lock_acquire(free_list_lock);
  /* split_block puts block->previous and block->next */
  BlockHeader *block_next = next_block_in_mem(block);
  if (block->next == block_next && is_free(block_next)) {
    block->size += block_next->size + BLOCK_SIZE;
    dllist_remove(&free_list, block_next);
  }
  if (block->previous_in_mem != NULL) {
    BlockHeader *next = next_block_in_mem(block->previous_in_mem);
    if (next == block && is_free(block_next)) {
      block->previous_in_mem->size += block->size + BLOCK_SIZE;
      lock_release(free_list_lock);
      return;
    }
  }
  block->flags &= ~MY_BLOCK_OCCUPIED;
  dllist_push_front(&free_list, block);
  lock_release(free_list_lock);
}

void *my_realloc(void *pointer, size_t new_size) { return pointer; }

void heap_block_free(HeapHeader *heap) {
  size_t true_size = heap->size + HEAP_HEADER_SIZE;
  // printf("Dellaloc page %p \n", heap);
  if (!page_free(heap, true_size)) {
    fprintf(stderr, "ERROR: Cannot free page at %p of size %zd\n", heap,
            true_size);
  }
}

void my_cleanup() {
  lock_acquire(heap_list_lock);
  ddlist_clean(&heap, heap_block_free);
  my_init();
  lock_release(heap_list_lock);
}