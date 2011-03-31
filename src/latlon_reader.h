#ifndef  HEADER_LATLON_READER
#define HEADER_LATLON_READER
#include <stdio.h>

#include "projector.h"

typedef struct latlon_reader {
   FILE *lat_file, *lon_file, *time_file;
   projector *input_projector;
   unsigned int num_records, current_record;
} latlon_reader_t;

latlon_reader_t *latlon_reader_init(char *lat_filename, char *lon_filename, char *time_filename, projector *input_projection);
unsigned int latlon_reader_get_num_records(latlon_reader_t *reader);
int latlon_reader_read(latlon_reader_t *reader, float *x, float *y, float *t);
void latlon_reader_free(latlon_reader_t *reader);
#endif
