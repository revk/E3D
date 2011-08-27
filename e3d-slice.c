// Extrude 3D model (e3d) Copyright ©2011 Adrian Kennard
// STL processing functions
// Released under GPL 3.0 http://www.gnu.org/copyleft/gpl.html

#include <stdio.h>
#include <string.h>
#include <err.h>
#include <ctype.h>
#include <stdlib.h>
#include <malloc.h>

#include "e3d-slice.h"

//#define       DEBUG

slice_t *
slice (stl_t * stl, poly_dim_t z, poly_dim_t tolerance)
{
  poly_dim_t tolerance2 = tolerance * tolerance;
  // Extract 2D line segments
  typedef struct segment_s segment_t;
  struct segment_s
  {
    segment_t *next, **prev;
    struct
    {
      poly_dim_t x, y;
    } point[2];
  };
  int segcount = 0;
  segment_t *segments = NULL, **next = &segments;
  facet_t *f;
  for (f = stl->facets; f; f = f->next)
    {
      int a, b, c;
      for (a = 0; a < 3 && f->vertex[a].z > z; a++);
      if (a == 3)
	continue;		// all below
      for (b = 0; b < 3 && f->vertex[b].z <= z; b++);
      if (b == 3)		// all above
	continue;
      for (c = 0; c == a || c == b; c++);
      // Line from vertex a->b is one facet... find point. Note a<=z and b>z
      int dir = 0;
      if (a == (b + 1) % 3)
	dir = 1;
      segment_t *s = mymalloc (sizeof (*s));
      *next = s;
      s->prev = next;
      next = &s->next;
      s->point[dir].x = f->vertex[a].x + (f->vertex[b].x - f->vertex[a].x) * (z - f->vertex[a].z) / (f->vertex[b].z - f->vertex[a].z);
      s->point[dir].y = f->vertex[a].y + (f->vertex[b].y - f->vertex[a].y) * (z - f->vertex[a].z) / (f->vertex[b].z - f->vertex[a].z);
      // Line a->c or c->b is another facet... find point
      if (f->vertex[c].z <= z)
	a = c;
      else
	b = c;
      s->point[1 - dir].x = f->vertex[a].x + (f->vertex[b].x - f->vertex[a].x) * (z - f->vertex[a].z) / (f->vertex[b].z - f->vertex[a].z);
      s->point[1 - dir].y = f->vertex[a].y + (f->vertex[b].y - f->vertex[a].y) * (z - f->vertex[a].z) / (f->vertex[b].z - f->vertex[a].z);
      segcount++;
#ifdef DEBUG
      fprintf (stderr, "Segment %s", dimout (s->point[0].x));
      fprintf (stderr, ",%s", dimout (s->point[0].y));
      fprintf (stderr, " %s", dimout (s->point[1].x));
      fprintf (stderr, ",%s\n", dimout (s->point[1].y));
#endif
    }
  if (!segments)
    return NULL;		// nothing found at this Z
  // Construct paths (and free segments)
  polygon_t *outline = poly_new ();
  // find left most segment to start...
  segment_t *s, *b = NULL;
  poly_dim_t x = 0, y = 0;
  for (s = segments; s; s = s->next)
    if (s->point[0].y != s->point[1].y)
      {
	int e;
	for (e = 0; e < 2; e++)
	  if (!b || s->point[e].x < x)
	    {
	      b = s;
	      x = s->point[e].x;
	      y = s->point[e].y;
	    }
      }
  // work out clockwise direction
  int dir = 0;
  if (b->point[0].y > b->point[1].y)
    dir = 1;
  while (segments)
    {
#ifdef DEBUG
      fprintf (stderr, "Starting %s", dimout (x));
      fprintf (stderr, ",%s\n", dimout (y));
#endif
      poly_start (outline);
      b = segments;		// pick a point
      while (1)
	{
	  // unlink
	  if (b->next)
	    b->next->prev = b->prev;
	  *b->prev = b->next;
	  poly_add (outline, b->point[dir].x, b->point[dir].y, 0);
	  poly_dim_t x = b->point[1 - dir].x;
	  poly_dim_t y = b->point[1 - dir].y;
	  free (b);
	  // find closest connected point
	  poly_dim_t bestd = 0;
	  b = NULL;
	  for (s = segments; s; s = s->next)
	    {
	      poly_dim_t d = (s->point[dir].x - x) * (s->point[dir].x - x) + (s->point[dir].y - y) * (s->point[dir].y - y);
	      if (!b || d < bestd)
		{
		  b = s;
		  bestd = d;
		}
	    }
	  if (!b || bestd > tolerance2)
	    break;
#ifdef DEBUG
	  fprintf (stderr, "Best %s\n", dimout (bestd));
#endif
	}
    }

  int seglost = 0;
  if (segments)
    {
      while (segments)
	{
	  seglost++;
#if 0
	  fprintf (stderr, "Segment %s", dimout (segments->point[0].x));
	  fprintf (stderr, ",%s", dimout (segments->point[0].y));
	  fprintf (stderr, " %s", dimout (segments->point[1].x));
	  fprintf (stderr, ",%s\n", dimout (segments->point[1].y));
#endif
	  segment_t *s = segments->next;
	  free (segments);
	  segments = s;
	}
    }
  if (debug)
    fprintf (stderr, "Slicing at %s made %d segments\n", dimout (z), segcount);
  slice_t *slice = mymalloc (sizeof (*slice));
  slice->z = z;
  poly_tidy (outline, tolerance / 10);
  slice->outline = poly_clip (POLY_UNION, 1, outline);
  poly_free (outline);
  return slice;
}
