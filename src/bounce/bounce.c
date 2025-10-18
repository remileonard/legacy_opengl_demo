
/**************************************************************************
 *									  *
 * 	 Copyright (C) 1988, 1989, 1990, Silicon Graphics, Inc.		  *
 *									  *
 *  These coded instructions, statements, and computer programs  contain  *
 *  unpublished  proprietary  information of Silicon Graphics, Inc., and  *
 *  are protected by Federal copyright law.  They  may  not be disclosed  *
 *  to  third  parties  or copied or duplicated in any form, in whole or  *
 *  in part, without the prior written consent of Silicon Graphics, Inc.  *
 *									  *
 **************************************************************************/

/*
 *  foo $Revision: 1.16 $
 */
#include <stdint.h>
#include <math.h>
#include <stdio.h>
#include <stdarg.h>
#include <GL/glut.h>

#include "trackball.h"
#include "glui.h"

#define UDIV 12
#define VDIV 12

#define WALLGRIDMAX 32
#define EYEZ 3.3

#define TOTALBALLS 3

#define R 0
#define G 1
#define B 2

#define X 0
#define Y 1
#define Z 2
#define W 3

int32_t wallgrid = 8; /* sqrt of the number of quads in a wall */
float fatt = 1.0;

int32_t freeze = GL_FALSE;
int32_t spin = GL_FALSE;
int32_t objecton = GL_FALSE;
int32_t normson = GL_FALSE;
int32_t lighton[3] = {GL_TRUE, GL_TRUE, GL_TRUE};

int32_t window; /* main window id */

GLboolean performance = GL_TRUE; /* performance indicator */

struct
{
    float p[3];
    float d[3];
    unsigned char color[3];
} balls[TOTALBALLS];

float ballobj[UDIV + 1][VDIV + 1][4];
float wallobj[WALLGRIDMAX + 1][WALLGRIDMAX + 1][4];
float wallnorms[WALLGRIDMAX + 1][WALLGRIDMAX + 1][3];
float wallnorm[3] = {0.0, 0.0, -1.0};

int32_t orx, ory;

float ballscale;
float ballsize;

int32_t DELTAX, DELTAY;

int32_t lflag = 0;

float newpos[] = {0.0, 0.0, 0.0, 1.0};

GLfloat light_Ka[] = {0.3, 0.3, 0.3, 1.0}; /* ambient */
GLfloat light_Ks[] = {0.0, 0.0, 0.0, 1.0}; /* specular */

GLfloat light0_Ka[] = {0.0, 0.0, 0.0, 1.0};  /* ambient */
GLfloat light0_Kd[] = {1.0, 0.1, 0.1, 1.0};  /* diffuse */
GLfloat light0_pos[] = {0.0, 0.0, 0.0, 1.0}; /* position */

GLfloat light1_Ka[] = {0.0, 0.0, 0.0, 1.0};  /* ambient */
GLfloat light1_Kd[] = {0.1, 1.0, 0.1, 1.0};  /* diffuse */
GLfloat light1_pos[] = {0.0, 0.0, 0.0, 1.0}; /* position */

GLfloat light2_Ka[] = {0.0, 0.0, 0.0, 1.0};  /* ambient */
GLfloat light2_Kd[] = {0.1, 0.1, 1.0, 1.0};  /* diffuse */
GLfloat light2_pos[] = {0.0, 0.0, 0.0, 1.0}; /* position */

GLfloat attenuation[] = {1.0, 3.0};

GLfloat plane_Ka[] = {0.0, 0.0, 0.0, 1.0}; /* ambient */
GLfloat plane_Kd[] = {0.4, 0.4, 0.4, 1.0}; /* diffuse */
GLfloat plane_Ks[] = {1.0, 1.0, 1.0, 1.0}; /* specular */
GLfloat plane_Ke[] = {0.0, 0.0, 0.0, 1.0}; /* emission */
GLfloat plane_Se = 30.0;                   /* shininess */

GLfloat wall_Ka[] = {0.1, 0.1, 0.1, 1.0}; /* ambient */
GLfloat wall_Kd[] = {0.8, 0.8, 0.8, 1.0}; /* diffuse */
GLfloat wall_Ks[] = {1.0, 1.0, 1.0, 1.0}; /* specular */
GLfloat wall_Ke[] = {0.0, 0.0, 0.0, 1.0}; /* emission */
GLfloat wall_Se = 20.0;                   /* shininess */

