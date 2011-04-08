#include <check.h>
#include <stdlib.h>

#include "../src/median.h"
#include "../src/data_handling.h"

START_TEST(test_median) {
   // Test with an odd length list
   NUMERIC_WORKING_TYPE test_numbers_1[] = {5.0, 1.0, 3.0};
   NUMERIC_WORKING_TYPE result_1 = median(test_numbers_1, 3);
   fail_unless(result_1 == 3.0);

   // Test with an even length list
   NUMERIC_WORKING_TYPE test_numbers_2[] = {5.0, 1.0, 3.0, 4.3, 2.8, 9.9};
   NUMERIC_WORKING_TYPE result_2 = median(test_numbers_2, 6);
   fail_unless(result_2 == 3.65);
} END_TEST

Suite *median_suite(void) {
   Suite *s = suite_create("median");

   // median test case
   TCase *median_testcase = tcase_create("median");
   tcase_add_test(median_testcase, test_median);
   suite_add_tcase(s, median_testcase);

   return s;
}

int main(void) {
   Suite *s = median_suite();
   SRunner *suite_runner = srunner_create(s);
   srunner_run_all(suite_runner, CK_NORMAL);
   int failures = srunner_ntests_failed(suite_runner);
   srunner_free(suite_runner);
   return (failures == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
