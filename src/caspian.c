/**
 * @file
 * @author Andrew Clegg
 *
 * Implements command-line functionality for Caspian.
 */
#define _XOPEN_SOURCE 600
#include <errno.h>
#include <getopt.h>
#include <libgen.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "coordinate_reader.h"
#include "data_handling.h"
#include "gridding.h"
#include "grid.h"
#include "io_helper.h"
#include "kd_tree.h"
#include "proj_projector.h"
#include "projector.h"
#include "rawfile_coordinate_reader.h"
#include "reduction_functions.h"

/**
 * Polar circumference of the earth according to WGS84 - used for calculating default values
 */
#define WGS84_POLAR_CIRCUMFERENCE 40007863.0
/**
 * Equatorial circumference of the earth according to WGS84 - used for calculating default values
 */
#define WGS84_EQUATORIAL_CIRCUMFERENCE 40075017.0

/**
 * Display the help text for the main executable program
 * @param executable The name or full path of the executable
 */
void help(char *executable) {
   printf("Usage: %s <options>\n", basename(executable));
   printf("Options                         Default                      Help\n");
   printf(" Index controls\n");
   printf("  --input-lats <filename>                                    Specify filename for input latitude\n");
   printf("  --input-lons <filename>                                    Specify filename for input longitude\n");
   printf("  --input-time <filename>                                    Specify filename for input time\n");
   printf("  --projection <string>         +proj=eqc +datum=WGS84       Specify projection using PROJ.4 compatible string\n");
   printf("  --save-index <filename>                                    Save the index to a file\n");
   printf("  --load-index <filename>                                    Load a pre-generated index from a file\n");
   printf("\n");
   printf(" Input data\n");
   printf("  --input-data <filename>                                    Specify filename for input data\n");
   printf("  --input-dtype <dtype>         float32                      Specify dtype for input data file\n");
   printf("  --input-fill-value <number>   -999.0                       Specify fill value for input data file\n");
   printf("\n");
   printf(" Output data\n");
   printf("  --output-data <filename>                                   Specify filename for output data\n");
   printf("  --output-dtype <dtype>        float32                      Specify dtype for output data file\n");
   printf("  --output-fill-value <number>  -999.0                       Specify fill value for output data file\n");
   printf("  --output-lats <filename>                                   Specify filename for output latitude\n");
   printf("  --output-lons <filename>                                   Specify filename for output longitude\n");
   printf("\n");
   printf(" Image generation\n");
   printf("  --height <integer>            360                          Height of output grid\n");
   printf("  --width <integer>             720                          Width of output grid\n");
   printf("  --vres <number>               polar circumf. / (2*height)  Vertical resolution of output grid, in projection units (metres)\n");
   printf("  --hres <number>               equatorial circumf. / width  Horizontal resolution of output grid, in projection units (metres)\n");
   printf("  --central-y <number>          0.0                          Vertical position of centre of output grid, in projection units (metres)\n");
   printf("  --central-x <number>          0.0                          Horizontal position of centre of output grid, in projection units (metres)\n");
   printf("  --vsample <number>            value of --vres              Vertical sampling resolution\n");
   printf("  --hsample <number>            value of --hres              Horizontal sampling resolution\n");
   printf("  --reduction-function <string> mean                         Choose reduction function to use\n");
   printf("  --time-min                    -inf                         Earliest time to select from\n");
   printf("  --time-max                    +inf                         Latest time to select from\n");
   printf("\n");
   printf(" General\n");
   printf("  --verbose                                                  Increase verbosity\n");
   printf("  --help                                                     Show this help message\n");
   printf("\n");
   printf("Numeric functions: mean, weighted_mean, median\n");
   printf("Numeric function dtypes: ");
   printf("uint8, uint16, uint32, ");
   #ifdef SIXTYFOURBIT
   printf("uint64, ");
   #endif
   printf("int8, int16, int32, ");
   #ifdef SIXTYFOURBIT
   printf("int64, ");
   #endif
   printf("float32, float64\n");
   printf("\n");
   printf("Coded functions: nearest_neighbour\n");
   printf("Coded function dtypes: coded8, coded16, coded32, coded64\n");
}

/**
 * Implementation of the gridding flow: Parse arguments, open files, build/load/save index, run gridding
 * @param argc Number of arguments
 * @param argv Arguments
 */
