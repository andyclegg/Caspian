#ifndef HEADER_PROJECTOR
#define HEADER_PROJECTOR

#include <stdio.h>

typedef struct {
   float y;
   float x;
} projected_coordinates;

typedef struct {
   float longitude;
   float latitude;
} spherical_coordinates;

typedef struct projector_s {
   void *internals;
   projected_coordinates (*project)(struct projector_s *p, float longitude, float latitude);
   spherical_coordinates (*inverse_project)(struct projector_s *p, float y, float x);
   void (*serialize_to_file)(struct projector_s *p, FILE *outputfile);
   void (*free)(struct projector_s *p);
} projector;

projector *get_proj_projector_from_string(char *projection_string);
projector *get_proj_projector_from_file(FILE *input_file);

#endif
