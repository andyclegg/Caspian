/**
 * @file
 * @author Andrew Clegg
 *
 * Defines a data structure representing a grid.
 */
#ifndef HEADER_GRID
#define HEADER_GRID

#include "projector.h"

/** A regular geospatial grid in projected coordinate space. */
typedef struct grid_s {
   /** The width in pixels. */
   int width;

   /** The height in pixels. */
   int height;

   /** The vertical resolution in metres. */
   float vertical_resolution;

   /** The horizontal resolution in metres. */
   float horizontal_resolution;

   /** The vertical sampling size in metres. */
   float vsample;

   /** The horizontal sampling size in metres. */
   float hsample;

   /** The x-coordinate of the centre of the grid, in metres. */
   float central_x;

   /** The y-coordinate of the centre of the grid, in metres. */
   float central_y;

   /**
    * The horizontal offset from a given point defining the sampling box.
    *
    * For a horizontal position x, the horizontal sampling range is {x - horizontal_sampling_offset, x + horizontal_sampling_offset}
    */
   float horizontal_sampling_offset;

   /**
    * The vertical offset from a given point defining the sampling box.
    *
    * For a vertical position y, the vertical sampling range is {y - vertical_sampling_offset, y + vertical_sampling_offset}
    */
   float vertical_sampling_offset;

   /** The start time for this grid (set to -inf by default) */
   float time_min;

   /** The end time for this grid (set to =inf by default) */
   float time_max;

   /** An initialised projector which transforms spherical coordinates to this grid. */
   projector *input_projector;

   /**
    * Free this grid.
    *
    * @param tofree The grid to free.
    */
   void (* free)(struct grid_s *tofree);
} grid;

// Function prototypes - implementations in grid.c
grid *initialise_grid(int width, int height, float vertical_resolution, float horizontal_resolution, float vsample, float hsample, float central_x, float central_y, projector *input_projector);
void set_time_constraints(grid *output_grid, float min, float max);

#endif
