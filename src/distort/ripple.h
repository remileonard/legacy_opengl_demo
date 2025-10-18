/*
	ripple.h
	Drew Olbrich, 1992
*/

#ifndef _RIPPLE
#define _RIPPLE

#include "defs.h"

#define RIPPLE_LENGTH     2048
#define RIPPLE_CYCLES     18
#define RIPPLE_AMPLITUDE  0.125
#define RIPPLE_STEP	  7
#define RIPPLE_COUNT	  7

typedef struct {	/* precomputed displacement vector table */
  float dx[2];
  int r;		/* distance from origin, in pixels */
} RIPPLE_VECTOR;

typedef struct {	/* precomputed ripple amplitude table */
  float amplitude;
} RIPPLE_AMP;

typedef struct {
  float x[2];		/* initial vertex location */
  float t[2];		/* texture coordinate */
  float dt[2];		/* default texture coordinate */
} RIPPLE_VERTEX;

void ripple_init();
void ripple_dynamics(int mousex, int mousey);
void ripple_redraw();
void ripple_click(int mousex, int mousey, int state);

#endif
