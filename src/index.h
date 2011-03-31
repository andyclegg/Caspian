#ifndef HEADER_INDEX
#define HEADER_INDEX
#include <stdio.h>
#include "result_set.h"

typedef struct index_s{
   void *data_structure;
   projector *input_projector;
   unsigned int num_elements;
   void (*write_to_file)(struct index_s *towrite, FILE *output_file);
   void (*free)(struct index_s *tofree);
   result_set *(*query)(struct index_s *toquery, float *dimension_bounds);
} index;

#endif