GLuint wall_material, plane_material; /* material display lists */

extern float frand();

char ofile[80];

/************************************************************/
/* XXX - The following is an excerpt from spin.h from spin  */
/************************************************************/

#define POLYGON 1
#define LINES 2
#define TRANSPERENT 3
#define DISPLAY 4
#define LMATERIAL 5

#define FASTMAGIC 0x5423

typedef struct fastobj
{
    int32_t npoints;
    int32_t colors;
    int32_t type;
    int32_t material;
    int32_t display;
    int32_t ablend;
    int32_t *data;
} fastobj;

fastobj *readfastobj();

/*
 * Wrappers to do either lines or polygons
 */
#define PolyOrLine()           \
    if (lflag == LINES)        \
    {                          \
        glBegin(GL_LINE_LOOP); \
    }                          \
    else if (POLYGON)          \
    {                          \
        glBegin(GL_POLYGON);   \
    }

#define EndPolyOrLine() \
    if (lflag == LINES) \
    {                   \
        glEnd();        \
    }                   \
    else if (POLYGON)   \
    {                   \
        glEnd();        \
    }

/************************* end of spin.h excerpt *************************/

fastobj *obj = NULL;

/*

   general purpose text routine.  draws a string according to the
   format in a stroke font at x, y after scaling it by the scale
   specified.  x, y and scale are all in window-space [i.e., pixels]
   with origin at the lower-left.

*/

void text(GLuint x, GLuint y, GLfloat scale, char *format, ...)
{
    va_list args;
    char buffer[255], *p;
    GLfloat font_scale = 119.05 + 33.33;

    va_start(args, format);
    vsprintf(buffer, format, args);
    va_end(args);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, glutGet(GLUT_WINDOW_WIDTH), 0, glutGet(GLUT_WINDOW_HEIGHT));

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glPushAttrib(GL_ENABLE_BIT);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glTranslatef(x, y, 0.0);

    glScalef(scale / font_scale, scale / font_scale, scale / font_scale);

    for (p = buffer; *p; p++)
        glutStrokeCharacter(GLUT_STROKE_ROMAN, *p);

    glPopAttrib();

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

void resetballs()
{
    register short i;

    balls[0].color[R] = 255;
    balls[0].color[G] = 64;
    balls[0].color[B] = 64;
    balls[1].color[R] = 64;
    balls[1].color[G] = 255;
    balls[1].color[B] = 64;
    balls[2].color[R] = 64;
    balls[2].color[G] = 64;
    balls[2].color[B] = 255;
    for (i = 0; i < TOTALBALLS; i++)
    {
        balls[i].p[0] = 0.0;
        balls[i].p[1] = 0.0;
        balls[i].p[2] = 0.0;
        balls[i].d[0] = .1 * frand();
        balls[i].d[1] = .1 * frand();
        balls[i].d[2] = .1 * frand();
    }
}

void drawface()
{
    register int32_t i, j;

    glNormal3fv(wallnorm);
    for (i = 0; i < wallgrid; i++)
    {
        glBegin(GL_TRIANGLE_STRIP);
        for (j = 0; j <= wallgrid; j++)
        {
            glVertex3fv(wallobj[i][j]);
            glVertex3fv(wallobj[i + 1][j]);
        }
        glEnd();
    }
}

void drawnorms()
{
    register int32_t i, j;

    glDisable(GL_LIGHTING);
    glColor3ub(255, 255, 0);
    for (i = 0; i <= wallgrid; i++)
    {
        for (j = 0; j <= wallgrid; j++)
        {
            glBegin(GL_LINES);
            glVertex3fv(wallobj[i][j]);
            glVertex3fv(wallnorms[i][j]);
            glEnd();
        }
    }
    glEnable(GL_LIGHTING);
}

void drawbox()
{
    glPushMatrix();

    /*  drawface();		*/
    glRotatef(90.0, 0.0, 1.0, 0.0);
    drawface();
    if (normson)
        drawnorms();
    glRotatef(90.0, 0.0, 1.0, 0.0);
    drawface();
    if (normson)
        drawnorms();
    glRotatef(90.0, 0.0, 1.0, 0.0);
    /*  drawface();		*/
    glRotatef(-90.0, 1.0, 0.0, 0.0);
    drawface();
    if (normson)
        drawnorms();
    glRotatef(180.0, 1.0, 0.0, 0.0);
    /*  drawface();		*/
    glPopMatrix();
}

