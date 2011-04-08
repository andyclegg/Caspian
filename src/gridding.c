/**
 * @file
 * @author Andrew Clegg
 *
 * Implements the standard gridding algorithm
 */
#include <time.h>
#include <omp.h>

#include "gridding.h"
#include "io_spec.h"
#include "result_set.h"

/**
 * Perform gridding based on input and output specifications, using the specified reduction function and data source.
 *
 * @param inspec Specification of the input data.
 * @param outspec Specification of the output grid.
 * @param reduce_func Selection reduction function.
 * @param attrs Attributes to be used by the reduction function.
 * @param verbosity Set as >=1 for verbose output, 0 for silence.
 */
void perform_gridding(input_spec inspec, output_spec outspec, reduction_function reduce_func, reduction_attrs *attrs, int verbosity) {

   if (verbosity > 0) printf("Building output image\n");
   time_t start_time = time(NULL);

   float32_t x_0 = outspec.grid_spec->central_x - (((float) outspec.grid_spec->width / 2.0) * outspec.grid_spec->horizontal_resolution);
   float32_t y_0 = outspec.grid_spec->central_y - (((float) outspec.grid_spec->height / 2.0) * outspec.grid_spec->vertical_resolution);

   #pragma omp parallel for
   for (int v=0; v<outspec.grid_spec->height; v++) {
      for (int u=0; u<outspec.grid_spec->width; u++) {
         int index = (outspec.grid_spec->height-v-1)*outspec.grid_spec->width + u;

         float32_t cr_x = x_0 + ((float) u + 0.5) * outspec.grid_spec->horizontal_resolution;
         float32_t cr_y = y_0 + ((float) v + 0.5) * outspec.grid_spec->vertical_resolution;

         float32_t bl_x = cr_x - outspec.grid_spec->horizontal_sampling_offset;
         float32_t bl_y = cr_y - outspec.grid_spec->vertical_sampling_offset;

         float32_t tr_x = cr_x + outspec.grid_spec->horizontal_sampling_offset;
         float32_t tr_y = cr_y + outspec.grid_spec->vertical_sampling_offset;

         // Perform gridding of data
         if (outspec.data_output != NULL) {
            float32_t query_dimensions[] = {bl_x, tr_x, bl_y, tr_y, outspec.grid_spec->time_min, outspec.grid_spec->time_max};

            result_set *current_result_set = inspec.coordinate_index->query(inspec.coordinate_index, query_dimensions);
            reduce_func.call(current_result_set, attrs, query_dimensions, inspec.data_input, outspec.data_output, index, inspec.input_dtype, outspec.output_dtype);
            current_result_set->free(current_result_set);
         }

         if (outspec.lats_output != NULL || outspec.lons_output != NULL) {
            // Get the central latitude and longitude for this cell, and store
            spherical_coordinates coords = inspec.coordinate_index->input_projector->inverse_project(inspec.coordinate_index->input_projector, (tr_y + bl_y) / 2.0, (tr_x + bl_x) / 2.0);
            if (outspec.lats_output != NULL) outspec.lats_output[index] = coords.latitude;
            if (outspec.lons_output != NULL) outspec.lons_output[index] = coords.longitude;
         }
      }
   }
   time_t end_time = time(NULL);
   if (verbosity > 0) {
      printf("Output image built.\n");
      printf("Building image took %d seconds\n", (int) (end_time - start_time));
   }
}