int main(int argc, char **argv) {

   char *input_data_filename = NULL, *input_lat_filename = NULL, *input_lon_filename = NULL, *input_time_filename = NULL;
   char *output_data_filename = NULL, *output_lat_filename = NULL, *output_lon_filename = NULL;
   int write_lats = 0, write_lons = 0, write_data = 0;
   char *input_index_filename = NULL, *output_index_filename = NULL;
   int loading_index = 0, saving_index = 0;
   int generating_image = 0;
   char *projection_string = "+proj=eqc +datum=WGS84";
   int using_default_projection_string = 1;
   int width = 720, height = 360;
   double horizontal_resolution = 0.0, vertical_resolution = 0.0;
   double horizontal_sampling = 0.0, vertical_sampling = 0.0;
   double central_x = 0.0, central_y = 0.0;
   dtype input_dtype = {float32, 4, numeric, "float32"}, output_dtype = {float32, 4, numeric, "float32"};
   reduction_function selected_reduction_function = get_reduction_function_by_name("mean");
   NUMERIC_WORKING_TYPE input_fill_value = -999.0, output_fill_value = -999.0;
   int verbosity = 0;
   float time_min = -INFINITY, time_max = +INFINITY;
   time_t start_time, end_time;

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
      {"input-time", 1, 0, 'e'},
      {"output-data", 1, 0, 'D'},
      {"output-lats", 1, 0, 'A'},
      {"output-lons", 1, 0, 'O'},
      {"reduction-function", 1, 0, 'r'},
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
      {"hsample", 1, 0, 's'},
      {"vsample", 1, 0, 'S'},
      {"load-index", 1, 0, 'i'},
      {"save-index", 1, 0, 'I'},
      {"help", 0, 0, '?'},
      {"verbose", 0, 0, '+'},
      {"time-min", 1, 0, 'q'},
      {"time-max", 1, 0, 'Q'},
   };

   #define save_optarg_string(x) x = calloc(strlen(optarg) + 1, sizeof(char)); strcpy(x, optarg)

   while((curarg = getopt_long(argc, argv, "", long_options, &option_index)) != -1) {
      switch (curarg) {
         case 'q':
            time_min = atof(optarg);
            break;
         case 'Q':
            time_max = atof(optarg);
            break;
         case 'e':
            save_optarg_string(input_time_filename);
            break;
         case '+':
            verbosity++;
            break;
         case '?':
            help(argv[0]);
            return 0;
         case 'r': // Reduction function
            selected_reduction_function = get_reduction_function_by_name(optarg);
            if (reduction_function_is_undef(selected_reduction_function)) {
               fprintf(stderr, "Unknown reduction function '%s'\n", optarg);
               exit(-1);
            }
            break;
         case 'i':
            save_optarg_string(input_index_filename);
            loading_index = 1;
            break;
         case 'I':
            save_optarg_string(output_index_filename);
            saving_index = 1;
            break;
         case 'p':
            save_optarg_string(projection_string);
            using_default_projection_string = 0;
            break;
         case 'd':
            save_optarg_string(input_data_filename);
            break;
         case 'a':
            save_optarg_string(input_lat_filename);
            break;
         case 'o':
            save_optarg_string(input_lon_filename);
            break;
         case 'D':
            save_optarg_string(output_data_filename);
            generating_image = 1;
            write_data = 1;
            break;
         case 'A':
            save_optarg_string(output_lat_filename);
            generating_image = 1;
            write_lats = 1;
            break;
         case 'O':
            save_optarg_string(output_lon_filename);
            generating_image = 1;
            write_lons = 1;
            break;
         case 'w':
            width = atoi(optarg);
            if (width <= 0) {
               fprintf(stderr, "Width must be a positive integer (got %d)\n", width);
               exit(-1);
            }
            break;
         case 'h':
            height = atoi(optarg);
            if (height <= 0) {
               fprintf(stderr, "Height must be a positive integer (got %d)\n", height);
               exit(-1);
            }
            break;
         case 'H':
            horizontal_resolution = atof(optarg);
            if (horizontal_resolution <= 0.0) {
               fprintf(stderr, "Horizontal resolution must be a positive number (got %f)\n", horizontal_resolution);
               exit(-1);
            }
            break;
         case 'V':
            vertical_resolution = atof(optarg);
            if (vertical_resolution <= 0.0) {
               fprintf(stderr, "Vertical resolution must be a positive number (got %f)\n", vertical_resolution);
               exit(-1);
            }
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
         case 's':
            horizontal_sampling = atof(optarg);
            if (horizontal_sampling <= 0.0) {
               fprintf(stderr, "Horizontal sampling resolution must be a positive number (got %f)\n", horizontal_sampling);
               exit(-1);
            }
            break;
         case 'S':
            vertical_sampling = atof(optarg);
            if (vertical_sampling <= 0.0) {
               fprintf(stderr, "Vertical sampling resolution must be a positive number (got %f)\n", vertical_sampling);
               exit(-1);
            }
            break;
         default:
            printf("Unrecognised option\n");
            help(argv[0]);
            return -1;
      }
   }

   // Check we have required options - required options depends on mode of operation
   if (saving_index || (generating_image && !loading_index)) {
      if (input_lat_filename == NULL || input_lon_filename == NULL || projection_string == NULL) {
         char *message;
         if (saving_index) {
            message = "To generate an index,";
         } else {
            message = "To generate an image without loading an index,";
         }
         fprintf(stderr, "%s you must provide --input-lats, --input-lons, --input-time, and --projection\n", saving_index ? "To generate an index," : "To generate an image without loading an index");
         help(argv[0]);
         return -1;
      }
   }

   if (generating_image) {
      if (input_data_filename == NULL || output_data_filename == NULL) {
         fprintf(stderr, "When generating an image, you must provide --input-data and --output-data\n");
         help(argv[0]);
         return -1;
      }
   }

   // Validate coded/non-coded functions/data types
   if (selected_reduction_function.type == coded) {
      // Input and output dtype must be the same and coded
      if (input_dtype.type != coded || output_dtype.type != coded || !dtype_equal(input_dtype, output_dtype)) {
         fprintf(stderr, "When using a coded mapping function, input and output dtype must be the same, and of coded type\n");
      }
   } else if (selected_reduction_function.type == numeric) {
      if (input_dtype.type != numeric || output_dtype.type != numeric) {
         fprintf(stderr, "When using a numeric mapping function, input and output dtype must be numeric\n");
      }
   }

   // Set horizontal and vertical resolutions to default values if not set
   if (horizontal_resolution == 0.0) {
      horizontal_resolution = WGS84_EQUATORIAL_CIRCUMFERENCE / (float) width;
   }
   if (vertical_resolution == 0.0) {
      vertical_resolution = WGS84_POLAR_CIRCUMFERENCE / (2.0 * (float) height);
   }

   if (verbosity > 0) {
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
   }


   // Reduction options
   reduction_attrs r_attrs;
   r_attrs.input_fill_value = input_fill_value;
   r_attrs.output_fill_value = output_fill_value;

   index *data_index = NULL;

   if (loading_index) {
      // Read from disk
      FILE *input_index_file = fopen(input_index_filename, "r");
      data_index = read_kdtree_index_from_file(input_index_file);
      fclose(input_index_file);
   } else {
      // Initialize the projection
      projector *input_projection = get_proj_projector_from_string(projection_string);
      if (input_projection == NULL) {
         fprintf(stderr, "Critical: Couldn't initialize projector\n");
         return -1;
      }

      coordinate_reader *reader = get_coordinate_reader_from_files(input_lat_filename, input_lon_filename, input_time_filename, input_projection);

      printf("Building indices\n");
      start_time = time(NULL);
      data_index = generate_kdtree_index_from_coordinate_reader(reader);
      if (!data_index) {
         fprintf(stderr, "Failed to build index\n");
         return -1;
      }
      end_time = time(NULL);
      printf("Building index took %d seconds\n", (int) (end_time - start_time));

      reader->free(reader);

      if (saving_index) {
         // Save to disk
         FILE *output_index_file = fopen(output_index_filename, "w");
         data_index->write_to_file(data_index, output_index_file);
         fclose(output_index_file);
      }
   }

   if (generating_image) {

      unsigned int input_data_number_bytes = data_index->num_elements * input_dtype.size;
      unsigned int output_data_number_bytes = width * height * output_dtype.size;
      unsigned int output_geo_number_bytes = width * height * sizeof(float32_t);

      memory_mapped_file *data_input_file = NULL, *data_output_file = NULL, *latitude_output_file = NULL, *longitude_output_file = NULL;

      // Setup input and output specs, and open files
      input_spec in;
      output_spec out;

      if (write_data) {
         data_input_file = open_memory_mapped_input_file(input_data_filename, input_data_number_bytes);
         data_output_file = open_memory_mapped_output_file(output_data_filename, output_data_number_bytes);

         in.data_input = data_input_file->memory_mapped_data;
         out.data_output = data_output_file->memory_mapped_data;
      } else {
         in.data_input = NULL;
         out.data_output = NULL;
      }

      if (write_lats) {
         latitude_output_file = open_memory_mapped_output_file(output_lat_filename, output_geo_number_bytes);
         out.lats_output = (float32_t *) latitude_output_file->memory_mapped_data;
      } else {
         out.lats_output = NULL;
      }

      if (write_lons) {
         longitude_output_file = open_memory_mapped_output_file(output_lon_filename, output_geo_number_bytes);
         out.lons_output = (float32_t *) longitude_output_file->memory_mapped_data;
      } else {
         out.lons_output = NULL;
      }

      in.input_dtype = input_dtype;
      in.coordinate_index = data_index;
      out.output_dtype = output_dtype;
      out.grid_spec = initialise_grid(width, height, vertical_resolution, horizontal_resolution, vertical_sampling, horizontal_sampling, central_x, central_y, data_index->input_projector);
      if (out.grid_spec == NULL) {
         fprintf(stderr, "Failed to initialise output grid\n");
         return -1;
      }
      set_time_constraints(out.grid_spec, time_min, time_max);

      // Perform gridding
      perform_gridding(in, out, selected_reduction_function, &r_attrs, verbosity);

      // Unmap and close files
      if (write_data) {
         data_input_file->close(data_input_file);
         data_output_file->close(data_output_file);
      }
      if (write_lats) {
         latitude_output_file->close(latitude_output_file);
      }
      if (write_lons) {
         longitude_output_file->close(longitude_output_file);
      }
   }

   // Free option strings and working data
   free(input_data_filename);
   free(input_lat_filename);
   free(input_lon_filename);
   free(output_data_filename);
   free(output_lat_filename);
   free(output_lon_filename);
   if (!using_default_projection_string) free(projection_string);
   data_index->free(data_index);

   return 0;
}