void
    drawfastobj(obj)
        fastobj *obj;
{
    register int32_t *p, *end;
    register int32_t npolys;

    p = obj->data;
    end = p + 8 * obj->npoints;

    if (obj->colors)
    {
        npolys = obj->npoints / 4;
        p = obj->data;
        while (npolys--)
        {
            PolyOrLine();
            glColor3iv(p);
            glVertex3fv((float *)p + 4);
            glColor3iv(p + 8);
            glVertex3fv((float *)p + 12);
            glColor3iv(p + 16);
            glVertex3fv((float *)p + 20);
            glColor3iv(p + 24);
            glVertex3fv((float *)p + 28);
            EndPolyOrLine();
            p += 32;
        }
    }
    else
    {
        while (p < end)
        {
            PolyOrLine();
            glNormal3fv((float *)p);
            glVertex3fv((float *)p + 4);
            glNormal3fv((float *)p + 8);
            glVertex3fv((float *)p + 12);
            glNormal3fv((float *)p + 16);
            glVertex3fv((float *)p + 20);
            glNormal3fv((float *)p + 24);
            glVertex3fv((float *)p + 28);
            EndPolyOrLine();
            p += 32;
        }
    }
}

void drawball()
{
    register int32_t i, j;

    for (i = 0; i < UDIV; i++)
    {
        for (j = 0; j < VDIV; j++)
        {
            glBegin(GL_POLYGON);
            glVertex4fv(ballobj[i][j]);
            glVertex4fv(ballobj[i + 1][j]);
            glVertex4fv(ballobj[i + 1][j + 1]);
            glVertex4fv(ballobj[i][j + 1]);
            glEnd();
        }
    }
}

void drawimage(void)
{
    register short i;
    static int32_t start, end, last;

    glutSetWindow(window);

    if (performance)
        start = glutGet(GLUT_ELAPSED_TIME);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glPushMatrix();
    tbMatrix();

    for (i = 0; i < TOTALBALLS; i++)
    {
        newpos[0] = balls[i].p[0];
        newpos[1] = balls[i].p[1];
        newpos[2] = balls[i].p[2];
        glLightfv(GL_LIGHT0 + i, GL_POSITION, newpos);
    }

    glCallList(wall_material);
    glEnable(GL_LIGHTING);
    drawbox();

    glEnable(GL_DEPTH_TEST);

    if (objecton)
    {
        glCallList(plane_material);
        glPushMatrix();
        glScalef(1.5, 1.5, 1.5);
        glRotatef(180.0, 0.0, 0.0, 1.0);
        if (spin)
        {
            orx += 50;
            ory += 50;
        }
        glRotatef(orx / 100.0, 1.0, 0.0, 0.0);
        glRotatef(ory / 100.0, 0.0, 1.0, 0.0);
        drawfastobj(obj);
        glPopMatrix();
    }

    glDisable(GL_LIGHTING);

    for (i = 0; i < TOTALBALLS; i++)
    {
        if (lighton[i])
        {
            glPushMatrix();
            glTranslatef(balls[i].p[0], balls[i].p[1], balls[i].p[2]);
            glColor3ubv(balls[i].color);
            drawball();
            glPopMatrix();
        }
    }

    glColor3f(1.0, 1.0, 1.0);
    if (performance)
    {
        text(10, 73, 20, "%.0f fps", 1.0 / ((end - last) / 1000.0));
        last = start;
    }
    text(10, 43, 14, "Attenuation [%.2f]", fatt);
    text(10, 13, 14, "Tesselation [%3d]", wallgrid);

    glPopMatrix();
    glutSwapBuffers();

    if (performance)
        end = glutGet(GLUT_ELAPSED_TIME);
}

