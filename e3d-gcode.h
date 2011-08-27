// Extrude 3D model (e3d) Copyright ©2011 Adrian Kennard
// Output GCODE
// Released under GPL 3.0 http://www.gnu.org/copyleft/gpl.html

#include "e3d.h"

unsigned int gcode_out (const char *filename, stl_t * stl, double feedrate, poly_dim_t layer, poly_dim_t speed0, poly_dim_t speed, poly_dim_t zspeed,
			double back, poly_dim_t hop, int mirror, double anchorflow, int eplaces,int tempbed,int temp0,int temp,int quiet);
