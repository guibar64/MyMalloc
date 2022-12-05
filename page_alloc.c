#include "page_alloc.h"

#ifdef WIN32
#include <memoryapi.h>

void *page_alloc(size_t size) {
  void *mem =
      VirtualAlloc(NULL, size, MEM_RESERVE | MEME_COMMIT, PAGE_READWRITE);
  return mem;
}

int page_free(void *pointer, size_t size) {
  return VirtualFree(pointer, 0, MEM_RELEASE);
}

#else
#include <sys/mman.h>
/*TODO: DEBUG*/
#include <stdlib.h>

void *page_alloc(size_t size) {
  void *mem = mmap(NULL, size, PROT_READ | PROT_WRITE,
                   MAP_ANONYMOUS | MAP_PRIVATE | MAP_STACK, -1, 0);
  if (mem == MAP_FAILED) {
    return NULL;
  } else {
    return mem;
  }
}

int page_free(void *pointer, size_t size) { return munmap(pointer, size) == 0; }
#endif