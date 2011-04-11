/**
 * @file
 * @author Andrew Clegg
 *
 * Implementation of an adaptive kd-tree, specific to 2-dimensional horizontal coordinates.
 */
#include <float.h>
#include <math.h>
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>

#include "coordinate_reader.h"
#include "data_handling.h"
#include "spatial_index.h"
#include "kd_tree.h"
#include "projector.h"
#include "proj_projector.h"
#include "result_set.h"

/** Generate the index of the left child of the given index, within a binary tree.*/
#define LEFT_CHILD(index) (2*index +1)

/** Generate the index of the right child of the given index, within a binary tree.*/
#define RIGHT_CHILD(index) (2*index +2)

/** Generate the index of the parent of the given index, within a binary tree.*/
#define PARENT(index) (((index+1)/2) - 1)

#define SQUARED(x) ((x)*(x))

/** A format specifier for the on-disk binary file format. This should be incremented whenever the on-disk format changes.*/
#define KDTREE_FILE_FORMAT 2

/**
 * Create a kdtree for a given number of observations.. This includes calculating the size of the tree, and allocating space for the tree and the observations.
 *
 * @param num_observations The number of observations to be stored in the kdtree.
 * @return A pointer to an allocated (but unconstructed) kdtree.
 */
kdtree *construct_tree(unsigned int num_observations) {

   // Keep track of total bytes allocated (for debug purposes)
   size_t total_allocation = 0;

   // Allocate the kdtree struct
   kdtree *output_tree = malloc(sizeof(kdtree));
   if (output_tree == NULL) {
      fprintf(stderr, "Could not allocate space for a kdtree struct.\n");
      exit(EXIT_FAILURE);
   }
   total_allocation += sizeof(kdtree);

   // Compute the number of nodes needed - the number of leaf nodes
   // is essentially the number of observations rounded up to the nearest
   // power of 2.`
   float tree_number_of_leaf_nodes = powf(2.0, ceilf(log2f((float) num_observations)));
   // The total number of nodes in the tree is calculated from the
   // number of leaf nodes
   float tree_number_of_nodes_f = (2 * tree_number_of_leaf_nodes) - 1;
   // Make sure the tree always has at least 1 nodes in it
   unsigned int tree_number_of_nodes = (unsigned int) fmax(tree_number_of_nodes_f, 1.0);

   #ifdef DEBUG_KDTREE
   printf("Allocating a tree of size %d for %d leaf nodes\n", tree_number_of_nodes, num_observations);
   #endif

   output_tree->num_observations = num_observations;
   output_tree->tree_num_nodes = tree_number_of_nodes;

   // Allocate space for the nodes, and mark them as uninitialised
   size_t kdtree_node_allocate_size = sizeof(kdtree_node) * tree_number_of_nodes;
   output_tree->tree_nodes = malloc(kdtree_node_allocate_size);
   if (output_tree->tree_nodes == NULL) {
      fprintf(stderr, "Could not allocate %Zd bytes to store the kdtree nodes\n", kdtree_node_allocate_size);
      exit(EXIT_FAILURE);
   }
   total_allocation += kdtree_node_allocate_size;
   for (unsigned int i=0; i<tree_number_of_nodes; i++) {
      output_tree->tree_nodes[i].tag = UNINITIALISED;
   }

   // Allocate space for the observations
   size_t observation_allocate_size = sizeof(observation) * num_observations;
   output_tree->observations = malloc(observation_allocate_size);
   if (output_tree->observations == NULL) {
      fprintf(stderr, "Could not allocate %Zd bytes to store the kdtree observations\n", observation_allocate_size);
      exit(EXIT_FAILURE);
   }
   total_allocation += observation_allocate_size;

   #ifdef DEBUG_KDTREE
   printf("construct_tree: Total allocation is %ld bytes\n", (long int) total_allocation);
   #endif

   return output_tree;
}

/**
 * Recursively print out the contents of the subtree stemming from
 * the current_index node
 *
 * @param tree_p The tree to print from.
 * @param current_index The current node to print from.
 * @param indent The current indentation level (increased on every recursion)
 * */
