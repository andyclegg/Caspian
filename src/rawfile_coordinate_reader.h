/**
 * @file
 * @author Andrew Clegg
 *
 * Defines a raw-file backed implementation of coordinate_reader.
 */
#ifndef  HEADER_LATLON_READER
#define HEADER_LATLON_READER
#include <stdio.h>

#include "coordinate_reader.h"
#include "projector.h"

typedef struct rawfile_coordinate_reader_s {
   FILE *lat_file, *lon_file, *time_file;
   unsigned int current_record;
} rawfile_coordinate_reader;

coordinate_reader *get_coordinate_reader_from_files(char *lat_filename, char *lon_filename, char *time_filename, projector *input_projection);
#endif
