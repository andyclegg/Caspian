#ifndef  HEADER_KD_TREE_MINIMAL
#define HEADER_KD_TREE_MINIMAL
#include "result_set.h"
#include "latlon_reader.h"
#include "index.h"

typedef struct {
   short int tag;
   union tree_node_union {
      float discriminator;
      unsigned int observation_index;
   } data;
} kdtree_node;

typedef struct {
   float dimensions[3];
   unsigned int file_record_index;
} observation;

typedef struct {
   unsigned int num_elements;
   unsigned int tree_num_nodes;
   kdtree_node *tree_nodes;
   observation *observations;
   projPJ *projection;
} kdtree;

index *generate_kdtree_index_from_latlon_reader(latlon_reader_t *reader);
index *read_kdtree_index_from_file(FILE *input_file);

#define X  0
#define Y 1
#define T 2
#define TERMINAL  3
#define UNINITIALISED 4
#define LOWER 0
#define UPPER 1
#endif
