/**
 * @file
 * @author Andrew Clegg
 *
 * Defines a PROJ.4-based projector
 */
#ifndef HEADER_PROJ_PROJECTOR
#define HEADER_PROJ_PROJECTOR
#include <stdio.h>

#include "projector.h"

// Function prototypes - implementations in proj_projector.c
projector *get_proj_projector_from_string(char *projection_string);
projector *get_proj_projector_from_file(FILE *input_file);
#endif
