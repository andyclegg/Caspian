/**
 * @file
 *
 * Implementation of various reduction algorithms.
 */
#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "median.h"
#include "reduction_functions.h"
#include "result_set.h"

/**
 * Reduce numeric data by taking the mean.
 *
 * @see reduction_function::call
 */
void reduce_numeric_mean(result_set *set, reduction_attrs *attrs, dimension_bounds bounds, void *input_data, void *output_data, int output_index, dtype input_dtype, dtype output_dtype) {
   register NUMERIC_WORKING_TYPE current_sum = 0.0;
   register NUMERIC_WORKING_TYPE query_data_value;
   register unsigned int current_number_of_values = 0;
   result_set_item *current_item;

   // Iterate over items in the result set, skipping fill values,
   // and adding up and counting the non-fill values
   while ((current_item = set->iterate(set)) != NULL) {
      query_data_value = numeric_get(input_data, input_dtype, current_item->record_index);

      if(query_data_value == attrs->input_fill_value) {
         continue;
      }
      current_sum += query_data_value;
      current_number_of_values++;
   }

   // Calculate the mean using the calculated sum and number of values
   // If no values were found, output the fill value
   NUMERIC_WORKING_TYPE output_value = (current_number_of_values == 0) ? (attrs->output_fill_value) : (current_sum / (NUMERIC_WORKING_TYPE) current_number_of_values);

   // Store the result
   numeric_put(output_data, output_dtype, output_index, output_value);
}

/**
 * Reduce coded data by using the nearest neighbour.
 *
 * @see reduction_function::call
 */
void reduce_coded_nearest_neighbour(result_set *set, reduction_attrs *attrs, dimension_bounds bounds, void *input_data, void *output_data, int output_index, dtype input_dtype, dtype output_dtype) {
   register float lowest_distance = FLT_MAX;
   void *best_value = malloc(input_dtype.size);
   if (best_value == NULL) {
      fprintf(stderr, "Couldn't allocate space to store a single %s value\n", input_dtype.string);
      exit(EXIT_FAILURE);
   }
   register short int value_stored = 0;

   // Calculate the midpoint of the cell
   float32_t central_x = (bounds[X + LOWER] + bounds[X + UPPER]) / 2.0;
   float32_t central_y = (bounds[Y + LOWER] + bounds[Y + UPPER]) / 2.0;

   result_set_item *current_item;

   register float current_distance;

   // Iterate over all items in the result set, calculating the distance
   // for each item and storing the value of the nearest item
   while ((current_item = set->iterate(set)) != NULL) {
      current_distance = powf(central_x - current_item->x, 2) + powf(central_y - current_item->y, 2);
      if (current_distance < lowest_distance) {
         // We have a new nearest neighbour, replace in the value
         lowest_distance = current_distance;
         coded_get(input_data, input_dtype, current_item->record_index, best_value);
         value_stored = value_stored || 1;
      }
   }

   // Store a hardcoded fill value of 0
   if (!value_stored) {
      memset(best_value, 0, input_dtype.size);
   }

   // Store the value
   coded_put(output_data, output_dtype, output_index, best_value);

   // Cleanup
   free(best_value);
}

/**
 * Reduce numeric data by using the nearest neighbour.
 *
 * @see reduction_function::call
 */
void reduce_numeric_nearest_neighbour(result_set *set, reduction_attrs *attrs, dimension_bounds bounds, void *input_data, void *output_data, int output_index, dtype input_dtype, dtype output_dtype) {
   register float lowest_distance = FLT_MAX;
   NUMERIC_WORKING_TYPE best_value = attrs->output_fill_value;

   // Calculate the midpoint of the cell
   float32_t central_x = (bounds[X + LOWER] + bounds[X + UPPER]) / 2.0;
   float32_t central_y = (bounds[Y + LOWER] + bounds[Y + UPPER]) / 2.0;

   result_set_item *current_item;
   register float current_distance;
   register float current_value;

   // Iterate over all items in the result set, calculating the distance
   // for each item and storing the value of the nearest item
   while ((current_item = set->iterate(set)) != NULL) {
      current_value = numeric_get(input_data, input_dtype, current_item->record_index);
      if (current_value == attrs->input_fill_value) {
         continue;
      }
      current_distance = powf(central_x - current_item->x, 2) + powf(central_y - current_item->y, 2);
      if (current_distance < lowest_distance) {
         // We have a new nearest neighbour, replace in the value
         lowest_distance = current_distance;
         best_value = current_value;
      }
   }

   // Store the value
   numeric_put(output_data, output_dtype, output_index, best_value);
}

/**
 * Reduce numeric data by using the value with the last time stamp.
 *
 * @see reduction_function::call
 */
