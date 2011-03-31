#include <stdlib.h>
#include <math.h>

#include "grid.h"
#include "data_handling.h"


grid *initialise_grid(int width, int height, float vertical_resolution, float horizontal_resolution, float vsample, float hsample, float central_x, float central_y, projector *input_projector) {
   grid *result = malloc(sizeof(grid));
   if (result == NULL) {
      return NULL;
   }

   // Store the values
   result->width = width;
   result->height = height;
   result->vertical_resolution = vertical_resolution;
   result->horizontal_resolution = horizontal_resolution;
   result->vsample = vsample;
   result->hsample = hsample;
   result->central_x = central_x;
   result->central_y = central_y;
   result->input_projector = input_projector;
   result->time_min = -INFINITY;
   result->time_max = +INFINITY;

   // Set sampling factor offset to be equal to resolution/2 if not set, otherwise to provided value/2
   if (hsample == 0.0) {
      result->horizontal_sampling_offset = horizontal_resolution / 2.0;
   } else {
      result->horizontal_sampling_offset = hsample / 2.0;
   }
   if (vsample == 0.0) {
      result->vertical_sampling_offset = vertical_resolution / 2.0;
   } else {
      result->vertical_sampling_offset = vsample / 2.0;
   }

   result->input_projector = input_projector;

   return result;
}

void set_time_constraints(grid *output_grid, float min, float max) {
   output_grid->time_min = min;
   output_grid->time_max = max;
}

void free_grid(grid *tofree) {
   free(tofree);
}
