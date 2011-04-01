/**
 * @file
 * @author Andrew Clegg
 *
 * Contains functions for initialising, configuring and freeing  a grid
 */
#include <math.h>
#include <stdlib.h>

#include "grid.h"

/**
 * Initialise a grid with the given parameters
 *
 * The sampling offsets are initialised to the given value divided by 2 if provided (e.g. vsample or hsample are not 0). Otherwise,
 * they are initialised to the appropriate resolution divided by 2.
 *
 * @param width The width in pixels.
 * @param height The height in pixels.
 * @vertical_resolution The vertical resolution in metres.
 * @horizontal_resolution The horizontal resolution in metres.
 * @vsample The vertical sampling size in metres.
 * @hsample The horizontal sampling size in metres.
 * @central_x The x-coordinate of the centre of the grid, in metres.
 * @central_y The y-coordinate of the centre of the grid, in metres.
 * @input_projector An initialised projector which transforms spherical coordinates to this grid.
 * @return A pointer to the initialised grid.
 */
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

/**
 * Set time constraints on the grid.
 *
 * @param output_grid The grid to set time constraints on.
 * @param start The start time.
 * @param end The end time.
 */
void set_time_constraints(grid *output_grid, float start, float end) {
   output_grid->time_min = start;
   output_grid->time_max = end;
}

/**
 * Free the given grid.
 *
 * @param tofree The grid to free.
 */
void free_grid(grid *tofree) {
   free(tofree);
}
