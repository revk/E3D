// Extrude 3D model (e3d) Copyright ©2011 Adrian Kennard
// Common functions
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



#include <stdio.h>
#include <string.h>
#include <err.h>
#include <ctype.h>
#include <malloc.h>

#include "e3d.h"

#ifdef	FIXED
poly_dim_t fixed, fixplaces;
#endif

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
dimplaces (poly_dim_t v, int places)
{
  static char val[100];
  char *c = val;
#ifdef FIXED
  if (v < 0)
    {
      v = 0 - v;
      *c++ = '-';
    }
  c += sprintf (c, "%lld.%0*lld", v / fixed, places, v % fixed / fixplaces);
#else
  c += sprintf (val, "%.*Lf", places, v);
#endif
  while (c > val && c[-1] == '0')
    c--;
  if (c > val && c[-1] == '.')
    c--;
  *c = 0;
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
  // Note that we consider c->dir==0 to be special and mean an unclosed path
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
	      if (!c->dir)
		break;		// special case of no closed path - only consider first point
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
