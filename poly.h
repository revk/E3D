// 2D Polygon library Copyright ©2011 Adrian Kennard
// Released under GPL 3.0 http://www.gnu.org/copyleft/gpl.html

//#define       POLY_FLOAT      // Define to use float, not recommended

#ifdef	POLY_FLOAT
typedef long double poly_dim_t;	// Type of co-ordinates
#define	POLY_DIM_MAX	INFINITY	// Max value
#else
typedef long long poly_dim_t;	// Type of co-ordinates
#define		POLY_DIM_MAX	LLONG_MAX	// Max value
#endif


// Main data structures for polygons
typedef struct polygon_s polygon_t;	// Contains contours
typedef struct poly_contour_s poly_contour_t;	// Contains vertices
typedef struct poly_vertex_s poly_vertex_t;	// Contains X and Y

struct polygon_s
{
  poly_contour_t *contours;
  poly_vertex_t **add;		// pointer to last vertex in first contour, cleared by poly_start, used by poly_add
};

struct poly_contour_s
{
  poly_contour_t *next;
  poly_vertex_t *vertices;
  int dir;
};

struct poly_vertex_s
{
  poly_vertex_t *next;
  int flag;			// General purpose flag on line segment from this vertex to next, poly_clip adds up flags used to make output
  poly_dim_t x, y;
};

// General functions
polygon_t *poly_new (void);	// New empty malloced polygon
void poly_free (polygon_t *);	// Free malloced polygon, contours and vertices
void poly_start (polygon_t *);	// Start of new contour
void poly_add (polygon_t *, poly_dim_t x, poly_dim_t y, int flag);	// Add point to end of new contour (adds new contour at start of contours if needed)

void poly_tidy (polygon_t *, poly_dim_t tolerance);	// Remove dead ends and redundant midpoints from contours in situ, and remove contours of <3 vertices
polygon_t *poly_inset (polygon_t *, poly_dim_t);	// make new polygon to right of (i.e. inside) existing contours at specified offset

// Basic polygon operations - use winding number logic, i.e. clockwise inside clockwise is not a hole.
#define POLY_UNION		1	// Union of all contours
#define POLY_INTERSECT		2	// Intersection of count contours (can be higher for more layers intersected)
#define POLY_DIFFERENCE		-2	// Union subtract intersect (takes intersect of this -ve level from union)
#define POLY_XOR		0	// Simple odd/even logic regardless of contour direction
polygon_t *poly_clip (int operation, int count, polygon_t *, ...);	// return set of simple contours from one or more input polygons

// Test
void poly_test (void);		// Run a series of tests and show output

// Useful inline 2D maths - done inline so compiler will optimise stuff out if not needed, hence lots of parameters...
static inline int
poly_intersect_point (poly_dim_t ax, poly_dim_t ay, poly_dim_t bx, poly_dim_t by, poly_dim_t cx, poly_dim_t cy, poly_dim_t * xp, poly_dim_t * yp,
		      long double *abp, poly_dim_t * pc2p, poly_dim_t *sp)
{
  // Determines P on A-B closet to C and put in xp/yp.
  // Returns non zero if P exists, else A==B
  // Position on A-B where A=0, B=1 is placed in abp
  // Square of distance P-C is placed in pc2p
  // Side -ve for left of A-B, +ve for right of AB place in sp and is distance P-C
  poly_dim_t l2 = (bx - ax) * (bx - ax) + (by - ay) * (by - ay);
  if (!l2)
    return 0;
  if (abp || xp || yp)
    {
      poly_dim_t abh = ((cx - ax) * (bx - ax) + (cy - ay) * (by - ay));
      if (abp)
	*abp = (long double) abh / l2;
      if (xp || yp)
	{
	  poly_dim_t px = ax + (abh * (bx - ax)) / l2;
	  poly_dim_t py = ay + (abh * (by - ay)) / l2;
	  if (xp)
	    *xp = px;
	  if (yp)
	    *yp = py;
	}
    }
  if (pc2p || sp)
    {
      poly_dim_t sh = ((ay - cy) * (bx - ax) - (ax - cx) * (by - ay));
      if (sp)
	*sp = sh *sqrtl(l2)/ l2;
      if (pc2p)
	*pc2p = sh * sh / l2;
    }
  return 1;
}

static inline int
poly_intersect_line (poly_dim_t ax, poly_dim_t ay, poly_dim_t bx, poly_dim_t by, poly_dim_t cx, poly_dim_t cy, poly_dim_t dx, poly_dim_t dy, poly_dim_t * xp,
		     poly_dim_t * yp, long double *abp, long double *cdp)
{				// Determines P on A-B and C-D and puts in xp/yp. returns non zero if P exists (else parallel or zero line segment)
  // Position on A-B where A=0, B=1 is placed in abp
  // Position on C-D where C=0, D=1 is placed in cdp
  poly_dim_t d = (dy - cy) * (bx - ax) - (dx - cx) * (by - ay);
  if (!d)
    return 0;
  if (xp || yp || abp)
    {
      long double abh = ((dx - cx) * (ay - cy) - (dy - cy) * (ax - cx));
      if (abp)
	*abp = (long double) abh / d;
      if (xp)
	*xp = ax + (abh * (bx - ax)) / d;
      if (yp)
	*yp = ay + (abh * (by - ay)) / d;
    }
  if (cdp)
    *cdp = (long double) ((bx - ax) * (ay - cy) - (by - ay) * (ax - cx)) / d;
  return 1;
}
