#ifndef HEADER_IO_SPEC
#define HEADER_IO_SPEC

#include "data_handling.h"
#include "grid.h"


typedef struct {
   char *data_output;
   float32_t *lats_output;
   float32_t *lons_output;
   dtype output_dtype;
   grid *grid_spec;
} output_spec;

typedef struct {
   char *data_input;
   dtype input_dtype;
} input_spec;

#endif
