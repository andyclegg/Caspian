#ifndef HEADER_GRID_GEN
#define HEADER_GRID_GEN

#include "result_set.h"
#include <stdint.h>

// Define float32_t and float64_t for consistency
typedef float float32_t;
typedef double float64_t;

// Enumerate the dtypes
enum dtype_e {uint8, uint16, uint32, uint64, int8, int16, int32, int64, float32, float64, coded8, coded16, coded32, coded64, undef_type};
// Enumerate mapping function types
enum style_e {coded, numeric, undef_style};

// Dtype struct encoding specifier, size and coded/non-coded
typedef struct dtype_s {
   enum dtype_e specifier;
   size_t size;
   enum style_e type;
   char* string;
} dtype;

#define dtype_equal(a, b) (a.specifier == b.specifier && a.size == b.size)


// Determine if 64bit
#ifdef INT64_MAX
#define NUMERIC_WORKING_TYPE float64_t
#define SIXTYFOURBIT
#else
#define NUMERIC_WORKING_TYPE float32_t
#endif


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
