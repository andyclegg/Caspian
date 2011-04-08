/**
 * @file
 * @author Andrew Clegg
 *
 * Implentation of a data structure which reads latitude/longitude/time observations, projects the latitude and longitude, and returns the results.
 */
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "coordinate_reader.h"
#include "rawfile_coordinate_reader.h"

/*
 * Free the rawfile coordinate reader and close the associated files.
 *
 * @param reader The rawfile coordinate reader to free.
 */
void _rawfile_coordinate_reader_free(coordinate_reader *tofree) {
   rawfile_coordinate_reader *internals = (rawfile_coordinate_reader *) tofree->internals;

   // Close files
   fclose(internals->lat_file);
   fclose(internals->lon_file);
   fclose(internals->time_file);

   free(internals);
   free(tofree);
}

/*
 * Read and project a single observation from a rawfile coordinate reader.
 *
 * @param reader The coordinate reader to read the observation from.
 * @param x A float pointer specifying where the X value should be stored.
 * @param y A float pointer specifying where the Y value should be stored.
 * @param t A float pointer specifying where the T value should be stored.
 * @return 0 if finished, 1 if more observations are available.
 */
int _rawfile_coordinate_reader_read(coordinate_reader *source, float *x, float *y, float *t) {

   rawfile_coordinate_reader *internals = (rawfile_coordinate_reader *) source->internals;

   float latitude, longitude;

   // Check to see if we are at the end
   if (internals->current_record >= source->num_records) {
      return 0;
   }

   // Read, project and store a single latitude and longitude pair
   fread(&latitude, sizeof(float), 1, internals->lat_file);
   fread(&longitude, sizeof(float), 1, internals->lon_file);
   fread(t, sizeof(float), 1, internals->time_file);
   if (!isfinite(latitude) || !isfinite(longitude) || !isfinite(*t)) {
      fprintf(stderr, "Non-finite latitude/longitude/time read (NaN or Inf)\n");
      exit(-1);
   }

   // Project the horizontal coordinates
   projected_coordinates output = source->input_projector->project(source->input_projector, longitude, latitude);

   // Store the projected results
   *y = (float) output.y;
   *x = (float) output.x;

   // Increment current record
   internals->current_record++;

   // Signal success
   return 1;
}

/**
 * Construct a coordinate reader from the given files, using a specified projector.
 *
 * @param lat_filename The path to the file containing latitudes (may be NULL).
 * @param lon_filename The path to the file containing longitudes (may be NULL).
 * @param time_filename The path to the file containing times (may be NULL).
 * @param input_projector A projector to project the horizontal coordinates from the files into latitude/longitude space.
 * @return A pointer to an initialised coordinate_reader, or NULL on failure.
 */
coordinate_reader *get_coordinate_reader_from_files(char *lat_filename, char *lon_filename, char *time_filename, projector *input_projector) {

   // Open the files for reading, check sizes
   struct stat lat_stat, lon_stat, time_stat;
   int num_records;
   FILE *lat_file, *lon_file, *time_file;

   // Stat all the files to get their sizes (and check their existence!)
   if(stat(lat_filename, &lat_stat) != 0) {
      fprintf(stderr, "Critical: Could not stat the latitude file %s (%s)\n", lat_filename, strerror(errno));
      return NULL;
   }
   if(stat(lon_filename, &lon_stat) != 0) {
      fprintf(stderr, "critical: could not stat the longitude file %s (%s)\n", lon_filename, strerror(errno));
      return NULL;
   }
   if (time_filename != NULL) {
      if(stat(time_filename, &time_stat) != 0) {
         fprintf(stderr, "critical: could not stat the time file %s (%s)\n", time_filename, strerror(errno));
         return NULL;
      }
   }

   //Check file sizes are all equal
   if (!(lat_stat.st_size == lon_stat.st_size)) {
      fprintf(stderr, "Critical: Lat size != Lon size\n");
      return NULL;
   }
   if (time_filename != NULL) {
      if (!(lat_stat.st_size == time_stat.st_size)) {
         fprintf(stderr, "Critical: Lat size != Time size\n");
         return NULL;
      }
   }

   // Check file size is a multiple of the size of a float
   if ((lat_stat.st_size % sizeof(float)) != 0) {
      fprintf(stderr, "Critical: Size not divisible by %Zd\n", sizeof(float));
      return NULL;
   }

   // Calculate number of records in the file
   num_records = (int) (lat_stat.st_size / sizeof(float));

   // Open the latitudes file
   lat_file = fopen(lat_filename, "r");
   if (lat_file == NULL) {
      fprintf(stderr, "Critical: Couldn't open lat file\n");
      return NULL;
   }

   // Open the longitudes file
   lon_file = fopen(lon_filename, "r");
   if (lon_file == NULL) {
      fprintf(stderr, "Critical: Couldn't open lon file\n");
      return NULL;
   }

   // Open the time file
   if (time_filename == NULL) {
      // Fall back to zeros if there is no time file
      time_file = fopen("/dev/zero", "r");
   } else {
      time_file = fopen(time_filename, "r");
   }
   if (time_file == NULL) {
      fprintf(stderr, "Critical: Couldn't open time file\n");
      return NULL;
   }

   // Allocate the new rawfile coordinate reader
   rawfile_coordinate_reader *new_rawfile_reader = malloc(sizeof(rawfile_coordinate_reader));
   if (new_rawfile_reader == NULL) {
      fprintf(stderr, "Failed to allocate space for a rawfile_coordinate_reader\n");
      exit(-1);
   }

   new_rawfile_reader->lat_file = lat_file;
   new_rawfile_reader->lon_file = lon_file;
   new_rawfile_reader->time_file = time_file;
   new_rawfile_reader->current_record = 0;

   // Allocate the new coordinate reader
   coordinate_reader *new_coordinate_reader = malloc(sizeof(coordinate_reader));
   if (new_coordinate_reader == NULL) {
      fprintf(stderr, "Failed to allocate space for a coordinate_reader\n");
      exit(-1);
   }

   new_coordinate_reader->internals = (void *)new_rawfile_reader;
   new_coordinate_reader->num_records = num_records;
   new_coordinate_reader->input_projector = input_projector;
   new_coordinate_reader->free = &_rawfile_coordinate_reader_free;
   new_coordinate_reader->read = &_rawfile_coordinate_reader_read;

   return new_coordinate_reader;
}
