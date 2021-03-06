/**
  * @file
  *
  * Implementation of a PROJ.4-based projector
  */
#include <proj_api.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "proj_projector.h"

/**
  * Project a latitude/longitude pair to an X/Y pair
  *
  * @param p The proj-based projector to use.
  * @param longitude The longitude part of the pair (degrees).
  * @param latitude The latitude part of the pair (degrees).
  * @return The projected coordinates.
  */
projected_coordinates _proj_project(projector *p, float longitude,
                                    float latitude) {

   // Construct a proj-compatible input, converting degrees to radians
   // at the same time
   projUV pj_input;
   pj_input.u = longitude * DEG_TO_RAD;
   pj_input.v = latitude * DEG_TO_RAD;

   // Project the input coordinates
   projUV pj_output = pj_fwd(pj_input, (projUV *) p->internals);

   projected_coordinates output = {pj_output.v, pj_output.u};
   return output;
}

/**
  * Project an X/Y pair to a latitude/longitude pair
  *
  * @param p The proj-based projector to use.
  * @param y The Y part of the pair (metres).
  * @param x The X part of the pair (metres).
  * @return The spherical coordinates.
  */
spherical_coordinates _proj_inverse_project(projector *p, float y, float x) {
   // Construct a proj-compatible input
   projUV pj_input;
   pj_input.u = y;
   pj_input.v = x;

   // Project the coordinates
   projUV pj_output = pj_inv(pj_input, (projUV *) p->internals);

   // Convert the result back to degrees
   spherical_coordinates output =
   {pj_output.v * RAD_TO_DEG, pj_output.u * RAD_TO_DEG};
   return output;
}

/**
  * Serialise a proj-based projector a file.
  *
  * This function generates the canonical string representation of a proj
  *projection,
  * using the function pj_get_def; the length of the string followed by the
  *string itself
  * are then written to the file.
  *
  * @param p The proj-based projector to serialize.
  * @param output_file The file to serialize the projector to.
  */
void _proj_serialize_to_file(projector *p, FILE *output_file) {
   projPJ *projection = (projPJ *) p->internals;

   // Get the canonnical representation of the projection string
   char *projection_string = pj_get_def(projection, 0);

   // Calculate the length of the string, including the null terminator
   unsigned int projection_string_length = strlen(projection_string) + 1;

   // Write the projection string length and string to file
   fwrite(&projection_string_length, sizeof(unsigned int), 1, output_file);
   fwrite(projection_string, sizeof(char), projection_string_length,
          output_file);
}

/**
  * Free a proj-based projector.
  *
  * @param p The proj-based projector to free.
  */
void _proj_free(projector *p) {
   pj_free((projPJ *)p->internals);
   free(p);
}

/**
  * Initialise a proj-based projector from a proj string.
  *
  * @param projection_string The proj compatible projection string.
  * @return A pointer to an initialised projector.
  */
projector *get_proj_projector_from_string(char *projection_string) {
   projPJ *projection = pj_init_plus(projection_string);
   if (projection == NULL) {
      fprintf(
         stderr,
         "Couldn't initialise projection '%s' (Proj error message: '%s')\n",
         projection_string, pj_strerrno(*pj_get_errno_ref()));
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

/**
  * Initialise a proj-based projector as specified in the given file (as
  *serialised by a proj-based projector.
  *
  * @param input_file The file which a proj-based projector has been serialised
  *to.
  * @return A pointer to an initialised projector.
  */
projector *get_proj_projector_from_file(FILE *input_file) {
   unsigned int projection_string_length;
   fread(&projection_string_length, sizeof(unsigned int), 1, input_file);

   // Check projection string length
   if (projection_string_length == 0) {
      fprintf(stderr, "Read a projection string length of 0\n");
      exit(EXIT_FAILURE);
   }

   // Read the projection string
   char *projection_string = calloc(projection_string_length, sizeof(char));
   if (projection_string == NULL) {
      fprintf(stderr,
              "Failed to allocate space (%d chars) for projection string\n",
              projection_string_length);
      exit(EXIT_FAILURE);
   }
   fread(projection_string, sizeof(char), projection_string_length, input_file);

   // Paranoid checks on projection string
   if (projection_string[projection_string_length-1] != '\0') {
      fprintf( stderr,
         "Corrupted string read from file (null terminator doesn't exist in "\
         "expected position (%d), found %d)\n",
         projection_string_length,
         projection_string[projection_string_length - 1]);
      // Don't attempt to print out the projection string as we know it's
      // corrupt - very bad things may happen!
      exit(EXIT_FAILURE);
   }
   if (strlen(projection_string) != projection_string_length -1) {
      fprintf(stderr,
              "Corrupted string read from file (string length is wrong)\n");
      // Don't attempt to print out the projection string as we know it's
      // corrupt - very bad things may happen!
      exit(EXIT_FAILURE);
   }

   // Initialize the projection
   projPJ *projection = pj_init_plus(projection_string);
   if (projection == NULL) {
      fprintf(
         stderr,
         "Couldn't initialise projection '%s' (Proj error message: '%s')\n",
         projection_string, pj_strerrno(*pj_get_errno_ref()));
      exit(EXIT_FAILURE);
   }

   projector *p = malloc(sizeof(projector));
   p->internals = (void *)projection;
   p->project = &_proj_project;
   p->inverse_project = &_proj_inverse_project;
   p->serialize_to_file = &_proj_serialize_to_file;
   p->free = &_proj_free;

   return p;
}
