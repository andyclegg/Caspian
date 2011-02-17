#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "data_handling.h"

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
         exit(-1);
   }
}

void coded_get(void *data, dtype input_dtype, int index, void *output) {
   memcpy(output, &((char *) data)[index*input_dtype.size], input_dtype.size);
}

void coded_put(void *data, dtype output_dtype, int index, void *input) {
   memcpy(&((char *) data)[index*output_dtype.size], input, output_dtype.size);
}


void numeric_put(void *data, dtype output_dtype, int index, NUMERIC_WORKING_TYPE data_item) {
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
         fprintf(stderr, "numeric_put received an invalid dtype (%s), quitting.\n", output_dtype.string);
         exit(-1);
   }
}

dtype dtype_string_parse(char *dtype_string) {
   dtype output;
   int parsed = 0;
   #define parse(ttyyppee, ssiizzee, ssttyyllee) if(strcmp(#ttyyppee, dtype_string) == 0) { output.specifier = ttyyppee; output.size = ssiizzee; output.type = ssttyyllee; output.string = #ttyyppee; parsed = 1;}
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
      fprintf(stderr, "Could not decode dtype '%s'.\n", dtype_string);
      exit(-1);
   }
}
