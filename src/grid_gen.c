#include <errno.h>
#include <fcntl.h>
#include <float.h>
#include <getopt.h>
#include <libgen.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "grid_gen.h"
#include "kd_tree.h"
#include "median.h"

NUMERIC_WORKING_TYPE numeric_get(void *data, dtype input_dtype, int index) {
   switch (input_dtype.specifier) {
      case uint8:
         return (NUMERIC_WORKING_TYPE) ((uint8_t *) data)[index];
      case uint16:
         return (NUMERIC_WORKING_TYPE) ((uint16_t *) data)[index];
      case uint32:
         return (NUMERIC_WORKING_TYPE) ((uint32_t *) data)[index];
      #ifdef SIXTYFOURBIT
      case uint64:
         return (NUMERIC_WORKING_TYPE) ((uint64_t *) data)[index];
      #endif
      case int8:
         return (NUMERIC_WORKING_TYPE) ((int8_t *) data)[index];
      case int16:
         return (NUMERIC_WORKING_TYPE) ((int16_t *) data)[index];
      case int32:
         return (NUMERIC_WORKING_TYPE) ((int32_t *) data)[index];
      #ifdef SIXTYFOURBIT
      case int64:
         return (NUMERIC_WORKING_TYPE) ((int64_t *) data)[index];
      #endif
      case float32:
         return (NUMERIC_WORKING_TYPE) ((float32_t *) data)[index];
      case float64:
         return (NUMERIC_WORKING_TYPE) ((float64_t *) data)[index];
      default:
         fprintf(stderr, "numeric_get received an invalid dtype (%d), quitting.\n", input_dtype.specifier);
         exit(-1);
   }
}

void coded_get(void *data, dtype input_dtype, int index, void *output) {
   memcpy(output, &((char *) data)[index*input_dtype.size], input_dtype.size);
}

void coded_put(void *data, dtype output_dtype, int index, void *input) {
   memcpy(&((char *) data)[index*output_dtype.size], input, output_dtype.size);
}


void numeric_put(void *data, dtype output_dtype, int index, NUMERIC_WORKING_TYPE data_item) {
   #define put(type) ((type *) data)[index] = (type) data_item
   switch (output_dtype.specifier) {
      case uint8:
         put(uint8_t);
         break;
      case uint16:
         put(uint16_t);
         break;
      case uint32:
         put(uint32_t);
         break;
      #ifdef SIXTYFOURBIT
      case uint64:
         put(uint64_t);
         break;
      #endif
      case int8:
         put(int8_t);
         break;
      case int16:
         put(int16_t);
         break;
      case int32:
         put(int32_t);
         break;
      #ifdef SIXTYFOURBIT
      case int64:
         put(int64_t);
         break;
      #endif
      case float32:
         put(float32_t);
         break;
      case float64:
         put(float64_t);
         break;
      default:
         fprintf(stderr, "numeric_put received an invalid dtype (%s), quitting.\n", output_dtype.string);
         exit(-1);
   }
}

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

dtype dtype_string_parse(char *dtype_string) {
   dtype output;
   int parsed = 0;
   #define parse(ttyyppee, ssiizzee, ssttyyllee) if(strcmp(#ttyyppee, dtype_string) == 0) { output.specifier = ttyyppee; output.size = ssiizzee; output.type = ssttyyllee; output.string = #ttyyppee; parsed = 1;}
   parse(uint8, 1, numeric);
   parse(uint16, 2, numeric);
   parse(uint32, 4, numeric);
   parse(uint64, 8, numeric);
   parse(int8, 1, numeric);
   parse(int16, 2, numeric);
   parse(int32, 4, numeric);
   parse(int64, 8, numeric);
   parse(float32, 4, numeric);
   parse(float64, 8, numeric);
   parse(coded8, 1, coded);
   parse(coded16, 2, coded);
   parse(coded32, 4, coded);
   parse(coded64, 8, coded);

   if (parsed) {
      return output;
   } else {
      fprintf(stderr, "Could not decode dtype '%s'.\n", dtype_string);
      exit(-1);
   }
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
      current_distance = powf(central_x - current_item->x, 2) + powf(central_y - current_item->y, 2);
      current_sum += query_data_value * current_distance;
      total_distance += current_distance;
   }

   numeric_put(output_data, output_dtype, output_index, (total_distance == 0.0) ? attrs->output_fill_value : current_sum / total_distance);
}

