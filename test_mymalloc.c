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
      (NULL == CU_ADD_TEST(pSuites, test_merge_free))) {
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
