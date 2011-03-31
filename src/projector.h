#ifndef HEADER_PROJECTOR
#define HEADER_PROJECTOR

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
   projected_coordinates (*project)(struct projector_s *p, float latitude, float longitude);
   spherical_coordinates (*inverse_project)(struct projector_s *p, float x, float y);
   void (*serialize_to_file)(struct projector_s *p, FILE *outputfile);
   void (*free)(struct projector_s *p);
} projector;

projector *get_proj_projector_from_string(char *projection_string);

#endif
