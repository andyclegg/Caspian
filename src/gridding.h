/**
  * @file
  */
#ifndef HEADER_GRIDDING
#define HEADER_GRIDDING

#include "io_spec.h"
#include "reduction_functions.h"

// Function prototype - implementation in gridding.c
void perform_gridding(input_spec inspec, output_spec outspec,
                      reduction_function reduce_func, reduction_attrs *attrs,
                      int verbosity);

#endif
