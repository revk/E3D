// Extrude 3D model (e3d) Copyright ©2011 Adrian Kennard
// Common definitions for all modules
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details. 
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#ifndef	INCLUDE_E3D
#define	INCLUDE_E3D
#include <math.h>
#include "poly.h"

#ifndef	POLY_FLOAT
#define       FIXED   3		// Used fixed point
extern poly_dim_t fixed, fixplaces;
extern int places;
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
  polygon_t *border;		// total outline of all layers
  polygon_t *anchor;		// Anchor extrude path
  polygon_t *anchorjoin;	// The joining part of anchor
};

struct facet_s
{				// Triangular facet
  facet_t *next;
  struct
  {
    poly_dim_t x, y, z;
  } vertex[3];
};

enum 
{
 EXTRUDE_PERIMETER,
 EXTRUDE_FILL,
 EXTRUDE_FLYING,
 EXTRUDE_PATHS,
};

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
#define dimout(v) dimplaces(v,places)
char *dimplaces (poly_dim_t v, int places);	// Output a dimension (static char space used)
#ifdef	FIXED
#define	dim2d(v)	((long double)(v)/fixed)
#define	d2dim(v)	((v)*fixed)
#else
#define	dim2d(v)	(v)
#define	d2dim(v)	(v)
#endif
polygon_t *poly_sub (polygon_t * a, polygon_t * b);	// subtract a polygon
void poly_order (polygon_t * p, poly_dim_t * xp, poly_dim_t * yp);	// reorder contours

// Extra polygon functions


#endif
