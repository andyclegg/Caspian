#ifndef HEADER_INDEX
#define HEADER_INDEX
#include <stdio.h>
#include <proj_api.h>
#include "result_set.h"

typedef struct index_s{
   void *data_structure;
   projPJ *projection;
   unsigned int num_elements;
   void (*write_to_file)(struct index_s *towrite, FILE *output_file);
   void (*free)(struct index_s *tofree);
   result_set_t *(*query)(struct index_s *toquery, float *dimension_bounds);
} index;

#endif
