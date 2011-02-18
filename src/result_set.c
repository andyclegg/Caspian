#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "result_set.h"

result_set_t *result_set_init() {
   result_set_t *set = malloc(sizeof(result_set_t));
   set->head = NULL;
   set->current = NULL;
   set->tail = NULL;
   set->length = 0;
   pthread_mutex_init(&set->write_lock, NULL);
   return set;
}

void result_set_insert(result_set_t *set, float x, float y, float t, int record_index) {
   struct result_set_item *new_item = malloc(sizeof(struct result_set_item));
   new_item->x = x;
   new_item->y = y;
   new_item->t = t;
   new_item->record_index = record_index;
   new_item->next = NULL;

   //Add the new item in at the appropriate position
   pthread_mutex_lock(&set->write_lock);
   if (set->head == NULL) {
      set->head = new_item; // Store the start of the list
      set->current = new_item; // Set 'current' at start of list
      set->tail = new_item;
   } else {
      set->tail->next = new_item;
      set->tail = new_item;
   }
   set->length++;
   pthread_mutex_unlock(&set->write_lock);
}

void result_set_reset_iteration(result_set_t *set) {
   set->current = set->head;
}

struct result_set_item *result_set_iterate(result_set_t *set) {
   struct result_set_item *to_return = set->current;
   if (set->current != NULL) {
      set->current = set->current->next;
   }
   return to_return;
}

void result_set_free(result_set_t *set) {
   set->current = set->head;
   while (set->current != NULL) {
      struct result_set_item *next = set->current->next;
      free(set->current);
      set->current = next;
   }
   free(set);
}

void print_result_set(result_set_t *set) {
   printf("Results:\n");
   set->current = set->head;
   struct result_set_item *current;
   while ((current = result_set_iterate(set)) != NULL) {
      printf("%d @ (%f, %f)\n", current->record_index, current->x, current->y);
   }
}

unsigned int result_set_len(result_set_t *set) {
   return set->length;
}
