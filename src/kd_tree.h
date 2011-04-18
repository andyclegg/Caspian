/**
  * @file
  *
  * Data structures and defines for use with kdtrees.
  */
#ifndef  HEADER_KD_TREE_MINIMAL
#define HEADER_KD_TREE_MINIMAL
#include "coordinate_reader.h"
#include "spatial_index.h"
#include "projector.h"
#include "result_set.h"

/**
  * A node of an adaptive KDtree.
  */
typedef struct {
   /** A tag representing the type of this node. Internal nodes are impliclty
    *defined by the use of #X or #Y (defining the dimension on which this node
    *discriminates). Leaf nodes are #TERMINAL or #UNINITIALISED.*/
   short int tag;

   /** Storage for either the discriminator (internal nodes), or the observation
    *index (leaf nodes).*/
   union tree_node_union {

      /** The discriminating value on this node's dimension (defined by tag)*/
      float discriminator;

      /** The observation index of this leaf node - used to lookup the
       *appropriate observation.*/
      unsigned int observation_index;
   } data;
} kdtree_node;

/**
  * Define a single observation, constructed from X & Y horizontal coordinates,
  *a time coordinate, and the index of this observation in the data files.*/
typedef struct {
   /** Array storing the X, Y and Time values.*/
   float dimensions[3];

   /** The corresponding index into the original data files.*/
   unsigned int file_record_index;
} observation;

/**
  * An adaptive KDtree index.
  */
typedef struct {
   /** The number of observations represented by this tree.*/
   unsigned int num_observations;

   /** The number of nodes (internal + leaf) in the tree.*/
   unsigned int tree_num_nodes;

   /** Pointer to a 1-dimensional array of nodes.*/
   kdtree_node *tree_nodes;

   /** Pointer to a 1-dimensional array of observations.*/
   observation *observations;

} kdtree;

// Function prototypes - implementations id kd_tree.c
spatial_index *generate_kdtree_index_from_coordinate_reader(
   coordinate_reader *reader);
spatial_index *read_kdtree_index_from_file(FILE *input_file);

/** Node tag for terminal (leaf) nodes */
#define TERMINAL  254

/** Node tag for unitialised nodes */
#define UNINITIALISED 255

#endif
