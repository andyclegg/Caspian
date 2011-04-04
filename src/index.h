/**
 * @file
 * @author Andrew Clegg
 *
 * Definition of the 'index' data type. Implementations are included elsewhere.
 * @see generate_kdtree_index_from_coordinate_reader
 */
#ifndef HEADER_INDEX
#define HEADER_INDEX
#include <stdio.h>
#include "projector.h"
#include "result_set.h"

/**
 * Generic representation of a spatial index.
 */
typedef struct index_s{
   /** Opaque pointer to implementation-specific data structures for this index.*/
   void *data_structure;

   /**
    * The projector used to project data from spherical coordinates into the X/Y domain used by this index.
    * @todo This should probably be removed.
    */
   projector *input_projector;

   /** The number of data elements represented by this index.*/
   unsigned int num_elements;

   /**
    * Write this index to file, such that it may be reloaded later.
    *
    * @param towrite The index to write out to file.
    * @param output_file The file to write the index to.
    */
   void (*write_to_file)(struct index_s *towrite, FILE *output_file);

   /**
    * Free this index.
    *
    * @param tofree The index to free.
    */
   void (*free)(struct index_s *tofree);

   /**
    * Query this index for a set of observations.
    *
    * @param toquery The index to query.
    * @param dimension_bounds The bounds of the query - an array of floats (lower X, upper X, lower Y, upper Y, lower T, upper T).
    */
   result_set *(*query)(struct index_s *toquery, float *dimension_bounds);
} index;

#endif
