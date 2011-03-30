#ifndef HEADER_GRIDDING
#define HEADER_GRIDDING

#include <proj_api.h>

#include "io_spec.h"
#include "kd_tree.h"
#include "reduction_functions.h"
#include "result_set.h"
#include "index.h"

int perform_gridding(input_spec inspec, output_spec outspec, reduction_function reduce_func, struct reduction_attrs *attrs, index *data_index, int verbose);

#endif
