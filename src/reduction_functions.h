#ifndef HEADER_REDUCTION_FUNCTIONS
#define HEADER_REDUCTION_FUNCTIONS

#include "data_handling.h"
#include "result_set.h"

typedef struct {
   NUMERIC_WORKING_TYPE input_fill_value;
   NUMERIC_WORKING_TYPE output_fill_value;
} reduction_attrs;

typedef struct {
   char* name;
   enum style_e type;
   void (*call)(
      result_set *set,
      reduction_attrs *attrs,
      float *dimension_bounds,
      void *input_data,
      void *output_data,
      int output_index,
      dtype input_dtype,
      dtype output_dtype
   );
} reduction_function;

reduction_function get_reduction_function_by_name(char *name);
int reduction_function_is_undef(reduction_function f);
#endif
