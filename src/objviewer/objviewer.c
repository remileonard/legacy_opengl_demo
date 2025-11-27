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
#include "porting/iris2ogl.h"

#define MAT_SWAMP	 1
#define MAT_PLANE	 2
#define MAT_DIRT	 3
#define MAT_GRAY0	 4
#define MAT_GRAY1	 5
#define MAT_GRAY2	 6
#define MAT_GRAY3	 7
#define MAT_GRAY4	 8
#define MAT_GRAY5	 9
#define MAT_GRAY6	10
#define MAT_GRAY7	11
#define MAT_GRAY8	12
#define MAT_GRAY9	13
#define MAT_GRAY10	14
#define MAT_GRAY11	15
#define MAT_GRAY12	16
#define MAT_THRUSTER	17
#define MAT_GLASS	18
#define MAT_PROP	19
#define MAT_BORANGE	20
#define MAT_BLIME	21
#define MAT_BTAN	22
#define MAT_BGRAY	23
#define MAT_PURPLE	24
#define MAT_LPURPLE	25
#define MAT_MTRAIL	26

#define MAT_F14BLACK	50
#define MAT_F14YELLOW	51
#define MAT_WHITE	52

#define IDM_APPLICATION_EXIT (101)

#define TARGET_FPS 60
#define FRAME_TIME_MS (1000.0 / TARGET_FPS)

#define M_PI 3.14159265358979323846
// Variables pour la rotation de la caméra
static float camera_angle = 0.0f;
static float camera_distance = 200.0f;
static float camera_height = 100.0f;

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


/*
 *  materials
 */
float mat_swamp[] = {AMBIENT,	0.3, 0.6, 0.3,
		     DIFFUSE,	0.3, 0.6, 0.3,
		     SPECULAR,	0.0, 0.0, 0.0,
		     SHININESS, 0.0,
		     LMNULL};

float mat_plane[] = {AMBIENT,	0.7, 0.7, 0.7,
		     DIFFUSE,	0.7, 0.7, 0.7,
		     SPECULAR,	1.0, 1.0, 1.0,
		     SHININESS, 30.0,
		     LMNULL};

float mat_thruster[] = {AMBIENT,   0.2, 0.2, 0.2,
			DIFFUSE,   0.2, 0.2, 0.2,
			SPECULAR,  0.3, 0.3, 0.3,
			SHININESS, 5.0,
			LMNULL};

float mat_dirt[] = {AMBIENT,	0.44, 0.37, 0.19,
		     DIFFUSE,	0.44, 0.37, 0.19,
		     SPECULAR,	0.0, 0.0, 0.0,
		     SHININESS, 0.0,
		     LMNULL};

float mat_gray0[] = {AMBIENT,	0.88, 0.88, 0.88,
		     DIFFUSE,	0.88, 0.88, 0.88,
		     SPECULAR,	0.0, 0.0, 0.0,
		     SHININESS, 0.0,
		     LMNULL};

float mat_gray1[] = {AMBIENT,	0.82, 0.82, 0.82,
		     DIFFUSE,	0.82, 0.82, 0.82,
		     SPECULAR,	0.0, 0.0, 0.0,
		     SHININESS, 0.0,
		     LMNULL};

float mat_gray2[] = {AMBIENT,	0.75, 0.75, 0.75,
		     DIFFUSE,	0.75, 0.75, 0.75,
		     SPECULAR,	0.0, 0.0, 0.0,
		     SHININESS, 0.0,
		     LMNULL};

float mat_gray3[] = {AMBIENT,	0.69, 0.69, 0.69,
		     DIFFUSE,	0.69, 0.69, 0.69,
		     SPECULAR,	0.0, 0.0, 0.0,
		     SHININESS, 0.0,
		     LMNULL};

float mat_gray4[] = {AMBIENT,	0.63, 0.63, 0.63,
		     DIFFUSE,	0.63, 0.63, 0.63,
		     SPECULAR,	0.0, 0.0, 0.0,
		     SHININESS, 0.0,
		     LMNULL};

