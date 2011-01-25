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

// Determine if 64bit
#ifdef INT64_MAX
#define SIXTYFOURBIT
#endif

// Define float32_t and float64_t for consistency
typedef float float32_t;
typedef double float64_t;

// Enumerate the dtypes
typedef enum dtype_e {uint8, uint16, uint32, uint64, int8, int16, int32, int64, float32, float64, undef} dtype;

void reduce(result_set_t *set, char *data, struct reduction_attrs *attrs, float *dimension_bounds, char *data_output, int output_index, dtype user_dtype, size_t user_dtype_size) {
#ifdef REDUCE_MEAN
   #define ALLOW_FLOAT
   register float64_t current_sum = 0.0;
   register float64_t query_data_value;
   register unsigned int current_number_of_values = 0;

   struct result_set_item *current_item;
   result_set_prepare_iteration(set);

   while ((current_item = result_set_iterate(set)) != NULL) {
      if (user_dtype == float32) {
         query_data_value = ((float32_t *) data)[current_item->record_index];
      } else {
         query_data_value = ((float64_t *) data)[current_item->record_index];
      }

      if(query_data_value == attrs->fill_value) {
         continue;
      }
      current_sum += query_data_value;
      current_number_of_values++;
   }

   float64_t output_value = (current_number_of_values == 0) ? (attrs->fill_value) : (current_sum / (float) current_number_of_values);

   if (user_dtype == float32) {
      ((float32_t *)data_output)[output_index] = (float32_t) output_value;
   } else {
      ((float64_t *)data_output)[output_index] = (float64_t) output_value;
   }
#endif

#ifdef REDUCE_NEAREST_NEIGHBOUR
   #define ALLOW_ALL
   register float lowest_distance = FLT_MAX;
   void *best_value = malloc(user_dtype_size);
   register float64_t query_data_value;
   register short int value_stored = 0;

   float central_x = (dimension_bounds[0] + dimension_bounds[1]) / 2.0;
   float central_y = (dimension_bounds[2] + dimension_bounds[3]) / 2.0;

   struct result_set_item *current_item;
   result_set_prepare_iteration(set);

   register float current_distance;
   while ((current_item = result_set_iterate(set)) != NULL) {
      if (user_dtype == float32) {
         query_data_value = ((float32_t *) data)[current_item->record_index];
      } else {
         query_data_value = ((float64_t *) data)[current_item->record_index];
      }
      if(query_data_value == attrs->fill_value) {
         continue;
      }
      current_distance = powf(central_x - current_item->x, 2) + powf(central_y - current_item->y, 2);
      if (current_distance < lowest_distance) {
         // We have a new nearest neighbour, replace in the value
         //printf("%f is better than %f, storing %f\n", current_distance, lowest_distance, ((float *) data)[current_item->record_index]);
         lowest_distance = current_distance;
         memcpy(best_value, &data[current_item->record_index * user_dtype_size], user_dtype_size);
         value_stored = value_stored || 1;
      }
   }

   if (!value_stored) {
      memcpy(best_value, &attrs->fill_value, 4); // TODO: Fix (fill value is hardcoded float)
   }

   // Store the value
   //printf("Storing %f as value\n", *((float *) best_value));
   memcpy(&data_output[output_index * user_dtype_size], best_value, user_dtype_size);
   free(best_value);
#endif

#ifdef REDUCE_MEDIAN
   #define ALLOW_FLOAT32

   unsigned int maximum_number_results = result_set_len(set); // maximum because some will be fill values
   unsigned int current_number_results = 0;
   float32_t *values = calloc(sizeof(float32_t), maximum_number_results);
   register float32_t query_data_value;
   if (values == NULL) { exit(1); }

   struct result_set_item *current_item;
   result_set_prepare_iteration(set);

   register float current_distance;
   while ((current_item = result_set_iterate(set)) != NULL) {
      query_data_value = ((float32_t *) data)[current_item->record_index];
      if(query_data_value == attrs->fill_value) {
         continue;
      }
      values[current_number_results] = query_data_value;
      current_number_results++;
   }

   // Now get the median
   ((float32_t *) data_output)[output_index] = (current_number_results == 0) ? attrs->fill_value : median(values, current_number_results);

   // Free our working array of floats
   free(values);
#endif

#ifdef REDUCE_WEIGHTED_MEAN
   #define ALLOW_FLOAT
   register float64_t current_sum = 0.0;
   register float64_t total_distance = 0.0;
   register float64_t query_data_value;
   register float current_distance; //Initialized on each loop

   float central_x = (dimension_bounds[0] + dimension_bounds[1]) / 2.0;
   float central_y = (dimension_bounds[2] + dimension_bounds[3]) / 2.0;

   struct result_set_item *current_item;
   result_set_prepare_iteration(set);

   while ((current_item = result_set_iterate(set)) != NULL) {
      if (user_dtype == float32) {
         query_data_value = ((float32_t *) data)[current_item->record_index];
      } else {
         query_data_value = ((float64_t *) data)[current_item->record_index];
      }

      if(query_data_value == attrs->fill_value) {
         continue;
      }
      current_distance = powf(central_x - current_item->x, 2) + powf(central_y - current_item->y, 2);
      current_sum += query_data_value * current_distance;
      total_distance += current_distance;
   }

   float64_t output_value = (total_distance == 0.0) ? attrs->fill_value : current_sum / total_distance;

   if (user_dtype == float32) {
      ((float32_t *)data_output)[output_index] = (float32_t) output_value;
   } else {
      ((float64_t *)data_output)[output_index] = (float64_t) output_value;
   }

#endif
}

