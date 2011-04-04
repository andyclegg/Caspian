/**
 * @file
 * @author Andrew Clegg
 *
 * Data structures and defines for use with kdtrees.
 */
#ifndef  HEADER_KD_TREE_MINIMAL
#define HEADER_KD_TREE_MINIMAL
#include "coordinate_reader.h"
#include "index.h"
#include "projector.h"
#include "result_set.h"

/**
 * Defines a node of an adaptive kdtree.
 */
typedef struct {
   /** A tag representing the type of this node. Internal nodes are impliclty defined by the use of #X or #Y (defining the dimension on which this node discriminates). Leaf nodes are #TERMINAL or #UNINITIALISED.*/
   short int tag;

   /** Storage for either the discriminator (internal nodes), or the observation index (leaf nodes).*/
   union tree_node_union {
      /** The discriminating value on this node's dimension (defined by tag)*/
      float discriminator;

      /** The observation index of this leaf node - used to lookup the appropriate observation.*/
      unsigned int observation_index;
   } data;
} kdtree_node;

/**
 * Define a single observation, constructed from X & Y horizontal coordinates, a time coordinate, and the index of this observation in the data files.*/
typedef struct {
   /** Array storing the X, Y and Time values.*/
   float dimensions[3];

   /** The corresponding index into the original data files.*/
   unsigned int file_record_index;
} observation;

/**
 * Representation of an adaptive kdtree.
 */
typedef struct {
   /** The number of data elements represented by this tree.*/
   unsigned int num_elements;

   /** The number of nodes (internal + leaf) in the tree.*/
   unsigned int tree_num_nodes;

   /** Pointer to a 1-dimensional array of nodes.*/
   kdtree_node *tree_nodes;

   /** Pointer to a 1-dimensional array of observations.*/
   observation *observations;

   /**
    * The projector which projected these values
    * @todo This really shouldn't be here.
    */
   projector *input_projector;
} kdtree;

index *generate_kdtree_index_from_coordinate_reader(coordinate_reader *reader);
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
