#include <stdio.h>
#include <stdlib.h>

#include <proj_api.h>

int main(int argc, char **argv) {

   // Parse command line
   if (argc != 8) {
      printf("Incorrect number of arguments (got %d, expected 8)\n", argc);
      printf("Usage: projection_string northern_latitude southern_latitude eastern_longitude western_longitude horizontal_resolution vertical_resolution\n");
      return EXIT_FAILURE;
   }

   char *projection_string = argv[1];
   float n_lat = atof(argv[2]);
   float s_lat = atof(argv[3]);
   float e_lon = atof(argv[4]);
   float w_lon = atof(argv[5]);
   int hres = atoi(argv[6]);
   int vres = atoi(argv[7]);

   // Initialize the projection
   projPJ *projection = pj_init_plus(projection_string);
   if (projection == NULL) {
      fprintf(stderr, "Couldn't initialize projection '%s'\n", projection_string);
      return EXIT_FAILURE;
   }

   projUV projection_input, tr_output, bl_output;

   // Calculate top left bound by projecting the top left corner
   projection_input.u = e_lon * DEG_TO_RAD;
   projection_input.v = n_lat * DEG_TO_RAD;
   tr_output = pj_fwd(projection_input, projection);

   // Calculate bottm right bound by projecting the bottom right corner
   projection_input.u = w_lon * DEG_TO_RAD;
   projection_input.v = s_lat * DEG_TO_RAD;
   bl_output = pj_fwd(projection_input, projection);

   // Calculate the centre of the grid (in projected space) by averaging the coordinates
   float centre_u = (tr_output.u + bl_output.u)/2.0;
   float centre_v = (tr_output.v + bl_output.v)/2.0;

   // Calculate the height and width by dividing the size of the grid by the resolution
   int height = (int) ceil(fabs(tr_output.v - bl_output.v) / (float) vres);
   int width = (int) ceil(fabs(tr_output.u - bl_output.u) / (float) hres);

   // Output the calculation result
   printf("Centre: (%f, %f)\n", centre_u, centre_v);
   printf("Width: %d\n", width);
   printf("Height: %d\n", height);

   return EXIT_SUCCESS;
}
