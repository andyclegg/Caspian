/**
 * @file
 * @author Andrew Clegg
 *
 * Implements common file tasks for gridding.
 */
#define _XOPEN_SOURCE 600
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "io_helper.h"

void _close(memory_mapped_file *toclose) {
   munmap(toclose->memory_mapped_data, toclose->mapped_bytes);
   close(toclose->file_descriptor);
   free(toclose);
}

memory_mapped_file *open_memory_mapped_input_file(char *filename, unsigned int number_bytes) {
   memory_mapped_file *f = malloc(sizeof(memory_mapped_file));
   if (f == NULL) {
      fprintf(stderr, "Could not allocate space for a memory_mapped_file struct (!)\n");
      exit(-1);
   }
   f->file_descriptor = open(filename, O_RDONLY);
   if (f->file_descriptor == -1) {
      fprintf(stderr, "Failed to open input file %s (%s)\n", filename, strerror(errno));
      exit(-1);
   }
   f->memory_mapped_data = mmap(0, number_bytes, PROT_READ, MAP_SHARED, f->file_descriptor, 0);
   if (f->memory_mapped_data == MAP_FAILED) {
      fprintf(stderr, "Failed to map data into memory (%s)\n", strerror(errno));
      exit(-1);
   }
   f->mapped_bytes = number_bytes;
   f->close = &_close;
   return f;
}

memory_mapped_file *open_memory_mapped_output_file(char *filename, unsigned int number_bytes) {
   memory_mapped_file *f = malloc(sizeof(memory_mapped_file));
   if (f == NULL) {
      fprintf(stderr, "Could not allocate space for a memory_mapped_file struct (!)\n");
      exit(-1);
   }
   int output_open_flags = O_CREAT | O_TRUNC | O_RDWR;
   mode_t creation_mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

   f->file_descriptor = open(filename, output_open_flags, creation_mode);
   if (f->file_descriptor == -1) {
      fprintf(stderr, "Failed to open output file %s (%s)\n", filename, strerror(errno));
      exit(-1);
   }
   int fallocate_result = posix_fallocate(f->file_descriptor, 0, number_bytes);
   if(fallocate_result != 0) {
      fprintf(stderr, "Could not allocate %d bytes of space in file sytem (", number_bytes);
      switch (fallocate_result) {
         case EBADF:
         case ENODEV:
            fprintf(stderr, "Output file was invalid or not a regular file");
            break;
         case EFBIG:
            fprintf(stderr, "Number of bytes exceeded maximum file size");
            break;
         case EINVAL:
            fprintf(stderr, "Invalid number of bytes (probably a bug in Caspian)");
            break;
         case ENOSPC:
            fprintf(stderr, "Not enough space in the filesytem");
            break;
         case ESPIPE:
            fprintf(stderr, "File descriptor refers to a pipe (probably a bug in Caspian)");
            break;
         default:
            fprintf(stderr, "Unknown error (number %d)", fallocate_result);
      }
      fprintf(stderr, ")\n");
      exit(-1);
   }
   f->memory_mapped_data = mmap(0, number_bytes, PROT_WRITE, MAP_SHARED, f->file_descriptor, 0);
   if (f->memory_mapped_data == MAP_FAILED) {
      printf("Failed to map output into memory (%s)\n", strerror(errno));
      exit(-1);
   }
   f->mapped_bytes = number_bytes;
   f->close = &_close;

   return f;
}
