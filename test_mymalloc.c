#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <CUnit/Basic.h>
#include <CUnit/CUnit.h>

#include "mymalloc.h"
#include "mymalloc_internal.h"

void test_alloc() {
  char *string = (char *)my_malloc(6);
  BlockHeader *block = (BlockHeader *)((char *)(string)-BLOCK_SIZE);
  CU_ASSERT_FATAL(string != NULL);
  strcpy(string, "Hello");
  CU_ASSERT((size_t)(string) % MEM_ALIGN == 0);
  CU_ASSERT(string - (char *)block == BLOCK_SIZE);
  BlockHeader *block2 = (BlockHeader *)(string + 8);
  CU_ASSERT(block->next == block2);
  CU_ASSERT(block->previous == NULL);
  char *string2 = (char *)my_malloc(10);
  CU_ASSERT_FATAL(string2 != NULL);
  CU_ASSERT(block2->size == 16);
  BlockHeader *block3 = (BlockHeader *)(string2 + 16);
  CU_ASSERT(block2->next == block3);
  CU_ASSERT(block2->previous == NULL);
  CU_ASSERT(block3->flags == 0);
  CU_ASSERT(string2 - (char *)block2 == BLOCK_SIZE);
  strcpy(string2, " World");
  CU_ASSERT(!strncmp(string, "Hello", 6));
  my_cleanup();
}

void test_alloc_free() {
  char *string1 = (char *)my_malloc(6);
  my_free(string1);
  char *string2 = (char *)my_malloc(6);
  CU_ASSERT_PTR_EQUAL(string1, string2);
  my_cleanup();
}

void test_10_allocs() {
  const size_t size = 320;
  char *string1 = (char *)my_malloc(size);
  char *string2;
  for (int i = 0; i < 9; i++) {
    string2 = (char *)my_malloc(size);
  }
  CU_ASSERT_PTR_EQUAL(string2, string1 + 9 * (BLOCK_SIZE + size));
  my_cleanup();
}

void test_merge_free() {
  char *string1 = (char *)my_malloc(6);
  BlockHeader *block = (BlockHeader *)((char *)(string1)-BLOCK_SIZE);
  char *string2 = (char *)my_malloc(10);
  char *string3 = (char *)my_malloc(30);
  my_free(string1);
  my_free(string3);
  my_free(string2);

  CU_ASSERT(block->size >= 142);

  my_cleanup();
}

void test_too_huge_alloc() {
#if INTPTR_MAX == INT64_MAX
  CU_ASSERT(my_malloc(0x6FFFFFFFFFFF) == NULL);
#endif
  my_cleanup();
}

void test_first_fit() {
  char *string1 = (char *)my_malloc(6);
  char *string2 = (char *)my_malloc(18);
  char *string3 = (char *)my_malloc(12);
  char *string4 = (char *)my_malloc(30);
  my_free(string3);
  my_free(string1);
  char *string5 = (char *)my_malloc(10);
  CU_ASSERT_PTR_EQUAL(string5, string3);
  my_free(string2);
  my_free(string4);
  my_cleanup();
}

void test_realloc() {
  char *string1 = (char *)my_malloc(5);
  char *string2 = (char *)my_malloc(10);
  char *string1_r = my_realloc(string1, 6);
  CU_ASSERT_PTR_EQUAL(string1, string1_r);
  char *string1_r2 = my_realloc(string1_r, 300);
  CU_ASSERT_PTR_NOT_EQUAL(string1_r, string1_r2);
  my_free(string2);
  my_cleanup();
}

#define TEST_NB_THREADS (2 * NUMBER_HEAPS)
#define TEST_NB_ALLOCS 10

size_t alloc_size_of_index(int i) { return (size_t)(8 + 2 * i); }

void *thread_malloc(void *args) {
  void **pointers = (void **)args;
  for (int i = 0; i < TEST_NB_ALLOCS; i++) {
    size_t size = alloc_size_of_index(i);
    pointers[i] = my_malloc(size);
    char *string = (char *)(pointers[i]);
    for (size_t i = 0; i < (size - 1); i++) {
      string[i] = 33 + (rand() % (127 - 33));
    }
    string[size - 1] = '\0';
  }
  return NULL;
}

