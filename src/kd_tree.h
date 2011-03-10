#ifndef  HEADER_KD_TREE_MINIMAL
#define HEADER_KD_TREE_MINIMAL
#include "result_set.h"
#include "latlon_reader.h"

struct tree_node {
   short int tag;
   union tree_node_union {
      float discriminator;
      unsigned int observation_index;
   } data;
};

struct observation {
   float dimensions[3];
   unsigned int file_record_index;
};

struct tree {
   unsigned int num_elements;
   unsigned int tree_num_nodes;
   struct tree_node *tree_nodes;
   struct observation *observations;
};


int fill_tree_from_reader(struct tree **tree_pp, latlon_reader_t *reader);
result_set_t *query_tree(struct tree *tree_p, float *dimension_bounds);
struct observation *nearest_neighbour(struct tree *tree_p, float *target_point);
void verify_tree(struct tree *tree_p);
void construct_tree(struct tree **tree_pp, unsigned int num_elements);
void free_tree(struct tree *tree_p);
void inspect_tree(struct tree *tree_p);
void kdtree_save_to_file(FILE *output_file, struct tree *tree_p);
struct tree *kdtree_read_from_file(FILE *input_file);

#define X  0
#define Y 1
#define T 2
#define TERMINAL  3
#define UNINITIALISED 4
#define LOWER 0
#define UPPER 1
#endif