float mat_gray5[] = {AMBIENT,	0.55, 0.55, 0.55,
		     DIFFUSE,	0.55, 0.55, 0.55,
		     SPECULAR,	0.0, 0.0, 0.0,
		     SHININESS, 0.0,
		     LMNULL};

float mat_gray6[] = {AMBIENT,	0.13, 0.13, 0.13,
		     DIFFUSE,	0.50, 0.50, 0.50,
		     SPECULAR,	0.0, 0.0, 0.0,
		     SHININESS, 0.0,
		     LMNULL};

float mat_gray7[] = {AMBIENT,	0.44, 0.44, 0.44,
		     DIFFUSE,	0.44, 0.44, 0.44,
		     SPECULAR,	0.0, 0.0, 0.0,
		     SHININESS, 0.0,
		     LMNULL};

float mat_gray8[] = {AMBIENT,	0.38, 0.38, 0.38,
		     DIFFUSE,	0.38, 0.38, 0.38,
		     SPECULAR,	0.0, 0.0, 0.0,
		     SHININESS, 0.0,
		     LMNULL};

float mat_gray9[] = {AMBIENT,	0.31, 0.31, 0.31,
		     DIFFUSE,	0.31, 0.31, 0.31,
		     SPECULAR,	0.0, 0.0, 0.0,
		     SHININESS, 0.0,
		     LMNULL};

float mat_gray10[] = {AMBIENT,	0.25, 0.25, 0.25,
		     DIFFUSE,	0.25, 0.25, 0.25,
		     SPECULAR,	0.0, 0.0, 0.0,
		     SHININESS, 0.0,
		     LMNULL};

float mat_gray11[] = {AMBIENT,	0.19, 0.19, 0.19,
		     DIFFUSE,	0.19, 0.19, 0.19,
		     SPECULAR,	0.0, 0.0, 0.0,
		     SHININESS, 0.0,
		     LMNULL};

float mat_gray12[] = {AMBIENT,	0.13, 0.13, 0.13,
		     DIFFUSE,	0.13, 0.13, 0.13,
		     SPECULAR,	0.0, 0.0, 0.0,
		     SHININESS, 0.0,
		     LMNULL};

float mat_glass[] = {AMBIENT,	0.0, 0.0, 1.0,
		     DIFFUSE,	0.5, 0.5, 0.6,
		     SPECULAR,	0.9, 0.9, 1.0,
		     SHININESS, 30.0,
		     ALPHA,	0.3,
		     LMNULL};

float mat_prop[] = {AMBIENT,	0.3, 0.3, 0.3,
		    DIFFUSE,	0.3, 0.3, 0.3,
		    SPECULAR,	0.0, 0.0, 0.0,
		    SHININESS,  0.0,
		    ALPHA,	0.5,
		    LMNULL};

float mat_borange[] = {AMBIENT,	  0.5, 0.25, 0.0,
		       DIFFUSE,	  0.5, 0.25, 0.0,
		       SPECULAR,  0.0, 0.0, 0.0,
		       SHININESS, 0.0,
		       LMNULL};

float mat_blime[] = {AMBIENT,	0.40, 0.5, 0.35,
		     DIFFUSE,	0.40, 0.5, 0.35,
		     SPECULAR,	0.0, 0.0, 0.0,
		     SHININESS, 0.0,
		     LMNULL};

float mat_btan[] = {
    AMBIENT,	0.42, 0.30, 0.25,
    DIFFUSE,	0.42, 0.30, 0.25,
    SPECULAR,	0.0, 0.0, 0.0,
    SHININESS,  0.0,
    LMNULL
};

float mat_bgray[] = {AMBIENT,	0.6, 0.6, 0.6,
		     DIFFUSE,	0.6, 0.6, 0.6,
		     SPECULAR,	0.0, 0.0, 0.0,
		     SHININESS, 0.0,
		     LMNULL};

