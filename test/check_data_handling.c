#include <check.h>
#include <stdlib.h>

#include "../src/data_handling.h"

START_TEST(test_parse_uint8) {
   dtype parsed = dtype_string_parse("uint8");
   fail_unless(parsed.specifier == uint8, NULL);
   fail_unless(parsed.size == 1, NULL);
   fail_unless(parsed.type == numeric, NULL);
   fail_unless(strcmp(parsed.string, "uint8") == 0, NULL);
} END_TEST

START_TEST(test_parse_invalid) {
   dtype parsed = dtype_string_parse("not_a_valid_dtype");
   fail_unless(parsed.specifier == undef_type);
   fail_unless(parsed.type == undef_style);
} END_TEST

START_TEST(test_numeric_handling) {
   void *data = malloc(sizeof(float) * 128);
   dtype float32_d = dtype_string_parse("float32");
   dtype uint8_d = dtype_string_parse("uint8");

   // Store some data
   float input = 3.14159;
   int index = 64;
   numeric_put(data, float32_d, index, input);

   // Retrieve it correctly
   float output = numeric_get(data, float32_d, index);
   fail_unless(input == output);

   // Retrieve it incorrectly
   float output2 = numeric_get(data, uint8_d, index);
   fail_if(input == output2);
} END_TEST

START_TEST(test_coded_handling) {
   void *data = malloc(128);
   dtype coded8_d = dtype_string_parse("coded8");

   // Store some data
   char input = 137;
   int index = 64;
   coded_put(data, coded8_d, index, (void *) &input);

   // Retrieve it correctly
   char output;
   coded_get(data, coded8_d, index, (void *) &output);
   fail_unless(input == output);
} END_TEST


Suite *dtype_suite(void) {
   Suite *s = suite_create("dtype");

   // Parsing test case
   TCase *parse_testcase = tcase_create("parse");
   tcase_add_test(parse_testcase, test_parse_uint8);
   tcase_add_exit_test(parse_testcase, test_parse_invalid, -1);
   suite_add_tcase(s, parse_testcase);

   // Numeric data handling
   TCase *numeric_testcase = tcase_create("numeric data");
   tcase_add_test(numeric_testcase, test_numeric_handling);
   suite_add_tcase(s, numeric_testcase);

   // Coded data handling
   TCase *coded_testcase = tcase_create("coded data");
   tcase_add_test(coded_testcase, test_coded_handling);
   suite_add_tcase(s, coded_testcase);

   return s;
}

int main(void) {
   Suite *s = dtype_suite();
   SRunner *suite_runner = srunner_create(s);
   srunner_run_all(suite_runner, CK_NORMAL);
   int failures = srunner_ntests_failed(suite_runner);
   srunner_free(suite_runner);
   return (failures == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
