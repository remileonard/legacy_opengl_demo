/* 

voronoi.c
Nate Robins, 1997.

Uses the depth buffer to intersect cones drawn at each point
selected by the user, thus creating a voronoi diagram.

*/


#include <math.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#if defined (__APPLE__) || defined (MACOSX)
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif


typedef struct _node {
    int x, y;
    unsigned char r, g, b;
    struct _node* next;
} node;


node* points = NULL;
int width = 256, height = 256;
GLboolean drawpoints = GL_FALSE;


void
init(void)
{
    node* point;

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glPointSize(4.0);

    srand(time(NULL));

    while (points) {
        point = points;
        points = points->next;
        free(point);
    }
}

void
reshape(int w, int h)
{
    node* point;

    /* rescale points so they are in their 'relative' position when the
       window size changes. */
    point = points;
    while (point) {
        point->x *= (float)w/width;
        point->y *= (float)h/height;
        point = point->next;
    }
    width = w;
    height = h;

    glViewport(0, 0, width, height);
  
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, width, height, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void
display(void)
{
    node* point;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    point = points;
    while (point) {
        glPushMatrix();
        glColor3ub(point->r, point->g, point->b);
        glTranslatef(point->x, point->y, 0.0);
        glutSolidCone(width > height ? width : height, 1.0, 32, 1);
        if (drawpoints) {
            glDepthFunc(GL_ALWAYS);
            glColor3ub(255, 255, 255);
            glBegin(GL_POINTS);
            glVertex2i(0, 0);
            glEnd();
            glDepthFunc(GL_LEQUAL);
        }
        glPopMatrix();
        point = point->next;
    }
  
    glutSwapBuffers();
}

void
mouse(int button, int state, int x, int y)
{
    node* point;

    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        point = (node*)malloc(sizeof(node));
        point->next = points;
        point->x = x;
        point->y = y;
        point->r = rand()%256;
        point->g = rand()%256;
        point->b = rand()%256;
        points = point;
        glutPostRedisplay();
    }
}

void
keyboard(unsigned char key, int x, int y)
{
    node* point;

    switch (key) {
    case 27:
        while (points) {
            point = points;
            points = points->next;
            free(point);
        }
        exit(0);
    case 'r':
        while (points) {
            point = points;
            points = points->next;
            free(point);
        }
        break;
    case 'p':
        drawpoints = !drawpoints;
        break;
    case '\r':
        init();
        break;
    default:
        return;
    }

    glutPostRedisplay();
}

void
menu(int value)
{
    keyboard((unsigned char)value, 0, 0);
}

int
main(int argc, char** argv)
{
    glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE);
    glutInitWindowSize(width, height);
    glutInitWindowPosition(50, 50);
    glutInit(&argc, argv);

    glutCreateWindow("Voronoi");
    glutKeyboardFunc(keyboard);
    glutReshapeFunc(reshape);
    glutDisplayFunc(display);
    glutMouseFunc(mouse);
    glutCreateMenu(menu);
    glutAddMenuEntry("Voronoi", 0);
    glutAddMenuEntry("", 0);
    glutAddMenuEntry("[r] Reset", 'r');
    glutAddMenuEntry("[p] Toggle points", 'p');
    glutAddMenuEntry("", 0);
    glutAddMenuEntry("Quit", 27);
    glutAttachMenu(GLUT_RIGHT_BUTTON);
  
    init();
  
    glutMainLoop();
    return 0;
}
