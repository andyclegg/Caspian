/**
 * @mainpage
 *
 * @section intro_sec Introduction
 * Caspian is written in a largely object-oriented fashion (although implemented in C, a language with no explicit object-orientation support); it consists of a number of interfaces and implementations of those interfaces. This documentation is intended to describe both interfaces and current implementations, as well as to provide a guide for providing new implementations of various interfaces.
 * @section style_sec Code Style
 * A typical aspect of Caspian (such as a grid, index or reduction function) is described using a struct, with members holding both data and function pointers. Many  of these structs will contain an opaque (void) pointer for storage of implementation-specific information. An implementation (for example, the kdtree implementation of an index) will normally provide a factory function, which allocates and initialises the struct, assigning the implementation's implementation of each method to the appropriate function pointer, and then returns the struct. In addition, each struct will contain an implementation-specific method to free or close the resources associated with that struct, such that each struct may be handled in a generic fashion throughout its lifetime.
 * @section style_structure Structure
 * Input data is read by a coordinate_reader, which projects the data using a \ref projector, and the results are stored in an \ref index. A \ref grid is then iterated over, querying the \ref index for a result_set of observations that fall within that \ref grid cell. The result_set is reduced to a single value by a reduction_function, and saved to the output.
 *
 * By default, coordinate_reader is implemented by \ref rawfile_coordinate_reader.h "a raw file backed reader", \ref projector is implemented by \ref proj_projector.h "the PROJ.4 library", and \ref index is implemented by \ref kd_tree.h "an adaptive kd-tree". A number of implementations of a reduction_function are included, namely \ref reduce_numeric_mean "mean", \ref reduce_numeric_weighted_mean "distance-weighted mean", \ref reduce_numeric_median "median", \ref reduce_coded_nearest_neighbour "nearest-neighbour", and \ref reduce_numeric_newest "newest".
 * @section section_implementation Implementation Details
 * Caspian is designed to handle data of various types and sizes, but to reduce the burden on reduction_function implementers, a system for representing and manipulating data of different types has been created. The types are split into two broad classes - data with a numeric interpretation, and data that should be handled opaquely (i.e. a collection of bits that must be preserved); these are known as numeric and coded data respectively.
 *
 * Numeric data is internally transformed into the data type represented by #NUMERIC_WORKING_TYPE - a floating point type of suitable precision for the machine architecture. By retrieving numeric input data using \ref numeric_get, an implementor of a reduction function can manipulate numeric data of any type using the #NUMERIC_WORKING_TYPE representation. Similarly, the reduction_function need only produce output in #NUMERIC_WORKING_TYPE format, and this will be automatically converted to the correct output representation.
 *
 * Coded data is merely given a unit size, and blocks of this data are copied around with no regard to the contents of each block. Reduction functions use \ref coded_get and \ref coded_put to retrieve the input data and store the output data.
 *
 * To facilitate the handling of these different types of data, the \ref dtype struct represents all information pertaining to a data type - this may be used by reduction functions to, for example, allocate a temporary storage area for coded data by using the dtype::size field to determine the amount of storage necessary.
 */

