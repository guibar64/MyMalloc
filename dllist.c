#include <stdio.h>

#include "dllist.h"

typedef DLLElement DLLElement;
typedef DLList DLList;

DLList dllist_new() {
  DLList ret = {NULL, NULL};
  return ret;
}

void dllist_push(DLList *list, DLLElement *element) {
  if (list->head == NULL) {
    list->head = element;
    list->tail = element;
    element->next = NULL;
    element->previous = NULL;
    return;
  }
  list->tail->next = element;
  element->previous = list->tail;
  element->next = NULL;
  list->tail = element;
}

void dllist_push_front(DLList *list, DLLElement *element) {
  if (list->head == NULL) {
    list->head = element;
    list->tail = element;
    element->previous = NULL;
    element->next = NULL;
    return;
  }
  list->head->previous = element;
  element->next = list->head;
  element->previous = NULL;
  list->head = element;
}

DLLElement *dllist_pop(DLList *list) {
  DLLElement *ret = NULL;
  DLLElement *last = list->tail;
  if (last != NULL) {
    if (last->previous != NULL) {
      last->previous->next = NULL;
    }
    list->tail = last->previous;
    if (list->head == last) {
      list->head = NULL;
    }
    ret = last;
  }
  return ret;
}

size_t dllist_length(DLList *list) {
  size_t count = 0;
  for (DLLElement *elem_it = list->head; elem_it != NULL;
       elem_it = elem_it->next) {
    count++;
  }
  return count;
}

void dllist_add(DLList *list, DLLElement *element, size_t index) {
  size_t count = 0;
  DLLElement *previous = NULL;
  DLLElement *elem_it;
  for (elem_it = list->head; elem_it != NULL && count < index;
       elem_it = elem_it->next) {
    previous = elem_it;
    count++;
  }
  if (count != index)
    return;
  if (previous != NULL) {
    previous->next = element;
    if (element != NULL) {
      element->previous = previous;
    }
  }
  if (elem_it != NULL) {
    element->next = elem_it;
    elem_it->previous = element;
  }
  if (index == 0) {
    list->head = element;
    if (list->tail == NULL) {
      list->tail = element;
    }
  }
}

void dllist_add_after(DLList *list, DLLElement *element, DLLElement *after) {
  if (after == NULL)
    return;
  if (element != NULL) {
    element->next = after->next;
    element->previous = after;
    if (after->next != NULL) {
      after->next->previous = element;
    }
  }
  after->next = element;
  if (list->tail == after) {
    list->tail = element;
  }
}

DLLElement *dllist_find_size(DLList *list, size_t size) {
  for (DLLElement *elem_it = list->head; elem_it != NULL;
       elem_it = elem_it->next) {
    if (elem_it->size == size) {
      return elem_it;
    }
  }
  return NULL;
}

void dllist_remove(DLList *list, DLLElement *element) {

  if (element == NULL)
    return;
  DLLElement *previous = element->previous;
  if (previous != NULL) {
    previous->next = element->next;
  }
  if (element->next != NULL) {
    element->next->previous = previous;
  }
  if (list->head == element) {
    list->head = element->next;
  }
  if (list->tail == element) {
    list->tail = element->previous;
  }
}

void ddlist_clean(DLList *list, void (*delete_element)(DLLElement *)) {
  DLLElement *elem_it = list->head;
  while (elem_it != NULL) {
    DLLElement *element = elem_it;
    elem_it = elem_it->next;
    delete_element(element);
  }
}

void dllist_print(DLList *list) {
  for (DLLElement *elem_it = list->head; elem_it != NULL;
       elem_it = elem_it->next) {
    printf("(%p,%zd) ", elem_it, elem_it->size);
  }

  printf("\n");
}