static void inspect_tree_node(kdtree *tree_p, unsigned int current_index, int indent) {
   printf("%d", current_index);
   for (int i=1; i<=indent; i++) {
      if (i==indent) {
         printf("\\");
      } else {
         printf(" ");
      }
   }

   kdtree_node *cur_node = &tree_p->tree_nodes[current_index];
   if (cur_node->tag == TERMINAL) {
      printf("Terminal Node [Data Node %d] (%f, %f, %d)\n",
         cur_node->data.observation_index,
         tree_p->observations[cur_node->data.observation_index].dimensions[Y],
         tree_p->observations[cur_node->data.observation_index].dimensions[X],
         tree_p->observations[cur_node->data.observation_index].file_record_index);
      return;
   }

   // Not a terminal node - presumably a X or Y discriminator node
   if (cur_node->tag == Y) {
      printf("X: %f\n", cur_node->data.discriminator);
   } else if (cur_node->tag == X) {
      printf("Y: %f\n", cur_node->data.discriminator);
   }

   // Recurse
   inspect_tree_node(tree_p, LEFT_CHILD(current_index), indent + 1);
   inspect_tree_node(tree_p, RIGHT_CHILD(current_index), indent + 1);
}

/**
 * Print out the contents of the given tree.
 *
 * @param tree_p A pointer to the kdtree to print.
 */
void inspect_tree(kdtree *tree_p) {
   printf("Inspecting tree at %ld (%d observations)\n", (long) tree_p, tree_p->num_observations);
   inspect_tree_node(tree_p, 0, 0);
}

/**
 * Free the given kdtree.
 *
 * @param tree_p The kdtree to free.
 */
void free_tree(kdtree *tree_p) {
   free(tree_p->tree_nodes);
   free(tree_p->observations);
   free(tree_p);
}

/**
 * Recursively query the subtree stemming from the current_node_index node,
 * looking for observations within the given dimension bounds, and
 * storing the results in the given result set.
 *
 * @param tree_p The tree to query.
 * @param bounds The dimension bounds defining the query.
 * @param results The result_set to store the found results in.
 * @param current_node_index The index of the node to be queried from.
 */
static void query_kdtree_at(kdtree *tree_p, dimension_bounds bounds, result_set *results, unsigned int current_node_index) {

   // Lookup the current node
   kdtree_node *current_node = &tree_p->tree_nodes[current_node_index];

   if (current_node->tag == TERMINAL) {
      // Lookup the observation pointed to by the node
      observation *current_observation = &tree_p->observations[current_node->data.observation_index];

      // Check to see if the observation falls within the bounds
      if (
         (current_observation->dimensions[Y] >= bounds[2*Y + LOWER]) &&
         (current_observation->dimensions[Y] <= bounds[2*Y + UPPER]) &&
         (current_observation->dimensions[X] >= bounds[2*X + LOWER]) &&
         (current_observation->dimensions[X] <= bounds[2*X + UPPER]) &&
         (current_observation->dimensions[T] >= bounds[2*T + LOWER]) &&
         (current_observation->dimensions[T] <= bounds[2*T + UPPER])) {

         // Result falls within bounds: store in the result set
         results->insert(results, current_observation->dimensions[X], current_observation->dimensions[Y], current_observation->dimensions[T], current_observation->file_record_index);
      }
   } else {
      // 3 cases - the discriminator can either be less than our search range, within it, or above it
      // less than: search the left child of this node
      // within: search both children of this node
      // above: search the right child of this node

      if (current_node->data.discriminator >= bounds[2*(current_node->tag) + LOWER]) {
         //Search left child
         query_kdtree_at(tree_p, bounds, results, LEFT_CHILD(current_node_index));
      };

      if (current_node->data.discriminator <= bounds[2*(current_node->tag) + UPPER]) {
         //Search right child
         query_kdtree_at(tree_p, bounds, results, RIGHT_CHILD(current_node_index));
      };
   };
};

/**
 * Query a kdtree for points within given bounds.
 * @see index::query
 */

result_set *query_kdtree(spatial_index *toquery, dimension_bounds bounds) {
   result_set *results = result_set_init();
   query_kdtree_at((kdtree *)(toquery->data_structure), bounds, results, 0);
   return results;
}

/**
 * Find the single-nearest neighbour to the given target point in the given subtree marked by tree_index.
 *
 * @param tree_p The kdtree to search.
 * @param target_point A pointer to a 2-array of floats (X, then Y)
 * @return The closest observation to the given point.
 */
