#include <check.h>
#include <stdlib.h>

#include "../src/coordinate_reader.h"
#include "../src/rawfile_coordinate_reader.h"
#include "../src/projector.h"
#include "../src/proj_projector.h"


START_TEST (test_valid_files) {
   projector *p = get_proj_projector_from_string("+proj=eqc +datum=WGS84");
   char *filename = "/dev/zero";
   coordinate_reader *c = get_coordinate_reader_from_files(
      filename,
      filename,
      filename,
      p);

   fail_if(c == NULL);
   float x, y, t;
   c->read(c, &x, &y, &t);
   fail_unless((int) x == 0);
   fail_unless((int) y == 0);
   fail_unless((int) t == 0);

   c->free(c);
   p->free(p);

} END_TEST

START_TEST (test_invalid_files) {
   projector *p = get_proj_projector_from_string("+proj=eqc +datum=WGS84");
   char *filename = "fake";
   coordinate_reader *c = get_coordinate_reader_from_files(
      filename,
      filename,
      filename,
      p);

   fail_unless(c == NULL);
   p->free(p);
} END_TEST

Suite *coordinate_reader_suite(void) {
   Suite *s = suite_create("coordinate_reader");

   // Valid files test case
   TCase *valid_testcase = tcase_create("valid_files");
   tcase_add_test(valid_testcase, test_valid_files);
   suite_add_tcase(s, valid_testcase);

   // Invalid files test case
   TCase *invalid_testcase = tcase_create("invalid_files");
   tcase_add_test(invalid_testcase, test_invalid_files);
   suite_add_tcase(s, invalid_testcase);

   return s;
}

int main(void) {
   Suite *s = coordinate_reader_suite();
   SRunner *suite_runner = srunner_create(s);
   srunner_run_all(suite_runner, CK_NORMAL);
   int failures = srunner_ntests_failed(suite_runner);
   srunner_free(suite_runner);
   return (failures == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

