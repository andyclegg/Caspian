#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <math.h>

#include "latlon_reader.h"

latlon_reader_t *latlon_reader_init(char *lat_filename, char *lon_filename, char *time_filename, projector *input_projector) {

   // Open the files for reading, check sizes
   struct stat lat_stat, lon_stat, time_stat;
   size_t no_elements;
   FILE *lat_file, *lon_file, *time_file;

   if(stat(lat_filename, &lat_stat) != 0) {
      printf("Critical: Could not stat the latitude file %s (%s)\n", lat_filename, strerror(errno));
      return NULL;
   }
   if(stat(lon_filename, &lon_stat) != 0) {
      printf("critical: could not stat the longitude file %s (%s)\n", lon_filename, strerror(errno));
      return NULL;
   }
   if (time_filename != NULL) {
      if(stat(time_filename, &time_stat) != 0) {
         printf("critical: could not stat the time file %s (%s)\n", time_filename, strerror(errno));
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
   if ((lat_stat.st_size % sizeof(float)) != 0) {
      fprintf(stderr, "Critical: Size not divisible by %lu\n", sizeof(float));
      return NULL;
   }
   no_elements = (size_t) lat_stat.st_size / sizeof(float);

   lat_file = fopen(lat_filename, "r");
   lon_file = fopen(lon_filename, "r");
   if (time_filename == NULL) {
      time_file = fopen("/dev/zero", "r");
   } else {
      time_file = fopen(time_filename, "r");
   }

   if (lat_file == NULL) {
      fprintf(stderr, "Critical: Couldn't open lat file\n");
      return NULL;
   }
   if (lon_file == NULL) {
      fprintf(stderr, "Critical: Couldn't open lon file\n");
      return NULL;
   }
   if (time_file == NULL) {
      fprintf(stderr, "Critical: Couldn't open time file\n");
      return NULL;
   }

   // Allocate the new reader
   latlon_reader_t *new_reader = malloc(sizeof(latlon_reader_t));
   if (new_reader == NULL) {
      return NULL;
   }
   new_reader->lat_file = lat_file;
   new_reader->lon_file = lon_file;
   new_reader->time_file = time_file;
   new_reader->input_projector = input_projector;
   new_reader->num_records = no_elements;
   new_reader->current_record = 0;

   return new_reader;
}

void latlon_reader_free(latlon_reader_t *reader) {

   // Close files
   fclose(reader->lat_file);
   fclose(reader->lon_file);

   free(reader);
}

unsigned int latlon_reader_get_num_records(latlon_reader_t *reader) {
   return reader->num_records;
}

int latlon_reader_read(latlon_reader_t *reader, float *x, float *y, float *t) {
   float latitude, longitude;

   // Check to see if we are at the end
   if (reader->current_record >= reader->num_records) {
      return 0;
   }

   // Read, project and store a single latitude and longitude pair
   fread(&latitude, sizeof(float), 1, reader->lat_file);
   fread(&longitude, sizeof(float), 1, reader->lon_file);
   fread(t, sizeof(float), 1, reader->time_file);
   if (!isfinite(latitude) || !isfinite(longitude) || !isfinite(*t)) {
      fprintf(stderr, "Non-finite latitude/longitude/time read (NaN or Inf)\n");
      exit(-1);
   }

   projected_coordinates output = reader->input_projector->project(reader->input_projector, longitude, latitude);

   *y = (float) output.y;
   *x = (float) output.x;

   // Increment current record
   reader->current_record++;

   // Signal success
   return 1;
}
