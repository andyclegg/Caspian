/**
 * @file
 *
 * Defines data structures for representing data types and establishes the numeric working type
 */
#ifndef HEADER_DATA_HANDLING
#define HEADER_DATA_HANDLING
#include <stdint.h>

/**
 * A float32_t (float) for consistency with other data types
 */
typedef float float32_t;
/**
 * A float64_t (double) for consistency with other data types
 */
typedef double float64_t;

/**
 * Enumeration of possible dtype names (e.g. uint8, coded32)
 */
typedef enum {uint8, uint16, uint32, uint64, int8, int16, int32, int64, float32, float64, coded8, coded16, coded32, coded64, undef_type} dtype_t;

/**
 * Enumeration of dtype styles (e.g. coded, numeric)
 */
typedef enum {coded, numeric, undef_style} style;

/**
 * The type of some data, including encoding and bytes per record.
 */
typedef struct dtype_s {
   /** The actual format of this particular dtype. */
   dtype_t specifier;

   /** The size of a single item of data of this type, in bytes.*/
   size_t size;

   /** The style of this dtype (numeric, coded). */
   style data_style;

   /** The string represention of this type. */
   char* string;
} dtype;

/** Boolean test for dtype equality */
#define dtype_equal(a, b) (a.specifier == b.specifier && a.size == b.size)

#ifdef INT64_MAX
/** Define a numeric working type as float32_t or float64_t on 32-bit and 64-bit systems respectively. */
#define NUMERIC_WORKING_TYPE float64_t
#define SIXTYFOURBIT
#else
/** Define a numeric working type as float32_t or float64_t on 32-bit and 64-bit systems respectively. */
#define NUMERIC_WORKING_TYPE float32_t
#endif

/** Define X as having index 0 (e.g. in arrays) */
#define X  0

/** Define Y as having index 1 (e.g. in arrays) */
#define Y 1

/** Define T as having index 2 (e.g. in arrays) */
#define T 2

/** Lower bounds positioned at dimension + 0 */
#define LOWER 0

/** Upper bounds positioned at dimension + 1 */
#define UPPER 1

/**
 * Define a type for dimension bounds.
 *
 * Dimension bounds are given as an array of floating point numbers. For each dimension, a lower and upper bound are required. The numbers are ordered as {x-lower, x-upper, y-lower, y-upper, t-lower, t-upper} (and this may be extended for more dimensions if necessary).
 */
typedef float *dimension_bounds;

// Function prototypes - implemented in data_handling.c
NUMERIC_WORKING_TYPE numeric_get(void *data, dtype input_dtype, int index);
void coded_get(void *data, dtype input_dtype, int index, void *output);
void coded_put(void *data, dtype output_dtype, int index, void *input);
void numeric_put(void *data, dtype output_dtype, int index, NUMERIC_WORKING_TYPE data_item);
dtype dtype_string_parse(char *dtype_string);

#endif