// Crazy preprecessor hacks: figure out a plain list of what types are allowed
// For signed ints
#if defined(ALLOW_ALL) || defined(ALLOW_INT) || defined(ALLOW_SIGNEDINT) || defined(ALLOW_INT8)
#define ACCEPTED_INT8
#endif
#if defined(ALLOW_ALL) || defined(ALLOW_INT) || defined(ALLOW_SIGNEDINT) || defined(ALLOW_INT16)
#define ACCEPTED_INT16
#endif
#if defined(ALLOW_ALL) || defined(ALLOW_INT) || defined(ALLOW_SIGNEDINT) || defined(ALLOW_INT32)
#define ACCEPTED_INT32
#endif
#if defined(SIXTYFOURBIT) && (defined(ALLOW_ALL) || defined(ALLOW_INT) || defined(ALLOW_SIGNEDINT) || defined(ALLOW_INT64))
#define ACCEPTED_INT64
#endif

// For unsigned ints
#if defined(ALLOW_ALL) || defined(ALLOW_INT) || defined(ALLOW_UNSIGNEDINT) || defined(ALLOW_UINT8)
#define ACCEPTED_UINT8
#endif
#if defined(ALLOW_ALL) || defined(ALLOW_INT) || defined(ALLOW_UNSIGNEDINT) || defined(ALLOW_UINT16)
#define ACCEPTED_UINT16
#endif
#if defined(ALLOW_ALL) || defined(ALLOW_INT) || defined(ALLOW_UNSIGNEDINT) || defined(ALLOW_UINT32)
#define ACCEPTED_UINT32
#endif
#if defined(SIXTYFOURBIT) && (defined(ALLOW_ALL) || defined(ALLOW_INT) || defined(ALLOW_UNSIGNEDINT) || defined(ALLOW_UINT64))
#define ACCEPTED_UINT64
#endif

// And for floats
#if defined(ALLOW_ALL) || defined(ALLOW_FLOAT) || defined(ALLOW_FLOAT32)
#define ACCEPTED_FLOAT32
#endif
#if defined(ALLOW_ALL) || defined(ALLOW_FLOAT) || defined(ALLOW_FLOAT64)
#define ACCEPTED_FLOAT64
#endif



