/*
	rubber.c
	Drew Olbrich, 1992

	Port to OpenGL
	Nate Robins, 1997
*/

#include <stdio.h>
#include <math.h>
#include <GL/glut.h>

#include "defs.h"
#include "rubber.h"

extern int win_size_x, win_size_y;

EFFECT rubber = { rubber_init, rubber_dynamics, rubber_redraw, rubber_click };

#define SPRING_KS  0.3
#define DRAG	   0.5

typedef struct {
  float x[3];
  float v[3];
  float t[2];
  int nail;
} MASS;

typedef struct {
  int i, j;
  float r;
} SPRING;

static int spring_count;

static MASS *mass = NULL;
static SPRING *spring = NULL;

static int grab = -1;	/* index of grabbed mass point */

void rubber_init()
{
  int i, j;
  int k;
  int m;

  glEnable(GL_DEPTH_TEST);

  if (mass == NULL)
  {
    mass = (MASS *) malloc(sizeof(MASS)*GRID_SIZE_X*GRID_SIZE_Y);
    if (mass == NULL)
    {
      fprintf(stderr, "rubber: Can't allocate memory.\n");	
      exit(-1);
    }
  }

  k = 0;
  for (i = 0; i < GRID_SIZE_X; i++)
    for (j = 0; j < GRID_SIZE_Y; j++)
    {
      mass[k].nail = (i == 0 || j == 0 || i == GRID_SIZE_X - 1
		      || j == GRID_SIZE_Y - 1);
      mass[k].x[0] = i/(GRID_SIZE_X - 1.0)*win_size_x;
      mass[k].x[1] = j/(GRID_SIZE_Y - 1.0)*win_size_y;
      mass[k].x[2] = -(CLIP_FAR - CLIP_NEAR)/2.0;

      mass[k].v[0] = 0.0;
      mass[k].v[1] = 0.0;
      mass[k].v[2] = 0.0;

      mass[k].t[0] = i/(GRID_SIZE_X - 1.0);
      mass[k].t[1] = j/(GRID_SIZE_Y - 1.0);

      k++;
    }

  if (spring == NULL)
  {
    spring_count = (GRID_SIZE_X - 1)*(GRID_SIZE_Y - 2)
      + (GRID_SIZE_Y - 1)*(GRID_SIZE_X - 2);
    
    spring = (SPRING *) malloc(sizeof(SPRING)*spring_count);
    if (spring == NULL)
    {
      fprintf(stderr, "rubber: Can't allocate memory.\n");	
      exit(-1);
    }
  }

  k = 0;
  for (i = 1; i < GRID_SIZE_X - 1; i++)
    for (j = 0; j < GRID_SIZE_Y - 1; j++)
    {
      m = GRID_SIZE_Y*i + j;
      spring[k].i = m;
      spring[k].j = m + 1;
      spring[k].r = (win_size_y - 1.0)/(GRID_SIZE_Y - 1.0);
      k++;
    }

  for (j = 1; j < GRID_SIZE_Y - 1; j++)
    for (i = 0; i < GRID_SIZE_X - 1; i++)
    {
      m = GRID_SIZE_Y*i + j;
      spring[k].i = m;
      spring[k].j = m + GRID_SIZE_X;
      spring[k].r = (win_size_x - 1.0)/(GRID_SIZE_X - 1.0);
      k++;
    }
}

/*
	Do the dynamics simulation for the next frame.
*/