void help(char *prog) {
   printf("Usage: %s <options>\n", basename(prog));
   printf("Options:\n");
   printf(" Input data\n");
   printf("\t--input-data\t\tSpecify filename for input data\n");
   printf("\t--input-dtype\t\tSpecify dtype for input data file\n");
   printf("\t--input-fill-value\tSpecify fill value for input data file\n");
   printf("\t--input-lats\t\tSpecify filename for input latitude\n");
   printf("\t--input-lons\t\tSpecify filename for input longitude\n");
   printf("\n");
   printf(" Output data\n");
   printf("\t--output-data\t\tSpecify filename for output data\n");
   printf("\t--output-dtype\t\tSpecify dtype for output data file\n");
   printf("\t--output-fill-value\tSpecify fill value for output data file\n");
   printf("\t--output-lats\t\tSpecify filename for output latitude\n");
   printf("\t--output-lons\t\tSpecify filename for output longitude\n");
   printf("\n");
   printf(" Output grid parameters\n");
   printf("\t--projection\t\tSpecify projection using PROJ.4 compatible string\n");
   printf("\t--height\t\tHeight of output grid\n");
   printf("\t--width \t\tWidth of output grid\n");
   printf("\t--vres\t\t\tVertical resolution of output grid, in projection units (metres)\n");
   printf("\t--hres\t\t\tHorizontal resolution of output grid, in projection units (metres)\n");
   printf("\t--central-y\t\tVertical position of centre of output grid, in projection units (metres)\n");
   printf("\t--central-x\t\tHorizontal position of centre of output grid, in projection units (metres)\n");
   printf("\n");
   printf("Valid dtypes for numeric functions are:\n");
   printf("uint8\n");
   printf("uint16\n");
   printf("uint32\n");
   #ifdef SIXTYFOURBIT
   printf("uint64\n");
   #endif
   printf("int8\n");
   printf("int16\n");
   printf("int32\n");
   #ifdef SIXTYFOURBIT
   printf("int64\n");
   #endif
   printf("float32\n");
   printf("float64\n");
   printf("\n");
   printf("Valid dtypes for coded functions are:\n");
   printf("coded8\n");
   printf("coded16\n");
   printf("coded32\n");
   printf("coded64\n");
}