void initobjects()
{
    register float u, v, du, dv;
    register short i, j;

    du = 2.0 * 3.1416 / UDIV;
    dv = 3.1416 / VDIV;

    u = 0.;
    for (i = 0; i <= UDIV; i++)
    {
        v = 0.;
        for (j = 0; j <= VDIV; j++)
        {
            ballobj[i][j][X] = ballsize * cos(u) * sin(v);
            ballobj[i][j][Y] = ballsize * sin(u) * sin(v);
            ballobj[i][j][Z] = ballsize * cos(v);
            ballobj[i][j][W] = 1.0;
            v += dv;
        }
        u += du;
    }

    for (i = 0; i <= wallgrid; i++)
    {
        for (j = 0; j <= wallgrid; j++)
        {
            wallobj[i][j][X] = -1.0 + 2.0 * i / wallgrid;
            wallobj[i][j][Y] = -1.0 + 2.0 * j / wallgrid;
            wallobj[i][j][Z] = 1.0;
            wallobj[i][j][W] = 1.0;
        }
    }

    for (i = 0; i <= wallgrid; i++)
    {
        for (j = 0; j <= wallgrid; j++)
        {
            wallnorms[i][j][X] = wallobj[i][j][X] + wallnorm[X] * 0.1;
            wallnorms[i][j][Y] = wallobj[i][j][Y] + wallnorm[Y] * 0.1;
            wallnorms[i][j][Z] = wallobj[i][j][Z] + wallnorm[Z] * 0.1;
        }
    }
}

void
    initialize(argv) char **argv;
{
    long time();
    void motion(int32_t, int32_t);
    void mouse(int32_t, int32_t, int32_t, int32_t);
    void keyboard(unsigned char, int32_t, int32_t);

    glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE);

    { /* Open window with name of executable */
        char *t, *strrchr();
        window = glutCreateWindow((t = strrchr(argv[0], '/')) != NULL ? t + 1 : argv[0]);
    }

    initobjects();

    srand((unsigned)time(0));

    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, light_Ka);

    plane_material = glGenLists(1);
    glNewList(plane_material, GL_COMPILE);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, plane_Ka);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, plane_Kd);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, plane_Ks);
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, plane_Ke);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, plane_Se);
    glEndList();

    wall_material = glGenLists(1);
    glNewList(wall_material, GL_COMPILE);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, wall_Ka);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, wall_Kd);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, wall_Ks);
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, wall_Ke);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, wall_Se);
    glEndList();

    glLightfv(GL_LIGHT0, GL_AMBIENT, light0_Ka);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light0_Kd);
    glLightf(GL_LIGHT0, GL_CONSTANT_ATTENUATION, attenuation[0]);
    glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION, attenuation[1]);
    /* OpenGL's light0 has different specular properties than the rest
       of the lights.... */
    glLightfv(GL_LIGHT0, GL_SPECULAR, light_Ks);

    glLightfv(GL_LIGHT1, GL_AMBIENT, light1_Ka);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, light1_Kd);
    glLightf(GL_LIGHT1, GL_CONSTANT_ATTENUATION, attenuation[0]);
    glLightf(GL_LIGHT1, GL_LINEAR_ATTENUATION, attenuation[1]);

    glLightfv(GL_LIGHT2, GL_AMBIENT, light2_Ka);
    glLightfv(GL_LIGHT2, GL_DIFFUSE, light2_Kd);
    glLightf(GL_LIGHT2, GL_CONSTANT_ATTENUATION, attenuation[0]);
    glLightf(GL_LIGHT2, GL_LINEAR_ATTENUATION, attenuation[1]);

    glutMotionFunc(motion);
    glutMouseFunc(mouse);
    glutKeyboardFunc(keyboard);
}

void calcball()
{
    register short i, j;

    for (j = 0; j < TOTALBALLS; j++)
    {
        for (i = 0; i < 3; i++)
        {
            balls[j].p[i] += balls[j].d[i];
            if (fabs(balls[j].p[i]) > ballscale)
            {
                balls[j].p[i] = (balls[j].p[i] > 0.0) ? ballscale : -ballscale;
                balls[j].d[i] = -balls[j].d[i];
            }
        }
    }
}

float frand()
{
    return 2.0 * (rand() / 32768.0 - .5);
}

