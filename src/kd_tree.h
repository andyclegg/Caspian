/**
 * @file
 * @author Andrew Clegg
 *
 * Data structures and defines for use with kdtrees.
 */
#ifndef  HEADER_KD_TREE_MINIMAL
#define HEADER_KD_TREE_MINIMAL
#include "result_set.h"
#include "latlon_reader.h"
#include "index.h"
#include "projector.h"

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
   projector *input_projector;
} kdtree;

index *generate_kdtree_index_from_latlon_reader(latlon_reader_t *reader);
index *read_kdtree_index_from_file(FILE *input_file);

/** Define X as having index 0 (e.g. in arrays) */
#define X  0

/** Define Y as having index 1 (e.g. in arrays) */
#define Y 1

/** Define T as having index 2 (e.g. in arrays) */
#define T 2

/** Node tag for terminal (leaf) nodes */
#define TERMINAL  3

/** Node tag for unitialised nodes */
#define UNINITIALISED 4

/** Lower bounds positioned at dimension + 0 */
#define LOWER 0

/** Upper bounds positioned at dimension + 1 */
#define UPPER 1
#endif
