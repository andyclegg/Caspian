#ifndef HEADER_DATA_HANDLING
#define HEADER_DATA_HANDLING
#include <stdint.h>

// Define float32_t and float64_t for consistency
typedef float float32_t;
typedef double float64_t;

// Enumerate the dtypes
typedef enum {uint8, uint16, uint32, uint64, int8, int16, int32, int64, float32, float64, coded8, coded16, coded32, coded64, undef_type} dtype_t;
// Enumerate reduction function types
typedef enum {coded, numeric, undef_style} style;

// Dtype struct encoding specifier, size and coded/non-coded
typedef struct dtype_s {
   dtype_t specifier;
   size_t size;
   style type;
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

NUMERIC_WORKING_TYPE numeric_get(void *data, dtype input_dtype, int index);
void coded_get(void *data, dtype input_dtype, int index, void *output);
void coded_put(void *data, dtype output_dtype, int index, void *input);
void numeric_put(void *data, dtype output_dtype, int index, NUMERIC_WORKING_TYPE data_item);
dtype dtype_string_parse(char *dtype_string);

#endif