observation *nearest_neighbour_recursive(kdtree *tree_p, float *target_point, unsigned int tree_index) {
   kdtree_node *current_node = &tree_p->tree_nodes[tree_index];

   if (current_node->tag == TERMINAL) {
      // Return the observation pointed to by the current node
      return &tree_p->observations[current_node->data.observation_index];
   } else {
      // Non-terminal

      // Calculate the distance between the current node and the target point
      float pivot_target_distance = current_node->data.discriminator - target_point[current_node->tag];

      // Always Search the 'near' branch (the side of the tree which the target point falls in)
      observation *best = nearest_neighbour_recursive(tree_p, target_point, (pivot_target_distance > 0) ? LEFT_CHILD(tree_index) : RIGHT_CHILD(tree_index));

      // Only search the 'away' branch if the squared distance between the current best and the target is greater
      // Than the squared distance between the target and the branch pivot
      float current_best_squared_distance = SQUARED(best->dimensions[X] - target_point[X]) + SQUARED(best->dimensions[Y] - target_point[Y]);

      if (current_best_squared_distance > SQUARED(pivot_target_distance)) {
         // Search the 'away' branch
         observation *potential_best = nearest_neighbour_recursive(tree_p, target_point, (pivot_target_distance > 0) ? RIGHT_CHILD(tree_index) : LEFT_CHILD(tree_index));
         // Is potential best better than best?
         float potential_best_squared_distance = SQUARED(potential_best->dimensions[X] - target_point[X]) + SQUARED(potential_best->dimensions[Y] - target_point[Y]);
         if (potential_best_squared_distance < current_best_squared_distance) {
            return potential_best;
         }
      }
      return best;
   }
}

/**
 * Find the single-nearest neighbour to the given target point in the given tree.
 *
 * @param tree_p The kdtree to search.
 * @param target_point A pointer to a 2-array of floats (X, then Y)
 * @return The closest observation to the given point.
 */
observation *nearest_neighbour(kdtree *tree_p, float *target_point) {
   return nearest_neighbour_recursive(tree_p, target_point, 0);
}


/**
 * Verify the correctness of a given kdtree by tracing the ancestry of each leaf node to ensure that the discriminators (internal nodes) correctly divide the space.
 *
 * @param tree_p Pointer to a kdtree to verify.
 */
void verify_tree(kdtree *tree_p) {
   // Calculate the index which represents the first leaf node
   unsigned int start_of_leaves = (tree_p->tree_num_nodes + 1) / 2;

   // Iterate over every leaf node
   for (unsigned int current_leaf_node = start_of_leaves;
         current_leaf_node < tree_p->tree_num_nodes;
         current_leaf_node++) {

      // Get the current leaf node
      kdtree_node *current_tree_node = &tree_p->tree_nodes[current_leaf_node];

      // Don't bother with unitialised nodes
      if (current_tree_node->tag == UNINITIALISED) {
         continue;
      }
      unsigned int observation_index = current_tree_node->data.observation_index;

      float dimensions[2];
      dimensions[Y] = tree_p->observations[observation_index].dimensions[Y];
      dimensions[X] = tree_p->observations[observation_index].dimensions[X];

      //We have a point - now traverse back up the tree, verifying that the point is always on the correct side of the discriminator
      //Note that left children are always stored in odd node slots, and right children in even node slots
      unsigned int temp_tree_node_index = current_leaf_node;
      while(temp_tree_node_index > 0) {
         unsigned int parent_tree_node = PARENT(temp_tree_node_index);

         // All left children are stored in odd indices
         int is_left_child_of_parent = ((temp_tree_node_index % 2) == 1);

         float parent_discriminator = tree_p->tree_nodes[parent_tree_node].data.discriminator;
         short int parent_discriminator_type = tree_p->tree_nodes[parent_tree_node].tag;
         float current_applicable_value = dimensions[parent_discriminator_type];

         // Check to see if the parent discriminator has the correct relationship to the observation value
         int is_correct;
         if (is_left_child_of_parent) {
            is_correct = (parent_discriminator >= current_applicable_value);
         } else {
            is_correct = (parent_discriminator <= current_applicable_value);
         }

         if (!is_correct) {
            printf("Point (%f, %f) had an incorrect lineage - specifically, as a ", dimensions[Y], dimensions[X]);
            if (is_left_child_of_parent) {
               printf("left");
            } else {
               printf("right");
            }
            printf(" child (%d) of a parent tree node stored at %d of discrimination type %d, the parent discriminator %f is invalid\n", temp_tree_node_index, parent_tree_node, parent_discriminator_type, parent_discriminator);
         }

         // Ascend one level in the tree
         temp_tree_node_index = parent_tree_node;

      }
  }
}


