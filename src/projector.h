/**
  * @file
  *
  * Defines a generic interface for projecting and inverse projecting
  *coordinates.
  */
#ifndef HEADER_PROJECTOR
#define HEADER_PROJECTOR

#include <stdio.h>

/**
  * Represent a set of projected coordinates (in the X/Y space, typically in
  *units of metres).
  */
typedef struct {
   /** The Y value */
   float y;

   /** The X value */
   float x;
} projected_coordinates;

/**
  * Represent a set of spherical coordinates (in the latitude/longitude space,
  *typically in units of degrees).
  */
typedef struct {
   /** The longitude value */
   float longitude;

   /** The latitude value */
   float latitude;
} spherical_coordinates;

/**
  * An object capable of projecting data between spherical and projected
  *coordinates.
  */
typedef struct projector_s {
   /** Generic pointer to a data structure containing projector-specific
    *information. */
   void *internals;

   /** Project a longitude/latitude pair into projected space. */
   projected_coordinates (*project)(struct projector_s *p, float longitude,
                                    float latitude);

   /** Inverse project an X/Y pair into spherical coordinate space */
   spherical_coordinates (*inverse_project)(struct projector_s *p, float y,
                                            float x);

   /** Serialise this projector to the a given file */
   void (*serialize_to_file)(struct projector_s *p, FILE *outputfile);

   /** Free this projector */
   void (*free)(struct projector_s *p);
} projector;

#endif
