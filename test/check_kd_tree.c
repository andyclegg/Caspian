#include <check.h>
#include <math.h>
#include <stdlib.h>

#include "../src/coordinate_reader.h"
#include "../src/data_handling.h"
#include "../src/spatial_index.h"
#include "../src/kd_tree.h"
#include "../src/projector.h"
#include "../src/proj_projector.h"
#include "../src/rawfile_coordinate_reader.h"
#include "../src/result_set.h"

START_TEST(test_valid_kdtree) {
   // Write some latitudes and longitudes to work with
   FILE *lats = fopen("test_kdtree_lats", "wb");
   FILE *lons = fopen("test_kdtree_lons", "wb");

   int records_stored = 0;
   for (float latitude = -90; latitude <= 90.0; latitude+=0.5) {
      for (float longitude = -180; longitude <= 180.0; longitude+=0.5) {
         records_stored++;
         fwrite(&latitude, sizeof(float), 1, lats);
         fwrite(&longitude, sizeof(float), 1, lons);
      }
   }

   fclose(lats);
   fclose(lons);

   // Create a projector to project these spherical coordinates
   projector *p = get_proj_projector_from_string("+proj=eqc +datum=WGS84");

   // Create a rawfile latlon coordinate reader to read these files back in
   coordinate_reader *c = get_coordinate_reader_from_files(
                              "test_kdtree_lats", "test_kdtree_lons", NULL, p);
   fail_if(c == NULL);

   // Build the tree
   spatial_index *si = generate_kdtree_index_from_coordinate_reader(c);

   // Verify
   verify_tree((kdtree *)si->data_structure);

   // Query
   float bounds[] = {-INFINITY, INFINITY, -INFINITY, INFINITY, -INFINITY, INFINITY};
   result_set *r = si->query(si, bounds);
   fail_unless(r->length == records_stored);

   // Serialize/Deserialize
   FILE *serialized_index = fopen("test_kdtree_index", "wb");
   si->write_to_file(si, serialized_index);
   fclose(serialized_index);

   serialized_index = fopen("test_kdtree_index", "rb");
   spatial_index *si2 = read_kdtree_index_from_file(serialized_index);

   // Query again
   r = si->query(si, bounds);
   fail_unless(r->length == records_stored);

   // Cleanup
   p->free(p);
   c->free(c);
   system("rm -f test_kdtree_lats test_kdtree_lons test_kdtree_index");

} END_TEST

Suite *kd_tree_suite(void) {
   Suite *s = suite_create("kd_tree");

   // Valid kdtree test case
   TCase *valid_kdtree_testcase = tcase_create("valid kdtree");
   tcase_add_test(valid_kdtree_testcase, test_valid_kdtree);
   suite_add_tcase(s, valid_kdtree_testcase);

   return s;
}

int main(void) {
   Suite *s = kd_tree_suite();
   SRunner *suite_runner = srunner_create(s);
   srunner_run_all(suite_runner, CK_NORMAL);
   int failures = srunner_ntests_failed(suite_runner);
   srunner_free(suite_runner);
   return (failures == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
