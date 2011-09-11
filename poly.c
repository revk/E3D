// 2D Polygon library Copyright ©2011 Adrian Kennard
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


#include <math.h>
#include <malloc.h>
#include <err.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <limits.h>
#include "poly.h"

#define	MIN(a,b) ((a)<(b)?(a):(b))
#define	MAX(a,b) ((a)>(b)?(a):(b))
#define	ABS(a)	(((a)<0)?(0-(a)):(a))

static void *
MALLOC (size_t n)
{
  void *r = calloc (n, 1);
  if (!r)
    errx (1, "Cannot allocate %d bytes\n", (int) n);
  return r;
}

// General functions
polygon_t *
poly_new (void)
{				// New empty malloced polygon
  return MALLOC (sizeof (polygon_t));
}

void
poly_free_contour (poly_contour_t * c)
{
  poly_vertex_t *v = c->vertices;
  while (v)
    {
      poly_vertex_t *n = v->next;
      free (v);
      v = n;
    }
  free (c);
}

void
poly_free (polygon_t * poly)
{				// Free malloced polygon, contours and vertices
  if (!poly)
    return;
  poly_contour_t *c = poly->contours;
  while (c)
    {
      poly_contour_t *n = c->next;
      poly_free_contour (c);
      c = n;
    }
  free (poly);
}

void
poly_start (polygon_t * poly)
{				// Add new empty contour to start of polygon
  if (!poly)
    return;
  poly->add = NULL;
}

void
poly_add (polygon_t * poly, poly_dim_t x, poly_dim_t y, int flag)
{				// Add new vertex to start of first contour in polygon (make new contour if needed)
  if (!poly)
    return;
  if (!poly->add)
    {
      poly_contour_t *c = MALLOC (sizeof (*c));
      c->next = poly->contours;
      poly->contours = c;
      poly->add = &c->vertices;
    }
  poly_vertex_t *v = MALLOC (sizeof (*v));
  v->x = x;
  v->y = y;
  v->flag = flag;
  *poly->add = v;
  poly->add = &v->next;
}

void
poly_tidy (polygon_t * poly, poly_dim_t tolerance)
{				// Remove dead ends and redundant midpoints from contours in situ, and remove contours of <3 vertices
  if (!poly)
    return;
  poly_contour_t **cc = &poly->contours;
  while (*cc)
    {
      poly_contour_t *contour = *cc;

      // remove loop backs and min points
      poly_vertex_t *a = contour->vertices;
      while (a)
	{
	  poly_vertex_t *b = (a->next ? : contour->vertices);
	  poly_vertex_t *c = (b->next ? : contour->vertices);
#ifdef	POLY_FLOAT
	  poly_dim_t e = a->x;	// work out a sensible epsilon
	  if (b->x > e)
	    e = b->x;
	  if (c->x > e)
	    e = c->x;
	  if (a->y > e)
	    e = a->y;
	  if (b->y > e)
	    e = b->y;
	  if (c->y > e)
	    e = c->y;
	  e /= (1LL << 50);
#else
	  poly_dim_t e = 1;	// easy when not floating point
#endif
	  poly_dim_t d2 = -1;
	  if ((b->x == c->x && b->y == c->y) || !poly_intersect_point (a->x, a->y, c->x, c->y, b->x, b->y, NULL, NULL, NULL, &d2, NULL) || d2 <= e)
	    {
	      if (a->next)
		a->next = b->next;
	      else
		contour->vertices = b->next;
	      free (b);
	      a = contour->vertices;	// start again, annoying as only need to go back one step really.
	      continue;
	    }
	  a = a->next;
	}

      if (tolerance)
	{			// smooth - subtly different as accumulates errors to avoid removing a string of small steps
	  poly_vertex_t *a = contour->vertices;
	  poly_dim_t acc = 0, tolerance2 = tolerance * tolerance;
	  while (a)
	    {
	      poly_vertex_t *b = (a->next ? : contour->vertices);
	      poly_vertex_t *c = (b->next ? : contour->vertices);
	      poly_dim_t ab2 = (a->x - b->x) * (a->x - b->x) + (a->y - b->y) * (a->y - b->y);
	      poly_dim_t bc2 = (c->x - b->x) * (c->x - b->x) + (c->y - b->y) * (c->y - b->y);
	      if ((ab2 > tolerance2 && bc2 > tolerance2) || (ab2 <= tolerance2 && bc2 <= tolerance2))
		{
		  poly_dim_t o = 0;
		  long double ab = 0;
		  if (poly_intersect_point (a->x, a->y, c->x, c->y, b->x, b->y, NULL, NULL, &ab, NULL, &o) && ab > 0 && ab < 1 && abs (acc + o) < tolerance)
		    {		// remove point but accumulate effect
		      acc += o;
		      if (a->next)
			a->next = b->next;
		      else
			contour->vertices = b->next;
		      free (b);
		      continue;
		    }
		}
	      acc = 0;		// moving on
	      a = a->next;
	    }
	}

      {				// check if too small
	int n = 0;
	poly_vertex_t *v;
	for (v = contour->vertices; v && n < 3; v = v->next)
	  n++;
	if (n < 3)
	  {			// delete
	    *cc = contour->next;
	    poly_free_contour (contour);
	    continue;
	  }
      }
      cc = &contour->next;
    }
}

