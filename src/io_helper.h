/**
 * @file
 * @author Andrew Clegg
 */
#ifndef HEADER_IO_HELPER
#define HEADER_IO_HELPER
#define _XOPEN_SOURCE 600

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>

typedef struct memory_mapped_file_s {
   int file_descriptor;
   void *memory_mapped_data;
   unsigned int mapped_bytes;
   void (*close)(struct memory_mapped_file_s *toclose);
} memory_mapped_file;

void _close(memory_mapped_file *toclose) {
   munmap(toclose->memory_mapped_data, toclose->mapped_bytes);
   close(toclose->file_descriptor);
   free(toclose);
}

memory_mapped_file *open_memory_mapped_input_file(char *filename, unsigned int number_bytes) {
   memory_mapped_file *f;
   f->file_descriptor open(filename, O_RDONLY);
   if (f->file_descriptor == -1) {
      fprintf(stderr, "Failed to open input data file %s (%s)\n", filename, strerror(errorno));
      exit(-1);
   }
   f->memory_mapped_data = mmap(0, number_bytes, PROT_READ, MAP_SHARED, data_input_fd, 0);
   if (f->memory_mapped_data == MAP_FAILED) {
      fprintf(stderr, "Failed to map data into memory (%s)\n", strerror(errno));
      exit(-1);
   }
   f->mapped_bytes = number_bytes;
   f->close = &_close;
   return f;
}

memory_mapped_file *open_memory_mapped_output_file(char *filename, unsigned int number_bytes) {
   memory_mapped_file *f;
   int output_open_flags = O_CREAT | O_TRUNC | O_RDWR;
   mode_t creation_mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

   f->file_descriptor = open(filename, output_open_flags, creation_mode);
   if (f->file_descriptor == -1) {
      fprintf(stderr, "Failed to open output data file %s (%s)\n", filename, strerror(errorno));
      exit(-1);
   }
   int fallocate_result = posix_fallocate(data_output_fd, 0, number_bytes);
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
         case EFBIG:
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
   f->memory_mapped_data = mmap(0, number_bytes, PROT_WRITE, MAP_SHARED, data_output_fd, 0);
   if (data_output == MAP_FAILED) {
      printf("Failed to map output data into memory (%s)\n", strerror(errno));
      return -1;
   }
   f->mapped_bytes = number_bytes;
   f->close = &_close;

   return f;
}


#endif

