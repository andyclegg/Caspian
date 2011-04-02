/**
 * @file
 * @author Andrew Clegg
 */
#ifndef HEADER_IO_HELPER
#define HEADER_IO_HELPER

typedef struct memory_mapped_file_s {
   int file_descriptor;
   void *memory_mapped_data;
   unsigned int mapped_bytes;
   void (*close)(struct memory_mapped_file_s *toclose);
} memory_mapped_file;

memory_mapped_file *open_memory_mapped_input_file(char *filename, unsigned int number_bytes);
memory_mapped_file *open_memory_mapped_output_file(char *filename, unsigned int number_bytes);

#endif

