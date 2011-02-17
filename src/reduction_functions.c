#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "median.h"
#include "reduction_functions.h"

void reduce_numeric_mean(result_set_t *set, struct reduction_attrs *attrs, float *dimension_bounds, void *input_data, void *output_data, int output_index, dtype input_dtype, dtype output_dtype) {
   register NUMERIC_WORKING_TYPE current_sum = 0.0;
   register NUMERIC_WORKING_TYPE query_data_value;
   register unsigned int current_number_of_values = 0;

   struct result_set_item *current_item;
   result_set_prepare_iteration(set);

   while ((current_item = result_set_iterate(set)) != NULL) {
      query_data_value = numeric_get(input_data, input_dtype, current_item->record_index);

      if(query_data_value == attrs->input_fill_value) {
         continue;
      }
      current_sum += query_data_value;
      current_number_of_values++;
   }

   NUMERIC_WORKING_TYPE output_value = (current_number_of_values == 0) ? (attrs->output_fill_value) : (current_sum / (NUMERIC_WORKING_TYPE) current_number_of_values);

   numeric_put(output_data, output_dtype, output_index, output_value);
}

void reduce_coded_nearest_neighbour(result_set_t *set, struct reduction_attrs *attrs, float *dimension_bounds, void *input_data, void *output_data, int output_index, dtype input_dtype, dtype output_dtype) {
   register float lowest_distance = FLT_MAX;
   void *best_value = malloc(input_dtype.size);
   register short int value_stored = 0;

   float32_t central_x = (dimension_bounds[0] + dimension_bounds[1]) / 2.0;
   float32_t central_y = (dimension_bounds[2] + dimension_bounds[3]) / 2.0;

   struct result_set_item *current_item;
   result_set_prepare_iteration(set);

   register float current_distance;
   while ((current_item = result_set_iterate(set)) != NULL) {
      current_distance = powf(central_x - current_item->x, 2) + powf(central_y - current_item->y, 2);
      if (current_distance < lowest_distance) {
         // We have a new nearest neighbour, replace in the value
         lowest_distance = current_distance;
         coded_get(input_data, input_dtype, current_item->record_index, best_value);
         value_stored = value_stored || 1;
      }
   }

   if (!value_stored) {
      memset(best_value, 0, input_dtype.size);
   }

   // Store the value
   coded_put(output_data, output_dtype, output_index, best_value);

   // Cleanup
   free(best_value);
}

void reduce_numeric_median(result_set_t *set, struct reduction_attrs *attrs, float *dimension_bounds, void *input_data, void *output_data, int output_index, dtype input_dtype, dtype output_dtype) {
   unsigned int maximum_number_results = result_set_len(set); // maximum because some will be fill values
   unsigned int current_number_results = 0;
   register NUMERIC_WORKING_TYPE query_data_value;

   NUMERIC_WORKING_TYPE *values = calloc(sizeof(NUMERIC_WORKING_TYPE), maximum_number_results);
   if (values == NULL) { exit(1); }

   struct result_set_item *current_item;
   result_set_prepare_iteration(set);

   while ((current_item = result_set_iterate(set)) != NULL) {
      query_data_value = numeric_get(input_data, input_dtype, current_item->record_index);
      if(query_data_value == attrs->input_fill_value) {
         continue;
      }
      values[current_number_results++] = query_data_value;
   }

   // Now get the median
   NUMERIC_WORKING_TYPE output_value = (current_number_results == 0) ? attrs->output_fill_value : median(values, current_number_results);
   numeric_put(output_data, output_dtype, output_index, output_value);

   // Free our working array of floats
   free(values);
}

void reduce_numeric_weighted_mean(result_set_t *set, struct reduction_attrs *attrs, float *dimension_bounds, void *input_data, void *output_data, int output_index, dtype input_dtype, dtype output_dtype) {
   register NUMERIC_WORKING_TYPE current_sum = 0.0, total_distance = 0.0;
   register NUMERIC_WORKING_TYPE query_data_value, current_distance; //Initialized on each loop

   NUMERIC_WORKING_TYPE central_x = (dimension_bounds[0] + dimension_bounds[1]) / 2.0;
   NUMERIC_WORKING_TYPE central_y = (dimension_bounds[2] + dimension_bounds[3]) / 2.0;

   struct result_set_item *current_item;
   result_set_prepare_iteration(set);

   while ((current_item = result_set_iterate(set)) != NULL) {
      query_data_value = numeric_get(input_data, input_dtype, current_item->record_index);

      if(query_data_value == attrs->input_fill_value) {
         continue;
      }
      current_distance = sqrt(powf(central_x - current_item->x, 2) + powf(central_y - current_item->y, 2));
      current_sum += query_data_value * current_distance;
      total_distance += current_distance;
   }

   numeric_put(output_data, output_dtype, output_index, (total_distance == 0.0) ? attrs->output_fill_value : current_sum / total_distance);
}

reduction_function get_reduction_function_by_name(char *name) {
   static reduction_function reduction_functions[] = {
      {"undef", undef_type, NULL},
      {"mean", numeric, &reduce_numeric_mean},
      {"weighted_mean", numeric, &reduce_numeric_weighted_mean},
      {"median", numeric, &reduce_numeric_median},
      {"nearest_neighbour", coded, &reduce_coded_nearest_neighbour}
   };
   static int number_reduction_functions = 5;

   reduction_function result = reduction_functions[0]; //undef

   for (int i=0; i<number_reduction_functions; i++) {
      if (strcmp(reduction_functions[i].name, name) == 0) {
         result = reduction_functions[i];
      }
   }
   return result;
}

int reduction_function_is_undef(reduction_function f) {
   return (f.type == undef_type);
}