float mat_purple[] = {AMBIENT,	 0.6, 0.0, 0.6,
		      DIFFUSE,	 0.6, 0.0, 0.6,
		      SPECULAR,	 0.0, 0.0, 0.0,
		      SHININESS, 0.0,
		      LMNULL};

float mat_lpurple[] = {AMBIENT,	  0.7, 0.0, 0.7,
		       DIFFUSE,	  0.7, 0.0, 0.7,
		       SPECULAR,  0.0, 0.0, 0.0,
		       SHININESS, 0.0,
		       LMNULL};

float mat_mtrail[] = {AMBIENT,	 0.7, 0.7, 0.7,
		      DIFFUSE,	 0.7, 0.7, 0.7,
		      SPECULAR,  0.0, 0.0, 0.0,
		      SHININESS, 0.0,
		      ALPHA,     0.8,
		      LMNULL};

float mat_f14black[] = {AMBIENT,   0.1, 0.1, 0.1,
			DIFFUSE,   1.0, 0.1, 0.1,
			SPECULAR,  0.3, 0.3, 0.3,
			SHININESS, 30.0,
			LMNULL};

float mat_f14yellow[] = {AMBIENT,   0.9, 0.7, 0.0,
			 DIFFUSE,   0.9, 0.7, 0.0,
			 SPECULAR,  0.8, 0.8, 0.8,
			 SHININESS, 30.0,
			 LMNULL};

float mat_white[] = {AMBIENT,   1.0, 1.0, 1.0,
		     DIFFUSE,   1.0, 1.0, 1.0,
		     SPECULAR,  1.0, 1.0, 1.0,
		     SHININESS, 30.0,
		     LMNULL};

float infinite[] = {AMBIENT, 0.1,  0.1, 0.1,
		    LOCALVIEWER, 0.0,
		    LMNULL};

/*
 *  lights
 */
float sun[] = {AMBIENT, 0.3, 0.3, 0.3,
	       LCOLOR,   1.0, 1.0, 1.0,
	       POSITION, 0.0, 1.0, 0.0, 0.0,
	       LMNULL};

float moon[] = {AMBIENT, 0.0, 0.0, 0.0,
		LCOLOR,   0.2, 0.2, 0.2,
		POSITION, 1.0, 1.0, 1.0, 0.0,
		LMNULL};

float inst_light[] = {AMBIENT, 0.3, 0.3, 0.3,
		      LCOLOR,   1.0, 1.0, 1.0,
		      POSITION, 0.0, 1.0, 0.5, 0.0,
		      LMNULL};


