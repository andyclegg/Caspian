#include <check.h>
#include <math.h>
#include <stdlib.h>

#include "../src/grid.h"
#include "../src/projector.h"
#include "../src/proj_projector.h"

START_TEST(test_grid_creation) {
   projector *p = get_proj_projector_from_string("+proj=eqc +datum=WGS84");
   grid *g = initialise_grid(1000, 1000, 1.0, 1.0, 0.0, 0.0, 0.0, 0.0, p);

   // Check a couple of basic fields
   fail_unless(g->width == 1000);
   fail_unless(g->central_y == 0.0);
   fail_unless(g->time_min == -INFINITY);

   // Check the value of a calculated field
   fail_unless(g->horizontal_sampling_offset == 0.5);

   // Now set time constraints
   set_time_constraints(g, 0.0, 1000.0);
   fail_unless(g->time_max == 1000.0);

   g->free(g);
   p->free(p);

} END_TEST

Suite *grid_suite(void) {
   Suite *s = suite_create("grid");

   // Grid creation test case
   TCase *grid_testcase = tcase_create("grid");
   tcase_add_test(grid_testcase, test_grid_creation);
   suite_add_tcase(s, grid_testcase);

   return s;
}

int main(void) {
   Suite *s = grid_suite();
   SRunner *suite_runner = srunner_create(s);
   srunner_run_all(suite_runner, CK_NORMAL);
   int failures = srunner_ntests_failed(suite_runner);
   srunner_free(suite_runner);
   return (failures == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
