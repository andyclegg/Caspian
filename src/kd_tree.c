#include <errno.h>
#include <float.h>
#include <math.h>
#include <proj_api.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "kd_tree.h"
#include "latlon_reader.h"
#include "result_set.h"

#define LEFT_CHILD(index) (2*(index+1) - 1)
#define RIGHT_CHILD(index) (2*(index+1))
#define PARENT(index) (((index+1)/2) - 1)
#define SQUARED(x) ((x)*(x))

#define KDTREE_FILE_FORMAT 1

void save_to_file(struct tree *tree_p, FILE *output_file) {
   // Write the header to file
   unsigned int file_format_number = KDTREE_FILE_FORMAT;
   fwrite(&file_format_number, sizeof(unsigned int), 1, output_file);
   fwrite(&tree_p->num_elements, sizeof(unsigned int), 1, output_file);
   fwrite(&tree_p->tree_num_nodes, sizeof(unsigned int), 1, output_file);

   // Write the data to file
   fwrite(tree_p->tree_nodes, sizeof(struct tree_node), tree_p->tree_num_nodes, output_file);
   fwrite(tree_p->observations, sizeof(struct observation), tree_p->num_elements, output_file);

   // Write an end of file marker for paranoia
   fwrite(&file_format_number, sizeof(unsigned int), 1, output_file);
}

struct tree *read_from_file(FILE *input_file) {
   // Read and check the header
   unsigned int file_format_number;
   unsigned int num_elements;
   unsigned int tree_num_nodes;
   fread(&file_format_number, sizeof(unsigned int), 1, input_file);
   fread(&num_elements, sizeof(unsigned int), 1, input_file);
   fread(&tree_num_nodes, sizeof(unsigned int), 1, input_file);

   if (file_format_number != KDTREE_FILE_FORMAT) {
      fprintf(stderr, "Wrong disk file format (read %d, expected %d)\n", file_format_number, KDTREE_FILE_FORMAT);
      exit(-1);
   }

   // Create tree
   struct tree *tree_p;
   construct_tree(&tree_p, num_elements);

   // Check the computed number of tree nodes against the number read from file
   if (tree_num_nodes != tree_p->tree_num_nodes) {
      fprintf(stderr, "Mismatch in number of tree nodes (read %d, computed %d)\n", tree_num_nodes, tree_p->tree_num_nodes);
      exit(-1);
   }

   // Read the data into the tree
   fread(tree_p->tree_nodes, sizeof(struct tree_node), tree_p->tree_num_nodes, input_file);
   fread(tree_p->observations, sizeof(struct observation), tree_p->num_elements, input_file);

   // Paranoidly check the end of file header
   fread(&file_format_number, sizeof(unsigned int), 1, input_file);
   if (file_format_number != KDTREE_FILE_FORMAT) {
      fprintf(stderr, "End of file marker incorrect (read %d, expected %d)\n", file_format_number, KDTREE_FILE_FORMAT);
      exit(-1);
   }

   // All done!
   return tree_p;
}

static void inspect_tree_node(struct tree *tree_p, unsigned int current_index, int indent) {
   printf("%d", current_index);
   for (int i=1; i<=indent; i++) {
      if (i==indent) {
         printf("\\");
      } else {
         printf(" ");
      }
   }

   struct tree_node *cur_node = &tree_p->tree_nodes[current_index];
   if (cur_node->tag == TERMINAL) {
      printf("Terminal Node [Data Node %d] (%f, %f, %d)\n",
         cur_node->data.observation_index,
         tree_p->observations[cur_node->data.observation_index].dimensions[Y],
         tree_p->observations[cur_node->data.observation_index].dimensions[X],
         tree_p->observations[cur_node->data.observation_index].file_record_index);
      return;
   }

   if (cur_node->tag == Y) {
      printf("X: %f\n", cur_node->data.discriminator);
   } else if (cur_node->tag == X) {
      printf("Y: %f\n", cur_node->data.discriminator);
   }
   inspect_tree_node(tree_p, LEFT_CHILD(current_index), indent + 1);
   inspect_tree_node(tree_p, RIGHT_CHILD(current_index), indent + 1);
}

void inspect_tree(struct tree *tree_p) {
   printf("Inspecting tree at %ld (%d elements)\n", (long) tree_p, tree_p->num_elements);
   inspect_tree_node(tree_p, 0, 0);
}

void free_tree(struct tree *tree_p) {
   free(tree_p->tree_nodes);
   free(tree_p->observations);
   free(tree_p);
}

