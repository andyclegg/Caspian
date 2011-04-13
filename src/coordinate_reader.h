/**
 * @file
 *
 * Defines the coordinate_reader interface.
 */
#ifndef  HEADER_COORDINATE_READER
#define HEADER_COORDINATE_READER

#include "projector.h"

/**
 * Generic reader that incrementally reads out sets of coordinates.
 *
 * Implementations could include reading coordinates from various file
 * formats, programatically generating coordinates, or reading coordinates
 * from a database.
 *
 * @see rawfile_coordinate_reader
 */
typedef struct coordinate_reader_s {
   /** Opaque pointer to the data structure used by the specific implementaiton of coordinate_reader.*/
   void *internals;

   /** The total number of records available from this coordinate reader.*/
   unsigned int num_records;

   /** The projector which this coordinate reader should use to project spherical coordinates into X/Y space.*/
   projector *input_projector;

   /**
    * Free this coordinate reader.
    *
    * @param tofree The coordinate reader to free.
    */
   void (*free)(struct coordinate_reader_s *tofree);

   /**
    * Read a single set of values from this coordinate reader.
    *
    * @param source The coordinate reader from which to read the values.
    * @param x The memory to store the x value into.
    * @param y The memory to store the y value into.
    * @param t The memory to store the time value into.
    * @return 0 if there are no more values available, 1 otherwise.
    */
   int (*read)(struct coordinate_reader_s *source, float *x, float *y, float *t);
} coordinate_reader;

#endif
