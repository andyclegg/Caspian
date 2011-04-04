/**
 * @file
 * @author Andrew Clegg
 *
 * Defines a memory-mapped file interface.
 */
#ifndef HEADER_IO_HELPER
#define HEADER_IO_HELPER

/**
 * Representation of a memory mapped file.
 */
typedef struct memory_mapped_file_s {
   /** The file descriptor of the opened file.*/
   int file_descriptor;

   /** The memory address where the mapped data can be accessed.*/
   void *memory_mapped_data;

   /** The number of bytes mapped into memory.*/
   unsigned int mapped_bytes;

   /**
    * Unmap and close the memory mapped file.
    *
    * @param toclose The memory mapped file to close.
    */
   void (*close)(struct memory_mapped_file_s *toclose);
} memory_mapped_file;

memory_mapped_file *open_memory_mapped_input_file(char *filename, unsigned int number_bytes);
memory_mapped_file *open_memory_mapped_output_file(char *filename, unsigned int number_bytes);

#endif