dtype dtype_string_parse(char *dtype_string) {
   #define allow(x) if (strcmp(#x, dtype_string) == 0) return x

   #ifdef ACCEPTED_UINT8
   allow(uint8);
   #endif

   #ifdef ACCEPTED_UINT16
   allow(uint16);
   #endif

   #ifdef ACCEPTED_UINT32
   allow(uint32);
   #endif

   #ifdef ACCEPTED_UINT64
   allow(uint64);
   #endif

   #ifdef ACCEPTED_INT8
   allow(int8);
   #endif

   #ifdef ACCEPTED_INT16
   allow(int16);
   #endif

   #ifdef ACCEPTED_INT32
   allow(int32);
   #endif

   #ifdef ACCEPTED_INT64
   allow(int64);
   #endif

   #ifdef ACCEPTED_FLOAT32
   allow(float32);
   #endif

   #ifdef ACCEPTED_FLOAT64
   allow(float64);
   #endif

   return undef;
}

size_t dtype_sizeof(dtype type) {
   switch (type) {
      case uint8:
      case int8:
         return 1;
      case uint16:
      case int16:
         return 2;
      case uint32:
      case int32:
      case float32:
         return 4;
      case uint64:
      case int64:
      case float64:
         return 8;
      default:
         return 0;
   }
}


void help(char *prog) {
   printf("Usage: %s <options>\n", basename(prog));
   printf("Options:\n");
   printf("\t--dtype\t\t\tSpecify dtype for input and output files\n");
   printf("\n");
   printf(" Input data\n");
   printf("\t--input-data\t\tSpecify filename for input data\n");
   printf("\t--input-lats\t\tSpecify filename for input latitude\n");
   printf("\t--input-lons\t\tSpecify filename for input longitude\n");
   printf("\n");
   printf(" Output data\n");
   printf("\t--output-data\t\tSpecify filename for output data\n");
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
   printf("Valid dtypes are:\n");
   #ifdef ACCEPTED_UINT8
   printf("uint8\n");
   #endif
   #ifdef ACCEPTED_UINT16
   printf("uint16\n");
   #endif
   #ifdef ACCEPTED_UINT32
   printf("uint32\n");
   #endif
   #ifdef ACCEPTED_UINT64
   printf("uint64\n");
   #endif
   #ifdef ACCEPTED_INT8
   printf("int8\n");
   #endif
   #ifdef ACCEPTED_INT16
   printf("int16\n");
   #endif
   #ifdef ACCEPTED_INT32
   printf("int32\n");
   #endif
   #ifdef ACCEPTED_INT64
   printf("int64\n");
   #endif
   #ifdef ACCEPTED_FLOAT32
   printf("float32\n");
   #endif
   #ifdef ACCEPTED_FLOAT64
   printf("float64\n");
   #endif
}