int main(int argc, char **argv) {

   char *input_data_filename = NULL, *input_lat_filename = NULL, *input_lon_filename = NULL;
   char *output_data_filename = NULL, *output_lat_filename = NULL, *output_lon_filename = NULL;
   char *projection_string = NULL;
   int width = 0, height = 0;
   double horizontal_resolution = 0.0, vertical_resolution = 0.0;
   double central_x = 0.0, central_y = 0.0;
   dtype input_dtype = {undef_type, 0, undef_style, "undef"}, output_dtype = {undef_type, 0, undef_style, "undef"};
   int selected_mapping_function_index = -1;
   NUMERIC_WORKING_TYPE input_fill_value = 0.0, output_fill_value = 0.0;

   // Paranoid dtype checks
   if (sizeof(float32_t) != 4 || sizeof(float64_t) != 8) {
      printf("Unsupported system: 'float' and 'double' not 4 and 8 bytes respectively\n");
   }

   mapping_function reduction_functions[] = {
      {"mean", numeric, &reduce_numeric_mean},
      {"weighted_mean", numeric, &reduce_numeric_weighted_mean},
      {"median", numeric, &reduce_numeric_median},
      {"nearest_neighbour", coded, &reduce_coded_nearest_neighbour}
   };

   int curarg, option_index = 0;
   // Parse command line arguments
   static struct option long_options[] = {
      {"input-data", 1, 0, 'd'},
      {"input-lats", 1, 0, 'a'},
      {"input-lons", 1, 0, 'o'},
      {"output-data", 1, 0, 'D'},
      {"output-lats", 1, 0, 'A'},
      {"output-lons", 1, 0, 'O'},
      {"mapping-function", 1, 0, 'm'},
      {"projection", 1, 0, 'p'},
      {"width", 1, 0, 'w'},
      {"height", 1, 0, 'h'},
      {"hres", 1, 0, 'H'},
      {"vres", 1, 0, 'V'},
      {"input-dtype", 1, 0, 't'},
      {"output-dtype", 1, 0, 'T'},
      {"input-fill-value", 1, 0, 'f'},
      {"output-fill-value", 1, 0, 'F'},
      {"central-x", 1, 0, 'x'},
      {"central-y", 1, 0, 'y'},
   };

   while((curarg = getopt_long(argc, argv, "", long_options, &option_index)) != -1) {
      switch (curarg) {
         case 'm': // Mapping function
            // Get the appropriate mapping function struct
            for (int i=0; i<4; i++) { // TODO FIX HARDCODEDNESS!
               if (strcmp(reduction_functions[i].name, optarg) == 0) {
                  selected_mapping_function_index = i;
                  break;
               }
            }
            if (selected_mapping_function_index == -1) {
               fprintf(stderr, "Unknown mapping function '%s'\n", optarg);
               exit(-1);
            }
            break;
         case 'p':
            projection_string = calloc(strlen(optarg), sizeof(char));
            strcpy(projection_string, optarg);
            break;
         case 'd':
            input_data_filename = calloc(strlen(optarg), sizeof(char));
            strcpy(input_data_filename, optarg);
            break;
         case 'a':
            input_lat_filename = calloc(strlen(optarg), sizeof(char));
            strcpy(input_lat_filename, optarg);
            break;
         case 'o':
            input_lon_filename = calloc(strlen(optarg), sizeof(char));
            strcpy(input_lon_filename, optarg);
            break;
         case 'D':
            output_data_filename = calloc(strlen(optarg), sizeof(char));
            strcpy(output_data_filename, optarg);
            break;
         case 'A':
            output_lat_filename = calloc(strlen(optarg), sizeof(char));
            strcpy(output_lat_filename, optarg);
            break;
         case 'O':
            output_lon_filename = calloc(strlen(optarg), sizeof(char));
            strcpy(output_lon_filename, optarg);
            break;
         case 'w':
            width = atoi(optarg);
            break;
         case 'h':
            height = atoi(optarg);
            break;
         case 'H':
            horizontal_resolution = atof(optarg);
            break;
         case 'V':
            vertical_resolution = atof(optarg);
            break;
         case 'x':
            central_x = atof(optarg);
            break;
         case 'y':
            central_y = atof(optarg);
            break;
         case 't':
            input_dtype = dtype_string_parse(optarg);
            break;
         case 'T':
            output_dtype = dtype_string_parse(optarg);
            break;
         case 'f':
            input_fill_value = atof(optarg);
            break;
         case 'F':
            output_fill_value = atof(optarg);
            break;
         default:
            printf("Unrecognised option\n");
            help(argv[0]);
            return -1;
      }
   }

   // Check we have required options
   if (
      input_data_filename == NULL ||
      input_lat_filename == NULL ||
      input_lon_filename == NULL ||
      output_data_filename == NULL ||
      output_lat_filename == NULL ||
      output_lon_filename == NULL ||
      projection_string == NULL ||
      width == 0 ||
      height == 0 ||
      horizontal_resolution == 0.0 ||
      vertical_resolution == 0.0 ||
      input_dtype.type == undef_type ||
      output_dtype.type == undef_type ||
      selected_mapping_function_index == -1
   ) {
      printf("Not all required options supplied\n");
      help(argv[0]);
      return -1;
   }

   #ifdef DEBUG
   printf("Start up parameters:\n");
   printf("Input Data filename: %s\n", input_data_filename);
   printf("Input Lats filename: %s\n", input_lat_filename);
   printf("Input Lons filename: %s\n", input_lon_filename);
   printf("Output Data filename: %s\n", output_data_filename);
   printf("Output Lats filename: %s\n", output_lat_filename);
   printf("Output Lons filename: %s\n", output_lon_filename);
   printf("Projection String: %s\n", projection_string);
   printf("Width/Height: %d/%d\n", width, height);
   printf("Horizontal/Vertical Resolution: %f/%f\n", horizontal_resolution, vertical_resolution);
   printf("Central X/Y: ");
   #endif

   // Validate coded/non-coded functions/data types
   mapping_function reduction_function = reduction_functions[selected_mapping_function_index];
   if (reduction_function.type == coded) {
      // Input and output dtype must be the same and coded
      if (input_dtype.type != coded || output_dtype.type != coded || !dtype_equal(input_dtype, output_dtype)) {
         fprintf(stderr, "When using a coded mapping function, input and output dtype must be the same, and of coded type\n");
      }
   } else if (reduction_function.type == numeric) {
      if (input_dtype.type != numeric || output_dtype.type != numeric) {
         fprintf(stderr, "When using a numeric mapping function, input and output dtype must be numeric\n");
      }
   }

   struct tree *root_p;
   int result;

   // Reduction options
   struct reduction_attrs r_attrs;
   r_attrs.input_fill_value = input_fill_value;
   r_attrs.output_fill_value = output_fill_value;

   // Initialize the projection
   projPJ *projection = pj_init_plus(projection_string);
   if (projection == NULL) {
      fprintf(stderr, "Critical: Couldn't initialize projection\n");
      return -1;
   }

   latlon_reader_t *reader = latlon_reader_init(input_lat_filename, input_lon_filename, projection);

   if (reader == NULL) {
      printf("Failed to initialise data reader\n");
      return -1;
   }

   unsigned int input_data_number_bytes = latlon_reader_get_num_records(reader) * input_dtype.size;
   unsigned int output_data_number_bytes = width * height * output_dtype.size;
   unsigned int output_geo_number_bytes = width * height * sizeof(float32_t);

   printf("Mapping %d bytes of input data into memory\n", input_data_number_bytes);
   int data_fd = open(input_data_filename, O_RDONLY);
   if (data_fd == -1) {
      printf("Failed to open input data file %s\n", input_data_filename);
      return -1;
   }
   char *data = mmap(0, input_data_number_bytes, PROT_READ, MAP_SHARED, data_fd, 0);
   if (data == MAP_FAILED) {
      printf("Failed to map data into memory (%s)\n", strerror(errno));
      return -1;
   }

   printf("Creating output files\n");
   mode_t creation_mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
   int output_open_flags = O_CREAT | O_TRUNC | O_RDWR;

   int data_output_fd = open(output_data_filename, output_open_flags, creation_mode);
   if(posix_fallocate(data_output_fd, 0, output_data_number_bytes) != 0) {
      printf("Failed to allocate space in file system\n");
      return -1;
   }
   char *data_output = mmap(0, output_data_number_bytes, PROT_WRITE, MAP_SHARED, data_output_fd, 0);
   if (data_output == MAP_FAILED) {
      printf("Failed to map output data into memory (%s)\n", strerror(errno));
      return -1;
   }

   int lats_output_fd = open(output_lat_filename, output_open_flags, creation_mode);
   if(posix_fallocate(lats_output_fd, 0, output_geo_number_bytes) != 0) {
      printf("Failed to allocate space in file system\n");
      return -1;
   }
   float *lats_output = mmap(0, output_geo_number_bytes, PROT_WRITE, MAP_SHARED, lats_output_fd, 0);
   if (lats_output == MAP_FAILED) {
      printf("Failed to map output lats file into memory (%s)\n", strerror(errno));
      return -1;
   }

   int lons_output_fd = open(output_lon_filename, output_open_flags, creation_mode);
   if(posix_fallocate(lons_output_fd, 0, output_geo_number_bytes) != 0) {
      printf("Failed to allocate space in file system\n");
      return -1;
   }
   float *lons_output = mmap(0, output_geo_number_bytes, PROT_WRITE, MAP_SHARED, lons_output_fd, 0);
   if (lons_output == MAP_FAILED) {
      printf("Failed to map output lons file into memory (%s)\n", strerror(errno));
      return -1;
   }

   time_t start_time, end_time;
   printf("Building indices\n");
   start_time = time(NULL);
   result = fill_tree_from_reader(&root_p, reader);
   if (!result) {
      printf("Failed to build tree\n");
      return result;
   }
   end_time = time(NULL);
   printf("Tree built\n");

   printf("Building tree took %d seconds\n", (int) (end_time - start_time));

   printf("Verifying tree\n");
   verify_tree(root_p);
   printf("Tree verified as correct\n");


   printf("Building output image\n");
   start_time = time(NULL);

   float32_t x_0 = central_x - (((float) width / 2.0) * horizontal_resolution);
   float32_t y_0 = central_y - (((float) height / 2.0) * vertical_resolution);

   for (int v=0; v<height; v++) {
      for (int u=0; u<width; u++) {
         int index = (height-v-1)*width + u;

         float32_t bl_x = x_0 + ((((float) u) - 2.0) * horizontal_resolution);
         float32_t bl_y = y_0 + ((((float) v) - 2.0) * vertical_resolution);

         float32_t tr_x = bl_x + (4.0 * horizontal_resolution);
         float32_t tr_y = bl_y + (4.0 * vertical_resolution);

         float32_t query_dimensions[4] = {bl_x, tr_x, bl_y, tr_y};

         result_set_t *current_result_set = query_tree(root_p, query_dimensions);
         reduction_function.call(current_result_set, &r_attrs, query_dimensions, data, data_output, index, input_dtype, output_dtype);
         result_set_free(current_result_set);

         // Get the central latitude and longitude for this cell, and store
         projUV projection_input, projection_output;
         projection_input.u = (tr_y + bl_y) / 2.0;
         projection_input.v = (tr_x + bl_x) / 2.0;

         projection_output = pj_inv(projection_input, projection);
         lats_output[index] = projection_output.v * RAD_TO_DEG;
         lons_output[index] = projection_output.u * RAD_TO_DEG;
         #ifdef DEBUG
         printf("(%d, %d) => (%f:%f, %f:%f)\n", u, v, bl_x, tr_x, bl_y, tr_y);
         #endif
      }
   }
   end_time = time(NULL);
   printf("Output image built.\n");
   printf("Building image took %d seconds\n", (int) (end_time - start_time));

   // Unmap all files
   munmap(data, input_data_number_bytes);
   munmap(data_output, output_data_number_bytes);
   munmap(lats_output, output_geo_number_bytes);
   munmap(lons_output, output_geo_number_bytes);

   // Close all files
   close(data_fd);
   close(data_output_fd);
   close(lats_output_fd);
   close(lons_output_fd);

   // Free option strings
   free(input_data_filename);
   free(input_lat_filename);
   free(input_lon_filename);
   free(output_data_filename);
   free(output_lat_filename);
   free(output_lon_filename);
   free(projection_string);

   // Free working data
   free_tree(root_p);
   pj_free(projection);

   return 0;
}
