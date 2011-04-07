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

/**
 * Define the rawfile-specific portion of a coordinate_reader.
 *
 * @see get_coordinate_reader_from_files
 */
typedef struct rawfile_coordinate_reader_s {
   /** The 32-bit float file to read latitudes from.*/
   FILE *lat_file;

   /** The 32-bit float file to read longitudes from.*/
   FILE *lon_file;

   /** The 32-bit float file to read times from.*/
   FILE *time_file;

   /** The index of the current record.*/
   unsigned int current_record;
} rawfile_coordinate_reader;

// Function prototype - implementation in rawfile_coordinate_reader.c
coordinate_reader *get_coordinate_reader_from_files(char *lat_filename, char *lon_filename, char *time_filename, projector *input_projection);
#endif
