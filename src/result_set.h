#ifndef  HEADER_RESULT_SET
#define HEADER_RESULT_SET
#include <pthread.h>

/**
 * Define a singly-linked list of result set items.
 */
typedef struct result_set_item_s {
   /** The x-value of the result item.*/
   float x;

   /** The y-value of the result item.*/
   float y;

   /** The time value of the result item.*/
   float t;

   /** The index of the result item.*/
   int record_index;

   /** The next result_set_item in the linked list.*/
   struct result_set_item_s *next;
} result_set_item;

/**
 * Define a result_set.
 */
typedef struct result_set_s {
   /** The head of the result_set_item linked list.*/
   result_set_item *head;

   /** The current item of the result_set_item linked list.*/
   result_set_item *current;

   /** The tail of the result_set_item linked list.*/
   result_set_item *tail;

   /** A write lock, such that the result_set can be constructed in a multithreaded environment.*/
   pthread_mutex_t write_lock;

   /** The length of the result set.*/
   unsigned int length;
} result_set;

// Function prototypes - implemented in result_set.c
result_set *result_set_init();
void result_set_insert(result_set *set, float x, float y, float t, int record_index);
void result_set_free(result_set *set);
result_set_item *result_set_iterate(result_set *set);
unsigned int result_set_len(result_set *set);
#endif