/**
 * Compare two observations along a given comparison dimension.
 *
 * @param a Pointer to the first observation to compare.
 * @param b Pointer to the second observation to compare.
 * @param comparison_dimension Integer to specify the dimension to sort on (e.g. #X, #Y)
 * @return -1 if a < b, 0 if a == b, 1 if a > b
 */
static int compare_observations(const void* a, const void* b, short int comparison_dimension) {
   observation *t_a = (observation *) a;
   observation *t_b = (observation *) b;
   if (t_a->dimensions[comparison_dimension] < t_b->dimensions[comparison_dimension]) {
      return -1;
   }
   return (t_a->dimensions[comparison_dimension] > t_b->dimensions[comparison_dimension]);
}

/**
 * Compare two observations based on their latitude value. Compatible with qsort
 *
 * @param a Pointer to the first observation to compare.
 * @param b Pointer to the second observation to compare.
 * @return -1 if a < b, 0 if a == b, 1 if a > b
 */
static int compare_latitudes(const void* a, const void* b) {
   return compare_observations(a, b, Y);
}

/**
 * Compare two observations based on their longitude value. Compatible with qsort
 *
 * @param a Pointer to the first observation to compare.
 * @param b Pointer to the second observation to compare.
 * @return -1 if a < b, 0 if a == b, 1 if a > b
 */
static int compare_longitudes(const void* a, const void* b) {
   return compare_observations(a, b, X);
}

/**
 * Recursively turn a section of data into an adaptive KDtree.
 *
 * @param tree_p The tree to build.
 * @param first_node_index The index of the start of the section of data being built.
 * @param last_node_index The index of teh end of the section of data being built.
 * @param current_tree_index The index of the node in the tree that represents this section of data.
 * @param current_sort_dimension The dimension (#X, #Y) by which the data is currently stored. Use -1 if the data is unsorted.
 */
