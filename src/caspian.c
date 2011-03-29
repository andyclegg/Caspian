#define _XOPEN_SOURCE 600
#include <errno.h>
#include <fcntl.h>
#include <float.h>
#include <getopt.h>
#include <libgen.h>
#include <math.h>
#include <omp.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "kd_tree.h"
#include "data_handling.h"
#include "reduction_functions.h"
#include "grid.h"
#include "gridding.h"

#define GRIDGEN_FILE_FORMAT 1
#define WGS84_POLAR_CIRCUMFERENCE 40007863.0
#define WGS84_EQUATORIAL_CIRCUMFERENCE 40075017.0

void write_index_to_file(FILE *output_file, struct tree *tree_p, projPJ *projection) {
   // Write the header to file
   unsigned int file_format_number = GRIDGEN_FILE_FORMAT;
   fwrite(&file_format_number, sizeof(unsigned int), 1, output_file);

   // Write the projection string length and string to file
   char *projection_string = pj_get_def(projection, 0);
   unsigned int projection_string_length = strlen(projection_string) + 1;
   fwrite(&projection_string_length, sizeof(unsigned int), 1, output_file);
   fwrite(projection_string, sizeof(char), projection_string_length, output_file);

   // Write the tree to file
   kdtree_save_to_file(output_file, tree_p);

   // Write a concluding header
   fwrite(&file_format_number, sizeof(unsigned int), 1, output_file);
}

void read_index_from_file(FILE *input_file, struct tree **tree_p, projPJ *projection) {
   // Read and check the header
   unsigned int file_format_number;
   unsigned int projection_string_length;
   fread(&file_format_number, sizeof(unsigned int), 1, input_file);
   fread(&projection_string_length, sizeof(unsigned int), 1, input_file);

   if (file_format_number != GRIDGEN_FILE_FORMAT) {
      fprintf(stderr, "Wrong disk file format (read %d, expected %d)\n", file_format_number, GRIDGEN_FILE_FORMAT);
      exit(-1);
   }

   // Read the projection string
   char *projection_string = calloc(projection_string_length, sizeof(char));
   if (projection_string == NULL) {
      fprintf(stderr, "Failed to allocate space (%d chars) for projection string\n", projection_string_length);
      exit(-1);
   }
   fread(projection_string, sizeof(char), projection_string_length, input_file);

   // Paranoid checks on projection string
   if (projection_string[projection_string_length-1] != '\0') {
      fprintf(stderr, "Corrupted string read from file (null terminator doesn't exist in expected position (%d), found %d)\n", projection_string_length, projection_string[projection_string_length - 1]);
      // Don't attempt to print out the projection string as we know it's
      // corrupt - very bad things may happen!
      exit(-1);
   }
   if (strlen(projection_string) != projection_string_length -1) {
      fprintf(stderr, "Corrupted string read from file (string length is wrong)\n");
      // Don't attempt to print out the projection string as we know it's
      // corrupt - very bad things may happen!
      exit(-1);
   }

   // Initialize the projection
   projection = pj_init_plus(projection_string);
   if (projection == NULL) {
      fprintf(stderr, "Critical: Couldn't initialize projection\n");
      exit(-1);
   }

   // Read kdtree
   *tree_p = kdtree_read_from_file(input_file);

   // Check concluding header
   fread(&file_format_number, sizeof(unsigned int), 1, input_file);
   if (file_format_number != GRIDGEN_FILE_FORMAT) {
      fprintf(stderr, "Wrong concluding header (read %d, expected %d)\n", file_format_number, GRIDGEN_FILE_FORMAT);
      exit(-1);
   }

   // Done! :-)
}

