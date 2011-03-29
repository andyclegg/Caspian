#ifndef HEADER_GRID
#define HEADER_GRID

#include <proj_api.h>

typedef struct {
   int width;
   int height;
   float vertical_resolution;
   float horizontal_resolution;
   float vsample;
   float hsample;
   float central_x;
   float central_y;
   float horizontal_sampling_offset; // Better defined elsewhere?
   float vertical_sampling_offset; // Better defined elsewhere?
   float time_min;
   float time_max;
   projPJ *projection;
} grid;

grid *initialise_grid(int width, int height, float vertical_resolution, float horizontal_resolution, float vsample, float hsample, float central_x, float central_y, projPJ *projection);
void set_time_constraints(grid *output_grid, float min, float max);
void free_grid(grid *tofree);

#endif