void menu(int32_t value)
{
    void make_menu(void);

    switch (value)
    {
    case 1:
        if (lighton[0] = !lighton[0])
            glEnable(GL_LIGHT0);
        else
            glDisable(GL_LIGHT0);
        break;
    case 2:
        if (lighton[1] = !lighton[1])
            glEnable(GL_LIGHT1);
        else
            glDisable(GL_LIGHT1);
        break;
    case 3:
        if (lighton[2] = !lighton[2])
            glEnable(GL_LIGHT2);
        else
            glDisable(GL_LIGHT2);
        break;
    case 4:
        freeze = !freeze;
        break;
    case 5:
        if (obj)
            objecton = !objecton;
        else
            exit(1);
        break;
    case 6:
        spin = !spin;
        break;
    case 7:
        normson = !normson;
        break;
    case 8:
        performance = !performance;
        break;
    case 9:
        exit(0);
        break;
    }
    make_menu();
}

void make_menu()
{
    static int32_t main_menu = 0;

    if (main_menu)
        glutDestroyMenu(main_menu);

    main_menu = glutCreateMenu(menu);
    glutAddMenuEntry("bounce", 0);
    glutAddMenuEntry("", 0);
    if (lighton[0])
        glutAddMenuEntry("red light off", 1);
    else
        glutAddMenuEntry("red light on", 1);
    if (lighton[1])
        glutAddMenuEntry("green light off", 2);
    else
        glutAddMenuEntry("green light on", 2);
    if (lighton[2])
        glutAddMenuEntry("blue light off", 3);
    else
        glutAddMenuEntry("blue light on", 3);

    if (freeze)
        glutAddMenuEntry("unfreeze lights", 4);
    else
        glutAddMenuEntry("freeze lights", 4);

    if (normson)
        glutAddMenuEntry("normals off", 7);
    else
        glutAddMenuEntry("normals on", 7);

    if (performance)
        glutAddMenuEntry("frame rate off", 8);
    else
        glutAddMenuEntry("frame rate on", 8);

    if (obj)
    {
        if (objecton)
            glutAddMenuEntry("object off", 5);
        else
            glutAddMenuEntry("object on", 5);
        if (spin)
            glutAddMenuEntry("object spin off", 6);
        else
            glutAddMenuEntry("object spin on", 6);
    }

    glutAddMenuEntry("", 0);
    glutAddMenuEntry("exit", 9);

    glutAttachMenu(GLUT_RIGHT_BUTTON);
}

/**********************************************************/
/* XXX - The following is a clone of fastobj.c from spin  */
/**********************************************************/
int32_t swap_int32(int32_t val)
{
    uint8_t *bytes = (uint8_t *)&val;
    return ((int32_t)bytes[0] << 24) |
           ((int32_t)bytes[1] << 16) |
           ((int32_t)bytes[2] << 8) |
           ((int32_t)bytes[3]);
}

fastobj *
readfastobj(name)
char *name;
{
    FILE *inf;
    fastobj *obj;
    int32_t i;
    int32_t nlongs;
    int32_t magic;
    int32_t *ip;
    char filename[512];

    inf = fopen(name, "rb");
    if (!inf)
    {
        sprintf(filename, "%s", name);
        inf = fopen(filename, "rb");
        if (!inf)
        {
            fprintf(stderr, "readfast: can't open input file %s\n", name);
            exit(1);
        }
    }
    fread(&magic, sizeof(int32_t), 1, inf);
    magic = swap_int32(magic); // swap endian
    if (magic != FASTMAGIC)
    {
        fprintf(stderr, "readfast: bad magic in object file\n");
        fclose(inf);
        exit(1);
    }
    obj = (fastobj *)malloc(sizeof(fastobj));
    fread(&obj->npoints, sizeof(int32_t), 1, inf);
    obj->npoints = swap_int32(obj->npoints);
    fread(&obj->colors, sizeof(int32_t), 1, inf);
    obj->colors = swap_int32(obj->colors);

    nlongs = 8 * obj->npoints;
    obj->data = (int32_t *)malloc(nlongs * sizeof(int32_t) + 4096);
    // obj->data = (int32_t *)(((intptr_t)(obj->data)) + 0xfff);
    // obj->data = (int32_t *)(((intptr_t)(obj->data)) & 0xfffff000);
    ip = obj->data;
    for (i = 0; i < nlongs / 4; i++, ip += 4)
    {
        fread(ip, 3 * sizeof(int32_t), 1, inf);
        ip[0] = swap_int32(ip[0]);
        ip[1] = swap_int32(ip[1]);
        ip[2] = swap_int32(ip[2]);
    }
    fclose(inf);
    return obj;
}

