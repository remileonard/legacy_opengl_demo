/*
	ripple.c
	Drew Olbrich, 1992

	This distortion effect approximates looking down at an
	image through a layer of water.  The user can poke
	at the water using the mouse, generating one or more
	ripple patterns.

	To create the effect, an large texture image is mapped
	onto a mesh of polygons.  Only the texture coordinates of
	the polygon vertices are distorted -- not the vertex
	coordinates.

	Port to OpenGL
	Nate Robins, 1997
*/

#include <stdio.h>
#include <math.h>
#include <GL/glut.h>

#include "defs.h"
#include "ripple.h"

extern RIPPLE_VECTOR ripple_vector[GRID_SIZE_X][GRID_SIZE_Y];
extern RIPPLE_AMP ripple_amp[RIPPLE_LENGTH];

extern int win_size_x, win_size_y;

EFFECT ripple = { ripple_init, ripple_dynamics, ripple_redraw, ripple_click };

static RIPPLE_VERTEX ripple_vertex[GRID_SIZE_X][GRID_SIZE_Y];

static int cx[RIPPLE_COUNT];
static int cy[RIPPLE_COUNT];
static int t[RIPPLE_COUNT];
static int max[RIPPLE_COUNT];

static int ripple_max;

/*
	Initialize ripple location and age information.

	Also, precompute the vertex coordinates and the default texture
	coordinates assigned to them.
*/

void ripple_init()
{
  int i, j;

  glDisable(GL_DEPTH_TEST);

  ripple_max = (int)sqrt(win_size_y*win_size_y+win_size_x*win_size_x);

  for (i = 0; i < RIPPLE_COUNT; i++)
  {
    t[i] = ripple_max + RIPPLE_LENGTH;
    cx[i] = 0;
    cy[i] = 0;
    max[i] = 0;
  }

  for (i = 0; i < GRID_SIZE_X; i++)
    for (j = 0; j < GRID_SIZE_Y; j++)
    {
      ripple_vertex[i][j].x[0] = i/(GRID_SIZE_X - 1.0)*win_size_x;
      ripple_vertex[i][j].x[1] = j/(GRID_SIZE_Y - 1.0)*win_size_y;
      ripple_vertex[i][j].dt[0] = i/(GRID_SIZE_X - 1.0);
      ripple_vertex[i][j].dt[1] = j/(GRID_SIZE_Y - 1.0);
    }
}

/*
	Advance one time step and compute new texture coordinates
	for the next frame of animation.
*/

void ripple_dynamics(int mousex, int mousey)
{
  int i, j, k;
  int x, y;
  int mi, mj;
  int r;
  float sx, sy;
  float amp;

  for (i = 0; i < RIPPLE_COUNT; i++)
    t[i] += RIPPLE_STEP;

  for (i = 0; i < GRID_SIZE_X; i++)
    for (j = 0; j < GRID_SIZE_Y; j++)
    {
      ripple_vertex[i][j].t[0] = ripple_vertex[i][j].dt[0];
      ripple_vertex[i][j].t[1] = ripple_vertex[i][j].dt[1];

      for (k = 0; k < RIPPLE_COUNT; k++)
      {
	x = i - cx[k];
	y = j - cy[k];
	if (x < 0)
	{
	  x *= -1;
	  sx = -1.0;
	}
	else
	  sx = 1.0;
	if (y < 0)
	{
	  y *= -1;
	  sy = -1.0;
	}
	else
	  sy = 1.0;
	mi = x;
	mj = y;
	
	r = t[k] - ripple_vector[mi][mj].r;
	
	if (r < 0)
	  r = 0;
	if (r > RIPPLE_LENGTH - 1)
	  r = RIPPLE_LENGTH - 1;

	amp = 1.0 - 1.0*t[k]/RIPPLE_LENGTH;
	amp *= amp;
	if (amp < 0.0)
	  amp = 0.0;
	
	ripple_vertex[i][j].t[0]
	  += ripple_vector[mi][mj].dx[0]*sx*ripple_amp[r].amplitude*amp;
	ripple_vertex[i][j].t[1]
	  += ripple_vector[mi][mj].dx[1]*sy*ripple_amp[r].amplitude*amp;
      }
    }
}

/*
	Draw the next frame of animation.
*/

void ripple_redraw()
{
  int i, j;

  glClear(GL_COLOR_BUFFER_BIT);

  for (i = 0; i < GRID_SIZE_X - 1; i++)
  {
    for (j = 0; j < GRID_SIZE_Y - 1; j++)
    {
      glBegin(GL_POLYGON);
      glTexCoord2fv(ripple_vertex[i][j].t);
      glVertex2fv(ripple_vertex[i][j].x);
      glTexCoord2fv(ripple_vertex[i][j + 1].t);
      glVertex2fv(ripple_vertex[i][j + 1].x);
      glTexCoord2fv(ripple_vertex[i + 1][j + 1].t);
      glVertex2fv(ripple_vertex[i + 1][j + 1].x);
      glTexCoord2fv(ripple_vertex[i + 1][j].t);
      glVertex2fv(ripple_vertex[i + 1][j].x);
      glEnd();
    }
  }

  glutSwapBuffers();
}

/*
	Calculate the distance between two points.
*/

float ripple_distance(int gx, int gy, int cx, int cy)
{
  return sqrt(1.0*(gx - cx)*(gx - cx) + 1.0*(gy - cy)*(gy - cy));
}

/*
	Compute the distance of the given window coordinate
	to the nearest window corner, in pixels.
*/

int ripple_max_distance(int gx, int gy)
{
  float d;
  float temp_d;

  d = ripple_distance(gx, gy, 0, 0);
  temp_d = ripple_distance(gx, gy, GRID_SIZE_X, 0);
  if (temp_d > d)
    d = temp_d;
  temp_d = ripple_distance(gx, gy, GRID_SIZE_X, GRID_SIZE_Y);
  if (temp_d > d)
    d = temp_d;
  temp_d = ripple_distance(gx, gy, 0, GRID_SIZE_Y);
  if (temp_d > d)
    d = temp_d;

  return (d/GRID_SIZE_X)*win_size_x + RIPPLE_LENGTH/6;
}

/*
	Generate a new ripple when the mouse is pressed.  There's
	a limit on the number of ripples that can be simultaneously
	generated.
*/

void ripple_click(int mousex, int mousey, int state)
{
  int index;

  if (state)
  {
    index = 0;
    while (t[index] < max[index] && index < RIPPLE_COUNT)
      index++;
    
    if (index < RIPPLE_COUNT)
    {
      cx[index] = 1.0*mousex/win_size_x*GRID_SIZE_X;
      cy[index] = 1.0*mousey/win_size_y*GRID_SIZE_Y;
      t[index] = 4*RIPPLE_STEP;
      max[index] = ripple_max_distance(cx[index], cy[index]);
    }
  }
}
