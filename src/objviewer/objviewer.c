#ifdef WIN32
    #include <windows.h>
#endif

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "libgobj/gobj.h"

#define IDM_APPLICATION_EXIT (101)

#define TARGET_FPS 60
#define FRAME_TIME_MS (1000.0 / TARGET_FPS)

#define M_PI 3.14159265358979323846
// Variables pour la rotation de la caméra
static float camera_angle = 0.0f;
static float camera_distance = 40.0f;
static float camera_height = 0.0f;

static int mouse_down = 0;
static int last_mouse_x = 0;
static int last_mouse_y = 0;
static float camera_pitch = 0.0f;  // Angle vertical (haut/bas)
static int auto_rotate = 0;  // Rotation automatique activée par défaut


static clock_t last_frame_time = 0;


void (*idlefunc)(void) = NULL;

static void idle(void);
static void menu_callback(int value);
static void keyboard(unsigned char key, int x, int y);
static void special(int key, int x, int y);
static void specialUp(int key, int x, int y);
static void display(void);
static void initGL(void);
object_t *obj = NULL;

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
    clock_t current_time = clock();
    double elapsed_ms = (double)(current_time - last_frame_time) * 1000.0 / CLOCKS_PER_SEC;
    
    if (elapsed_ms >= FRAME_TIME_MS) {
        last_frame_time = current_time;
        
        // Rotation automatique seulement si activée
        if (auto_rotate) {
            camera_angle += 30.0f * (float)(elapsed_ms / 1000.0);
            if (camera_angle >= 360.0f) {
                camera_angle -= 360.0f;
            }
        }
        
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
    case 32: // SPACE - Toggle auto rotation
        auto_rotate = !auto_rotate;
        break;
    case 'r': // Reset camera
    case 'R':
        camera_angle = 0.0f;
        camera_pitch = 0.0f;
        camera_distance = 40.0f;
        camera_height = 0.0f;
        auto_rotate = 0;
        break;
    default:
        break;
    }
}
static void mouse(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON) {
        if (state == GLUT_DOWN) {
            mouse_down = 1;
            last_mouse_x = x;
            last_mouse_y = y;
            auto_rotate = 0;  // Désactiver la rotation auto quand on clique
        } else {
            mouse_down = 0;
        }
    } else if (button == 3) {  // Molette vers le haut
        camera_distance -= 2.0f;
        if (camera_distance < 5.0f) camera_distance = 5.0f;
        glutPostRedisplay();
    } else if (button == 4) {  // Molette vers le bas
        camera_distance += 2.0f;
        if (camera_distance > 100.0f) camera_distance = 100.0f;
        glutPostRedisplay();
    }
}
static void motion(int x, int y) {
    if (mouse_down) {
        int dx = x - last_mouse_x;
        int dy = y - last_mouse_y;
        
        // Rotation horizontale (azimuth)
        camera_angle += -dx * 0.5f;
        if (camera_angle >= 360.0f) camera_angle -= 360.0f;
        if (camera_angle < 0.0f) camera_angle += 360.0f;
        
        // Rotation verticale (pitch)
        camera_pitch += dy * 0.5f;
        if (camera_pitch > 89.0f) camera_pitch = 89.0f;
        if (camera_pitch < -89.0f) camera_pitch = -89.0f;
        
        last_mouse_x = x;
        last_mouse_y = y;
        
        glutPostRedisplay();
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
        
        // Configuration de la vue
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        
        // Calcul de la position de la caméra avec pitch (angle vertical)
        float pitch_rad = camera_pitch * M_PI / 180.0f;
        float angle_rad = camera_angle * M_PI / 180.0f;
        
        float cam_x = camera_distance * cosf(pitch_rad) * sinf(angle_rad);
        float cam_y = camera_distance * sinf(pitch_rad);
        float cam_z = camera_distance * cosf(pitch_rad) * cosf(angle_rad);
        
        // Positionner la caméra qui regarde vers l'origine
        gluLookAt(cam_x, cam_y, cam_z,   // Position de la caméra
                  0.0, 0.0, 0.0,           // Point visé (centre de l'objet)
                  0.0, 1.0, 0.0);          // Vecteur "haut"
        
        // Lumière positionnée au-dessus
        glLightfv(GL_LIGHT0, GL_POSITION, (GLfloat[]){5.0f, 10.0f, 5.0f, 1.0f});
        glEnable(GL_LIGHT0);
        glEnable(GL_LIGHTING);
        
        glEnable(GL_DEPTH_TEST);
        
        // Dessiner l'objet
        if (obj) {
            drawobj(obj, 0xFFFFFFFF);
        }
        
        glutSwapBuffers();
    }
}
static void initGL(void) {
    last_frame_time = clock();
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
    obj = readobj("f18.d");
    if (!obj) {
        fprintf(stderr, "Failed to load object file 'f18.d'\n");
        return EXIT_FAILURE;
    }
    // Double buffer + RGB + z-buffer
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("Object Viewer (Libgobj) - [SPACE] auto-rotate | [R] reset | [LMB] drag | [WHEEL] zoom");

    int menu = glutCreateMenu(menu_callback);
    glutAddMenuEntry("Exit", IDM_APPLICATION_EXIT);

    glutAttachMenu(GLUT_RIGHT_BUTTON);

    initGL();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutIdleFunc(idle);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(special);
    glutSpecialUpFunc(specialUp);
    glutMouseFunc(mouse);        // Nouvelle callback
    glutMotionFunc(motion);      // Nouvelle callback

    glutMainLoop();
    return 0;
}