static void recursive_build_kd_tree(kdtree *tree_p,unsigned int first_node_index, unsigned int last_node_index,unsigned int current_tree_index, short int current_sort_dimension) {

   observation *observations = tree_p->observations;
   kdtree_node *current_node = &tree_p->tree_nodes[current_tree_index];

   if (first_node_index == last_node_index) {
      // Bottom out - store a terminal node in the tree
      current_node->tag = TERMINAL;
      current_node->data.observation_index = first_node_index;
      return;
   }

   //Choose the axis that varies most in observations[first_node_index:last_node_index], compute the median, store a discriminator node
   //representing the division of this data, and recurse for both sides of the split data.
   float y_min = FLT_MAX;
   float x_min = FLT_MAX;
   float y_max = -FLT_MAX;
   float x_max = -FLT_MAX;

   // Because the data is usually sorted, we can save some calls fo fmin & fmax by just reading off the min and max directly
   // This yields a small improvement in build times for large datasets.
   if (current_sort_dimension == X) {
      x_min = observations[first_node_index].dimensions[X];
      x_max = observations[last_node_index].dimensions[X];
      for (unsigned int current_index = first_node_index; current_index <= last_node_index; current_index++) {
         y_min = fmin(y_min, observations[current_index].dimensions[Y]);
         y_max = fmax(y_max, observations[current_index].dimensions[Y]);
      }
   } else if (current_sort_dimension == Y) {
      y_min = observations[first_node_index].dimensions[Y];
      y_max = observations[last_node_index].dimensions[Y];
      for (unsigned int current_index = first_node_index; current_index <= last_node_index; current_index++) {
         x_min = fmin(x_min, observations[current_index].dimensions[X]);
         x_max = fmax(x_max, observations[current_index].dimensions[X]);
      }
   } else {
      for (unsigned int current_index = first_node_index; current_index <= last_node_index; current_index+=2) {
         y_min = fmin(y_min, observations[current_index].dimensions[Y]);
         x_min = fmin(x_min, observations[current_index].dimensions[X]);
         y_max = fmax(y_max, observations[current_index].dimensions[Y]);
         x_max = fmax(x_max, observations[current_index].dimensions[X]);
      }
   }


   // Select the dimension to discriminate on
   int (*comparison_function)(const void *, const void *);
   short int discrimination_dimension;
   if (fabsf(y_max - y_min) >= fabsf(x_max - x_min)) {
      discrimination_dimension = Y;
      comparison_function = compare_latitudes;
   } else {
      discrimination_dimension = X;
      comparison_function = compare_longitudes;
   }

   // Sort the data according to the current discrimination dimension if necessary
   if (discrimination_dimension != current_sort_dimension) {
      qsort(&observations[first_node_index], last_node_index - first_node_index + 1, sizeof(observation), comparison_function);
   }

   // Calculate the discriminator value, and the indices of the split point in the data
   float discriminator;
   unsigned int split_node_index;
   if (((last_node_index - first_node_index) % 2) != 0) {
      //even number of nodes
      split_node_index = first_node_index + ((last_node_index - first_node_index - 1) / 2);

      // median is the mean of the 2 central values
      discriminator = (observations[split_node_index].dimensions[discrimination_dimension] + observations[split_node_index + 1].dimensions[discrimination_dimension]) / 2.0;
   } else {
      //odd number of nodes
      split_node_index = first_node_index + ((last_node_index - first_node_index) / 2);

      // median is the central value
      discriminator = observations[split_node_index].dimensions[discrimination_dimension];
   }

   //Store this information back into the tree
   current_node->tag = discrimination_dimension;
   current_node->data.discriminator = discriminator;

   //Recurse - advise OpenMP that it may split into 2 threads at this point (running each section in parallel)
   #pragma omp parallel sections num_threads(2)
   {
      #pragma omp section
      recursive_build_kd_tree(tree_p, first_node_index, split_node_index, LEFT_CHILD(current_tree_index), discrimination_dimension);
      #pragma omp section
      recursive_build_kd_tree(tree_p, split_node_index + 1, last_node_index, RIGHT_CHILD(current_tree_index), discrimination_dimension);
   }
}

/**
 * Fill a constructed kdtree from the values found in the given reader.
 *
 * @param tree_p The constructed kdtree to fill.
 * @param reader The coordinate_reader to read the values from.
 */
void fill_tree_from_reader(kdtree *tree_p, coordinate_reader *reader) {

   observation *observations = tree_p->observations;

   register int result;
   for(unsigned int current_index = 0; current_index < reader->num_records; current_index++) {
      // Link the observations in the tree to the current record from the coordinate reader
      observations[current_index].file_record_index = current_index;

      // Read the values from the coordinate reader into the observation
      result = reader->read(reader, &observations[current_index].dimensions[X], &observations[current_index].dimensions[Y], &observations[current_index].dimensions[T]);
      if (!result) {
         printf("Failed to read all observations from files\n");
         exit(EXIT_FAILURE);
      }
   }

   // Call recursive_build_kd_tree, accross the entire range of data, with current node index as 0 (the root), and current sort order as -1 (equivalent to unsorted)
   recursive_build_kd_tree(tree_p, 0, reader->num_records - 1, 0, -1);
}

/**
 * Write the given kdtree-based index to the given file.
 *
 * @param towrite The index to write (must be a kdtree based index).
 * @param output_file The file to write the binary representation of the index to.
 */
void write_kdtree_index_to_file(spatial_index *towrite, FILE *output_file) {
   kdtree *tree_p = (kdtree *) towrite->data_structure;

   // Write the header to file
   unsigned int file_format_number = KDTREE_FILE_FORMAT;
   fwrite(&file_format_number, sizeof(unsigned int), 1, output_file);

   // Serialize the projector to the file
   towrite->input_projector->serialize_to_file(towrite->input_projector, output_file);

   // Write the sizes of the data
   fwrite(&tree_p->num_observations, sizeof(unsigned int), 1, output_file);
   fwrite(&tree_p->tree_num_nodes, sizeof(unsigned int), 1, output_file);

   // Write the tree data to the file
   fwrite(tree_p->tree_nodes, sizeof(kdtree_node), tree_p->tree_num_nodes, output_file);
   fwrite(tree_p->observations, sizeof(observation), tree_p->num_observations, output_file);

   // Write a concluding header
   fwrite(&file_format_number, sizeof(unsigned int), 1, output_file);
}

