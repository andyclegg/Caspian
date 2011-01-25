#ifndef  HEADER_LATLON_READER
#define HEADER_LATLON_READER
#include <stdio.h>
#include <proj_api.h>

typedef struct latlon_reader {
   FILE *lat_file, *lon_file;
   projPJ *projection;
   unsigned int num_records, current_record;
} latlon_reader_t;

latlon_reader_t *latlon_reader_init(char *lat_filename, char *lon_filename, projPJ *projection);
unsigned int latlon_reader_get_num_records(latlon_reader_t *reader);
int latlon_reader_read(latlon_reader_t *reader, float *x, float *y);
void latlon_reader_free(latlon_reader_t *reader);
#endif
