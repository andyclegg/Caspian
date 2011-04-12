#include <check.h>
#include <proj_api.h>
#include <stdio.h>
#include <stdlib.h>

#include "../src/proj_projector.h"
#include "../src/projector.h"

START_TEST(test_proj) {

   // Create the projector
   projector *p = get_proj_projector_from_string("+proj=eqc +datum=WGS84");
   fail_if(p == NULL);

   // Project some data and compare
   projected_coordinates pc = p->project(p, 45.0, 30.0);
   fail_unless(pc.x == 5009377.0);
   fail_unless(pc.y == 3339584.75);

   // Inverse-project some data and compare
   spherical_coordinates sc = p->inverse_project(p, 5009377.0, 3339584.75);
   fail_unless(sc.latitude == 30.0);
   fail_unless(sc.longitude == 45.0);

   // Serialize and de-serialize
   FILE *f = fopen("test_proj_serialize", "wb");
   p->serialize_to_file(p, f);
   fclose(f);

   f = fopen("test_proj_serialize", "rb");
   projector *q = get_proj_projector_from_file(f);

   fail_unless(
      strcmp(
         pj_get_def((projPJ *)p->internals, 0),
         pj_get_def((projPJ *)q->internals, 0)
      ) == 0);

   // Cleanup
   p->free(p);
   q->free(q);
   system("rm -f test_proj_serialize");

} END_TEST

START_TEST(test_proj_invalid) {
   // Create the projector
   projector *p = get_proj_projector_from_string("not a valid projection");
   fail_unless(p == NULL);
} END_TEST

START_TEST(test_proj_invalid_deserialize) {
   FILE *f = fopen("/dev/zero", "rb");
   projector *p = get_proj_projector_from_file(f);
   fclose(f);
} END_TEST

Suite *proj_projector_suite(void) {
   Suite *s = suite_create("proj");

   // Correct test case
   TCase *proj_testcase = tcase_create("proj");
   tcase_add_test(proj_testcase, test_proj);
   suite_add_tcase(s, proj_testcase);

   // Testt error handling
   TCase *invalid_proj_testcase = tcase_create("proj-invalid");
   tcase_add_test(invalid_proj_testcase, test_proj_invalid);
   tcase_add_exit_test(invalid_proj_testcase, test_proj_invalid_deserialize, EXIT_FAILURE);
   suite_add_tcase(s, invalid_proj_testcase);

   // Test invalid deserialization


   return s;
}

int main(void) {
   Suite *s = proj_projector_suite();
   SRunner *suite_runner = srunner_create(s);
   srunner_run_all(suite_runner, CK_NORMAL);
   int failures = srunner_ntests_failed(suite_runner);
   srunner_free(suite_runner);
   return (failures == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
