/*
	distort.c
	Drew Olbrich, 1992

	This program demonstrates how texture mapping can be
	used for interactive image distortion effects.

	In this demo, an arbitrarily-sized image is mapped onto
	a large array of texture mapped polygons.  The pop-up menu
	can be used to choose between different kinds of distortion.

	Port to OpenGL
	Nate Robins, 1997
*/

#include <stdio.h>
#include <stdlib.h>
#include <GL/glut.h>

#include "texture.h"
#include "defs.h"

static EFFECT *effect = &DEFAULT_EFFECT;

int mousex, mousey;     /* current mouse values */
int win_size_x = WIN_SIZE_X, win_size_y = WIN_SIZE_Y;


/*
	Load the image to distort, make a texture out of it, and enable 
	texturing.
 */

void image_init(char *fn)
{
  unsigned int *buf;
  int width, height, depth;

  buf = read_texture(fn, &width, &height, &depth);
  if (buf == NULL)
  {
    fprintf(stderr, "distort: Can't load image file \"%s\".\n", fn);
    exit(-1);
  }

  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexImage2D(GL_TEXTURE_2D, 0, depth, width, height, 
	       0, GL_RGBA, GL_UNSIGNED_BYTE, buf);
  glEnable(GL_TEXTURE_2D);
  
  free(buf);
}


/*
	Escape at all costs.
 */

void keyboard(unsigned char key, int x, int y)
{
  if (key == 27)
    exit(0);
}

/*
	Respond to the mouse events.
 */

void mouse(int button, int state, int x, int y)
{
  mousex = x;
  mousey = (glutGet(GLUT_WINDOW_HEIGHT) - y);

  if (button == GLUT_LEFT_BUTTON)
    effect->click(mousex, mousey, state == GLUT_DOWN);
}

/*
	Follow the mouse around.
 */

void motion(int x, int y)
{
  mousex = x;
  mousey = (glutGet(GLUT_WINDOW_HEIGHT) - y);
}

/*
	This function handles the menu actions.
 */

void menu(int item)
{
  switch (item)
  {
  case 1 :
    effect = &ripple;
    effect->init();
    break;
  case 2 :
    effect = &rubber;
    effect->init();
    break;
  case 3 :
    exit(0);
    break;
  }
}

/*
	Allow the user to reshape the window, we can adapt.
 */

void reshape(int width, int height)
{
  if (width != height) {
    glutReshapeWindow(width > height ? height : width,
		      width > height ? height : width);
    return;
  }

  glViewport(0, 0, width, height);

  win_size_x = width;
  win_size_y = height;
  
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(-0.5, height + 0.5, -0.5, width + 0.5, CLIP_NEAR, CLIP_FAR);
  
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  effect->init();
}

/*
	Show the world our stuff.
 */

void display(void)
{
  effect->redraw();
}

/*
	Spend idle time drawing dynamics.
 */

void idle(void)
{
  effect->dynamics(mousex, mousey);
  glutPostRedisplay();
}

/*
	Open and initialize an OpenGL window.
 */

void display_init()
{
  glutInitWindowSize(WIN_SIZE_X, WIN_SIZE_Y);
  glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE);

  glutCreateWindow(WIN_TITLE);
  glutSetIconTitle(ICON_TITLE);
  
  glViewport(0, 0, WIN_SIZE_X, WIN_SIZE_Y);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(-0.5, WIN_SIZE_X - 0.5, -0.5, WIN_SIZE_Y - 0.5, CLIP_NEAR, CLIP_FAR);

  glutReshapeFunc(reshape);
  glutDisplayFunc(display);
  glutMouseFunc(mouse);
  glutMotionFunc(motion);
  glutKeyboardFunc(keyboard);
  glutIdleFunc(idle);

  glutCreateMenu(menu);
  glutAddMenuEntry("Distortion", 0);
  glutAddMenuEntry("", 0);
  glutAddMenuEntry("Ripple", 1);
  glutAddMenuEntry("Rubber", 2);
  glutAddMenuEntry("", 0);
  glutAddMenuEntry("Quit", 3);
  glutAttachMenu(GLUT_RIGHT_BUTTON);
}

/*
	If a command line argument is provided, it is interpreted
	as the name of an alternate source image.
 */

void main(int argc, char **argv)
{
  glutInit(&argc, argv);

  display_init();

  if (argc == 1)
    image_init(DEFAULT_IMAGE_FN);
  else
    image_init(argv[1]);

  glutMainLoop();
}