void construct_tree(struct tree **tree_pp, unsigned int num_elements) {
   size_t total_allocation = 0;
   *tree_pp = malloc(sizeof(struct tree));
   total_allocation += sizeof(struct tree);
   struct tree *tree_p = *tree_pp;
   float tree_number_of_leaf_elements = powf(2.0, ceilf(log2f((float) num_elements)));
   float tree_number_of_elements_f = (2 * tree_number_of_leaf_elements) - 1;
   unsigned int tree_number_of_elements = (unsigned int) fmax(tree_number_of_elements_f, 1.0);

   #ifdef DEBUG_KDTREE
   printf("Allocating a tree of size %d for %d leaf elements\n", tree_number_of_elements, num_elements);
   #endif

   tree_p->num_elements = num_elements;
   tree_p->tree_num_nodes = tree_number_of_elements;

   tree_p->tree_nodes = calloc(sizeof(struct tree_node), tree_number_of_elements);
   total_allocation += sizeof(struct tree_node) * tree_number_of_elements;
   for (unsigned int i=0; i<tree_number_of_elements; i++) {
      tree_p->tree_nodes[i].tag = UNINITIALISED;
   }

   tree_p->observations = calloc(sizeof(struct observation), num_elements);
   total_allocation += sizeof(struct observation) * num_elements;

   #ifdef DEBUG_KDTREE
   printf("construct_tree: Total allocation is %ld bytes\n", (long int) total_allocation);
   #endif
}

static void query_tree_at(struct tree *tree_p, float *dimension_bounds, result_set_t *results, unsigned int current_element) {
   #ifdef DEBUG_KDTREE
   printf("Querying tree at %d\n", current_element);
   #endif

   struct tree_node *current_node = &tree_p->tree_nodes[current_element];

   if (current_node->tag == TERMINAL) {
      struct observation *current_observation = &tree_p->observations[current_node->data.observation_index];
      if (
         (current_observation->dimensions[Y] >= dimension_bounds[2*Y + LOWER]) &&
         (current_observation->dimensions[Y] <= dimension_bounds[2*Y + UPPER]) &&
         (current_observation->dimensions[X] >= dimension_bounds[2*X + LOWER]) &&
         (current_observation->dimensions[X] <= dimension_bounds[2*X + UPPER])) {
         #ifdef DEBUG_KDTREE
         printf("Result: (%f, %f, %d)\n", current_observation->dimensions[Y], current_observation->dimensions[X], current_observation->file_record_index);
         #endif
         result_set_insert(results, current_observation->dimensions[X], current_observation->dimensions[Y], current_observation->file_record_index);
      }
      #ifdef DEBUG_KDTREE
        else {
         printf("No match: (%f, %f, %d) against (%f:%f:%f:%f)\n", current_observation->dimensions[Y], current_observation->dimensions[X], current_observation->file_record_index, dimension_bounds[0], dimension_bounds[1], dimension_bounds[2], dimension_bounds[3]);
      }
      #endif
   } else {
      // 3 cases - the discriminator can either be less than our search range, within it, or above it
      // less than: search right of this node
      // within: search both ways
      // above: search left of this node
      //
      #ifdef DEBUG_KDTREE
      printf("Discriminator: %f (%d), Comparison: (%f:%f)\n", current_node->data.discriminator, current_node->tag, dimension_bounds[2*(current_node->tag) + LOWER], dimension_bounds[2*(current_node->tag) + UPPER]);
      #endif

      if (current_node->data.discriminator >= dimension_bounds[2*(current_node->tag) + LOWER]) {
         //Search left
         #ifdef DEBUG_KDTREE
         printf("Searching left\n");
         #endif
      };

      if (current_node->data.discriminator <= dimension_bounds[2*(current_node->tag) + UPPER]) {
         //Search right
         #ifdef DEBUG_KDTREE
         printf("Searching right\n");
         #endif
      };

      if (current_node->data.discriminator >= dimension_bounds[2*(current_node->tag) + LOWER]) {
         //Search left
         query_tree_at(tree_p, dimension_bounds, results, LEFT_CHILD(current_element));
      };

      if (current_node->data.discriminator <= dimension_bounds[2*(current_node->tag) + UPPER]) {
         //Search right
         query_tree_at(tree_p, dimension_bounds, results, RIGHT_CHILD(current_element));
      };
   };
};

