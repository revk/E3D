// Extrude 3D model (e3d) Copyright ©2011 Adrian Kennard
// Output GCODE
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

#include "e3d.h"

unsigned int gcode_out (const char *filename, stl_t * stl, double feedrate, poly_dim_t layer, poly_dim_t speed0, poly_dim_t speed, poly_dim_t zspeed,
			double back, poly_dim_t hop, int mirror, double anchorflow, double fillflow,int eplaces,int tempbed,int temp0,int temp,int quiet);
