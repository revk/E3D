// Extrude 3D model (e3d) Copyright ©2011 Adrian Kennard
// Make extrude path for each slice

#include <stdio.h>
#include <string.h>
#include <err.h>
#include <ctype.h>

#include "e3d-fill.h"

static void
add_extrude (polygon_t ** pp, polygon_t * p)
{
  if (!p)
    return;
  if (!p->contours)
    {
      poly_free (p);
      return;
    }
  if (!*pp)
    {				// first polygon to add
      *pp = p;
      return;
    }
  poly_contour_t *c;
  for (c = p->contours; c && c->next; c = c->next);
  c->next = (*pp)->contours;
  (*pp)->contours = p->contours;
  p->contours = NULL;
  poly_free (p);
}

void
fill_perimeter (slice_t * slice, poly_dim_t width, int loops, int fast)
{
  if (!loops)
    {
      slice->fill = slice->outline;
      return;
    }
  int l;
  polygon_t *p[loops];
  // work out the loops going in
  polygon_t *q = poly_inset (slice->outline, width / 2);
  for (l = 0; l < loops; l++)
    {
      p[l] = q;
      q = poly_inset (q, (l + 1 < loops) ? width : width / 2);
      if (fast)
	poly_tidy (q, width / 4);	// inner surfaces need way less detail
    }
  slice->fill = q;
  // process loops in reverse order
  while (loops--)
    {				// add each contour - try to make them in order per contour
      q = p[loops];
      if (!q)
	continue;
      poly_contour_t *c = q->contours;
      if (c)
	{
	  q->contours = NULL;
	  while (c)
	    {
	      poly_contour_t *next = c->next;
	      c->next = NULL;
	      polygon_t **pp = &slice->extrude[(c->dir < 0) ? 0 : 1];
	      if (!*pp)
		{		// first contour
		  *pp = poly_new ();
		  (*pp)->contours = c;
		}
	      else
		{		// find closest (by starting point) and append after it
		  poly_contour_t *best = NULL, *z;
		  poly_dim_t bestd = 0;
		  for (z = (*pp)->contours; z; z = z->next)
		    {
		      poly_dim_t d =
			sqrtl ((z->vertices->x - c->vertices->x) * (z->vertices->x - c->vertices->x) +
			       (z->vertices->y - c->vertices->y) * (z->vertices->y - c->vertices->y));
		      if (!best || d < bestd)
			{
			  best = z;
			  bestd = d;
			}
		    }
		  c->next = best->next;
		  best->next = c;
		}
	      c = next;
	    }
	}
      poly_free (q);
    }
}

void
fill_area (stl_t * stl, poly_dim_t width, int layers)
{				// work out types of fill area based on layers
  polygon_t *p, *q;
  slice_t *prev = NULL, *base = stl->slices, *s;
  int count = 0;
  for (s = stl->slices; s; s = s->next)
    {
      q = poly_clip (POLY_UNION, 2, stl->border, s->outline);
      poly_free (stl->border);
      stl->border = q;
      // flying layers
      if (prev)
	{
	  p = poly_sub (s->fill, prev->outline);
	  q = poly_inset (p, -width);
	  poly_free (p);
	  p = poly_clip (POLY_INTERSECT, 2, s->fill, q);
	  poly_free (q);
	  s->flying = p;
	}

      // union of fill from adjacent layers
      p = NULL;
      if (count >= layers)
	{
	  p = poly_clip (POLY_UNION, 1, s->fill);
	  int n = layers * 2 + 1;
	  slice_t *l = base;
	  while (l && n-- > 0)
	    {
	      if (l != s)
		{
		  q = poly_clip (POLY_INTERSECT, 2, l->fill, p);
		  poly_free (p);
		  p = q;
		}
	      l = l->next;
	    }
	  if (n > 0)
	    {
	      poly_free (p);
	      p = NULL;
	    }
	}
      q = poly_sub (s->fill, p);
      poly_free (p);
      p = poly_sub (q, s->flying);
      poly_free (q);

      q = poly_inset (p, width);
      poly_free (p);
      p = poly_inset (q, -width);
      poly_free (q);
      q = poly_clip (POLY_INTERSECT, 2, s->fill, p);
      poly_free (p);
      s->solid = q;

      q = poly_sub (s->fill, s->solid);
      s->infill = poly_sub (q, s->flying);
      poly_free (q);
      prev = s;
      if (count >= layers)
	base = base->next;
      count++;
    }
}