polygon_t *
poly_inset (polygon_t * poly, poly_dim_t inset)
{				// Make new polygon offset from old by the inset distance, inset is +ve to make smaller
  // This is a convoluted way to do it but reliable.
  if (!poly || !poly->contours)
    return poly_new ();
  poly_dim_t width = (inset < 0 ? 0 - inset : inset);
  poly_tidy (poly, width / 20);
  polygon_t *border = poly_new ();
  poly_contour_t *contour;
  poly_vertex_t *a;
  for (contour = poly->contours; contour; contour = contour->next)
    for (a = contour->vertices; a; a = a->next)
      {
	poly_vertex_t *b = (a->next ? : contour->vertices);
	poly_dim_t dx = b->x - a->x;
	poly_dim_t dy = b->y - a->y;
	poly_dim_t l = sqrtl (dx * dx + dy * dy);
	dx = width * dx / l;
	dy = width * dy / l;
	// now add a thicker version of this line
	poly_start (border);
	poly_add (border, b->x - dy, b->y + dx, a->flag);
	poly_add (border, b->x - dy / 2 + dx * 866 / 1000, b->y + dx / 2 + dy * 866 / 1000, a->flag);
	poly_add (border, b->x + dy / 2 + dx * 866 / 1000, b->y - dx / 2 + dy * 866 / 1000, a->flag);
	poly_add (border, b->x + dy, b->y - dx, a->flag);
	poly_add (border, a->x + dy, a->y - dx, a->flag);
	poly_add (border, a->x + dy / 2 - dx * 866 / 1000, a->y - dx / 2 - dy * 866 / 1000, a->flag);
	poly_add (border, a->x - dy / 2 - dx * 866 / 1000, a->y + dx / 2 - dy * 866 / 1000, a->flag);
	poly_add (border, a->x - dy, a->y + dx, a->flag);
      }
  if (inset < 0)
    {				// outset
      polygon_t *out = poly_clip (POLY_UNION, 2, border, poly);
      poly_free (border);
      poly_tidy (out, width / 20);
      return out;
    }
  //inset
  polygon_t *thick = poly_clip (POLY_UNION, 1, border);
  poly_free (border);
  polygon_t *diff = poly_clip (POLY_DIFFERENCE, 2, thick, poly);
  poly_free (thick);
  polygon_t *out = poly_clip (POLY_INTERSECT, 2, diff, poly);
  poly_free (diff);
  poly_tidy (out, width / 20);
  return out;
}

