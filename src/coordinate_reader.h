/**
 * @file
 * @author Andrew Clegg
 *
 * Defines the coordinate_reader interface.
 */
#ifndef  HEADER_COORDINATE_READER
#define HEADER_COORDINATE_READER

#include "projector.h"

typedef struct coordinate_reader_s {
   void *internals;
   unsigned int num_records;
   projector *input_projector;
   void (*free)(struct coordinate_reader_s *tofree);
   int (*read)(struct coordinate_reader_s *source, float *x, float *y, float *t);
} coordinate_reader;

#endif