struct observation *nearest_neighbour_recursive(struct tree *tree_p, float *target_point, unsigned int tree_index) {
   struct tree_node *current_node = &tree_p->tree_nodes[tree_index];
   if (current_node->tag == TERMINAL) {
      return &tree_p->observations[current_node->data.observation_index];
   } else { // Non-terminal

      float pivot_target_distance = current_node->data.discriminator - target_point[current_node->tag];

      // Search the 'near' branch
      struct observation *best = nearest_neighbour_recursive(tree_p, target_point, (pivot_target_distance > 0) ? LEFT_CHILD(tree_index) : RIGHT_CHILD(tree_index));

      // Only search the 'away' branch if the squared distance between the current best and the target is greater
      // Than the squared distance between the target and the branch pivot
      float current_best_squared_distance = SQUARED(best->dimensions[X] - target_point[X]) + SQUARED(best->dimensions[Y] - target_point[Y]);
      if (current_best_squared_distance > SQUARED(pivot_target_distance)) {
         // Search the 'away' branch
         struct observation *potential_best = nearest_neighbour_recursive(tree_p, target_point, (pivot_target_distance > 0) ? RIGHT_CHILD(tree_index) : LEFT_CHILD(tree_index));
         // Is potential best better than best?
         float potential_best_squared_distance = SQUARED(potential_best->dimensions[X] - target_point[X]) + SQUARED(potential_best->dimensions[Y] - target_point[Y]);
         if (potential_best_squared_distance < current_best_squared_distance) {
            return potential_best;
         }
      }

      return best;
   }
}

struct observation *nearest_neighbour(struct tree *tree_p, float *target_point) {
   return nearest_neighbour_recursive(tree_p, target_point, 0);
}


void verify_tree(struct tree *tree_p) {
   unsigned int start_of_leaves = (tree_p->tree_num_nodes + 1) / 2;

   for (unsigned int current_leaf_element = start_of_leaves;
         current_leaf_element < tree_p->tree_num_nodes;
         current_leaf_element++) {
      struct tree_node *current_tree_node = &tree_p->tree_nodes[current_leaf_element];
      if (current_tree_node->tag == UNINITIALISED) {
         continue;
      }
      unsigned int observation_index = current_tree_node->data.observation_index;

      float dimensions[2];
      dimensions[Y] = tree_p->observations[observation_index].dimensions[Y];
      dimensions[X] = tree_p->observations[observation_index].dimensions[X];

      //We have a point - now traverse back up the tree, verifying that the point is always on the correct side of the discriminator
      //Note that left children are always stored in odd node slots, and right children in even node slots
      unsigned int temp_tree_node_index = current_leaf_element;
      while(temp_tree_node_index > 0) {
         unsigned int parent_tree_node = PARENT(temp_tree_node_index);
         int is_left_child_of_parent = ((temp_tree_node_index % 2) == 1);

         float parent_discriminator = tree_p->tree_nodes[parent_tree_node].data.discriminator;
         short int parent_discriminator_type = tree_p->tree_nodes[parent_tree_node].tag;
         float current_applicable_value = dimensions[parent_discriminator_type];

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

         temp_tree_node_index = parent_tree_node;

      }
  }
}

result_set_t *query_tree(struct tree *tree_p, float *dimension_bounds) {
   result_set_t *results = result_set_init();
   query_tree_at(tree_p, dimension_bounds, results, 0);
   return results;
}


static int compare_observations(const void* a, const void* b, short int comparison_dimension) {
   struct observation *t_a = (struct observation *) a;
   struct observation *t_b = (struct observation *) b;
   if (t_a->dimensions[comparison_dimension] < t_b->dimensions[comparison_dimension]) {
      return -1;
   }
   return (t_a->dimensions[comparison_dimension] > t_b->dimensions[comparison_dimension]);
}

static int compare_latitudes(const void* a, const void* b) {
   return compare_observations(a, b, Y);
}

static int compare_longitudes(const void* a, const void* b) {
   return compare_observations(a, b, X);
}