void *thread_free(void *args) {
  void **pointers = (void **)args;
  for (int i = 0; i < TEST_NB_ALLOCS; i++) {
    my_free(pointers[i]);
  }
  return NULL;
}

void test_threaded() {
  srand(216478638);
  pthread_t threads[TEST_NB_THREADS];
  void *pointers[TEST_NB_THREADS][TEST_NB_ALLOCS];
  for (int i = 0; i < TEST_NB_THREADS; i++) {
    CU_ASSERT_FATAL(
        pthread_create(threads + i, NULL, thread_malloc, pointers[i]) == 0)
  }
  for (int i = 0; i < TEST_NB_THREADS; i++) {
    CU_ASSERT_FATAL(pthread_join(threads[i], NULL) == 0);
  }
  for (int i = 0; i < TEST_NB_THREADS - 1; i++) {
    for (int k = 0; k < TEST_NB_ALLOCS; k++) {
      CU_ASSERT(pointers[i][k] != NULL);
      BlockHeader *block =
          (BlockHeader *)((char *)(pointers[i][k]) - BLOCK_SIZE);
      CU_ASSERT(block->size >= alloc_size_of_index(k));
    }
  }

  for (int i = 0; i < TEST_NB_THREADS - 1; i++) {
    for (int j = i + 1; j < TEST_NB_THREADS; j++) {
      for (int k = 0; k < TEST_NB_ALLOCS; k++) {
        CU_ASSERT(pointers[i][k] != pointers[j][k]);
        CU_ASSERT(strcmp(pointers[i][k], pointers[j][k]));
      }
    }
  }
  for (int i = 0; i < TEST_NB_THREADS; i++) {
    CU_ASSERT_FATAL(pthread_create(threads + TEST_NB_THREADS - 1 - i, NULL,
                                   thread_free, pointers[i]) == 0)
  }
  for (int i = 0; i < TEST_NB_THREADS; i++) {
    CU_ASSERT_FATAL(pthread_join(threads[i], NULL) == 0);
  }
  my_cleanup();
}

int init_suite(void) { return 0; }
int clean_suite(void) { return 0; }

int main(int argc, char *argv[]) {
  CU_pSuite pSuites = NULL;

  /* initialize the CUnit test registry */
  if (CUE_SUCCESS != CU_initialize_registry())
    return CU_get_error();

  pSuites = CU_add_suite("mymalloc", init_suite, clean_suite);
  if (NULL == pSuites) {
    CU_cleanup_registry();
    return CU_get_error();
  }
  /* add the tests to the suite */
  if ((NULL == CU_ADD_TEST(pSuites, test_alloc)) ||
      (NULL == CU_ADD_TEST(pSuites, test_alloc_free)) ||
      (NULL == CU_ADD_TEST(pSuites, test_10_allocs)) ||
      (NULL == CU_ADD_TEST(pSuites, test_merge_free)) ||
      (NULL == CU_ADD_TEST(pSuites, test_threaded)) ||
      (NULL == CU_ADD_TEST(pSuites, test_too_huge_alloc)) ||
      (NULL == CU_ADD_TEST(pSuites, test_first_fit)) ||
      (NULL == CU_ADD_TEST(pSuites, test_realloc))) {
    CU_cleanup_registry();
    return CU_get_error();
  }
  if (argc > 1) {
    while (--argc) {
      CU_pTest pTest;
      if ((pTest = CU_get_test_by_name(*(++argv), pSuites)) != NULL) {
        CU_basic_run_test(pSuites, pTest);
      } else {
        printf("WARNING: Unkown test: '%s'\n", *argv);
      }
    }
  } else {
    /* Run all tests using the CUnit Basic interface */
    CU_basic_run_tests();
  }
  unsigned int fails = CU_get_number_of_failures();
  CU_cleanup_registry();
  if (fails > 0) {
    return 1;
  } else {
    return CU_get_error();
  }
}
