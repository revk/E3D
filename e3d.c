// Extrude 3D model (e3d) Copyright ©2011 Adrian Kennard
// Processes 3D STL files to make GCODE suitable for extrusion printers
//

#include <stdio.h>
#include <string.h>
#include <err.h>
#include <popt.h>
#include <malloc.h>
#include "e3d.h"
#include "e3d-stl.h"
#include "e3d-slice.h"
#include "e3d-fill.h"
#include "e3d-gcode.h"
#include "e3d-svg.h"

int debug = 0;

int places = 4;

int
main (int argc, const char *argv[])
{
  const char *configfile = NULL;
  const char *stlfile = NULL;
  const char *gcodefile = NULL;
  const char *svgfile = NULL;
  double layer = 0.4;
  double widthratio = 1.6;
  double startz = -1;
  double endz = -1;
  double tolerance = -1;
  double density = 0.2;
  int skins = 2;
  int skins0 = 1;
  int altskins = 0;
  int layers = 3;
  int test = 0;
  int anchorloops = 5;
  double anchorgap = 2;
  double anchorstep = 5;
  double anchorflow = 2;
  double filament = 2.9;
  double packing = 1;
  double speed = 50;
  double speed0 = 20;
  double zspeed = 2;
  double hop = 0.5;
  double back = 2;
  int mirror = 0;
  int fast = 0;
  int eplaces = 5;
  int tempbed = 0;
  int temp0 = 0;
  int temp = 0;
  int quiet = 0;

  char c;
  poptContext optCon;		// context for parsing command-line options
  const struct poptOption optionsTable[] = {
    {"config-file", 'c', POPT_ARG_STRING, &configfile, 0, "Config file", "filename"},
    {"stl", 'i', POPT_ARG_STRING, &stlfile, 0, "Input file", "filename.stl"},
    {"gcode", 'o', POPT_ARG_STRING, &gcodefile, 0, "Output file", "filename.gcode"},
    {"svg", 's', POPT_ARG_STRING, &svgfile, 0, "Output svg", "filename.svg"},
    {"layer-height", 'l', POPT_ARGFLAG_SHOW_DEFAULT | POPT_ARG_DOUBLE, &layer, 0, "Layer height", "Units"},
    {"width-ratio", 'w', POPT_ARGFLAG_SHOW_DEFAULT | POPT_ARG_DOUBLE, &widthratio, 0, "Layer width to height", "Ratio"},
    {"start-z", 'z', POPT_ARG_DOUBLE, &startz, 0, "Start Z (default half layer)", "Units"},
    {"end-z", 'e', POPT_ARG_DOUBLE, &endz, 0, "End Z (default top)", "Units"},
    {"e-places", 0, POPT_ARGFLAG_SHOW_DEFAULT | POPT_ARG_INT, &eplaces, 0, "Number of decimal places in output for extrude", "N"},
    {"skins", 'k', POPT_ARGFLAG_SHOW_DEFAULT | POPT_ARG_INT, &skins, 0, "Number of skins (perimeter loops)", "N"},
    {"skins0", 0, POPT_ARGFLAG_SHOW_DEFAULT | POPT_ARG_INT, &skins0, 0, "Number of skins on layer 0", "N"},
    {"alt-skins", 0, POPT_ARGFLAG_SHOW_DEFAULT | POPT_ARG_INT, &altskins, 0, "Extra skins on alt layers", "N"},
    {"layers", 'L', POPT_ARGFLAG_SHOW_DEFAULT | POPT_ARG_INT, &layers, 0, "Number of solid layers", "N"},
    {"fill-density", 0, POPT_ARGFLAG_SHOW_DEFAULT | POPT_ARG_DOUBLE, &density, 0, "Fill density for non solid layers", "0-1"},
    {"anchor", 'A', POPT_ARGFLAG_SHOW_DEFAULT | POPT_ARG_INT, &anchorloops, 0, "Layer 0 anchor loops around perimeter", "N"},
    {"anchor-gap", 0, POPT_ARGFLAG_SHOW_DEFAULT | POPT_ARG_DOUBLE, &anchorgap, 0, "Gap between perimeter and anchor in widths", "Widths"},
    {"anchor-step", 0, POPT_ARGFLAG_SHOW_DEFAULT | POPT_ARG_DOUBLE, &anchorstep, 0, "Spacing of joins between perimeter and anchor in widths", "Widths"},
    {"anchor-flow", 0, POPT_ARGFLAG_SHOW_DEFAULT | POPT_ARG_DOUBLE, &anchorflow, 0, "Extrude multiplier for anchor join loop", "Ratio"},
    {"filament", 'f', POPT_ARGFLAG_SHOW_DEFAULT | POPT_ARG_DOUBLE, &filament, 0, "Filament diameter", "Units"},
    {"packing", 0, POPT_ARGFLAG_SHOW_DEFAULT | POPT_ARG_DOUBLE, &packing, 0, "Multiplier for feed rate", "Ratio"},
    {"speed", 'S', POPT_ARGFLAG_SHOW_DEFAULT | POPT_ARG_DOUBLE, &speed, 0, "Speed", "Units/sec"},
    {"speed0", 0, POPT_ARGFLAG_SHOW_DEFAULT | POPT_ARG_DOUBLE, &speed0, 0, "Speed (layer0)", "Units/sec"},
    {"z-speed", 0, POPT_ARGFLAG_SHOW_DEFAULT | POPT_ARG_DOUBLE, &zspeed, 0, "Max Z Speed", "Units/sec"},
    {"temp0", 0, POPT_ARG_INT, &temp0, 0, "Set layer 0 temp (M109)", "C"},
    {"temp", 0, POPT_ARG_INT, &temp, 0, "Set temp (M109)", "C"},
    {"bed", 0, POPT_ARG_INT, &tempbed, 0, "Set temp of bed (M140)", "C"},
    {"hop", 0, POPT_ARGFLAG_SHOW_DEFAULT | POPT_ARG_DOUBLE, &hop, 0, "Hop up when moving and not extruding", "Units"},
    {"back", 0, POPT_ARGFLAG_SHOW_DEFAULT | POPT_ARG_DOUBLE, &back, 0, "Pull back extrude when not extruding", "Units"},
    {"fast", 0, POPT_ARG_NONE, &fast, 0, "Fast print of infill by reduced precision", 0},
    {"mirror", 'm', POPT_ARG_NONE, &mirror, 0, "Mirror image GCODE output", 0},
    {"debug", 'v', POPT_ARG_NONE, &debug, 0, "Debug", 0},
    {"quiet", 'q', POPT_ARG_NONE, &quiet, 0, "Quiet (don't print timings, etc)", 0},
    {"test", 0, POPT_ARG_NONE, &test, 0, "Poly library tests", 0},
    POPT_AUTOHELP {NULL, 0, 0, NULL, 0}
  };

  optCon = poptGetContext (NULL, argc, argv, optionsTable, 0);
  poptSetOtherOptionHelp (optCon, "");

  if ((c = poptGetNextOpt (optCon)) < -1)
    errx (1, "%s: %s\n", poptBadOption (optCon, POPT_BADOPTION_NOALIAS), poptStrerror (c));


  if (test)
    {
#if 0
      long double ab, cd, s;
      poly_dim_t x, y, pc2;
      poly_intersect_point (16051, 4677, 17963, 4164, 16051, 4677, &x, &y, &ab, &pc2, &s);
      fprintf (stderr, "X=%d Y=%d ab=%Lf pc2=%d s=%Lf pc=%Lf\n", (int) x, (int) y, ab, (int) pc2, s, sqrtl (pc2));
      poly_intersect_point (16051, 4677, 17963, 4164, 17565, 4271, &x, &y, &ab, &pc2, &s);
      fprintf (stderr, "X=%d Y=%d ab=%Lf pc2=%d s=%Lf pc=%Lf\n", (int) x, (int) y, ab, (int) pc2, s, sqrtl (pc2));
      poly_intersect_point (16051, 4677, 17565, 4271, 16051, 4677, &x, &y, &ab, &pc2, &s);
      fprintf (stderr, "X=%d Y=%d ab=%Lf pc2=%d s=%Lf pc=%Lf\n", (int) x, (int) y, ab, (int) pc2, s, sqrtl (pc2));
      poly_intersect_point (16051, 4677, 17565, 4271, 17963, 4164, &x, &y, &ab, &pc2, &s);
      fprintf (stderr, "X=%d Y=%d ab=%Lf pc2=%d s=%Lf pc=%Lf\n", (int) x, (int) y, ab, (int) pc2, s, sqrtl (pc2));
      poly_intersect_line (16051, 4677, 17963, 4164, 17480, 3952, 17565, 4271, &x, &y, &ab, &cd);
      fprintf (stderr, "X=%d Y=%d ab=%Lf cd=%Lf\n", (int) x, (int) y, ab, cd);
      poly_intersect_line (16051, 4677, 17565, 4271, 17480, 3952, 17565, 4271, &x, &y, &ab, &cd);
      fprintf (stderr, "X=%d Y=%d ab=%Lf cd=%Lf\n", (int) x, (int) y, ab, cd);
      poly_intersect_line (0, -13575, 0, -13574, 0, -13574, 30445, -905, &x, &y, &ab, &cd);
      fprintf (stderr, "X=%d Y=%d ab=%Lf cd=%Lf\n", (int) x, (int) y, ab, cd);
      return 0;
#endif
      poly_test ();
      return 0;
    }

  if (configfile && poptReadConfigFile (optCon, configfile))
    errx (1, "Cannot read config file %s", configfile);

  if (!stlfile && poptPeekArg (optCon))
    stlfile = poptGetArg (optCon);
  if (!gcodefile && poptPeekArg (optCon))
    gcodefile = poptGetArg (optCon);

  if (poptPeekArg (optCon) || !stlfile)
    {
      poptPrintUsage (optCon, stderr, 0);
      return -1;
    }

#ifdef	FIXED
  {
    int d;
    fixed = 1;
    for (d = 0; d < FIXED; d++)
      fixed *= 10LL;
    fixplaces = 1;
    if (places >= FIXED)
      places = FIXED;
    else
      for (d = places; d < FIXED; d++)
	fixplaces *= 10LL;
  }
#endif

  // Process steps
  stl_t *stl = stl_read (stlfile);	// Read in file
  if (!stl)
    errx (1, "Cannot read %s", stlfile);

  stl_origin (stl);		// Origin file at X/Y/Z zero

  poly_dim_t sz = d2dim (startz);
  poly_dim_t l = d2dim (layer);
  poly_dim_t tol = d2dim (tolerance);
  poly_dim_t ez = d2dim (endz);

  if (ez < 0)
    ez = stl->max.z;
  if (sz < 0)
    sz = l / 2;
  if (sz < stl->min.z)
    sz = stl->min.z;
  if (ez > stl->max.z)
    ez = stl->max.z;
  poly_dim_t width = l * widthratio;

  {				// Slice the STL
    if (tol < 0)
      tol = layer;
    slice_t **last = &stl->slices;
    poly_dim_t z;
    for (z = sz; z <= ez; z += l)
      {
	slice_t *this = slice (stl, z, tol);
	if (this)
	  {
	    *last = this;
	    last = &this->next;
	  }
      }
  }

  {				// Fill
    int count = 1;
    slice_t *s = stl->slices;
    fill_perimeter (s, width, skins0, 0);
    s = s->next;
    for (; s; s = s->next)
      fill_perimeter (s, width, skins + (((count++) & 1) ? altskins : 0), fast);
    fill_area (stl, width, layers);
    fill_extrude (stl, width, density);
  }

  if (anchorloops)
    fill_anchor (stl, anchorloops, width, width * anchorgap, width * anchorstep);

  if (!stl->anchor)
    {				// No anchor
      polygon_t *q = poly_inset (stl->border, -width);
      poly_free (stl->border);
      stl->border = q;
    }
  poly_tidy (stl->border, width);	// faster

  poly_dim_t zspeeds = d2dim (zspeed);
  poly_dim_t speed0s = d2dim (speed0);
  poly_dim_t speeds = d2dim (speed);
  poly_dim_t hops = d2dim (hop);

  // GCODE output
  if (gcodefile)
    {
      unsigned int t =
	gcode_out (gcodefile, stl, layer * layer * widthratio / filament / filament * packing, l, speed0s, speeds, zspeeds, back, hops, mirror, anchorflow,
		   eplaces, tempbed, temp0, temp, quiet);
      if (!quiet)
	{
	  if (tempbed)
	    printf ("Bed temperature %dC\n", tempbed);
	  if (temp0 && temp0 != temp)
	    printf ("Initial extrude temperature %dC\n", temp0);
	  if (temp && temp0 != temp)
	    printf ("Ongoing extrude temperature %dC\n", temp);
	  printf ("Time estimate %d:%02d:%02d\n", t / 3600, t / 60 % 60, t % 60);
	}
    }

  // SVG output
  if (svgfile)
    svg_out (svgfile, stl, width);

  poptFreeContext (optCon);

  return 0;
}
