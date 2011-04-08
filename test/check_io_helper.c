#include <check.h>
#include <stdlib.h>

#include "../src/io_helper.h"

START_TEST(test_input) {
   // Create an input file to test with
   system("dd if=/dev/urandom of=check_io_helper_input_test bs=128c count=1");

   // Open this file as an input file
   memory_mapped_file *m = open_memory_mapped_input_file("check_io_helper_input_test", 128);
   m->close(m);

   // Delete the file
   system("rm -f check_io_helper_input_test");
} END_TEST

START_TEST(test_output) {
   // Open this file as an input file
   memory_mapped_file *m = open_memory_mapped_output_file("check_io_helper_output_test", 128);
   m->close(m);

   // Remove the created file
   system("rm -f check_io_helper_output_test");
} END_TEST


Suite *io_helper_suite(void) {
   Suite *s = suite_create("io helper");

   // Input test case
   TCase *input_testcase = tcase_create("input files");
   tcase_add_test(input_testcase, test_input);
   suite_add_tcase(s, input_testcase);

   // Output test case
   TCase *output_testcase = tcase_create("output files");
   tcase_add_test(output_testcase, test_output);
   suite_add_tcase(s, output_testcase);

   return s;
}

int main(void) {
   Suite *s = io_helper_suite();
   SRunner *suite_runner = srunner_create(s);
   srunner_run_all(suite_runner, CK_NORMAL);
   int failures = srunner_ntests_failed(suite_runner);
   srunner_free(suite_runner);
   return (failures == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