/**
 * Free a kdtree-based index.
 *
 * @param tofree The kdtree-based index to free.
 */
void free_kdtree_index(spatial_index *tofree) {
   kdtree *tree_p = (kdtree *) tofree->data_structure;
   free_tree(tree_p);
   free(tofree);
}

/**
 * Return a kdtree-based index from the given file.
 *
 * @param input_file The file from which to read the index.
 * @return A pointer to a constructed and initialised kdtree-based index.
 */
spatial_index *read_kdtree_index_from_file(FILE *input_file) {
   // Read and check the header
   unsigned int file_format_number;
   fread(&file_format_number, sizeof(unsigned int), 1, input_file);

   if (file_format_number != KDTREE_FILE_FORMAT) {
      fprintf(stderr, "Wrong disk file format (read %d, expected %d)\n", file_format_number, KDTREE_FILE_FORMAT);
      exit(EXIT_FAILURE);
   }

   // Get the projector from the file
   projector *input_projector = get_proj_projector_from_file(input_file);
   if (input_projector == NULL) {
      fprintf(stderr, "Couldn't obtain input projection from file\n");
      exit(EXIT_FAILURE);
   }

   // Read the sizes of data for the kdtree
   unsigned int num_observations;
   unsigned int tree_num_nodes;
   fread(&num_observations, sizeof(unsigned int), 1, input_file);
   fread(&tree_num_nodes, sizeof(unsigned int), 1, input_file);

   // Create tree
   kdtree *tree_p = construct_tree(num_observations);

   // Check the computed number of tree nodes against the number read from file
   if (tree_num_nodes != tree_p->tree_num_nodes) {
      fprintf(stderr, "Mismatch in number of tree nodes (read %d, computed %d)\n", tree_num_nodes, tree_p->tree_num_nodes);
      exit(EXIT_FAILURE);
   }

   // Read the data into the tree
   fread(tree_p->tree_nodes, sizeof(kdtree_node), tree_p->tree_num_nodes, input_file);
   fread(tree_p->observations, sizeof(observation), tree_p->num_observations, input_file);

   // Check concluding header
   fread(&file_format_number, sizeof(unsigned int), 1, input_file);
   if (file_format_number != KDTREE_FILE_FORMAT) {
      fprintf(stderr, "Wrong concluding header (read %d, expected %d)\n", file_format_number, KDTREE_FILE_FORMAT);
      exit(EXIT_FAILURE);
   }

   #ifdef DEBUG
   printf("Verifying tree\n");
   verify_tree(tree_p);
   printf("Tree verified as correct\n");
   #endif

   // Turn this into an index
   spatial_index *output_index = malloc(sizeof(spatial_index));
   if (output_index == NULL) {
      fprintf(stderr, "Failed to allocate space for index\n");
      return NULL;
   }

   output_index->data_structure = tree_p;
   output_index->input_projector = input_projector;
   output_index->num_observations = tree_p->num_observations;
   output_index->write_to_file = &write_kdtree_index_to_file;
   output_index->free = &free_kdtree_index;
   output_index->query = &query_kdtree;

   return output_index;
}

/**
 * Construct an adaptive kdtree from a set of geolocation information.
 *
 * @param reader A coordinate_reader instance (source of gelocation information)
 * @return Pointer to an index structure.
 */
spatial_index *generate_kdtree_index_from_coordinate_reader(coordinate_reader *reader) {
   kdtree *root_p = construct_tree(reader->num_records);

   fill_tree_from_reader(root_p, reader);

   #ifdef DEBUG
   printf("Verifying tree\n");
   verify_tree(root_p);
   printf("Tree verified as correct\n");
   #endif

   // Compile this into an index
   spatial_index *output_index = malloc(sizeof(spatial_index));
   if (output_index == NULL) {
      fprintf(stderr, "Failed to allocate space for index\n");
      exit(EXIT_FAILURE);
   }

   output_index->data_structure = root_p;
   output_index->input_projector = reader->input_projector;
   output_index->num_observations = root_p->num_observations;
   output_index->write_to_file = &write_kdtree_index_to_file;
   output_index->free = &free_kdtree_index;
   output_index->query = &query_kdtree;

   return output_index;
}
