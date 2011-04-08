/**
 * @file
 * @author Andrew Clegg
 *
 * Specifies data structure to represent input data (actual data + type), and output data (output files, type + grid)
 */
#ifndef HEADER_IO_SPEC
#define HEADER_IO_SPEC

#include "data_handling.h"
#include "grid.h"
#include "spatial_index.h"

/**
 * Represents the output requirements of a gridding job. This struct should be constructed manually.
 */
typedef struct {
   /** Pointer to memory where the gridded data should be stored.*/
   char *data_output;

   /** The data type of the output @a data.*/
   dtype output_dtype;

   /** Pointer to memory of type float32_t where the generated latitudes should be stored.*/
   float32_t *lats_output;

   /** Pointer to memory of type float32_t where the generated longitudes should be stored.*/
   float32_t *lons_output;

   /** The grid specification of the output.*/
   grid *grid_spec;
} output_spec;

/**
 * Represents a set of input data. This struct should be constructed manually.
 */
typedef struct {
   /** Pointer to memory where the input data is stored.*/
   char *data_input;

   /** The type of the data */
   dtype input_dtype;

   /** The spatial index for the input.*/
   spatial_index *coordinate_index;
} input_spec;

#endif
