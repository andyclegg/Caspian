#include <stdio.h>
#include <stdlib.h>

#include <proj_api.h>

int main(int argc, char **argv) {

   if (argc != 8) {
      printf("Incorrect number of args (%d)\n", argc);
      printf("Usage: projection_string n_lat s_lat e_lon w_lon hres vres\n");
      return -1;
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
      printf("Critical: Couldn't initialize projection\n"); //TODO: Stderr
      return -1;
   }

   projUV projection_input, tr_output, bl_output;

   // Calculate top left bound
   projection_input.u = e_lon * DEG_TO_RAD;
   projection_input.v = n_lat * DEG_TO_RAD;
   tr_output = pj_fwd(projection_input, projection);

   projection_input.u = w_lon * DEG_TO_RAD;
   projection_input.v = s_lat * DEG_TO_RAD;
   bl_output = pj_fwd(projection_input, projection);

   float centre_u = (tr_output.u + bl_output.u)/2.0;
   float centre_v = (tr_output.v + bl_output.v)/2.0;

   int height = (int) ceil(fabs(tr_output.v - bl_output.v) / (float) vres);
   int width = (int) ceil(fabs(tr_output.u - bl_output.u) / (float) hres);


   printf("Centre: (%f, %f)\n", centre_u, centre_v);
   printf("Width: %d, Height: %d\n", width, height);


   return 0;
}
