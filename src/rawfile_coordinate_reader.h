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

/**
 * Construct a raw-file backed coordinate_reader from a set of files and a projector.
 *
 * @param lat_filename The 32-bit float file to read latitudes from.
 * @param lon_filename The 32-bit float file to read longitudes from.
 * @param time_filename The 32-bit float file to read times from.
 * @param input_projection The projector with which to project the coordinates read from the files.
 * @return A pointer to an initialised coordinate_reader.
 */
coordinate_reader *get_coordinate_reader_from_files(char *lat_filename, char *lon_filename, char *time_filename, projector *input_projection);
#endif