polygon_t *
poly_clip (int operation, int count, polygon_t * poly, ...)
{				// return set of simple contours from one or more input polygons
  //fprintf (stderr, "Poly clip operation %d on %d polygons\n", operation, count);
  polygon_t *new = poly_new ();
  typedef struct segment_s segment_t;
  struct segment_s
  {
    segment_t *next;
    int flag;			// sum of flag
    int dir;			// +ve for a->b, -ve for b->a, may be more than 1 if accumulated segments
    poly_dim_t ax, ay, bx, by;	// ax<bx, or same and ay<by
  };
  segment_t *stage1 = NULL;
  int segcount = 0;
  polygon_t *q = poly;
  poly_vertex_t *a;
  va_list ap;
  va_start (ap, poly);
  int polies = count;
  while (polies--)
    {
      poly_tidy (q, 0);
      poly_contour_t *p;
      if (q)
	for (p = q->contours; p; p = p->next)
	  {
	    for (a = p->vertices; a; a = a->next)
	      {
		poly_vertex_t *b = (a->next ? : p->vertices);
		segment_t *s = MALLOC (sizeof (*s));
		if (a->x < b->x || (a->x == b->x && a->y < b->y))
		  {
		    s->ax = a->x;
		    s->ay = a->y;
		    s->bx = b->x;
		    s->by = b->y;
		    s->dir = 1;
		  }
		else
		  {
		    s->ax = b->x;
		    s->ay = b->y;
		    s->bx = a->x;
		    s->by = a->y;
		    s->dir = -1;
		  }
		s->flag = a->flag;
		s->next = stage1;
		stage1 = s;
		segcount++;
	      }
	  }
      if (count)
	q = va_arg (ap, polygon_t *);
    }
  va_end (ap);
  segment_t *sortsegs (segment_t * s, int n)
  {
    int p;
    segment_t **index = MALLOC (n * sizeof (*index));
    for (p = 0; p < n; p++)
      {
	index[p] = s;
	s = s->next;
      }
    int order (const void *ap, const void *bp)
    {
      segment_t *a = *(segment_t **) ap;
      segment_t *b = *(segment_t **) bp;
      if (a->ax < b->ax)
	return -1;
      if (a->ax > b->ax)
	return 1;
      if (a->ay < b->ay)
	return -1;
      if (a->ay > b->ay)
	return 1;
      return (b->bx - b->ax) * (a->by - a->ay) - (a->bx - a->ax) * (b->by - b->ay);
    }
    qsort (index, n, sizeof (*index), order);
    segment_t **sp = &s;
    for (p = 0; p < n; p++)
      {
	*sp = index[p];
	sp = &(*sp)->next;
      }
    *sp = NULL;
    free (index);
    return s;
  }
  if (!stage1)
    return new;
  segment_t *stage2;
  while (1)
    {				// may run more than once, and splitting lines can change their angle and cause earlier non intersects to be intersects
      int splits = 0;
      stage1 = sortsegs (stage1, segcount);
      segment_t *sweep = NULL;	// Segments in current sweep line, in Y order
      segment_t *queue = NULL;	// Queue of fragments from intersections to be added later, in X order
      inline void recheck (segment_t * p)
      {				// split can change to be vertical when not before
	if (p->ax != p->bx)
	  return;
	if (p->ay <= p->by)
	  return;
	poly_dim_t t = p->ay;
	p->ay = p->by;
	p->by = t;
	p->dir = 0 - p->dir;
      }
      inline void split_line (segment_t * p, poly_dim_t x, poly_dim_t y)
      {
	if (x == p->ax && y == p->ay)
	  return;
	if (x == p->bx && y == p->by)
	  return;
	if (x < MIN (p->ax, p->bx) || x > MAX (p->ax, p->bx))
	  return;
	if (y < MIN (p->ay, p->by) || y > MAX (p->ay, p->by))
	  return;
	splits++;
	//fprintf (stderr, "Split %3d,%-3d %3d,%-3d %3d,%-3d\n", (int) p->ax, (int) p->ay, (int) x, (int) y, (int) p->bx, (int) p->by);
	segment_t *n = MALLOC (sizeof (*n));
	n->ax = x;
	n->ay = y;
	n->bx = p->bx;
	n->by = p->by;
	n->flag = p->flag;
	n->dir = p->dir;
	p->bx = x;
	p->by = y;
	recheck (n);
	recheck (p);
	segment_t **qq;
	for (qq = &queue; (*qq) && (*qq)->ax < x; qq = &(*qq)->next);
	n->next = *qq;
	*qq = n;
	n = p->next;
      }
      inline void intersect_check (segment_t * a, segment_t * b)
      {				// check for segments that cross
	if (!a || !b || a == b)
	  return;
	if (MIN (b->ay, b->by) > MAX (a->ay, a->by))
	  return;		// not close
	if (MIN (a->ay, a->by) > MAX (b->ay, b->by))
	  return;		// not close
	//fprintf (stderr, "Check %3d,%-3d %3d,%-3d %3d,%-3d %3d,%-3d\n", (int) a->ax, (int) a->ay, (int) a->bx, (int) a->by, (int) b->ax, (int) b->ay, (int) b->bx, (int) b->by);
	poly_dim_t x, y;
	if (poly_intersect_line (a->ax, a->ay, a->bx, a->by, b->ax, b->ay, b->bx, b->by, &x, &y, NULL, NULL))
	  {			// simple overlap
	    if (x >= MIN (a->ax, a->bx) && x <= MAX (a->ax, a->bx) && x >= MIN (b->ax, b->bx) && x <= MAX (b->ax, b->bx) &&
		y >= MIN (a->ay, a->by) && y <= MAX (a->ay, a->by) && y >= MIN (b->ay, b->by) && y <= MAX (b->ay, b->by))
	      {
		//fprintf (stderr, "Split %3d,%-3d %3d,%-3d %3d,%-3d %3d,%-3d\n", (int) a->ax, (int) a->ay, (int) a->bx, (int) a->by, (int) b->ax, (int) b->ay, (int) b->bx, (int) b->by);
		split_line (a, x, y);
		split_line (b, x, y);
	      }
	  }
	poly_dim_t pc2;
	// parallel - may overlap
	if (poly_intersect_point (a->ax, a->ay, a->bx, a->by, b->ax, b->ay, &x, &y, NULL, &pc2, NULL) && !pc2)
	  split_line (a, b->ax, b->ay);
	if (poly_intersect_point (a->ax, a->ay, a->bx, a->by, b->bx, b->by, &x, &y, NULL, &pc2, NULL) && !pc2)
	  split_line (a, b->bx, b->by);
	if (poly_intersect_point (b->ax, b->ay, b->bx, b->by, a->ax, a->ay, &x, &y, NULL, &pc2, NULL) && !pc2)
	  split_line (b, a->ax, a->ay);
	if (poly_intersect_point (b->ax, b->ay, b->bx, b->by, a->bx, a->by, &x, &y, NULL, &pc2, NULL) && !pc2)
	  split_line (b, a->bx, a->by);
      }
      poly_dim_t lastx = stage1->ax;
      stage2 = NULL;
      segcount = 0;
      void segment_tidy (poly_dim_t x)
      {
	segment_t *s, **ss = &sweep;
	while ((s = *ss))
	  {
	    if (s->bx < x)
	      {
		*ss = s->next;
		s->next = stage2;
		stage2 = s;
		segcount++;
		continue;
	      }
	    ss = &s->next;
	  }
      }
      inline void segment_add (segment_t * q)
      {				// add segment, in order
	//fprintf (stderr, "Add %3d,%-3d %3d,%-3d %d\n", (int) q->ax, (int) q->ay, (int) q->bx, (int) q->by, q->dir);
	// Not trying to keep ordered.. Simply check against all existing segments
	segment_t *s;
	for (s = sweep; s; s = s->next)
	  intersect_check (s, q);
	q->next = sweep;
	sweep = q;
	if (q->ax != lastx)
	  segment_tidy (lastx = q->ax);
      }
      while (stage1 || queue)
	{			// Sweep
	  segment_t *s = NULL;
	  if (stage1 && (!queue || queue->ax > stage1->ax))
	    {
	      s = stage1;
	      stage1 = s->next;
	    }
	  else
	    {
	      s = queue;
	      queue = s->next;
	    }
	  segment_add (s);
	}
      segment_tidy (POLY_DIM_MAX);
      if (!splits)
	break;
      stage1 = stage2;		// try again
    }

  if (!stage2)
    return new;
  stage2 = sortsegs (stage2, segcount);
  // make paths
  typedef struct path_s path_t;
  struct path_s
  {
    path_t *next;
    poly_vertex_t *a, *b;
  };
  path_t *paths = NULL;
  typedef struct point_s point_t;
  struct point_s
  {
    point_t *next;
    int flag;
    int dir;
    int use;
    poly_dim_t ax, ay, bx, by;
  };
  point_t *points = NULL, **yp = NULL, *p;
  poly_dim_t lastx = stage2->ax - 1;
  int wind = 0;
  void paths_close (poly_dim_t x)
  {
    point_t *p, **pp = &points;
    while ((p = *pp))
      {
	if (p->bx <= x)
	  {
	    *pp = p->next;
	    if (p->use)
	      {
		//fprintf (stderr, "Use %3d,%-3d %3d,%-3d %d\n", (int) p->ax, (int) p->ay, (int) p->bx, (int) p->by, p->use);
#define swap(a,b){poly_dim_t t=a;a=b;b=t;}
		if (p->use < 0)
		  {		// make so this is A->B
		    swap (p->ax, p->bx);
		    swap (p->ay, p->by);
		  }
		// Any paths that can run on to A
		path_t *A, *B;
		for (A = paths; A && (A->b->x != p->ax || A->b->y != p->ay); A = A->next);
		for (B = paths; B && (B->a->x != p->bx || B->a->y != p->by); B = B->next);
		if (A && B)
		  {		// closed/joined path
		    if (A == B)
		      {		// close path
			poly_contour_t *c = MALLOC (sizeof (*c));
			c->next = new->contours;
			new->contours = c;
			c->vertices = A->a;
			if (p->use > 0)
			  c->dir = 1;
			else if (p->use < 0)
			  c->dir = -1;
			if (p->ax == p->bx)
			  c->dir = 0 - c->dir;	// vertical
		      }
		    else
		      {		// join path
			A->b->next = B->a;
			A->b = B->b;
			A->b->flag=p->flag;
		      }
		    path_t **pp;
		    for (pp = &paths; *pp && *pp != B; pp = &(*pp)->next);
		    *pp = B->next;
		    free (B);
		  }
		else if (A)
		  {		// tack on A
		    poly_vertex_t *v = MALLOC (sizeof (*v));
		    v->x = p->bx;
		    v->y = p->by;
		    A->b->flag = p->flag;
		    A->b->next = v;
		    A->b = v;
		  }
		else if (B)
		  {		// tack on B
		    poly_vertex_t *v = MALLOC (sizeof (*v));
		    v->x = p->ax;
		    v->y = p->ay;
		    v->flag = p->flag;
		    v->next = B->a;
		    B->a = v;
		  }
		else
		  {		// new
		    poly_vertex_t *v = MALLOC (sizeof (*v));
		    A = MALLOC (sizeof (*A));
		    A->next = paths;
		    paths = A;
		    v->x = p->ax;
		    v->y = p->ay;
		    v->flag = p->flag;
		    A->a = v;
		    v = MALLOC (sizeof (*v));
		    v->x = p->bx;
		    v->y = p->by;
		    A->b = v;
		    A->a->next = v;
		  }
	      }
	    free (p);
	    continue;
	  }
	pp = &p->next;
      }
  }
  while (stage2)
    {
      segment_t *s = stage2;
      stage2 = s->next;
      while (stage2 && stage2->ax == s->ax && stage2->ay == s->ay && stage2->bx == s->bx && stage2->by == s->by)
	{			// combine multiple segments to cancel out as needed
	  s->flag += stage2->flag;
	  s->dir += stage2->dir;
	  segment_t *n = stage2;
	  stage2 = n->next;
	  free (n);
	}
      if (!s->dir)
	{
	  free (s);
	  continue;
	}
      if (s->ax != lastx)
	{			// start new column
	  //fprintf (stderr, "Sweep X=%d\n", (int) s->ax);
	  paths_close (lastx = s->ax);
	  yp = &points;
	  wind = 0;
	}
      while ((p = *yp))
	{
	  if (p->ay * (p->bx - p->ax) + (p->by - p->ay) * (s->ax - p->ax) > s->ay * (p->bx - p->ax))
	    break;
	  //fprintf (stderr, "Pass %3d,%-3d %3d,%-3d %d\n", (int) p->ax, (int) p->ay, (int) p->bx, (int) p->by, p->dir);
	  wind -= p->dir;
	  yp = &p->next;
	}
      int use = 0, dir = -s->dir;
      if (operation > 0)
	{			// Union/intersect
	  if (wind < operation && wind + dir >= operation)
	    use--;
	  else if (wind >= operation && wind + dir < operation)
	    use++;
	}
      else if (!operation)
	{			// Simple XOR
	  if (dir & 1)
	    use = ((wind & 1) ? 1 : -1);
	}
      else
	{			// Take intersect from union(1)
	  if (wind < 1 && wind + dir >= 1)
	    use--;
	  else if (wind >= 1 && wind + dir < 1)
	    use++;
	  if (wind < -operation && wind + dir >= -operation)
	    use++;
	  else if (wind >= -operation && wind + dir < -operation)
	    use--;
	}
      //fprintf (stderr, "Process %3d,%-3d %3d,%-3d %d->%d %d %s\n", (int) s->ax, (int) s->ay, (int) s->bx, (int) s->by, wind, wind + dir, use, (s->ax == s->bx) ? "V" : "");
      if (s->bx > s->ax)	// don't count passing a vertical
	wind += dir;

      p = MALLOC (sizeof (*p));
      p->next = *yp;
      *yp = p;
      p->ax = s->ax;
      p->ay = s->ay;
      p->bx = s->bx;
      p->by = s->by;
      p->dir = s->dir;
      p->flag = s->flag;
      p->use = use;
      yp = &p->next;
      free (s);
    }
  paths_close (POLY_DIM_MAX);
  if (paths)
    {				// should not happen, but seems some bug still exists.
      while (paths)
	{			// close the paths as probably creates a sensible result
	  poly_vertex_t *v;
	  fprintf (stderr, "Unclosed path (bug)");
	  for (v = paths->a; v; v = v->next)
	    fprintf (stderr, " %3d,%-3d", (int) v->x, (int) v->y);
	  fprintf (stderr, "\n");
	  poly_contour_t *c = MALLOC (sizeof (*c));
	  c->next = new->contours;
	  new->contours = c;
	  c->vertices = paths->a;
	  path_t *p = paths;
	  paths = p->next;
	  free (p);
	}
      // errx (1, "Unclosed paths\n");
    }
  poly_tidy (new, 0);
  return new;
}

