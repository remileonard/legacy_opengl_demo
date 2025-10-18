/* bounce.c */

/*
 * Bouncing ball demo.
 *
 * This program is in the public domain
 *
 * Brian Paul
 */

/* Conversion to GLUT by Mark J. Kilgard */

#include <math.h>
#include <stdlib.h>
#include <GL/glut.h>

#define COS(X) cos((X) * 3.14159 / 180.0)
#define SIN(X) sin((X) * 3.14159 / 180.0)

#define RED 1
#define WHITE 2
#define CYAN 3

GLuint Ball;
GLenum Mode;
GLfloat Zrot = 0.0, Zstep = 6.0;
GLfloat Xpos = 0.0, Ypos = 1.0;
GLfloat Xvel = 0.2, Yvel = 0.0;
GLfloat Xmin = -4.0, Xmax = 4.0;
GLfloat Ymin = -3.8, Ymax = 4.0;
GLfloat G = -0.1;

static GLuint
make_ball(void)
{
  GLuint list;
  GLfloat a, b;
  GLfloat da = 18.0, db = 18.0;
  GLfloat radius = 1.0;
  GLuint color;
  GLfloat x, y, z;

  list = glGenLists(1);

  glNewList(list, GL_COMPILE);

  color = 0;
  for (a = -90.0; a + da <= 90.0; a += da)
  {

    glBegin(GL_QUAD_STRIP);
    for (b = 0.0; b <= 360.0; b += db)
    {

      if (color)
      {
        glIndexi(RED);
      }
      else
      {
        glIndexi(WHITE);
      }

      x = COS(b) * COS(a);
      y = SIN(b) * COS(a);
      z = SIN(a);
      glVertex3f(x, y, z);

      x = radius * COS(b) * COS(a + da);
      y = radius * SIN(b) * COS(a + da);
      z = radius * SIN(a + da);
      glVertex3f(x, y, z);

      color = 1 - color;
    }
    glEnd();
  }

  glEndList();

  return list;
}

static void
reshape(int width, int height)
{
  glViewport(0, 0, (GLint)width, (GLint)height);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(-6.0, 6.0, -6.0, 6.0, -6.0, 6.0);
  glMatrixMode(GL_MODELVIEW);
}

/* ARGSUSED1 */
static void
key(unsigned char k, int x, int y)
{
  switch (k)
  {
  case 27: /* Escape */
    exit(0);
  }
}

static void
draw(void)
{
  GLint i;
  glClear(GL_COLOR_BUFFER_BIT);

  glIndexi(CYAN);
  glBegin(GL_LINES);
  for (i = -5; i <= 5; i++)
  {
    glVertex2i(i, -5);
    glVertex2i(i, 5);
  }
  for (i = -5; i <= 5; i++)
  {
    glVertex2i(-5, i);
    glVertex2i(5, i);
  }
  for (i = -5; i <= 5; i++)
  {
    glVertex2i(i, -5);
    glVertex2f(i * 1.15, -5.9);
  }
  glVertex2f(-5.3, -5.35);
  glVertex2f(5.3, -5.35);
  glVertex2f(-5.75, -5.9);
  glVertex2f(5.75, -5.9);
  glEnd();

  glPushMatrix();
  glTranslatef(Xpos + 0.6, Ypos + 0.2, 0.0);
  GLubyte pattern[128] = {
      0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55,
      0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA,
      0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55,
      0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA,
      0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55,
      0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA,
      0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55,
      0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA,
      0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55,
      0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA,
      0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55,
      0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA,
      0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55,
      0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA,
      0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55,
      0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA};
  glEnable(GL_POLYGON_STIPPLE);
  glPolygonStipple(pattern);
  glutSetColor(5, 0.5f, 0.5f, 0.5f);

  GLfloat radius = 2.0f;
  int numSegments = 40;
  glBegin(GL_TRIANGLE_FAN);
  glIndexi(5);
  glVertex2f(0.0f, 0.0f); // centre
  for (int j = 0; j <= numSegments; ++j)
  {
    float angle = 2.0f * 3.14159f * j / numSegments;
    glVertex2f(cos(angle) * radius, sin(angle) * radius);
  }
  glEnd();
  glDisable(GL_POLYGON_STIPPLE);
  glPopMatrix();

  glPushMatrix();
  glTranslatef(Xpos, Ypos, 0.0);
  glScalef(2.0, 2.0, 2.0);
  glRotatef(8.0, 0.0, 0.0, 1.0);
  glRotatef(90.0, 1.0, 0.0, 0.0);
  glRotatef(Zrot, 0.0, 0.0, 1.0);

  glCallList(Ball);

  glPopMatrix();

  glFlush();
  glutSwapBuffers();
}

static void
idle(void)
{
  static float vel0 = -100.0;
  static int previousTime = 0;
  int currentTime;
  float deltaTime;

  // Obtenir le temps écoulé en millisecondes
  currentTime = glutGet(GLUT_ELAPSED_TIME);

  // Initialiser previousTime lors du premier appel
  if (previousTime == 0)
  {
    previousTime = currentTime;
    return;
  }

  // Calculer le delta time en secondes
  deltaTime = (currentTime - previousTime) / 1000.0f;
  previousTime = currentTime;

  // Limiter deltaTime pour éviter de gros sauts
  if (deltaTime > 0.1f)
  {
    deltaTime = 0.1f;
  }

  // Rotation de la balle (vitesse en degrés/seconde)
  Zrot += Zstep * 15.0f * deltaTime;

  // Mouvement horizontal (vitesse en unités/seconde)
  Xpos += Xvel * 15.0f * deltaTime;
  if (Xpos >= Xmax)
  {
    Xpos = Xmax;
    Xvel = -Xvel;
    Zstep = -Zstep;
  }
  if (Xpos <= Xmin)
  {
    Xpos = Xmin;
    Xvel = -Xvel;
    Zstep = -Zstep;
  }

  // Mouvement vertical avec gravité (vitesse en unités/seconde)
  Ypos += Yvel * 15.0f * deltaTime;
  Yvel += G * 15.0f * deltaTime;
  if (Ypos < Ymin)
  {
    Ypos = Ymin;
    if (vel0 == -100.0)
      vel0 = fabs(Yvel);
    Yvel = vel0;
  }
  glutPostRedisplay();
}

void visible(int vis)
{
  if (vis == GLUT_VISIBLE)
    glutIdleFunc(idle);
  else
    glutIdleFunc(NULL);
}

main(int argc, char *argv[])
{
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_INDEX | GLUT_DOUBLE);
  glutInitWindowSize(500,550);
  glutCreateWindow("Bounce");

  Ball = make_ball();
  glCullFace(GL_BACK);
  glEnable(GL_CULL_FACE);
  glDisable(GL_DITHER);
  glShadeModel(GL_FLAT);

  glutDisplayFunc(draw);
  glutReshapeFunc(reshape);
  glutVisibilityFunc(visible);
  glutKeyboardFunc(key);
  glutSetColor(4, 160.0f / 255.0f, 160.0f / 255.0f, 160.0f / 255.0f);
  glutSetColor(RED, 1.0, 0.0, 0.0);
  glutSetColor(WHITE, 1.0, 1.0, 1.0);
  glutSetColor(CYAN, 160.0f / 255.0f, 80.0f / 255.0f, 160.0f / 255.0f);
  glClearIndex(4);
  glutMainLoop();
  return 0; /* ANSI C requires main to return int. */
}
