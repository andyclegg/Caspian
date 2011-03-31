#include <proj_api.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "projector.h"

projected_coordinates _proj_project(projector *p, float longitude, float latitude) {
   projUV pj_input;
   pj_input.u = longitude * DEG_TO_RAD;
   pj_input.v = latitude * DEG_TO_RAD;
   projUV pj_output = pj_fwd(pj_input, (projUV *) p->internals);
   projected_coordinates output = {pj_output.v, pj_output.u};
   #ifdef DEBUG
   printf("(%f, %f) -project-> [%f, %f]\n", longitude, latitude, output.y, output.x);
   #endif
   return output;
}

spherical_coordinates _proj_inverse_project(projector *p, float y, float x) {
   projUV pj_input;
   pj_input.u = y;
   pj_input.v = x;
   projUV pj_output = pj_inv(pj_input, (projUV *) p->internals);
   spherical_coordinates output = {pj_output.u * RAD_TO_DEG, pj_output.v * RAD_TO_DEG};
   return output;
}

void _proj_serialize_to_file(projector *p, FILE *output_file) {
   projPJ *projection = (projPJ *) p->internals;

   // Write the projection string length and string to file
   char *projection_string = pj_get_def(projection, 0);
   unsigned int projection_string_length = strlen(projection_string) + 1;
   fwrite(&projection_string_length, sizeof(unsigned int), 1, output_file);
   fwrite(projection_string, sizeof(char), projection_string_length, output_file);

}

void _proj_free(projector *p) {
   pj_free((projPJ *)p->internals);
   free(p);
}


projector *get_proj_projector_from_string(char *projection_string) {
   projPJ *projection = pj_init_plus(projection_string);
   if (projection == NULL) {
      fprintf(stderr, "Critical: Couldn't initialise projection\n");
      return NULL;
   }

   projector *p = malloc(sizeof(projector));
   p->internals = (void *)projection;
   p->project = &_proj_project;
   p->inverse_project = &_proj_inverse_project;
   p->serialize_to_file = &_proj_serialize_to_file;
   p->free = &_proj_free;

   return p;
}

projector *get_proj_projector_from_file(FILE *input_file) {
   unsigned int projection_string_length;
   fread(&projection_string_length, sizeof(unsigned int), 1, input_file);

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
   projPJ *projection = pj_init_plus(projection_string);
   if (projection == NULL) {
      fprintf(stderr, "Critical: Couldn't initialize projection\n");
      exit(-1);
   }

   projector *p = malloc(sizeof(projector));
   p->internals = (void *)projection;
   p->project = &_proj_project;
   p->inverse_project = &_proj_inverse_project;
   p->serialize_to_file = &_proj_serialize_to_file;
   p->free = &_proj_free;

   return p;
}
