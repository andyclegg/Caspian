/**
 * @file
 * @author Andrew Clegg
 *
 * Implementation of result_set
 */
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "result_set.h"

/**
 * Insert a single item into a result set.
 *
 * @param set The initialised result_set to insert the item into.
 * @param x The x-value of the new item.
 * @param y The y-value of the new item.
 * @param t The time value of the new item.
 * @param record_index The index of the new item.
 */
void result_set_insert(result_set *set, float x, float y, float t, int record_index) {
   result_set_item *new_item = malloc(sizeof(result_set_item));
   if (new_item == NULL) {
      fprintf(stderr, "Could not allocate space for a single result_set_item\n");
      exit(-1);
   }
   new_item->x = x;
   new_item->y = y;
   new_item->t = t;
   new_item->record_index = record_index;
   new_item->next = NULL;

   //Add the new item in at the appropriate position

   // Acquire the write lock
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

   // Release the write lock
   pthread_mutex_unlock(&set->write_lock);
}

/**
 * Return the next result_set_item from a result_set.
 *
 * @param set The result_set to retrieve the next item from.
 * @return A pointer to the next result_set_item.
 */
result_set_item *result_set_iterate(result_set *set) {
   // Get the value which will be returned (possibly NULL)
   result_set_item *to_return = set->current;

   // Advance the current pointer on to the next value
   if (set->current != NULL) {
      set->current = set->current->next;
   }
   return to_return;
}

/**
 * Free a result_set.
 *
 * @param set The result_set to free.
 */
void result_set_free(result_set *set) {
   // Iterate over the linked list, freeing each item
   set->current = set->head;
   while (set->current != NULL) {
      result_set_item *next = set->current->next;
      free(set->current);
      set->current = next;
   }

   // Free the result set itself
   free(set);
}

/**
 * Initialise an empty result set.
 *
 * @return A pointer to an initialised result set.
 */
result_set *result_set_init() {
   result_set *set = malloc(sizeof(result_set));
   if (set == NULL) {
      fprintf(stderr, "Could not allocate space for a result_set struct\n");
      exit(-1);
   }
   set->head = NULL;
   set->current = NULL;
   set->tail = NULL;
   set->length = 0;
   pthread_mutex_init(&set->write_lock, NULL);

   // Set up function pointers
   set->insert = &result_set_insert;
   set->free = &result_set_free;
   set->iterate = &result_set_iterate;
   return set;
}