/*
 * objmaxpoint
 *
 * find the vertex farthest from the origin,
 * so we can set the near and far clipping planes tightly.
 */

#define MAXVERT(v)                                   \
    if ((len = sqrt((*(v)) * (*(v)) +                \
                    (*(v + 1)) * (*(v + 1)) +        \
                    (*(v + 2)) * (*(v + 2)))) > max) \
        max = len;

float objmaxpoint(obj)
fastobj *obj;
{
    register float *p, *end;
    register int32_t npolys;
    register float len;
    register float max = 0.0;

    p = (float *)(obj->data);

    if (obj->colors)
    {
        npolys = obj->npoints / 4;
        while (npolys--)
        {
            MAXVERT(p + 4);
            MAXVERT(p + 12);
            MAXVERT(p + 20);
            MAXVERT(p + 28);
            p += 32;
        }
    }
    else
    {
        end = p + 8 * obj->npoints;
        while (p < end)
        {
            MAXVERT(p + 4);
            MAXVERT(p + 12);
            MAXVERT(p + 20);
            MAXVERT(p + 28);
            p += 32;
        }
    }

    return max;
}

void keyboard(unsigned char key, int32_t x, int32_t y)
{
    switch (key)
    {
    case 27: /* ESC */
        exit(0);
        break;

    case '+':
        wallgrid++;
        if (wallgrid > WALLGRIDMAX)
            wallgrid = WALLGRIDMAX;
        initobjects();
        break;

    case '-':
        wallgrid--;
        if (wallgrid < 1)
            wallgrid = 1;
        initobjects();
        break;
    }
}

int32_t MOUSEX, MOUSEY;

void mouse(int32_t button, int32_t state, int32_t x, int32_t y)
{
    MOUSEX = x;
    MOUSEY = y;
    tbMouse(button, state, x, y);
}

void motion(int32_t x, int32_t y)
{
    DELTAX -= MOUSEX - x;
    DELTAY += MOUSEY - y;
    MOUSEX = x;
    MOUSEY = y;
    tbMotion(x, y);
}

void idle(void)
{
    if (!freeze)
        calcball();
    drawimage();
}

void reshape(int32_t width, int32_t height)
{
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, (float)width / height, EYEZ - 2.0, EYEZ + 2.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0.0, 0.25, -EYEZ);

    tbReshape(width, height);
    gluiReshape(width, height);
}

void update_fatt(float value)
{
    fatt = 5 * value;
    glLightf(GL_LIGHT0, GL_CONSTANT_ATTENUATION, fatt);
    glLightf(GL_LIGHT1, GL_CONSTANT_ATTENUATION, fatt);
    glLightf(GL_LIGHT2, GL_CONSTANT_ATTENUATION, fatt);
    glutPostRedisplay();
}

void update_grid(float value)
{
    wallgrid = WALLGRIDMAX * value;
    if (wallgrid < 1)
        wallgrid = 1;
    initobjects();
    glutPostRedisplay();
}

void
    main(argc, argv) int32_t argc;
char **argv;
{
    glutInitWindowSize(512, 512);
    glutInitWindowPosition(64, 64);
    glutInit(&argc, argv);

    if (argc > 1)
    {
        strcpy(ofile, argv[1]);
    }
    else
    {
        strcpy(ofile, "x29.bin");
    }
    if (obj = readfastobj(ofile))
        objecton = GL_TRUE;
    ballsize = .08;
    ballscale = 1.0 - ballsize;

    initialize(argv);

    make_menu();

    resetballs();

    /* Use local lights for the box */
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHT1);
    glEnable(GL_LIGHT2);

    make_menu();
    glutAttachMenu(GLUT_RIGHT_BUTTON);

    glutDisplayFunc(drawimage);
    glutReshapeFunc(reshape);
    glutIdleFunc(idle);

    gluiHorizontalSlider(window, 130, -10, -10, 20,
                         (float)wallgrid / WALLGRIDMAX, update_grid);
    gluiHorizontalSlider(window, 130, -40, -10, 20, fatt / 5.0, update_fatt);

    tbInit(GLUT_LEFT_BUTTON);
    tbAnimate(GL_FALSE);

    glutMainLoop();
}
