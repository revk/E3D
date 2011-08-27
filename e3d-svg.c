// Extrude 3D model (e3d) Copyright ©2011 Adrian Kennard
// Output SVG
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

#include "e3d-svg.h"

void
svg_out (const char *filename, stl_t * stl, poly_dim_t width)
{
  FILE *f = fopen (filename, "w");
  if (!f)
    err (1, "Cannot open %s for SVG", filename);
  fprintf (f, "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n");
  fprintf (f, "<svg xmlns=\"http://www.w3.org/2000/svg\" xmlns:inkscape=\"http://www.inkscape.org/namespaces/inkscape\" version=\"1.1\"");
  fprintf (f, " width=\"%s\"", dimout (stl->max.x));
  fprintf (f, " height=\"%s\">\n", dimout (stl->max.y));

  int count = 0;
  slice_t *s;
  for (s = stl->slices; s; s = s->next)
    {
      fprintf (f, "<g inkscape:label=\"%s\" inkscape:groupmode=\"layer\"%s>\n", dimout (s->z), count++ ? " style=\"display:none\"" : "");
      void outpath (polygon_t * p, const char *style, int dir)
      {
	if (!p)
	  return;
	fprintf (f, "<path style=\"%s\" d=\"", style);
	poly_contour_t *c;
	for (c = p->contours; c; c = c->next)
	  if (c->vertices && (!dir || c->dir == dir))
	    {
	      poly_vertex_t *v;
	      char t = 'M';
	      for (v = c->vertices; v; v = v->next)
		{
		  if (t)
		    {
		      fprintf (f, " %c", t);
		      if (t == 'M')
			t = 'L';
		      else
			t = 0;
		    }
		  fprintf (f, " %s", dimout (v->x));
		  fprintf (f, " %s", dimout (stl->max.y - v->y));
		}
	      fprintf (f, " Z");
	    }
	fprintf (f, "\"/>\n");
      }
      char temp[1000];
      outpath (s->outline, "fill:#ff8;stroke:none;fill-opacity:0.5", 0);
      outpath (s->solid, "fill:#f88;stroke:none;fill-opacity:0.5", 0);
      outpath (s->infill, "fill:#8ff;stroke:none;fill-opacity:0.5", 0);
      outpath (s->flying, "fill:#f8f;stroke:none;fill-opacity:0.5", 0);
      snprintf (temp, sizeof (temp), "fill:none;stroke:black;stroke-width:%s;stroke-linecap:round;stroke-linejoin:round;", dimout (width / 10));
      outpath (s->fill, temp, 0);
      int e;
      for (e = 0; e < EXTRUDE_PATHS; e++)
	{
	  snprintf (temp, sizeof (temp), "fill:none;stroke:#%X8f;stroke-width:%s;stroke-linecap:round;stroke-linejoin:round;stroke-opacity:0.5", e * 4,
		    dimout (width * 9 / 10));
	  outpath (s->extrude[e], temp, 1);
	  outpath (s->extrude[e], temp, -1);
	}
      if (s == stl->slices)
	{
	  snprintf (temp, sizeof (temp), "fill:none;stroke:#84f;stroke-width:%s;stroke-linecap:round;stroke-linejoin:round;stroke-opacity:0.5",
		    dimout (width * 9 / 10));
	  outpath (stl->anchor, temp, 0);
	  snprintf (temp, sizeof (temp), "fill:none;stroke:#8cf;stroke-width:%s;stroke-linecap:round;stroke-linejoin:round;stroke-opacity:0.5",
		    dimout (width * 9 / 10));
	  outpath (stl->anchorjoin, temp, 0);
	  snprintf (temp, sizeof (temp), "fill:none;stroke:green;stroke-width:%s;stroke-linecap:round;stroke-linejoin:round;", dimout (width / 10));
	  outpath (stl->border, temp, 1);
	}
      fprintf (f, "</g>\n");
    }

  fprintf (f, "</svg>\n");
  fclose (f);
}
