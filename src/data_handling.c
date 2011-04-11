/**
 * @file
 * @author Andrew Clegg
 *
 * Implements methods for retrieving and storing data of different types from untyped memory arrays.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "data_handling.h"

/**
 * Get a single number from an array, formatted as the current working numeric type
 * @param data Pointer to the memory array
 * @param input_dtype The type of the memory array
 * @param index Index of the desired number
 * @return The number from the array, formatted according to NUMERIC_WORKING_TYPE (normally a float or double)
 */
NUMERIC_WORKING_TYPE numeric_get(void *data, dtype input_dtype, int index) {
   switch (input_dtype.specifier) {
      case uint8:
         return (NUMERIC_WORKING_TYPE) ((uint8_t *) data)[index];
      case uint16:
         return (NUMERIC_WORKING_TYPE) ((uint16_t *) data)[index];
      case uint32:
         return (NUMERIC_WORKING_TYPE) ((uint32_t *) data)[index];
      #ifdef SIXTYFOURBIT
      case uint64:
         return (NUMERIC_WORKING_TYPE) ((uint64_t *) data)[index];
      #endif
      case int8:
         return (NUMERIC_WORKING_TYPE) ((int8_t *) data)[index];
      case int16:
         return (NUMERIC_WORKING_TYPE) ((int16_t *) data)[index];
      case int32:
         return (NUMERIC_WORKING_TYPE) ((int32_t *) data)[index];
      #ifdef SIXTYFOURBIT
      case int64:
         return (NUMERIC_WORKING_TYPE) ((int64_t *) data)[index];
      #endif
      case float32:
         return (NUMERIC_WORKING_TYPE) ((float32_t *) data)[index];
      case float64:
         return (NUMERIC_WORKING_TYPE) ((float64_t *) data)[index];
      default:
         fprintf(stderr, "numeric_get received an invalid dtype (%d), quitting.\n", input_dtype.specifier);
         exit(EXIT_FAILURE);
   }
}

/**
 * Store a single number into an array
 * @param data Pointer to the memory array
 * @param output_dtype The type of the memory array
 * @param index Index of the desired storage position
 * @param data_item The number to be stored
 */
void numeric_put(void *data, dtype output_dtype, int index, NUMERIC_WORKING_TYPE data_item) {

   // Shortcut macro - cast the data to the appropriate type and
   // store it in a similarly-cast array. In this way, the index
   // will resolve properly
   #define put(type) ((type *) data)[index] = (type) data_item

   switch (output_dtype.specifier) {
      case uint8:
         put(uint8_t);
         break;
      case uint16:
         put(uint16_t);
         break;
      case uint32:
         put(uint32_t);
         break;
      #ifdef SIXTYFOURBIT
      case uint64:
         put(uint64_t);
         break;
      #endif
      case int8:
         put(int8_t);
         break;
      case int16:
         put(int16_t);
         break;
      case int32:
         put(int32_t);
         break;
      #ifdef SIXTYFOURBIT
      case int64:
         put(int64_t);
         break;
      #endif
      case float32:
         put(float32_t);
         break;
      case float64:
         put(float64_t);
         break;
      default:
         fprintf(stderr, "Unknown dtype '%s' (this is probably a bug in Caspian).\n", output_dtype.string);
         exit(EXIT_FAILURE);
   }
}

/**
 * Get a single piece of coded data from an array.
 * @param data Pointer to the memory array.
 * @param input_dtype The type of the memory array.
 * @param index Index of the desired number.
 * @param output Pointer to where the retrieved data should be stored
 */
void coded_get(void *data, dtype input_dtype, int index, void *output) {
   // Cast the data to char (so that bytes can be indexed), calculate
   // the correct offset, then copy the data from that offset to the
   // specified memory address
   memcpy(output, &((char *) data)[index*input_dtype.size], input_dtype.size);
}

/**
 * Store a single piece of coded data into an array
 * @param data Pointer to the memory array
 * @param output_dtype The type of the memory array
 * @param index Index of the desired storage position
 * @param input The coded data to be stored
 */
void coded_put(void *data, dtype output_dtype, int index, void *input) {
   memcpy(&((char *) data)[index*output_dtype.size], input, output_dtype.size);
}

/**
 * Parse a string representing a dtype ('uint8', 'float64', 'coded16' etc)
 *
 * The dtype string is constructed as type + size, where size is 8, 16, 32 or 64, and type may be one of:
 *   * uint: Unsigned Integer
 *   * int: Signed Integer
 *   * float: Floating Point
 *   * coded: Coded data (treated as an opaque block of memory)
 * If a non-parseable string is passed to this function, it will call exit()
 *
 * @param dtype_string String representing the dtype.
 * @return A dtype struct representing the parsed dtype.
 */
dtype dtype_string_parse(char *dtype_string) {

   // Storage for the parsed dtype
   dtype output;

   // Has the dtype string been parsed?
   int parsed = 0;

   // Shortcut - compare the dtype_string to the data type name, if it
   // matches then store the dtype and set parsed to 1
   #define parse(_type_, _size_, _style_) if(strcmp(#_type_, dtype_string) == 0) { output.specifier = _type_; output.size = _size_; output.data_style = _style_; output.string = #_type_; parsed = 1;}
   parse(uint8, 1, numeric);
   parse(uint16, 2, numeric);
   parse(uint32, 4, numeric);
   parse(uint64, 8, numeric);
   parse(int8, 1, numeric);
   parse(int16, 2, numeric);
   parse(int32, 4, numeric);
   parse(int64, 8, numeric);
   parse(float32, 4, numeric);
   parse(float64, 8, numeric);
   parse(coded8, 1, coded);
   parse(coded16, 2, coded);
   parse(coded32, 4, coded);
   parse(coded64, 8, coded);

   if (parsed) {
      return output;
   } else {
      fprintf(stderr, "Could not parse dtype '%s'.\n", dtype_string);
      exit(EXIT_FAILURE);
   }
}
