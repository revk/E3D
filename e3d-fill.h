// Extrude 3D model (e3d) Copyright ©2011 Adrian Kennard
// Make extrude path for each slice
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

void fill_perimeter (slice_t *, poly_dim_t width, int loops, int fast);	// create perimeter and remaining fill area
void fill_area (stl_t * stl, poly_dim_t width, int layers);	// Break down fill areas based on layers
void fill_extrude (stl_t * stl, poly_dim_t width, double density);	// Generate extrude path for fills
void fill_anchor (stl_t * stl, int loops, poly_dim_t width, poly_dim_t offset, poly_dim_t step);	// Add anchor to layer 0
