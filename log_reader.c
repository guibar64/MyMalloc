#include "mymalloc_internal.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
  FILE *flog;
  void *pointer;
  size_t size;
  if ((flog = fopen(LOG_FILE, "r")) != NULL) {
    while (!feof(flog)) {
      if (fscanf(flog,
                 "[%*s %*d %*d][%*d:%*d] malloc'd %zu %*s at address %p%*c",
                 &size, &pointer) == 2) {
        printf("%p %zu byte%s\n", pointer, size, size <= 1 ? "" : "s");
      }
    }
    fclose(flog);
  }
}