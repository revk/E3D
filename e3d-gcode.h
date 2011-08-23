// Extrude 3D model (e3d) Copyright ©2011 Adrian Kennard
// Output GCODE

#include "e3d.h"

unsigned int gcode_out (const char *filename, stl_t * stl, double feedrate, poly_dim_t layer, poly_dim_t speed0,poly_dim_t speed, poly_dim_t zspeed,poly_dim_t back, poly_dim_t hop, int mirror,double anchorflow);
