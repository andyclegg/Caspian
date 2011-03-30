#ifndef  HEADER_RESULT_SET
#define HEADER_RESULT_SET
#include <pthread.h>

typedef struct result_set_item_s {
   float x;
   float y;
   float t;
   int record_index;
   struct result_set_item_s *next;
} result_set_item;

typedef struct result_set_s {
   result_set_item *head, *current, *tail;
   pthread_mutex_t write_lock;
   unsigned int length;
} result_set;

result_set *result_set_init();
void result_set_insert(result_set *set, float x, float y, float t, int record_index);
void result_set_free(result_set *set);
void print_result_set(result_set *set);
void result_set_reset_iteration(result_set *set);
result_set_item *result_set_iterate(result_set *set);
unsigned int result_set_len(result_set *set);
#endif
