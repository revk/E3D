// Extrude 3D model (e3d) Copyright ©2011 Adrian Kennard
// Common functions

#include <stdio.h>
#include <string.h>
#include <err.h>
#include <ctype.h>
#include <malloc.h>

#include "e3d.h"

// Common functions
void *
mymalloc (size_t n)
{
  void *m = malloc (n);
  if (!n)
    errx (1, "Cannot malloc %d", (int) n);
  memset (m, 0, n);
  return m;
}

char *
dimout (poly_dim_t v)
{
  static char val[100];
#ifdef FIXED
  char *c = val;
  if (v < 0)
    {
      v = 0 - v;
      *c++ = '-';
    }
  c += sprintf (c, "%lld.%0*lld", v / fixed, places, v % fixed / fixplaces);
#else
  sprintf (val, "%.*Lf", places, v);
#endif
  return val;
}

polygon_t *
poly_sub (polygon_t * a, polygon_t * b)
{
  polygon_t *d = poly_clip (POLY_DIFFERENCE, 2, a, b);
  polygon_t *r = poly_clip (POLY_INTERSECT, 2, a, d);
  poly_free (d);
  return r;
}

void
poly_order (polygon_t * p, poly_dim_t * xp, poly_dim_t * yp)
{				// reorder contours in a polygon - first polygon stays same, but rest ordered to follow on from first if possible.
  if (!p)
    return;
  poly_contour_t *c = p->contours, *n = NULL, **np = &n, **cp;
  while (c && !c->vertices)
    c = c->next;
  if (!c)
    return;
  poly_dim_t x = c->vertices->x, y = c->vertices->y;
  if (xp)
    x = *xp;
  if (yp)
    y = *yp;
  while (p->contours)
    {				// find closes contour
      poly_contour_t *best = NULL;
      poly_dim_t bestd = 0;
      poly_vertex_t *bestv = NULL, *bestvp = NULL, *bestve = NULL;
      for (c = p->contours; c; c = c->next)
	{			// work out distance
	  poly_vertex_t *v, *vp = NULL;
	  for (v = c->vertices; v; v = v->next)
	    {
	      poly_dim_t d = sqrtl ((v->x - x) * (v->x - x) + (v->y - y) * (v->y - y));
	      if (!best || d < bestd)
		{
		  best = c;
		  bestd = d;
		  bestv = v;
		  bestvp = vp;
		}
	      vp = v;
	    }
	  if (best == c)
	    bestve = vp;
	}
      if (bestvp)
	{			// reorder within contour
	  bestve->next = best->vertices;
	  bestvp->next = NULL;
	  best->vertices = bestv;
	}
      for (cp = &p->contours; *cp != best; cp = &(*cp)->next);
      *cp = best->next;
      *np = best;
      np = &best->next;
      x = best->vertices->x;
      y = best->vertices->y;
    }
  p->contours = n;
  if (xp)
    *xp = x;
  if (yp)
    *yp = y;
}
