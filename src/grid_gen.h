#ifndef HEADER_GRID_GEN
#define HEADER_GRID_GEN

#include "result_set.h"
#include "data_handling.h"

struct reduction_attrs {
   NUMERIC_WORKING_TYPE input_fill_value;
   NUMERIC_WORKING_TYPE output_fill_value;
};

typedef struct mapping_function_s {
   char* name;
   enum style_e type;
   void (*call)(
      result_set_t *set,
      struct reduction_attrs *attrs,
      float *dimension_bounds,
      void *input_data,
      void *output_data,
      int output_index,
      dtype input_dtype,
      dtype output_dtype
   );
} mapping_function;

#endif