init_lighting()
{
    resetmaterials();
    lmdef (DEFMATERIAL, MAT_SWAMP, 0, mat_swamp);
    lmdef (DEFMATERIAL, MAT_PLANE, 0, mat_plane);
    lmdef (DEFMATERIAL, MAT_THRUSTER, 0, mat_thruster);
    lmdef (DEFMATERIAL, MAT_DIRT, 0, mat_dirt);
    lmdef (DEFMATERIAL, MAT_GRAY0, 0, mat_gray0);
    lmdef (DEFMATERIAL, MAT_GRAY1, 0, mat_gray1);
    lmdef (DEFMATERIAL, MAT_GRAY2, 0, mat_gray2);
    lmdef (DEFMATERIAL, MAT_GRAY3, 0, mat_gray3);
    lmdef (DEFMATERIAL, MAT_GRAY4, 0, mat_gray4);
    lmdef (DEFMATERIAL, MAT_GRAY5, 0, mat_gray5);
    lmdef (DEFMATERIAL, MAT_GRAY6, 0, mat_gray6);
    lmdef (DEFMATERIAL, MAT_GRAY7, 0, mat_gray7);
    lmdef (DEFMATERIAL, MAT_GRAY8, 0, mat_gray8);
    lmdef (DEFMATERIAL, MAT_GRAY9, 0, mat_gray9);
    lmdef (DEFMATERIAL, MAT_GRAY10, 0, mat_gray10);
    lmdef (DEFMATERIAL, MAT_GRAY11, 0, mat_gray11);
    lmdef (DEFMATERIAL, MAT_GRAY12, 0, mat_gray12);
    lmdef (DEFMATERIAL, MAT_GLASS, 0, mat_glass);
    lmdef (DEFMATERIAL, MAT_PROP, 0, mat_prop);
    lmdef (DEFMATERIAL, MAT_BORANGE, 0, mat_borange);
    lmdef (DEFMATERIAL, MAT_BLIME, 0, mat_blime);
    lmdef (DEFMATERIAL, MAT_BTAN, 0, mat_btan);
    lmdef (DEFMATERIAL, MAT_BGRAY, 0, mat_bgray);
    lmdef (DEFMATERIAL, MAT_PURPLE, 0, mat_purple);
    lmdef (DEFMATERIAL, MAT_LPURPLE, 0, mat_lpurple);
    lmdef (DEFMATERIAL, MAT_MTRAIL, 0, mat_mtrail);
    lmdef (DEFMATERIAL, MAT_F14BLACK, 0, mat_f14black);
    lmdef (DEFMATERIAL, MAT_F14YELLOW, 0, mat_f14yellow);
    lmdef (DEFMATERIAL, MAT_WHITE, 0, mat_white);

    lmdef (DEFLMODEL, INFINITE, 0, infinite);
    lmdef(DEFLIGHT, 1, 0, sun);
    lmbind(LIGHT0, 1);
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
    gluPerspective(60.0, (h > 0) ? (float)w / (float)h : 1.0f, 0.1, 1500000.0);
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
        if (camera_distance > 1000.0f) camera_distance = 1000.0f;
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
static void test_opengl_state(void) {
    // Test simple : dessiner un triangle avec les macros
    printf("Testing OpenGL macros...\n");
    
    float normal[3] = {0.0f, 0.0f, 1.0f};
    float vertex1[3] = {-1.0f, -1.0f, 0.0f};
    float vertex2[3] = {1.0f, -1.0f, 0.0f};
    float vertex3[3] = {0.0f, 1.0f, 0.0f};
    float texcoord1[2] = {0.0f, 0.0f};
    float texcoord2[2] = {1.0f, 0.0f};
    float texcoord3[2] = {0.5f, 1.0f};
    
    bgnpolygon();
    n3f(normal);
    t2f(texcoord1); v3f(vertex1);
    t2f(texcoord2); v3f(vertex2);
    t2f(texcoord3); v3f(vertex3);
    endpolygon();
    
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        printf("ERROR: OpenGL error during macro test: %d\n", error);
    } else {
        printf("Macros work correctly!\n");
    }
}
static void display(void) {
    if (idlefunc) {
        idlefunc();
    } else {
        glClearColor(0.4f, 0.4f, 0.4f, 1.0f);
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
    
        glEnable(GL_LIGHTING);
        glEnable(GL_DEPTH_TEST);
        
        GLfloat light_position[] = {5.0f, 10.0f, 5.0f, 1.0f};
        glLightfv(GL_LIGHT0, GL_POSITION, light_position);
        //glEnable(GL_COLOR_MATERIAL);
        glEnable(GL_LIGHT0);

        // Dessiner l'objet
        if (obj) {
            //glColor3f(0.8f, 0.8f, 0.8f);
            
            drawobj(obj, 0x1);
            GLenum error = glGetError();
            if (error != GL_NO_ERROR) {
                printf("ERROR: OpenGL error during macro test: %d\n", error);
            } else {
                printf("Macros work correctly!\n");
            }
        }
        
        glutSwapBuffers();
    }
}
static void initGL(void) {
    last_frame_time = clock();
    iris_init_colormap();
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, 1.0, 0.1, 6000.0);
    glMatrixMode(GL_MODELVIEW);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
    glLoadIdentity();
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    idlefunc = NULL;
}
int main(int argc, char **argv) {
    glutInit(&argc, argv);
    //init_lighting();
    obj = readobj("f14.d");
    init_lighting();
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