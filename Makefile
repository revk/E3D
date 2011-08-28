# Copyright ©2011 Adrian Kennard
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details. 
# 
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

BIN=./bin/
LIB=./lib/

ALL=${BIN}e3d

all: ${ALL}

clean:
	rm -rf ${BIN} ${LIB}

ifeq ($(shell uname),Linux)
CCOPTS=-Wall -D_GNU_SOURCE -O -fPIC -g
OPTS= ${CCOPTS}
endif

ifeq ($(shell uname),Darwin)
CCOPTS= -fnested-functions
OPTS=-L/opt/local/lib -I/opt/local/include -I/usr/include/malloc ${CCOPTS}
endif

${LIB}%.o: %.c e3d.h
	#-indent $<
	mkdir -p ${LIB}
	cc -c -o $@ $< ${OPTS}

${BIN}%: %.c
	#-indent $<
	mkdir -p ${BIN}
	cc -o $@ $< ${OPTS} -lpopt

${BIN}e3d: e3d.c ${LIB}e3d-common.o ${LIB}e3d-stl.o ${LIB}e3d-slice.o ${LIB}e3d-fill.o ${LIB}e3d-gcode.o ${LIB}e3d-svg.o ${LIB}poly.o
	#-indent $<
	mkdir -p ${BIN}
	cc -o $@ $< ${OPTS} -lpopt -lm ${LIB}e3d-common.o ${LIB}e3d-stl.o ${LIB}e3d-slice.o ${LIB}e3d-fill.o ${LIB}e3d-gcode.o ${LIB}e3d-svg.o ${LIB}poly.o

