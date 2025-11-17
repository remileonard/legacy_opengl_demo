
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
static int window_width = 800;
static int window_height = 600;
static float cube_angle = 0.0f;

void (*idlefunc)(void) = NULL;

static void idle(void);
static void menu_callback(int value);
static void keyboard(unsigned char key, int x, int y);
static void special(int key, int x, int y);
static void specialUp(int key, int x, int y);
static void display(void);
static void initGL(void);

static void draw_rect_outline(int x, int y, int width, int height) {
    glBegin(GL_LINE_LOOP);
    glVertex2i(x, y);
    glVertex2i(x + width, y);
    glVertex2i(x + width, y + height);
    glVertex2i(x, y + height);
    glEnd();
}

static void draw_layout_borders(int top_left_width,
                                int top_right_width,
                                int top_height,
                                int bottom_height,
                                int top_y) {
    glViewport(0, 0, window_width, window_height);
    glDisable(GL_DEPTH_TEST);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0.0, window_width, 0.0, window_height);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glColor3f(0.0f, 1.0f, 0.0f);
    glLineWidth(2.0f);

    draw_rect_outline(0, top_y, top_left_width, top_height);
    draw_rect_outline(top_left_width, top_y, top_right_width, top_height);
    draw_rect_outline(0, 0, window_width, bottom_height);
}
static void render_top_left_2d(int viewport_width, int viewport_height) {
    (void)viewport_width;
    (void)viewport_height;

    glDisable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0.0, 1.0, 0.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glBegin(GL_QUADS);
    glColor3f(0.1f, 0.4f, 0.8f);
    glVertex2f(0.1f, 0.1f);
    glVertex2f(0.4f, 0.1f);
    glVertex2f(0.4f, 0.4f);
    glVertex2f(0.1f, 0.4f);
    glEnd();

    glBegin(GL_QUADS);
    glColor3f(0.8f, 0.4f, 0.1f);
    glVertex2f(0.6f, 0.2f);
    glVertex2f(0.9f, 0.2f);
    glVertex2f(0.9f, 0.5f);
    glVertex2f(0.6f, 0.5f);
    glEnd();
}

static void render_top_right_2d(int viewport_width, int viewport_height) {
    (void)viewport_width;
    (void)viewport_height;

    glDisable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0.0, 1.0, 0.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glBegin(GL_LINES);
    glColor3f(0.9f, 0.9f, 0.9f);
    for (int i = 0; i <= 10; ++i) {
        float t = i / 10.0f;
        glVertex2f(t, 0.0f);
        glVertex2f(t, 1.0f);
        glVertex2f(0.0f, t);
        glVertex2f(1.0f, t);
    }
    glEnd();

    glBegin(GL_TRIANGLES);
    glColor3f(0.2f, 0.8f, 0.3f);
    glVertex2f(0.2f, 0.2f);
    glVertex2f(0.8f, 0.2f);
    glVertex2f(0.5f, 0.7f);
    glEnd();
}

static void render_bottom_3d(int viewport_width, int viewport_height) {
    if (viewport_height <= 0) {
        return;
    }

    glEnable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0,
                   (viewport_height > 0) ? (double)viewport_width / (double)viewport_height : 1.0,
                   0.1,
                   100.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(3.0, 3.0, 6.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);

    glRotatef(cube_angle, 0.0f, 1.0f, 0.0f);
    glRotatef(cube_angle * 0.5f, 1.0f, 0.0f, 0.0f);

    glBegin(GL_QUADS);
    glColor3f(1.0f, 0.0f, 0.0f);   glVertex3f(-1.0f, -1.0f, 1.0f);  glVertex3f(1.0f, -1.0f, 1.0f);
    glVertex3f(1.0f,  1.0f, 1.0f); glVertex3f(-1.0f,  1.0f, 1.0f);
    glColor3f(0.0f, 1.0f, 0.0f);   glVertex3f(-1.0f, -1.0f,-1.0f);  glVertex3f(-1.0f, 1.0f,-1.0f);
    glVertex3f(1.0f,  1.0f,-1.0f); glVertex3f(1.0f, -1.0f,-1.0f);
    glColor3f(0.0f, 0.0f, 1.0f);   glVertex3f(-1.0f, -1.0f,-1.0f);  glVertex3f(-1.0f,-1.0f, 1.0f);
    glVertex3f(-1.0f, 1.0f, 1.0f); glVertex3f(-1.0f, 1.0f,-1.0f);
    glColor3f(1.0f, 1.0f, 0.0f);   glVertex3f(1.0f, -1.0f,-1.0f);   glVertex3f(1.0f, 1.0f,-1.0f);
    glVertex3f(1.0f,  1.0f, 1.0f); glVertex3f(1.0f,-1.0f, 1.0f);
    glColor3f(0.0f, 1.0f, 1.0f);   glVertex3f(-1.0f, 1.0f,-1.0f);   glVertex3f(-1.0f, 1.0f, 1.0f);
    glVertex3f(1.0f,  1.0f, 1.0f); glVertex3f(1.0f, 1.0f,-1.0f);
    glColor3f(1.0f, 0.0f, 1.0f);   glVertex3f(-1.0f,-1.0f,-1.0f);   glVertex3f(1.0f,-1.0f,-1.0f);
    glVertex3f(1.0f,-1.0f, 1.0f);  glVertex3f(-1.0f,-1.0f, 1.0f);
    glEnd();

    cube_angle += 0.5f;
    if (cube_angle >= 360.0f) {
        cube_angle -= 360.0f;
    }
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
    window_width = (w > 0) ? w : 1;
    window_height = (h > 0) ? h : 1;
    glViewport(0, 0, window_width, window_height);
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
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    int top_height = window_height / 2;
    int bottom_height = window_height - top_height;
    int top_left_width = window_width / 3;
    int top_right_width = window_width - top_left_width;
    int top_y = bottom_height;

    glViewport(0, top_y, top_left_width, top_height);
    render_top_left_2d(top_left_width, top_height);

    glViewport(top_left_width, top_y, top_right_width, top_height);
    render_top_right_2d(top_right_width, top_height);

    glViewport(0, 0, window_width, bottom_height);
    render_bottom_3d(window_width, bottom_height);

    draw_layout_borders(top_left_width, top_right_width, top_height, bottom_height, top_y);

    glutSwapBuffers();
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

    idlefunc = NULL;
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