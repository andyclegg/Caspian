#ifndef  HEADER_RESULT_SET
#define HEADER_RESULT_SET
#include <pthread.h>

struct result_set_item {
   float x;
   float y;
   int record_index;
   struct result_set_item *next;
};

typedef struct result_set {
   struct result_set_item *head, *current, *tail;
   pthread_mutex_t write_lock;
} result_set_t;

result_set_t *result_set_init();
void result_set_insert(result_set_t *set, float x, float y, int record_index);
void result_set_free(result_set_t *set);
void print_result_set(result_set_t *set);
void result_set_prepare_iteration(result_set_t *set);
struct result_set_item *result_set_iterate(result_set_t *set);
unsigned int result_set_len(result_set_t *set);
#endif
