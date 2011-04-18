/**
  * @file
  *
  * Implements common file tasks for gridding.
  */

// Define xopen source macro to enable posix_fallocate
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
   // Unmap the data from memory
   munmap(toclose->memory_mapped_data, toclose->mapped_bytes);

   // Close the backing file
   close(toclose->file_descriptor);

   // Free the memory_mapped_file struct
   free(toclose);
}

/**
  * Open and memory map an input file.
  *
  * @param filename The file path to open.
  * @param number_bytes The number of bytes to map into memory
  * @return An instance of memory_mapped_file, or NULL on failure.
  */
memory_mapped_file *open_memory_mapped_input_file(char *filename,
                                                  unsigned int number_bytes) {
   // Allocate space for the memory_mapped_file struct
   memory_mapped_file *f = malloc(sizeof(memory_mapped_file));
   if (f == NULL) {
      fprintf(stderr,
              "Could not allocate space for a memory_mapped_file struct (!)\n");
      exit(EXIT_FAILURE);
   }

   // Open the actual file
   f->file_descriptor = open(filename, O_RDONLY);
   if (f->file_descriptor == -1) {
      fprintf(stderr, "Failed to open input file %s (%s)\n", filename,
              strerror(errno));
      exit(EXIT_FAILURE);
   }

   // Map the data from the file into memory
   f->memory_mapped_data =
      mmap(0, number_bytes, PROT_READ, MAP_SHARED, f->file_descriptor,
           0);
   if (f->memory_mapped_data == MAP_FAILED) {
      fprintf(stderr, "Failed to map data into memory (%s)\n", strerror(errno));
      exit(EXIT_FAILURE);
   }

   // Finish off the memory_mapped_file struct and return
   f->mapped_bytes = number_bytes;
   f->close = &_close;
   return f;
}

/**
  * Open and memory map an output file.
  *
  * @param filename The file path to open.
  * @param number_bytes The number of bytes to map into memory
  * @return An instance of memory_mapped_file, or NULL on failure.
  */
memory_mapped_file *open_memory_mapped_output_file(char *filename,
                                                   unsigned int number_bytes) {
   // Allocate space for the memory_mapped_file struct
   memory_mapped_file *f = malloc(sizeof(memory_mapped_file));
   if (f == NULL) {
      fprintf(stderr,
              "Could not allocate space for a memory_mapped_file struct (!)\n");
      exit(EXIT_FAILURE);
   }

   // Create the actual file
   int output_open_flags = O_CREAT | O_TRUNC | O_RDWR;
   mode_t creation_mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

   f->file_descriptor = open(filename, output_open_flags, creation_mode);
   if (f->file_descriptor == -1) {
      fprintf(stderr, "Failed to open output file %s (%s)\n", filename,
              strerror(
                 errno));
      exit(EXIT_FAILURE);
   }

   // Allocate space in the file system for the requested number of bytes
   int fallocate_result = posix_fallocate(f->file_descriptor, 0, number_bytes);
   if(fallocate_result != 0) {
      fprintf(stderr, "Could not allocate %d bytes of space in file sytem (",
              number_bytes);
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
         fprintf(stderr,
                 "File descriptor refers to a pipe (probably a bug in Caspian)");
         break;
      default:
         fprintf(stderr, "Unknown error (number %d)", fallocate_result);
      }
      fprintf(stderr, ")\n");
      exit(EXIT_FAILURE);
   }

   // Map the output data into memory
   f->memory_mapped_data =
      mmap(0, number_bytes, PROT_WRITE, MAP_SHARED, f->file_descriptor,
           0);
   if (f->memory_mapped_data == MAP_FAILED) {
      printf("Failed to map output into memory (%s)\n", strerror(errno));
      exit(EXIT_FAILURE);
   }

   // Finish off the memory_mapped_file struct and return
   f->mapped_bytes = number_bytes;
   f->close = &_close;

   return f;
}
