// Extrude 3D model (e3d) Copyright ©2011 Adrian Kennard
// Output GCODE

#include <stdio.h>
#include <string.h>
#include <err.h>
#include <ctype.h>

#include "e3d-gcode.h"

unsigned int
gcode_out (const char *filename, stl_t * stl, double flowrate, poly_dim_t layer, poly_dim_t speed0, poly_dim_t speed, poly_dim_t zspeed, double back,
	   poly_dim_t hop, int mirror, double anchorflow, int eplaces)
{				// returns time estimate in seconds
  FILE *o = fopen (filename, "w");
  if (!o)
    err (1, "Cannot open %s for SVG", filename);
  poly_dim_t cx = (stl->min.x + stl->max.x) / 2;
  poly_dim_t cy = (stl->min.y + stl->max.y) / 2;

  // pre
  fprintf (o,			//
	   "G21             ; metric\n"	//
	   "G90             ; absolute\n"	//
	   "G92 Z0 E0       ; reset Z and E \n"	//
	   "M106            ; fan on\n"	//
	   "G1 Z2 F60	    ; up\n"	//  avoid issues with end stop
	   "G1 Z0.1	    ; down\n"	//
	   "G92 Z0	    ; origin\n"	//
    );
  fprintf (o, "G92");		// origin starts assuming in middle of print
  fprintf (o, " X%s", dimout (cx));
  fprintf (o, " Y%s", dimout (cy));
  fprintf (o, "\n");
  long long t = 0;
  void g1 (poly_dim_t x, poly_dim_t y, poly_dim_t z, long double e, poly_dim_t f)
  {
    static long double le = 0;
    static poly_dim_t lx = 0, ly = 0, lz = 0, lf = 0;
    if (mirror)
      x = cx * 2 - x;
    if (x == lx && y == ly && z == lz && e == le && f == lf)
      return;
    if (z != lz && zspeed)
      {				// check max speed
	poly_dim_t d = sqrtl ((x - lx) * (x - lx) + (y - ly) * (y - ly) + (z - lz) * (z - lz) + (d2dim (e) - d2dim (le)) * (d2dim (e) - d2dim (le)));
	poly_dim_t dz = lz - z;
	if (dz < 0)
	  dz = 0 - dz;
	if (d * zspeed < dz * f)
	  f = d * zspeed / dz;
      }
    fprintf (o, "G1");
    if (x != lx)
      fprintf (o, " X%s", dimout (x));
    if (y != ly)
      fprintf (o, " Y%s", dimout (y));
    if (z != lz)
      fprintf (o, " Z%s", dimout (z));
    if (e != le)
      fprintf (o, " E%.*Lf", eplaces, e);
    if (f != lf)
      fprintf (o, " F%s", dimout (f * 60));	// feeds are per minute
    poly_dim_t d = sqrtl ((x - lx) * (x - lx) + (y - ly) * (y - ly) + (z - lz) * (z - lz) + (d2dim (e) - d2dim (le)) * (d2dim (e) - d2dim (le)));
    if (d && f)
      t += d * 1000000LL / f;
    lx = x;
    ly = y;
    lz = z;
    le = e;
    lf = f;
    fprintf (o, "\n");
  }
  poly_dim_t px = 0, py = 0;
  long double pe = 0;
  void move (poly_dim_t x, poly_dim_t y, poly_dim_t z, double back)
  {
    g1 (px = x, py = y, z, pe - back, speed);
  }
  void extrude (poly_dim_t x, poly_dim_t y, poly_dim_t z, poly_dim_t speed, double flowrate)
  {
    poly_dim_t d = sqrtl ((x - px) * (x - px) + (y - py) * (y - py));
    g1 (px = x, py = y, z, pe = pe + (dim2d (d) * flowrate), speed);
  }
  poly_dim_t z = 0;
  void plot_loops (polygon_t * p, poly_dim_t speed, double flowrate, int dir)
  {
    if (!p)
      return;
    poly_contour_t *c;
    for (c = p->contours; c; c = c->next)
      if (c->vertices && (!dir || dir == c->dir))
	{
	  poly_vertex_t *v = c->vertices;
	  poly_dim_t d = sqrtl ((px - v->x) * (px - v->x) + (py - v->y) * (py - v->y));
	  if (pe && d > layer * 5)
	    {			// hop and pull back extruder while moving
	      move (px, py, z + hop, back);
	      move (v->x, v->y, z + hop, back);
	    }
	  move (v->x, v->y, z, 0);
	  for (v = c->vertices->next; v; v = v->next)
	    extrude (v->x, v->y, z, speed, flowrate);
	  v = c->vertices;
	  extrude (v->x, v->y, z, speed, flowrate);
	}
  }
  // layers
  slice_t *s;
  s = stl->slices;
  plot_loops (stl->border, speed, stl->anchor ? 0 : flowrate, 1);	// Ensures end-stops hit if no space, and if no anchor then ensures extrusion working
  plot_loops (stl->anchor, speed0, flowrate * anchorflow, 1);
  plot_loops (stl->anchor, speed0, flowrate * anchorflow, -1);
  poly_dim_t sp = speed0;
  while (s)
    {
      int e;
      plot_loops (s->extrude[0], sp, flowrate, 1);
      plot_loops (s->extrude[0], sp, flowrate, -1);
      for (e = 1; e < EXTRUDE_PATHS - 1; e++)
	{
	  poly_dim_t x = px, y = py;
	  poly_order (s->extrude[e], &x, &y);
	  plot_loops (s->extrude[e], sp, flowrate, 0);
	}
      plot_loops (s->extrude[e], speed0, flowrate, -1);	// flying layer - in order it was made
      plot_loops (s->extrude[e], speed0, flowrate, 1);	// flying layer - in order it was made
      z += layer;
      s = s->next;
      sp = speed;
    }
  move (px, py, z + hop, back);
  move (cx, cy, z + hop, back);
  move (cx, cy, z + layer * 10, back);
  move (cx, cy, z + layer * 20, 0);
  // post
  fprintf (o,			//
	   "M108 S0         ; Cold hot end\n"	//
	   "M140 S0         ; Cold bed\n"	//
	   "M084            ; Disable steppers\n"	//
	   "M107            ; fan off\n"	//
    );
  fclose (o);
  return t / 1000000LL;
}