static int recursive_build_kd_tree(struct tree *tree_p,unsigned int first_element,unsigned int last_element,unsigned int current_tree_index, short int current_sort_dimension) {

   #ifdef DEBUG_KDTREE
   printf("->Entering recursive_build_kd_tree\ncurrent_tree_index = %d\nfirst_element = %d\nlast_element = %d\n", current_tree_index, first_element, last_element);
   #endif

   struct observation *observations = tree_p->observations;
   struct tree_node *current_element = &tree_p->tree_nodes[current_tree_index];

   //Choose the axis that varies most in observations[first_element:last_element], then fill out current_tree_index's children, and call itself recursively
   //bottom out when first_element == last_element, then build the data section of the tree
   if (first_element == last_element) {
      // Bottom out
      #ifdef DEBUG_KDTREE
      printf("Bottoming out - setting element %d's tag to TERMINAL and pointing it to observation %d\n", current_tree_index, first_element);
      #endif
      current_element->tag = TERMINAL;
      current_element->data.observation_index = first_element;
   } else {
      float lat_min = FLT_MAX;
      float lon_min = FLT_MAX;
      float lat_max = -FLT_MAX;
      float lon_max = -FLT_MAX;
      for (int current_index = first_element; current_index <= last_element; current_index++) {
         lat_min = fmin(lat_min, observations[current_index].dimensions[Y]);
         lon_min = fmin(lon_min, observations[current_index].dimensions[X]);
         lat_max = fmax(lat_max, observations[current_index].dimensions[Y]);
         lon_max = fmax(lon_max, observations[current_index].dimensions[X]);
      }
      #ifdef DEBUG_KDTREE
      printf("Lat: %f -> %f\nLon: %f -> %f\n", lat_min, lat_max, lon_min, lon_max);
      #endif
      //TODO: Bias towards the dimension that we are already sorted on?
      int (*comparison_function)(const void *, const void *);
      short int d_type;
      float discriminator;
     unsigned int end_left, start_right;

      if (fabsf(lat_max - lat_min) >= fabsf(lon_max - lon_min)) {
         #ifdef DEBUG_KDTREE
         printf("Discriminating on latitude\n");
         #endif
         d_type = Y;
         comparison_function = compare_latitudes;
      } else {
         #ifdef DEBUG_KDTREE
         printf("Discriminating on longitude\n");
         #endif
         d_type = X;
         comparison_function = compare_longitudes;
      }

      if (d_type != current_sort_dimension) {

         #ifdef DEBUG_KDTREE
         printf("Calling qsort from %d [%ld] with %d elements\n", first_element, (long int) &observations[first_element], last_element - first_element);
         #endif

         qsort(&observations[first_element], last_element - first_element + 1, sizeof(struct observation), comparison_function);
      }

      if (((last_element - first_element) % 2) != 0) {
         //even number of elements
         #ifdef DEBUG_KDTREE
         printf("Even number of elements\n");
         #endif
         end_left = first_element + ((last_element - first_element - 1) / 2);
         start_right = end_left + 1;
         discriminator = (observations[end_left].dimensions[d_type] + observations[start_right].dimensions[d_type]) / 2.0;
      } else {
         //odd number of elements
         #ifdef DEBUG_KDTREE
         printf("Odd number of elements\n");
         #endif
         end_left = ((last_element - first_element) / 2) + first_element;
         start_right = end_left + 1;
         discriminator = observations[end_left].dimensions[d_type];
      }
      #ifdef DEBUG_KDTREE
      printf("Left: (%d:%d), Right: (%d:%d), Discriminator: %f (%d)\n", first_element, end_left, start_right, last_element, discriminator, d_type);
      #endif

      //Store this information back into the tree
      current_element->tag = d_type;
      current_element->data.discriminator = discriminator;

      //Recurse
      #pragma omp parallel sections num_threads(2)
      {
         #pragma omp section
         recursive_build_kd_tree(tree_p, first_element, end_left, LEFT_CHILD(current_tree_index), d_type);
         #pragma omp section
         recursive_build_kd_tree(tree_p, start_right, last_element, RIGHT_CHILD(current_tree_index), d_type);
      }
   }

   #ifdef DEBUG_KDTREE
   printf("->Leaving recursive_build_kd_tree\n");
   #endif

   return 0;
}

int fill_tree_from_reader(struct tree **tree_pp, latlon_reader_t *reader) {

   unsigned int no_elements = latlon_reader_get_num_records(reader);

   //Construct the tree
   construct_tree(tree_pp, no_elements);
   struct tree *tree_p = *tree_pp;

   struct observation *observations = tree_p->observations;

   register int result;
   for(int current_index = 0; current_index < no_elements; current_index++) {
      observations[current_index].file_record_index = current_index;
      result = latlon_reader_read(reader, &observations[current_index].dimensions[X], &observations[current_index].dimensions[Y]);
      if (!result) {
         printf("Critical: Failed to read all elements from files\n");
         return 0;
      }
   }

   // Call recursive_build_kd_tree, accross the entire range of data, with current element as 0, and current sort order as -1 (equivalent to unsorted)
   recursive_build_kd_tree(tree_p, 0, no_elements - 1, 0, -1);
   return 1;
}

