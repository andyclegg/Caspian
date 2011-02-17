#ifndef HEADER_GRID
#define HEADER_GRID

#include <proj_api.h>

#include "result_set.h"
#include "reduction_functions.h"
#include "kd_tree.h"

typedef struct {
   int width;
   int height;
   float vres;
   float hres;
   float vsample;
   float hsample;
   float central_x;
   float central_y;
   projPJ *projection;
} grid;

int perform_gridding(grid destination_grid, reduction_function reduce_func, tree *source_tree);

#endif
