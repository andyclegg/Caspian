/**
 * @file
 * @author Andrew Clegg
 *
 * A generic interface for data reduction functions.
 */
#ifndef HEADER_REDUCTION_FUNCTIONS
#define HEADER_REDUCTION_FUNCTIONS

#include "data_handling.h"
#include "result_set.h"

/**
 * Define a standard set of parameters that can be passed to a reduction function.
 */
typedef struct {
   /** The 'fill value' of the input data (i.e. the value that indicates a particular observation should be discarded) */
   NUMERIC_WORKING_TYPE input_fill_value;

   /** The 'fill value' of the output data (i.e. the value that indicates that there was insufficient data for a particular grid cell */
   NUMERIC_WORKING_TYPE output_fill_value;
} reduction_attrs;

/**
 * Define a standard interface for a reduction function - the name, type, and a callable function pointer.
 */
typedef struct {
   /** The name of this reduction function (use only alphanumerics and underscores).*/
   char* name;

   /** The style of this function (e.g. coded or numeric) */
   style type;

   /** The actual function call
    *
    * @param set The result set of observations which this function should reduce.
    * @param attrs A reduction_attrs instance.
    * @param bounds The dimension_bounds for the cell this function is producing a value for.
    * @param input_data Pointer to the memory where the input data is stored.
    * @param output_data Pointer to the memory where the output data is stored.
    * @param output_index The index in the output array where the reduced value should be stored.
    * @param input_dtype The data type of the input array.
    * @param output_dtype The data type of the output array.
    */
   void (*call)(
      result_set *set,
      reduction_attrs *attrs,
      dimension_bounds bounds,
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