static void
fill (int e, stl_t * s, slice_t * a, polygon_t * p, int dir, poly_dim_t width, double density)
{
  if (density <= 0)
    return;
  if (!p || !p->contours)
    return;
  polygon_t *q = poly_inset (p, width / 2);
  poly_dim_t w = s->max.x - s->min.x, y, d = width * sqrtl (2.0), dy = d * (2.0 / density), iy = dy - d;
  for (y = s->min.y - w; y < s->max.y + dy; y += dy)
    {
      polygon_t *n = poly_new ();
      poly_dim_t oy = y + (d * dir / 4) % dy;
      if (dir & 1)
	{
	  poly_add (n, s->min.x, oy, 0);
	  poly_add (n, s->min.x, oy + iy, 0);
	  poly_add (n, s->max.x, oy + w + iy, 0);
	  poly_add (n, s->max.x, oy + w, 0);
	}
      else
	{
	  poly_add (n, s->max.x, oy + iy, 0);
	  poly_add (n, s->max.x, oy, 0);
	  poly_add (n, s->min.x, oy + w, 0);
	  poly_add (n, s->min.x, oy + w + iy, 0);
	}
      add_extrude (&a->extrude[e], poly_clip (POLY_INTERSECT, 2, n, q));
      poly_free (n);
    }
  poly_free (q);
}

void
fill_extrude (stl_t * s, poly_dim_t width, double density)
{				// Generate extrude path for fills
  int layer = 0;
  slice_t *a;
  for (a = s->slices; a; a = a->next)
    {
      fill (2, s, a, a->infill, layer, width, density);
      fill (2, s, a, a->solid, layer, width, 1);
      fill (3, s, a, a->flying, layer, width, 1);	// TODO direction best for flying??
      layer++;
    }
}

void
fill_anchor (stl_t * stl, int loops, poly_dim_t width, poly_dim_t offset, poly_dim_t step)
{				// Add anchor to layer 0
  slice_t *s = stl->slices;
  if (!s)
    return;
  polygon_t *p = poly_inset (s->outline, width / 2);
  polygon_t *ol = poly_inset (p, -width - offset);
  poly_free (p);
  {				// add the join points
    polygon_t *j = poly_new ();
    poly_contour_t *contour;
    for (contour = ol->contours; contour; contour = contour->next)
      {
	poly_vertex_t *a;
	poly_dim_t d = step;
	for (a = contour->vertices; a; a = a->next)
	  {
	    poly_vertex_t *b = (a->next ? : contour->vertices);
	    poly_dim_t l = sqrtl ((b->x - a->x) * (b->x - a->x) + (b->y - a->y) * (b->y - a->y));
	    poly_dim_t dx = (b->x - a->x) * offset / l;
	    poly_dim_t dy = (b->y - a->y) * offset / l;
	    while (d < l)
	      {
		poly_dim_t x = a->x + (b->x - a->x) * d / l;
		poly_dim_t y = a->y + (b->y - a->y) * d / l;
		poly_start (j);
		poly_add (j, x + dx, y + dy, 0);
		poly_add (j, x + dy, y - dx, 0);
		poly_add (j, x - dx, y - dy, 0);
		poly_add (j, x - dy, y + dx, 0);
		d += step;
	      }
	    d -= l;
	  }
      }
    p = poly_sub (ol, j);
    poly_free (j);
  }
  polygon_t *ol2 = NULL;
  if (s->next)
    {				// don't join to layer above by mistake
      polygon_t *t1 = poly_inset (s->next->outline, width / 2);
      polygon_t *ol2 = poly_inset (t1, -width);
      polygon_t *t2 = poly_clip (POLY_UNION, 2, p, ol2);
      poly_free (p);
      poly_free (t1);
      p = t2;
    }
  add_extrude (&stl->anchor, p);
  if (!--loops)
    {
      poly_free (ol);
      poly_free (ol2);
      p = poly_clip (POLY_UNION, 2, stl->border, stl->anchor);
      poly_free (stl->border);
      stl->border = p;
      return;
    }
  // Additional loops - dont loop round the joins as messy and unnecessary
  if (s->next)
    {				// don't join to layer above by mistake
      p = poly_clip (POLY_UNION, 2, ol, ol2);
      poly_free (ol2);
      poly_free (ol);
    }
  else
    p = ol;			// ol2 not set so no need to free it
  polygon_t *next = poly_inset (p, -width);
  poly_free (p);
  p = next;
  while (loops--)
    {				// add the layers
      next = poly_inset (p, -width);
      add_extrude (&stl->anchor, p);
      p = next;
    }
  polygon_t *q = poly_clip (POLY_UNION, 2, stl->border, p);
  poly_free (p);
  poly_free (stl->border);
  stl->border = q;
}