int main(int argc, char **argv) {

   char *input_data_filename = NULL, *input_lat_filename = NULL, *input_lon_filename = NULL;
   char *output_data_filename = NULL, *output_lat_filename = NULL, *output_lon_filename = NULL;
   char *projection_string = NULL;
   int width = 0, height = 0;
   double horizontal_resolution = 0.0, vertical_resolution = 0.0;
   double central_x = 0.0, central_y = 0.0;
   dtype user_dtype;
   size_t user_dtype_size;

   // Paranoid dtype checks
   if (sizeof(float32_t) != 4 || sizeof(float64_t) != 8) {
      printf("Unsupported system: 'float' and 'double' not 4 and 8 bytes respectively\n");
   }

   int curarg, option_index = 0;
   // Parse command line arguments
   static struct option long_options[] = {
      {"input-data", 1, 0, 'd'},
      {"input-lats", 1, 0, 'a'},
      {"input-lons", 1, 0, 'o'},
      {"output-data", 1, 0, 'D'},
      {"output-lats", 1, 0, 'A'},
      {"output-lons", 1, 0, 'O'},
      {"projection", 1, 0, 'p'},
      {"width", 1, 0, 'w'},
      {"height", 1, 0, 'h'},
      {"hres", 1, 0, 'H'},
      {"vres", 1, 0, 'V'},
      {"dtype", 1, 0, 't'},
      {"central-x", 1, 0, 'x'},
      {"central-y", 1, 0, 'y'},
   };

   while((curarg = getopt_long(argc, argv, "", long_options, &option_index)) != -1) {
      switch (curarg) {
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
            user_dtype = dtype_string_parse(optarg);
            if (user_dtype == undef) {
               printf("Invalid input dtype\n");
               help(argv[0]);
               return -1;
            }
            user_dtype_size = dtype_sizeof(user_dtype);
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
      vertical_resolution == 0.0
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

   struct tree *root_p;
   int result;

   // Figure out bytes per record (input & output)

   // Reduction options
   struct reduction_attrs r_attrs;
   r_attrs.fill_value = -999.0;

   // Initialize the projection
   projPJ *projection = pj_init_plus(projection_string);
   if (projection == NULL) {
      printf("Critical: Couldn't initialize projection\n"); //TODO: Stderr
      return -1;
   }

   latlon_reader_t *reader = latlon_reader_init(input_lat_filename, input_lon_filename, projection);

   if (reader == NULL) {
      printf("Failed to initialise data reader\n");
      return -1;
   }

   unsigned int data_number_bytes = latlon_reader_get_num_records(reader) * user_dtype_size;

   printf("Mapping %d bytes of input data into memory\n", data_number_bytes);
   int data_fd = open(input_data_filename, O_RDONLY);
   if (data_fd == -1) {
      printf("Failed to open input data file %s\n", input_data_filename);
      return -1;
   }
   char *data = mmap(0, data_number_bytes, PROT_READ, MAP_SHARED, data_fd, 0);
   if (data == MAP_FAILED) {
      printf("Failed to map data into memory (%s)\n", strerror(errno));
      return -1;
   }
   //madvise(data, data_number_bytes, MADV_WILLNEED | MADV_SEQUENTIAL);

   printf("Creating output files\n");
   mode_t creation_mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
   int output_open_flags = O_CREAT | O_TRUNC | O_RDWR;

   int data_output_fd = open(output_data_filename, output_open_flags, creation_mode);
   if(posix_fallocate(data_output_fd, 0, user_dtype_size * width * height) != 0) {
      printf("Failed to allocate space in file system\n");
      return -1;
   }
   char *data_output = mmap(0, width * height * user_dtype_size, PROT_WRITE, MAP_SHARED, data_output_fd, 0);
   if (data_output == MAP_FAILED) {
      printf("Failed to map output data into memory (%s)\n", strerror(errno));
      return -1;
   }

   int lats_output_fd = open(output_lat_filename, output_open_flags, creation_mode);
   if(posix_fallocate(lats_output_fd, 0, sizeof(float) * width * height) != 0) {
      printf("Failed to allocate space in file system\n");
      return -1;
   }
   float *lats_output = mmap(0, width * height * sizeof(float), PROT_WRITE, MAP_SHARED, lats_output_fd, 0);
   if (lats_output == MAP_FAILED) {
      printf("Failed to map output lats file into memory (%s)\n", strerror(errno));
      return -1;
   }

   int lons_output_fd = open(output_lon_filename, output_open_flags, creation_mode);
   if(posix_fallocate(lons_output_fd, 0, sizeof(float) * width * height) != 0) {
      printf("Failed to allocate space in file system\n");
      return -1;
   }
   float *lons_output = mmap(0, width * height * sizeof(float), PROT_WRITE, MAP_SHARED, lons_output_fd, 0);
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

   float x_0 = central_x - (((float) width / 2.0) * horizontal_resolution);
   float y_0 = central_y - (((float) height / 2.0) * vertical_resolution);

   for (int v=0; v<height; v++) {
      for (int u=0; u<width; u++) {
         int index = (height-v-1)*width + u;

         float bl_x = x_0 + ((((float) u) - 2.0) * horizontal_resolution);
         float bl_y = y_0 + ((((float) v) - 2.0) * vertical_resolution);

         float tr_x = bl_x + (4.0 * horizontal_resolution);
         float tr_y = bl_y + (4.0 * vertical_resolution);

         float query_dimensions[4] = {bl_x, tr_x, bl_y, tr_y};

         result_set_t *current_result_set = query_tree(root_p, query_dimensions);
         reduce(current_result_set, data, &r_attrs, query_dimensions, data_output, index, user_dtype, user_dtype_size);
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
   munmap(data, data_number_bytes);
   munmap(data_output, data_number_bytes);
   munmap(lats_output, data_number_bytes);
   munmap(lons_output, data_number_bytes);

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
