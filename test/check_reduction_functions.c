#include <check.h>
#include <math.h>
#include <stdlib.h>

#include "../src/data_handling.h"
#include "../src/reduction_functions.h"
#include "../src/result_set.h"

START_TEST(accessing_by_name) {
   // Access a reduction function, and check it is correct
   reduction_function f = get_reduction_function_by_name("mean");
   fail_unless(strcmp(f.name, "mean") == 0);

   // Try to get a non-existent function
   f = get_reduction_function_by_name("does_not_exist");
   fail_unless(reduction_function_is_undef(f));

} END_TEST

result_set *results;
void *input_data, *output_data;
float bounds[] = {25.0, 75.0, 25.0, 75.0, -INFINITY, +INFINITY};
reduction_attrs r_attrs = {-999.0, -999.0};
dtype float32_d;

void setup(void) {
   float32_d = dtype_string_parse("float32");
   results = result_set_init();

   for (int i=0; i<10; i++) {
      for (int j=0; j<10; j++) {
         results->insert(results, (10*i)+j, (10*i)+j+1, (10*i)+j+2, (10*i)+j);
      }
   }

   input_data = malloc(sizeof(float)*100);
   for (int i = 0; i < 100; i++) {
      if (i % 4 == 0) {
         ((float *)input_data)[i] = -999.0;
      } else {
         ((float *)input_data)[i] = (float) i * 5.0;
      }
   }

   output_data = malloc(sizeof(float)*100);

}

void teardown(void) {
   results->free(results);
   free(input_data);
   free(output_data);
}

START_TEST(test_numeric_mean) {
   // Get the reduction function
   reduction_function f = get_reduction_function_by_name("mean");
   fail_if(reduction_function_is_undef(f));

   // Reduce the data
   f.call(results, &r_attrs, bounds, input_data, output_data, 10, float32_d, float32_d);

   // Check the result
   NUMERIC_WORKING_TYPE result = numeric_get(output_data, float32_d, 10);
   fail_unless(result == 250.0);

} END_TEST

START_TEST(test_numeric_weighted_mean) {
   // Get the reduction function
   reduction_function f = get_reduction_function_by_name("weighted_mean");
   fail_if(reduction_function_is_undef(f));
   // Reduce the data
   f.call(results, &r_attrs, bounds, input_data, output_data, 10, float32_d, float32_d);

   // Check the result
   NUMERIC_WORKING_TYPE result = numeric_get(output_data, float32_d, 10);
   fail_unless(fabs(result - 252.496750) < 1E-6);

} END_TEST

START_TEST(test_numeric_median) {
   // Get the reduction function
   reduction_function f = get_reduction_function_by_name("median");
   fail_if(reduction_function_is_undef(f));
   // Reduce the data
   f.call(results, &r_attrs, bounds, input_data, output_data, 10, float32_d, float32_d);

   // Check the result
   NUMERIC_WORKING_TYPE result = numeric_get(output_data, float32_d, 10);
   fail_unless(result == 250.0);

} END_TEST

START_TEST(test_numeric_nearest_neighbour) {
   // Get the reduction function
   reduction_function f = get_reduction_function_by_name("numeric_nearest_neighbour");
   fail_if(reduction_function_is_undef(f));

   // Reduce the data
   f.call(results, &r_attrs, bounds, input_data, output_data, 10, float32_d, float32_d);

   // Get the result
   NUMERIC_WORKING_TYPE result = numeric_get(output_data, float32_d, 10);
   fail_unless(result == 245.0);

} END_TEST

START_TEST(test_coded_nearest_neighbour) {
   // Get the reduction function
   reduction_function f = get_reduction_function_by_name("coded_nearest_neighbour");
   fail_if(reduction_function_is_undef(f));

   dtype coded32_d = dtype_string_parse("coded32");

   // Reduce the data
   f.call(results, &r_attrs, bounds, input_data, output_data, 10, coded32_d, coded32_d);

   // Get the result
   float result;
   coded_get(output_data, coded32_d, 10, &result);
   fail_unless(result == 245.0);

} END_TEST

START_TEST(test_numeric_newest) {
   // Get the reduction function
   reduction_function f = get_reduction_function_by_name("newest");
   fail_if(reduction_function_is_undef(f));
   // Reduce the data
   f.call(results, &r_attrs, bounds, input_data, output_data, 10, float32_d, float32_d);

   // Check the result
   NUMERIC_WORKING_TYPE result = numeric_get(output_data, float32_d, 10);
   fail_unless(result == 495.0);

} END_TEST

Suite *reduction_function_suite(void) {
   Suite *s = suite_create("reduction functions");

   // Accessing by names test case
   TCase *name_access_testcase = tcase_create("access_by_name");
   tcase_add_test(name_access_testcase, accessing_by_name);
   suite_add_tcase(s, name_access_testcase);

   // Mean test case
   TCase *mean_testcase = tcase_create("mean");
   tcase_add_checked_fixture(mean_testcase, setup, teardown);
   tcase_add_test(mean_testcase, test_numeric_mean);
   suite_add_tcase(s, mean_testcase);

   // Weighted mean test case
   TCase *weighted_mean_testcase = tcase_create("weighted_mean");
   tcase_add_checked_fixture(weighted_mean_testcase, setup, teardown);
   tcase_add_test(weighted_mean_testcase, test_numeric_weighted_mean);
   suite_add_tcase(s, weighted_mean_testcase);

   // Median testcase
   TCase *median_testcase = tcase_create("median");
   tcase_add_checked_fixture(median_testcase, setup, teardown);
   tcase_add_test(median_testcase, test_numeric_median);
   suite_add_tcase(s, median_testcase);

   // Coded Nearest neighbour testcase
   TCase *coded_nearest_neighbour_testcase = tcase_create("coded_nearest_neighbour");
   tcase_add_checked_fixture(coded_nearest_neighbour_testcase, setup, teardown);
   tcase_add_test(coded_nearest_neighbour_testcase, test_coded_nearest_neighbour);
   suite_add_tcase(s, coded_nearest_neighbour_testcase);

   // Numeric Nearest neighbour testcase
   TCase *numeric_nearest_neighbour_testcase = tcase_create("numeric_nearest_neighbour");
   tcase_add_checked_fixture(numeric_nearest_neighbour_testcase, setup, teardown);
   tcase_add_test(numeric_nearest_neighbour_testcase, test_numeric_nearest_neighbour);
   suite_add_tcase(s, numeric_nearest_neighbour_testcase);

   // Newest testcase
   TCase *newest_testcase = tcase_create("newest");
   tcase_add_checked_fixture(newest_testcase, setup, teardown);
   tcase_add_test(newest_testcase, test_numeric_newest);
   suite_add_tcase(s, newest_testcase);
   return s;
}

int main(void) {
   Suite *s = reduction_function_suite();
   SRunner *suite_runner = srunner_create(s);
   srunner_run_all(suite_runner, CK_NORMAL);
   int failures = srunner_ntests_failed(suite_runner);
   srunner_free(suite_runner);
   return (failures == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
