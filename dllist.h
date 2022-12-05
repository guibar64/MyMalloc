#ifndef DLLIST_HEADER
#define DLLIST_HEADER

#include <stddef.h>

typedef struct DLLElement_ {
  size_t size;
  int flags;
  struct DLLElement_ *previous_in_mem;
  struct DLLElement_ *next, *previous;
} DLLElement;

typedef struct DList_ {
  DLLElement *head, *tail;
} DLList;

DLList dllist_new();

void dllist_push(DLList *list, DLLElement *element);

DLLElement *dllist_pop(DLList *list);

size_t dllist_length(DLList *list);

void dllist_add(DLList *list, DLLElement *element, size_t index);

void dllist_add_after(DLList *list, DLLElement *element, DLLElement *after);

DLLElement *dllist_find_size(DLList *list, size_t size);

void dllist_remove(DLList *list, DLLElement *element);

void ddlist_clean(DLList *list, void (*delele_element)(DLLElement *));

void dllist_print(DLList *list);

#endif /*DLLIST_HEADER*/