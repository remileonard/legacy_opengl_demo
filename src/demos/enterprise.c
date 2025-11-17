
#ifdef WIN32
    #include <windows.h>
#endif

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/freeglut.h>

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define IDM_APPLICATION_EXIT (101)

#define MAZE_HEIGHT (16)
#define MAZE_WIDTH (16)
#define TARGET_FPS 60
#define FRAME_TIME_MS (1000.0 / TARGET_FPS)


static clock_t last_frame_time = 0;

void (*idlefunc)(void) = NULL;

static void idle(void);
static void menu_callback(int value);
static void keyboard(unsigned char key, int x, int y);
static void special(int key, int x, int y);
static void specialUp(int key, int x, int y);
static void display(void);
static void initGL(void);

static void drawSaucer(void) {
    glBegin(GL_TRIANGLE_FAN);
    glNormal3f(0, 0, 1);
    glVertex3f(0, 0, 0.1f); // centre
    for (int i = 0; i <= 36; ++i) {
        float a = (float)i * 10.0f * 3.1415926f / 180.0f;
        glVertex3f(3.0f * cosf(a), 2.4f * sinf(a), 0.0f);
    }
    glEnd();
}

static void drawNeck(void) {
    glBegin(GL_QUADS);
    glNormal3f(0, 0, 1);
    glVertex3f(-0.4f, -0.6f, 0.0f);
    glVertex3f( 0.4f, -0.6f, 0.0f);
    glVertex3f( 0.6f, -2.4f, -0.4f);
    glVertex3f(-0.6f, -2.4f, -0.4f);
    glEnd();
}

static void drawSecondaryHull(void) {
    glBegin(GL_TRIANGLE_STRIP);
    for (int i = 0; i <= 36; ++i) {
        float a = (float)i * 10.0f * 3.1415926f / 180.0f;
        float x = cosf(a);
        float y = sinf(a);
        glNormal3f(x, 0, y);
        glVertex3f(x * 0.9f, -2.4f, y * 0.9f - 1.2f);
        glVertex3f(x * 0.7f, -4.0f, y * 0.7f - 1.2f);
    }
    glEnd();
}

static void drawNacelle(float offset) {
    glPushMatrix();
    glTranslatef(offset, -3.0f, -0.8f);
    glScalef(0.3f, 1.8f, 0.3f);
    glBegin(GL_QUADS);
    // faces
    glNormal3f(0, 0, 1);
    glVertex3f(-1,-1, 1); glVertex3f(1,-1, 1); glVertex3f(1,1, 1); glVertex3f(-1,1, 1);
    glNormal3f(0, 0,-1);
    glVertex3f(-1,-1,-1); glVertex3f(-1,1,-1); glVertex3f(1,1,-1); glVertex3f(1,-1,-1);
    glNormal3f(-1,0,0);
    glVertex3f(-1,-1,-1); glVertex3f(-1,-1,1); glVertex3f(-1,1,1); glVertex3f(-1,1,-1);
    glNormal3f(1,0,0);
    glVertex3f(1,-1,-1); glVertex3f(1,1,-1); glVertex3f(1,1,1); glVertex3f(1,-1,1);
    glNormal3f(0,1,0);
    glVertex3f(-1,1,-1); glVertex3f(-1,1,1); glVertex3f(1,1,1); glVertex3f(1,1,-1);
    glNormal3f(0,-1,0);
    glVertex3f(-1,-1,-1); glVertex3f(1,-1,-1); glVertex3f(1,-1,1); glVertex3f(-1,-1,1);
    glEnd();
    glPopMatrix();
}

void ncc1701dDisplay(void) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    gluLookAt(6, -8, 5, 0, -2, 0, 0, 0, 1);

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    GLfloat lightPos[] = {4, -4, 6, 1};
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);

    glPushMatrix();
    glColor3f(0.7f, 0.7f, 0.8f);
    drawSaucer();
    drawNeck();
    drawSecondaryHull();
    drawNacelle(1.5f);
    drawNacelle(-1.5f);
    glPopMatrix();

    glutSwapBuffers();
}

static void menu_callback(int value) {
    switch (value) {
    case IDM_APPLICATION_EXIT:
        glutLeaveMainLoop();
        break;
    default:
        break;
    }
}

static void reshape(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, (h > 0) ? (float)w / (float)h : 1.0f, 0.1, 60.0);
    glMatrixMode(GL_MODELVIEW);
}

static void idle(void) {
    // on se contente de redessiner en continu
    clock_t current_time = clock();
    double elapsed_ms = (double)(current_time - last_frame_time) * 1000.0 / CLOCKS_PER_SEC;
    
    if (elapsed_ms >= FRAME_TIME_MS) {
        last_frame_time = current_time;
        glutPostRedisplay();
    }
}

static void keyboard(unsigned char key, int x, int y) {
    (void)x;
    (void)y;

    switch (key) {
    case 27: // ESC
        glutLeaveMainLoop();
        break;
    default:
        break;
    }
}

static void special(int key, int x, int y) {
    (void)x;
    (void)y;

    int modifiers = glutGetModifiers();
    int altDown = modifiers & GLUT_ACTIVE_ALT;

    switch (key) {
    default:
        break;
    }
}

static void specialUp(int key, int x, int y) {
    (void)x;
    (void)y;

    switch (key) {
    default:
        break;
    }
}

static void display(void) {
    if (idlefunc) {
        idlefunc();
    } else {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glutSwapBuffers();
    }
}
static void initGL(void) {
    last_frame_time = clock();
    glEnable(GL_DEPTH_TEST);
    
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, 1.0, 0.1, 60.0);
    glMatrixMode(GL_MODELVIEW);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
    glLoadIdentity();
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    idlefunc = ncc1701dDisplay;
}
int main(int argc, char **argv) {
    glutInit(&argc, argv);
    // Double buffer + RGB + z-buffer
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("Legacy OpenGL Demo");

    int menu = glutCreateMenu(menu_callback);
    glutAddMenuEntry("Exit", IDM_APPLICATION_EXIT);
    

    // bouton droit de la souris = menu
    glutAttachMenu(GLUT_RIGHT_BUTTON);

    initGL();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutIdleFunc(idle);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(special);
    glutSpecialUpFunc(specialUp);

    glutMainLoop();
    return 0;
}