BIN=../bin/
LIB=../lib/
ALL=${BIN}e3d ${LIB}e3d-stl.o ${BIN}gcode-touchup

all: ${ALL}

ifeq ($(shell uname),Linux)
CCOPTS=-Wall -D_GNU_SOURCE -O -fPIC -g
OPTS= ${CCOPTS}
endif

ifeq ($(shell uname),Darwin)
CCOPTS= -fnested-functions
OPTS=-L/opt/local/lib -I/opt/local/include ${CCOPTS}
endif

${LIB}%.o: %.c e3d.h
	-indent $<
	cc -c -o $@ $< ${OPTS}

${BIN}%: %.c
	-indent $<
	cc -o $@ $< ${OPTS} -lpopt

${BIN}e3d: e3d.c ${LIB}e3d-common.o ${LIB}e3d-stl.o ${LIB}e3d-slice.o ${LIB}e3d-fill.o ${LIB}e3d-gcode.o ${LIB}e3d-svg.o ${LIB}poly.o
	-indent $<
	cc -o $@ $< ${OPTS} -lpopt -lm ${LIB}e3d-common.o ${LIB}e3d-stl.o ${LIB}e3d-slice.o ${LIB}e3d-fill.o ${LIB}e3d-gcode.o ${LIB}e3d-svg.o ${LIB}poly.o