void help(char *prog) {
   printf("Usage: %s <options>\n", basename(prog));
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
   time_t start_time, end_time;
   float time_min = -INFINITY, time_max = +INFINITY;

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
   struct reduction_attrs r_attrs;
   r_attrs.input_fill_value = input_fill_value;
   r_attrs.output_fill_value = output_fill_value;

   projPJ *projection = NULL;
   struct tree *root_p;

   if (loading_index) {
      // Read from disk
      FILE *input_index_file = fopen(input_index_filename, "r");
      read_index_from_file(input_index_file, &root_p, projection);
      fclose(input_index_file);
   } else {
      // Initialize the projection
      projection = pj_init_plus(projection_string);
      if (projection == NULL) {
         fprintf(stderr, "Critical: Couldn't initialize projection\n");
         return -1;
      }

      latlon_reader_t *reader = latlon_reader_init(input_lat_filename, input_lon_filename, input_time_filename, projection);

      if (reader == NULL) {
         printf("Failed to initialise data reader\n");
         return -1;
      }

      printf("Building indices\n");
      start_time = time(NULL);
      int result = fill_tree_from_reader(&root_p, reader);
      if (!result) {
         printf("Failed to build tree\n");
         return result;
      }
      end_time = time(NULL);
      printf("Tree built\n");
      printf("Building index took %d seconds\n", (int) (end_time - start_time));

      latlon_reader_free(reader);

      if (saving_index) {
         // Save to disk
         FILE *output_index_file = fopen(output_index_filename, "w");
         write_index_to_file(output_index_file, root_p, projection);
         fclose(output_index_file);
      }
   }


   printf("Verifying tree\n");
   verify_tree(root_p);
   printf("Tree verified as correct\n");

   if (generating_image) {

      unsigned int input_data_number_bytes = root_p->num_elements * input_dtype.size;
      unsigned int output_data_number_bytes = width * height * output_dtype.size;
      unsigned int output_geo_number_bytes = width * height * sizeof(float32_t);
      mode_t creation_mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
      int output_open_flags = O_CREAT | O_TRUNC | O_RDWR;

      int data_input_fd = -1, data_output_fd = -1;
      char *data_input = NULL, *data_output = NULL;
      if (write_data) {
         printf("Mapping %d bytes of input data into memory\n", input_data_number_bytes);
         data_input_fd = open(input_data_filename, O_RDONLY);
         if (data_input_fd == -1) {
            printf("Failed to open input data file %s\n", input_data_filename);
            return -1;
         }
         data_input = mmap(0, input_data_number_bytes, PROT_READ, MAP_SHARED, data_input_fd, 0);
         if (data_input == MAP_FAILED) {
            printf("Failed to map data into memory (%s)\n", strerror(errno));
            return -1;
         }

         printf("Mapping %d bytes of output data into memory\n", output_data_number_bytes);
         data_output_fd = open(output_data_filename, output_open_flags, creation_mode);
         if(posix_fallocate(data_output_fd, 0, output_data_number_bytes) != 0) {
            printf("Failed to allocate space in file system\n");
            return -1;
         }
         data_output = mmap(0, output_data_number_bytes, PROT_WRITE, MAP_SHARED, data_output_fd, 0);
         if (data_output == MAP_FAILED) {
            printf("Failed to map output data into memory (%s)\n", strerror(errno));
            return -1;
         }
      }

      int lats_output_fd  = -1;
      float *lats_output = NULL;
      if (write_lats) {
         lats_output_fd= open(output_lat_filename, output_open_flags, creation_mode);
         if(posix_fallocate(lats_output_fd, 0, output_geo_number_bytes) != 0) {
            printf("Failed to allocate space in file system\n");
            return -1;
         }
         lats_output = mmap(0, output_geo_number_bytes, PROT_WRITE, MAP_SHARED, lats_output_fd, 0);
         if (lats_output == MAP_FAILED) {
            printf("Failed to map output lats file into memory (%s)\n", strerror(errno));
            return -1;
         }
      }

      int lons_output_fd = -1;
      float *lons_output = NULL;
      if (write_lons) {
         lons_output_fd = open(output_lon_filename, output_open_flags, creation_mode);
         if(posix_fallocate(lons_output_fd, 0, output_geo_number_bytes) != 0) {
            printf("Failed to allocate space in file system\n");
            return -1;
         }
         lons_output = mmap(0, output_geo_number_bytes, PROT_WRITE, MAP_SHARED, lons_output_fd, 0);
         if (lons_output == MAP_FAILED) {
            printf("Failed to map output lons file into memory (%s)\n", strerror(errno));
            return -1;
         }
      }

      // Setup input and output specs
      input_spec in;
      output_spec out;

      in.input_dtype = input_dtype;
      in.data_input = data_input;

      out.data_output = data_output;
      out.lats_output = lats_output;
      out.lons_output = lons_output;
      out.output_dtype = output_dtype;
      out.grid_spec = initialise_grid(width, height, vertical_resolution, horizontal_resolution, vertical_sampling, horizontal_sampling, central_x, central_y, projection);
      if (out.grid_spec == NULL) {
         fprintf(stderr, "Failed to initialise output grid\n");
         return -1;
      }
      set_time_constraints(out.grid_spec, time_min, time_max);


      // Perform gridding
      int gridding_result = perform_gridding(in, out, selected_reduction_function, &r_attrs, root_p, verbosity);
      if (!gridding_result) {
         fprintf(stderr, "Gridding failed\n");
         return -1;
      }


      // Unmap and close files
      if (write_data) {
         munmap(data_input, input_data_number_bytes);
         munmap(data_output, output_data_number_bytes);
         close(data_input_fd);
         close(data_output_fd);
      }
      if (write_lats) {
         munmap(lats_output, output_geo_number_bytes);
         close(lats_output_fd);
      }
      if (write_lons) {
         munmap(lons_output, output_geo_number_bytes);
         close(lons_output_fd);
      }
   }


   // Free option strings
   free(input_data_filename);
   free(input_lat_filename);
   free(input_lon_filename);
   free(output_data_filename);
   free(output_lat_filename);
   free(output_lon_filename);
   if (!using_default_projection_string) free(projection_string);

   // Free working data
   free_tree(root_p);
   pj_free(projection);

   return 0;
}