void rubber_dynamics(int mousex, int mousey)
{
  int k;
  float d[3];
  int i, j;
  float l;
  float a;

  /* calculate all the spring forces acting on the mass points */

  for (k = 0; k < spring_count; k++)
  {
    i = spring[k].i;
    j = spring[k].j;

    d[0] = mass[i].x[0] - mass[j].x[0];
    d[1] = mass[i].x[1] - mass[j].x[1];
    d[2] = mass[i].x[2] - mass[j].x[2];

    l = sqrt(d[0]*d[0] + d[1]*d[1] + d[2]*d[2]);

    if (l != 0.0)
    {
      d[0] /= l;
      d[1] /= l;
      d[2] /= l;

      a = l - spring[k].r;

      mass[i].v[0] -= d[0]*a*SPRING_KS;
      mass[i].v[1] -= d[1]*a*SPRING_KS;
      mass[i].v[2] -= d[2]*a*SPRING_KS;

      mass[j].v[0] += d[0]*a*SPRING_KS;
      mass[j].v[1] += d[1]*a*SPRING_KS;
      mass[j].v[2] += d[2]*a*SPRING_KS;
    }
  }

  /* update the state of the mass points */

  for (k = 0; k < GRID_SIZE_X*GRID_SIZE_Y; k++)
    if (!mass[k].nail)
    {
      mass[k].x[0] += mass[k].v[0];
      mass[k].x[1] += mass[k].v[1];
      mass[k].x[2] += mass[k].v[2];
      
      mass[k].v[0] *= (1.0 - DRAG);
      mass[k].v[1] *= (1.0 - DRAG);
      mass[k].v[2] *= (1.0 - DRAG);

      if (mass[k].x[2] > -CLIP_NEAR - 0.01)
	mass[k].x[2] = -CLIP_NEAR - 0.01;
      if (mass[k].x[2] < -CLIP_FAR + 0.01)
	mass[k].x[2] = -CLIP_FAR + 0.01;
    }

  /* if a mass point is grabbed, attach it to the mouse */

  if (grab != -1 && !mass[grab].nail)
  {
    mass[grab].x[0] = mousex;
    mass[grab].x[1] = mousey;
    mass[grab].x[2] = -(CLIP_FAR - CLIP_NEAR)/4.0;
  }
}

/*
	Draw the next frame of animation.
*/

void rubber_redraw()
{
  int k;
  int i, j;

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

#define _WIREFRAME
#ifdef WIREFRAME
  for (k = 0; k < spring_count; k++)
  {
    glBegin(GL_LINES);
    glVertex3fv(mass[spring[k].i].x);
    glVertex3fv(mass[spring[k].j].x);
    glEnd();
  }
#else
  k = 0;
  for (i = 0; i < GRID_SIZE_X - 1; i++)
  {
    for (j = 0; j < GRID_SIZE_Y - 1; j++)
    {
      glBegin(GL_POLYGON);
      glTexCoord2fv(mass[k].t);
      glVertex3fv(mass[k].x);
      glTexCoord2fv(mass[k + 1].t);
      glVertex3fv(mass[k + 1].x);
      glTexCoord2fv(mass[k + GRID_SIZE_Y + 1].t);
      glVertex3fv(mass[k + GRID_SIZE_Y + 1].x);
      glTexCoord2fv(mass[k + GRID_SIZE_Y].t);
      glVertex3fv(mass[k + GRID_SIZE_Y].x);
      glEnd();
      k++;
    }
    k++;
  }
#endif

  glutSwapBuffers();
}

/*
	Return the index of the mass point that's nearest to the
	given screen coordinate.
*/

int rubber_grab(int x, int y)
{
  float dx[2];
  float d;
  float min_d;
  float min_i;
  int i;

  for (i = 0; i < GRID_SIZE_X*GRID_SIZE_Y; i++)
  {
    dx[0] = mass[i].x[0] - x;
    dx[1] = mass[i].x[1] - y;
    d = sqrt(dx[0]*dx[0] + dx[1]*dx[1]);
    if (i == 0 || d < min_d)
    {
      min_i = i;
      min_d = d;
    }
  }

  return min_i;
}

/*
	If the mouse is pressed down, grab the nearest mass point.
*/

void rubber_click(int mousex, int mousey, int state)
{
  if (state)
    grab = rubber_grab(mousex, mousey);
  else
    grab = -1;
}