/**
 * @file
 * @author Andrew Clegg
 *
 * Implements command-line functionality for Caspian.
 */
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
#include "spatial_index.h"

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
   printf("Options                            Default                      Help\n");
   printf(" Index controls\n");
   printf("  -a/--input-lats <filename>                                    Specify filename for input latitude\n");
   printf("  -o/--input-lons <filename>                                    Specify filename for input longitude\n");
   printf("  -e/--input-time <filename>                                    Specify filename for input time\n");
   printf("  -p/--projection <string>         +proj=eqc +datum=WGS84       Specify projection using PROJ.4 compatible string\n");
   printf("  -I/--save-index <filename>                                    Save the index to a file\n");
   printf("  -i/--load-index <filename>                                    Load a pre-generated index from a file\n");
   printf("\n");
   printf(" Input data\n");
   printf("  -d/--input-data <filename>                                    Specify filename for input data\n");
   printf("  -t/--input-dtype <dtype>         float32                      Specify dtype for input data file\n");
   printf("  -f/--input-fill-value <number>   -999.0                       Specify fill value for input data file\n");
   printf("\n");
   printf(" Output data\n");
   printf("  -D/--output-data <filename>                                   Specify filename for output data\n");
   printf("  -T/--output-dtype <dtype>        float32                      Specify dtype for output data file\n");
   printf("  -F/--output-fill-value <number>  -999.0                       Specify fill value for output data file\n");
   printf("  -A/--output-lats <filename>                                   Specify filename for output latitude\n");
   printf("  -O/--output-lons <filename>                                   Specify filename for output longitude\n");
   printf("\n");
   printf(" Image generation\n");
   printf("  -h/--height <integer>            360                          Height of output grid\n");
   printf("  -w/--width <integer>             720                          Width of output grid\n");
   printf("  -V/--vres <number>               polar circumf. / (2*height)  Vertical resolution of output grid, in projection units (metres)\n");
   printf("  -H/--hres <number>               equatorial circumf. / width  Horizontal resolution of output grid, in projection units (metres)\n");
   printf("  -y/--central-y <number>          0.0                          Vertical position of centre of output grid, in projection units (metres)\n");
   printf("  -x/--central-x <number>          0.0                          Horizontal position of centre of output grid, in projection units (metres)\n");
   printf("  -S/--vsample <number>            value of --vres              Vertical sampling resolution\n");
   printf("  -s/--hsample <number>            value of --hres              Horizontal sampling resolution\n");
   printf("  -r/--reduction-function <string> mean                         Choose reduction function to use\n");
   printf("  -q/--time-min                    -inf                         Earliest time to select from\n");
   printf("  -Q/--time-max                    +inf                         Latest time to select from\n");
   printf("\n");
   printf(" General\n");
   printf("  -+/--verbose                                                  Increase verbosity\n");
   printf("  -?/--help                                                     Show this help message\n");
   printf("\n");
   printf("Numeric functions: mean, weighted_mean, median, newest\n");
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

   // Initialization checks for paranoia
   if (sizeof(float32_t) != 4 || sizeof(float64_t) != 8) {
      printf("Unsupported system: 'float' and 'double' not 4 and 8 bytes respectively\n");
   }

   /*****************************************
    *  Command line options                 *
    ****************************************/

   // Setup variables to store command line options, and default values

   // Index controls
   char *input_lat_filename = NULL;
   char *input_lon_filename = NULL;
   char *input_time_filename = NULL;
   char *projection_string = "+proj=eqc +datum=WGS84";
   char *output_index_filename = NULL;
   char *input_index_filename = NULL;

   // Input data
   char *input_data_filename = NULL;
   dtype input_dtype = {float32, 4, numeric, "float32"};
   NUMERIC_WORKING_TYPE input_fill_value = -999.0;

   // Output data
   char *output_data_filename = NULL;
   dtype output_dtype = {float32, 4, numeric, "float32"};
   NUMERIC_WORKING_TYPE output_fill_value = -999.0;
   char *output_lat_filename = NULL;
   char *output_lon_filename = NULL;

   // Image generation
   int height = 360;
   int width = 720;
   double vertical_resolution = 0.0; // Default is calculated later
   double horizontal_resolution = 0.0; // Default is calculated later
   double central_y = 0.0;
   double central_x = 0.0;
   double vertical_sampling = 0.0; // Default is calculated later
   double horizontal_sampling = 0.0; // Default is calculated later
   reduction_function selected_reduction_function = get_reduction_function_by_name("mean");
   float time_min = -INFINITY;
   float time_max = +INFINITY;

   // General
   int verbosity = 0;


   // Control flow variables
   int generating_image = 0;
   int loading_index = 0;
   int saving_index = 0;
   int using_default_projection_string = 1;
   int write_data = 0;
   int write_lats = 0;
   int write_lons = 0;


   // Parse command line arguments
   static struct option long_options[] = {
      // Index controls
      {"input-lats", 1, 0, 'a'},
      {"input-lons", 1, 0, 'o'},
      {"input-time", 1, 0, 'e'},
      {"projection", 1, 0, 'p'},
      {"save-index", 1, 0, 'I'},
      {"load-index", 1, 0, 'i'},

      // Input data
      {"input-data", 1, 0, 'd'},
      {"input-dtype", 1, 0, 't'},
      {"input-fill-value", 1, 0, 'f'},

      // Output data
      {"output-data", 1, 0, 'D'},
      {"output-dtype", 1, 0, 'T'},
      {"output-fill-value", 1, 0, 'F'},
      {"output-lats", 1, 0, 'A'},
      {"output-lons", 1, 0, 'O'},

      // Image generation
      {"height", 1, 0, 'h'},
      {"width", 1, 0, 'w'},
      {"vres", 1, 0, 'V'},
      {"hres", 1, 0, 'H'},
      {"central-y", 1, 0, 'y'},
      {"central-x", 1, 0, 'x'},
      {"vsample", 1, 0, 'S'},
      {"hsample", 1, 0, 's'},
      {"reduction-function", 1, 0, 'r'},
      {"time-min", 1, 0, 'q'},
      {"time-max", 1, 0, 'Q'},

      // General
      {"verbose", 0, 0, '+'},
      {"help", 0, 0, '?'},
   };

   // Shortcut macro - allocate and check storage for the option string,
   // and copy the option string into the allocated space.
   #define save_optarg_string(x) x = calloc(strlen(optarg) + 1, sizeof(char)); if (x == NULL) { fprintf(stderr, "Failed to allocate space to store option string"); exit(-1); }; strcpy(x, optarg)

   int curarg, option_index = 0;
   while((curarg = getopt_long(argc, argv, "", long_options, &option_index)) != -1) {
      switch (curarg) {
         // Index controls
         case 'a':
            save_optarg_string(input_lat_filename);
            break;
         case 'o':
            save_optarg_string(input_lon_filename);
            break;
         case 'e':
            save_optarg_string(input_time_filename);
            break;
         case 'p':
            save_optarg_string(projection_string);
            using_default_projection_string = 0;
            break;
         case 'I':
            save_optarg_string(output_index_filename);
            saving_index = 1;
            break;
         case 'i':
            save_optarg_string(input_index_filename);
            loading_index = 1;
            break;

         // Input data
         case 'd':
            save_optarg_string(input_data_filename);
            break;
         case 't':
            input_dtype = dtype_string_parse(optarg);
            break;
         case 'f':
            input_fill_value = atof(optarg);
            break;

         // Output data
         case 'D':
            save_optarg_string(output_data_filename);
            generating_image = 1;
            write_data = 1;
            break;
         case 'T':
            output_dtype = dtype_string_parse(optarg);
            break;
         case 'F':
            output_fill_value = atof(optarg);
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

         // Image generation
         case 'h':
            height = atoi(optarg);
            if (height <= 0) {
               fprintf(stderr, "Height must be a positive integer (got %d)\n", height);
               exit(-1);
            }
            break;
         case 'w':
            width = atoi(optarg);
            if (width <= 0) {
               fprintf(stderr, "Width must be a positive integer (got %d)\n", width);
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
         case 'H':
            horizontal_resolution = atof(optarg);
            if (horizontal_resolution <= 0.0) {
               fprintf(stderr, "Horizontal resolution must be a positive number (got %f)\n", horizontal_resolution);
               exit(-1);
            }
            break;
         case 'y':
            central_y = atof(optarg);
            break;
         case 'x':
            central_x = atof(optarg);
            break;
         case 'S':
            vertical_sampling = atof(optarg);
            if (vertical_sampling <= 0.0) {
               fprintf(stderr, "Vertical sampling resolution must be a positive number (got %f)\n", vertical_sampling);
               exit(-1);
            }
            break;
         case 's':
            horizontal_sampling = atof(optarg);
            if (horizontal_sampling <= 0.0) {
               fprintf(stderr, "Horizontal sampling resolution must be a positive number (got %f)\n", horizontal_sampling);
               exit(-1);
            }
            break;
         case 'r': // Reduction function
            selected_reduction_function = get_reduction_function_by_name(optarg);
            if (reduction_function_is_undef(selected_reduction_function)) {
               fprintf(stderr, "Unknown reduction function '%s'\n", optarg);
               exit(-1);
            }
            break;
         case 'q':
            time_min = atof(optarg);
            break;
         case 'Q':
            time_max = atof(optarg);
            break;

         // General
         case '+':
            verbosity++;
            break;
         case '?':
            help(argv[0]);
            return 0;

         default:
            printf("Unrecognised option\n");
            help(argv[0]);
            return -1;
      }
   }

   #ifdef DEBUG
   // Print the control flow variables
   printf("generating image: %d\n", generating_image);
   printf("loading index: %d\n", loading_index);
   printf("saving index: %d\n", saving_index);
   printf("using default projection string: %d\n", using_default_projection_string);
   printf("writing data: %d\n", write_data);
   printf("writing lats: %d\n", write_lats);
   printf("writing lons: %d\n", write_lons);
   #endif

   // Check we have required options - required options depends on mode of operation
   if (!loading_index) {
      if (input_lat_filename == NULL || input_lon_filename == NULL || projection_string == NULL) {
         fprintf(stderr, "Unless you are loading a pre-generated index from disk, you must provide --input-lats, --input-lons, --input-time, and --projection\nSee --help for more information.\n");
         return -1;
      }
   }

   if (generating_image) {
      if (input_data_filename == NULL || output_data_filename == NULL) {
         fprintf(stderr, "When generating an image, you must provide --input-data and --output-data\nSee --help for more information.");
         return -1;
      }
   }

   // Validate coded/non-coded functions/data types
   if (selected_reduction_function.data_style == coded) {
      // Input and output dtype must be the same and coded
      if (input_dtype.data_style != coded || output_dtype.data_style != coded || !dtype_equal(input_dtype, output_dtype)) {
         fprintf(stderr, "When using a coded mapping function, input and output dtype must be the same, and of coded style\n");
      }
   } else if (selected_reduction_function.data_style == numeric) {
      if (input_dtype.data_style != numeric || output_dtype.data_style != numeric) {
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

   /*****************************************
    * Index (load or generate)              *
    ****************************************/

   spatial_index *data_index = NULL;

   if (loading_index) {
      // Read from disk
      FILE *input_index_file = fopen(input_index_filename, "r");
      if (input_index_file == NULL) {
         fprintf(stderr, "Could not open index file %s (%s)\n", input_index_filename, strerror(errno));
         return -1;
      }
      data_index = read_kdtree_index_from_file(input_index_file);
      fclose(input_index_file);
   } else {
      // Generate the index in memory

      // Initialize the projection
      projector *input_projection = get_proj_projector_from_string(projection_string);
      if (input_projection == NULL) {
         fprintf(stderr, "Could not initialize projector\n");
         return -1;
      }

      // Build a coordinate reader
      coordinate_reader *reader = get_coordinate_reader_from_files(input_lat_filename, input_lon_filename, input_time_filename, input_projection);
      if (reader == NULL) {
         fprintf(stderr, "Could not initialize coordinate reader\n");
         return -1;
      }

      // Build the index (kdtree is currently hardcoded)
      if (verbosity > 0) printf("Building indices\n");
      time_t start_time = time(NULL);
      data_index = generate_kdtree_index_from_coordinate_reader(reader);
      if (!data_index) {
         fprintf(stderr, "Failed to build index\n");
         return -1;
      }
      time_t end_time = time(NULL);
      if (verbosity > 0) printf("Building index took %d seconds\n", (int) (end_time - start_time));

      // Get rid of the coordinate reader - no longer needed
      reader->free(reader);

      if (saving_index) {
         // Save the index to disk
         FILE *output_index_file = fopen(output_index_filename, "w");
         data_index->write_to_file(data_index, output_index_file);
         fclose(output_index_file);
      }
   }

   /*****************************************
    * Generate image                        *
    ****************************************/

   if (generating_image) {
      // Calculate file sizes from provided information
      unsigned int input_data_number_bytes = data_index->num_observations * input_dtype.size;
      unsigned int output_data_number_bytes = width * height * output_dtype.size;
      unsigned int output_geo_number_bytes = width * height * sizeof(float32_t);

      memory_mapped_file *data_input_file = NULL, *data_output_file = NULL, *latitude_output_file = NULL, *longitude_output_file = NULL;

      // Setup input and output specs, and open files
      input_spec in;
      in.input_dtype = input_dtype;
      in.coordinate_index = data_index;

      output_spec out;
      out.output_dtype = output_dtype;
      out.grid_spec = initialise_grid(width, height, vertical_resolution, horizontal_resolution, vertical_sampling, horizontal_sampling, central_x, central_y, data_index->input_projector);
      if (out.grid_spec == NULL) {
         fprintf(stderr, "Failed to initialise output grid\n");
         return -1;
      }
      set_time_constraints(out.grid_spec, time_min, time_max);

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

      // Setup reduction options
      reduction_attrs r_attrs;
      r_attrs.input_fill_value = input_fill_value;
      r_attrs.output_fill_value = output_fill_value;

      // Perform gridding
      perform_gridding(in, out, selected_reduction_function, &r_attrs, verbosity);

      // Free the grid
      out.grid_spec->free(out.grid_spec);

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
   free(input_index_filename);
   free(input_lat_filename);
   free(input_lon_filename);
   free(input_time_filename);
   free(output_data_filename);
   free(output_index_filename);
   free(output_lat_filename);
   free(output_lon_filename);
   if (!using_default_projection_string) free(projection_string);
   data_index->free(data_index);

   return 0;
}