void
poly_test (void)
{
  polygon_t *i;
  void show (const char *name)
  {
    void list (polygon_t * p, char prefix)
    {
      if (!p || !p->contours)
	{
	  printf ("%c: -\n", prefix);
	  return;
	}
      poly_contour_t *c;
      for (c = p->contours; c; c = c->next)
	{
	  printf ("%c%c", prefix, c->dir ? c->dir < 0 ? '-' : '+' : ':');
	  poly_vertex_t *v;
	  for (v = c->vertices; v; v = v->next)
	    printf (" %3d%c%-3d", (int) v->x, v->flag ? 'x' : ',', (int) v->y);
	  printf ("\n");
	}
    }
    list (i, '-');
    polygon_t *o = poly_clip (POLY_UNION, 1, i);
    list (o, 'U');
    poly_free (o);
    o = poly_clip (POLY_INTERSECT, 1, i);
    list (o, 'I');
    poly_free (o);
    o = poly_clip (POLY_DIFFERENCE, 1, i);
    list (o, 'D');
    poly_free (o);
    o = poly_clip (POLY_XOR, 1, i);
    list (o, 'X');
    poly_free (o);
    poly_free (i);
  }
#define test(x) i=poly_new();{printf("Test %s\n",#x);int a;for(a=0;a<sizeof(x)/sizeof(*x);a++){poly_start(i);int*p=x[a];int f=((*p<0)?1:0);int b=abs(*p++);while(b--){poly_add(i,p[0],p[1],f);p+=2;}};show(#x);}
  int *overlap2[] = {
    (int[]) {
	     6, 0, 50, 100, 100, 200, 100, 250, 50, 200, 0, 100, 0}, (int[]) {
									      6, 0, 50, 100, 100, 200, 100, 150, 50, 200, 0, 100, 0}
  };
  int *boxc[] = {
    (int[]) {
	     4, 0, 0, 0, 100, 100, 100, 100, 0}
  };
  int *boxa[] = {
    (int[]) {
	     4, 0, 0, 100, 0, 100, 100, 0, 100}
  };
  int *box2[] = {
    (int[]) {
	     4, 0, 0, 0, 100, 100, 100, 100, 0}, (int[]) {
							  4, 10, 10, 10, 90, 90, 90, 90, 10}
  };
  int *hole[] = {
    (int[]) {
	     4, 0, 0, 0, 100, 100, 100, 100, 0}, (int[]) {
							  4, 10, 10, 90, 10, 90, 90, 10, 90}
  };
  int *hole2[] = {
    (int[]) {
	     4, 0, 100, 100, 200, 200, 100, 100, 0}, (int[]) {
							      4, 10, 100, 100, 10, 190, 100, 100, 190}
  };
  int *sidebyside[] = {
    (int[]) {
	     4, 0, 0, 0, 100, 100, 100, 100, 0}, (int[]) {
							  4, 100, 0, 100, 100, 200, 100, 200, 0}
  };
  int *figure8[] = {
    (int[]) {
	     8, 0, 0, 0, 100, 100, 100, 200, 0, 300, 0, 300, 100, 200, 100, 100, 0}
  };
  int *figure8box[] = {
    (int[]) {
	     8, 0, 0, 0, 100, 100, 100, 100, 0, 200, 0, 200, 100, 100, 100, 100, 0}
  };
  int *overlap[] = {
    (int[]) {
	     4, 0, 0, 0, 100, 100, 100, 100, 0}, (int[]) {
							  4, 0, 0, 0, 100, 100, 100, 100, 0}, (int[]) {
												       4, 0, 0, 0, 100, 100, 100, 100, 0}
  };
  int *cancel[] = {
    (int[]) {
	     4, 0, 0, 0, 100, 100, 100, 100, 0}, (int[]) {
							  4, 0, 0, 100, 0, 100, 100, 0, 100}
  };
  int *cshape1[] = {
    (int[]) {
	     4, 0, 0, 0, 100, 100, 100, 100, 0}, (int[]) {
							  4, 10, 10, 100, 10, 100, 90, 10, 90}
  };
  int *cshape2[] = {
    (int[]) {
	     4, 0, 0, 0, 100, 100, 100, 100, 0}, (int[]) {
							  4, 0, 10, 90, 10, 90, 90, 0, 90}
  };
  int *boxes[] = {
    (int[]) {
	     4, 0, 0, 0, 100, 100, 100, 100, 0}, (int[]) {
							  4, 10, 10, 10, 90, 90, 90, 90, 10},	//
    (int[]) {
	     4, 20, 20, 80, 20, 80, 80, 20, 80}, (int[]) {
							  4, 30, 30, 70, 30, 70, 70, 30, 70}
  };
  int *h[] = {
    (int[]) {
	     12, 0, 0, 0, 100, 10, 100, 10, 55, 90, 55, 90, 100, 100, 100, 100, 0, 90, 0, 90, 45, 10, 45, 10, 0}
  };
  int *x[] = {
    (int[]) {-4, 0, 10, 0, 90, 100, 90, 100, 10},
    (int[]) {4, 10, 0, 10, 100, 90, 100, 90, 0},
  };
  test (boxc);
  test (boxa);
  test (box2);
  test (hole);
  test (hole2);
  test (sidebyside);
  test (overlap);
  test (cancel);
  test (cshape1);
  test (figure8box);
  test (boxes);
  test (overlap2);
  test (figure8);
  test (cshape2);
  test (h);
  test (x);
}
