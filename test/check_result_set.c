#include <check.h>
#include <stdlib.h>

#include "../src/result_set.h"

START_TEST(test_result_set) {

   // Setup the result set
   result_set *s = result_set_init();

   // Check that it is marked as empty
   fail_unless(s->length == 0);

   // Add 10 records, and check the length is correct after each insertion
   for (int i = 1; i<=10; i++) {
      s->insert(s, i + 1.0, i + 2.0, i + 3.0, i);
      fail_unless(s->length == i);
   }

   // Iterate over result set, checking the values and count the number of items
   result_set_item *current_item;
   int iterated_results = 0;

   while ((current_item = s->iterate(s)) != NULL) {
      iterated_results++;
      fail_unless(current_item->x == iterated_results + 1.0);
      fail_unless(current_item->y == iterated_results + 2.0);
      fail_unless(current_item->t == iterated_results + 3.0);
      fail_unless(current_item->record_index == iterated_results);
   }

   fail_unless(iterated_results == 10);

   // Cleanup
   s->free(s);


} END_TEST

Suite *result_set_suite(void) {
   Suite *s = suite_create("result set");

   // Result Set test case
   TCase *result_set_testcase = tcase_create("result set");
   tcase_add_test(result_set_testcase, test_result_set);
   suite_add_tcase(s, result_set_testcase);

   return s;
}

int main(void) {
   Suite *s = result_set_suite();
   SRunner *suite_runner = srunner_create(s);
   srunner_run_all(suite_runner, CK_NORMAL);
   int failures = srunner_ntests_failed(suite_runner);
   srunner_free(suite_runner);
   return (failures == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
