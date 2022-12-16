#include "dllist.h"
#include "lock.h"
#include "page_alloc.h"
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "mymalloc.h"
#include "mymalloc_internal.h"

static Heap heaps[NUMBER_HEAPS] = {
    {LIST_INITIALIZER, LIST_INITIALIZER, LOCK_INITIALIZER},
    {LIST_INITIALIZER, LIST_INITIALIZER, LOCK_INITIALIZER},
    {LIST_INITIALIZER, LIST_INITIALIZER, LOCK_INITIALIZER},
    {LIST_INITIALIZER, LIST_INITIALIZER, LOCK_INITIALIZER},
    {LIST_INITIALIZER, LIST_INITIALIZER, LOCK_INITIALIZER},
    {LIST_INITIALIZER, LIST_INITIALIZER, LOCK_INITIALIZER},
    {LIST_INITIALIZER, LIST_INITIALIZER, LOCK_INITIALIZER},
    {LIST_INITIALIZER, LIST_INITIALIZER, LOCK_INITIALIZER}};

_Thread_local int16_t thread_index = -1;
_Atomic int16_t global_thread_count = 0;

static inline size_t fit_to_memalign(size_t size) {
  return (MEM_ALIGN * ((size + MEM_ALIGN - 1) / MEM_ALIGN));
}

void heap_init(int16_t heap_index) {
  Lock defaut = LOCK_INITIALIZER;
  heaps[heap_index].heap = dllist_new();
  heaps[heap_index].free_list = dllist_new();
  heaps[heap_index].lock = defaut;
}

void my_init() {
  for (int16_t i = 0; i < NUMBER_HEAPS; i++) {
    heap_init(i);
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
    // In heap_free a check on next block in mem is made,
    // so a last dead block is reserved to avoid invalid read
    // TODO: find a better solution
    block->size = block_size - HEAP_HEADER_SIZE - BLOCK_SIZE;
    block->next = NULL;
    block->previous = NULL;
    dllist_push(&heap->heap, block);
    return block;
  } else {
    return NULL;
  }
}

static void *heap_malloc(int16_t heap_index, size_t size) {
  Heap *heap = heaps + heap_index;
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
  block->heap_index = heap_index;
  return (void *)get_start(block);
}

static int try_malloc_on_heap(int16_t heap_index, size_t size,
                              void **allocated_mem) {
  *allocated_mem = NULL;
  if (lock_try_acquire(heaps[heap_index].lock) == 0) {
    *allocated_mem = heap_malloc(heap_index, size);
    lock_release(heaps[heap_index].lock);
    return 1;
  }
  return 0;
}

void *my_malloc(size_t size) {
  if (thread_index == -1) {
    thread_index = global_thread_count % NUMBER_HEAPS;
    global_thread_count++;
  }
  void *ret = NULL;
  int found = 0;
  int wait_time = 1;
  while (1) {
    if (try_malloc_on_heap(thread_index, size, &ret))
      return ret;

    for (int16_t i = 0; i < NUMBER_HEAPS; i++) {
      if (try_malloc_on_heap(i, size, &ret))
        return ret;
    }
    usleep(wait_time);
    wait_time *= 2;
  }
  return ret;
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
  BlockHeader *block = (BlockHeader *)((char *)(pointer)-BLOCK_SIZE);
  int16_t heap_index = block->heap_index;
  lock_acquire(heaps[heap_index].lock);
  heap_free(heaps + 0, pointer);
  lock_release(heaps[heap_index].lock);
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
  size_t true_size = heap->size + HEAP_HEADER_SIZE + BLOCK_SIZE;
  // printf("Dellaloc page %p \n", heap);
  if (!page_free(heap, true_size)) {
    fprintf(stderr, "ERROR: Cannot free page at %p of size %zd\n", heap,
            true_size);
  }
}

void my_cleanup() {
  for (int16_t i = 0; i < NUMBER_HEAPS; i++) {
    lock_acquire(heaps[i].lock);
    ddlist_clean(&heaps[i].heap, heap_block_free);
    heap_init(i);
    lock_release(heaps[i].lock);
  }
}