void reduce_numeric_newest(result_set *set, reduction_attrs *attrs, dimension_bounds bounds, void *input_data, void *output_data, int output_index, dtype input_dtype, dtype output_dtype) {
   register float latest = -FLT_MAX;
   register NUMERIC_WORKING_TYPE query_data_value;
   register NUMERIC_WORKING_TYPE newest_data_value = attrs->output_fill_value;

   result_set_item *current_item;

   // Iterate over items in the result set, skipping fill values,
   // and storing the item with the greatest time value
   while ((current_item = set->iterate(set)) != NULL) {
      query_data_value = numeric_get(input_data, input_dtype, current_item->record_index);
      if(query_data_value == attrs->input_fill_value) {
         continue;
      }
      if (current_item->t > latest) {
         latest = current_item->t;
         newest_data_value = query_data_value;
      }
   }

   // Store the value
   numeric_put(output_data, output_dtype, output_index, newest_data_value);
}

/**
 * Reduce numeric data by taking the median.
 *
 * @see reduction_function::call
 */
void reduce_numeric_median(result_set *set, reduction_attrs *attrs, dimension_bounds bounds, void *input_data, void *output_data, int output_index, dtype input_dtype, dtype output_dtype) {
   unsigned int maximum_number_results = set->length; // maximum because some will be fill values
   unsigned int current_number_results = 0;
   register NUMERIC_WORKING_TYPE query_data_value;

   // Create an array to store the numeric values of the results
   NUMERIC_WORKING_TYPE *values = calloc(sizeof(NUMERIC_WORKING_TYPE), maximum_number_results);
   if (values == NULL) {
      fprintf(stderr, "Couldn't allocate memory to store %d results in reduce_numeric_median\n", maximum_number_results);
      exit(EXIT_FAILURE);
   }

   result_set_item *current_item;

   // Iterate over result set, skipping fill values, and storing the numeric
   // value in the array just defined
   while ((current_item = set->iterate(set)) != NULL) {
      query_data_value = numeric_get(input_data, input_dtype, current_item->record_index);
      if(query_data_value == attrs->input_fill_value) {
         continue;
      }
      values[current_number_results++] = query_data_value;
   }

   // Compute the median
   NUMERIC_WORKING_TYPE output_value = (current_number_results == 0) ? attrs->output_fill_value : median(values, current_number_results);

   // Store the median
   numeric_put(output_data, output_dtype, output_index, output_value);

   // Free our working array
   free(values);
}

/**
 * Reduce numeric data by taking distance-weighted mean.
 *
 * @see reduction_function::call
 */
void reduce_numeric_weighted_mean(result_set *set, reduction_attrs *attrs, dimension_bounds bounds, void *input_data, void *output_data, int output_index, dtype input_dtype, dtype output_dtype) {
   register NUMERIC_WORKING_TYPE current_sum = 0.0, total_distance = 0.0;
   register NUMERIC_WORKING_TYPE query_data_value, current_distance; //Initialized on each loop

   // Compute the midpoint of the query cell
   NUMERIC_WORKING_TYPE central_x = (bounds[X + LOWER] + bounds[X + UPPER]) / 2.0;
   NUMERIC_WORKING_TYPE central_y = (bounds[Y + LOWER] + bounds[Y + UPPER]) / 2.0;

   result_set_item *current_item;

   // Iterate over each result, skipping fill values, calculating the
   // distance for each result, and counting both the total distance
   // between all results and the centre, and the distance-weighted
   // value
   while ((current_item = set->iterate(set)) != NULL) {
      query_data_value = numeric_get(input_data, input_dtype, current_item->record_index);

      if(query_data_value == attrs->input_fill_value) {
         continue;
      }
      current_distance = sqrt(powf(central_x - current_item->x, 2) + powf(central_y - current_item->y, 2));
      current_sum += query_data_value * current_distance;
      total_distance += current_distance;
   }

   // Normalise the weighted mean by dividing the weighted sum by the
   // total sum, and store. Store fill value if no results were found.
   numeric_put(output_data, output_dtype, output_index, (total_distance == 0.0) ? attrs->output_fill_value : current_sum / total_distance);
}

/**
 * Retrieve an instance of the named reduction_function.
 *
 * @param name The name of the reduction function.
 * @return A reduction_function instance.
 */
reduction_function get_reduction_function_by_name(char *name) {
   static reduction_function reduction_functions[] = {
      {"undef", undef_style, NULL},
      {"mean", numeric, &reduce_numeric_mean},
      {"weighted_mean", numeric, &reduce_numeric_weighted_mean},
      {"median", numeric, &reduce_numeric_median},
      {"coded_nearest_neighbour", coded, &reduce_coded_nearest_neighbour},
      {"numeric_nearest_neighbour", numeric, &reduce_numeric_nearest_neighbour},
      {"newest", numeric, &reduce_numeric_newest},
   };
   static int number_reduction_functions = 7;

   reduction_function result = reduction_functions[0]; //undef

   for (int i=0; i<number_reduction_functions; i++) {
      if (strcmp(reduction_functions[i].name, name) == 0) {
         result = reduction_functions[i];
      }
   }
   return result;
}

int reduction_function_is_undef(reduction_function f) {
   return (f.data_style == undef_style);
}
