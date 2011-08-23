// Extrude 3D model (e3d) Copyright ©2011 Adrian Kennard
// Common definitions for all modules

#ifndef	INCLUDE_E3D
#define	INCLUDE_E3D
#include <math.h>
#include "poly.h"

#ifndef	POLY_FLOAT
#define       FIXED   3		// Used fixed point
#endif

// Types

typedef struct stl_s stl_t;
typedef struct facet_s facet_t;
typedef struct slice_s slice_t;

struct stl_s
{				// STL object
  const char *filename;
  const char *name;
  int count;
  struct
  {
    poly_dim_t x, y, z;
  } min, max;
  facet_t *facets;
  slice_t *slices;
  polygon_t *border;	// total outline of all layers
  polygon_t *anchor;	// Anchor extrude path
};

struct facet_s
{				// Triangular facet
  facet_t *next;
  struct
  {
    poly_dim_t x, y, z;
  } vertex[3];
};

#define	EXTRUDE_PATHS	4	// perimeter (clockwise/anticlockwise) and two types of fill

struct slice_s
{				// main 2D slice of an STL, defines the areas for the slice
  slice_t *next;
  poly_dim_t z;
  polygon_t *outline;		// Outline of layer - from slice
  polygon_t *fill;		// Inside of perimeter
  polygon_t *infill;		// Sparse fill
  polygon_t *solid;		// Solid fill
  polygon_t *flying;		// Flying area
  // extrusion loops
  polygon_t *extrude[EXTRUDE_PATHS];	// Extrude layers in order
};

// Variables
extern int debug;
extern int places;
#ifdef  FIXED
extern poly_dim_t fixed, fixplaces;
#endif

// Macros
#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAX(a,b) ((a)>(b)?(a):(b))


// Common functions
void *mymalloc (size_t n);	// alloc with fatal error if no space, and clearing content to zero
char *dimout (poly_dim_t v);	// Output a dimension (static char space used)
polygon_t * poly_sub (polygon_t * a, polygon_t * b); // subtract a polygon
void poly_order(polygon_t *p,poly_dim_t *xp,poly_dim_t *yp);	// reorder contours

// Extra polygon functions


#endif
