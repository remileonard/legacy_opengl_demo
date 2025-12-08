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


#define C_BLACK		0
#define C_WHITE		1
#define C_RED		2
#define C_DRED		3
#define C_GREEN		4
#define C_BLUE		5
#define C_YELLOW	6
#define C_ORANGE	7
#define C_INST_BROWN	8
#define C_HBLUE		9
#define C_GREY		10

/*
 *  missile contrail
 */
#define C_MC_FLAME	16
#define C_MC_TRAIL	17

/*
 *  non ramp
 */
#define C_DIRT		18
#define C_SWAMP		19
#define C_SKY		20

/*
 *  ramps
 */
#define C_GREY_0	21
#define C_GREY_1	22
#define C_GREY_2	23
#define C_GREY_3	24
#define C_GREY_4	25
#define C_GREY_5	26
#define C_GREY_6	27
#define C_GREY_7	28
#define C_GREY_8	29
#define C_GREY_9	30
#define C_GREY_10	31
#define C_GREY_11	32
#define C_GREY_12	33

#define C_SILVER_0	34
#define C_SILVER_1	35
#define C_SILVER_2	36
#define C_SILVER_3	37
#define C_SILVER_4	38
#define C_SILVER_5	39
#define C_SILVER_6	40
#define C_SILVER_7	41
#define C_SILVER_8	42
#define C_SILVER_9	43

#define C_PURPLE_0	44
#define C_PURPLE_1	45
#define C_PURPLE_2	46
#define C_PURPLE_3	47
#define C_PURPLE_4	48

#define C_LIME_0	49
#define C_LIME_1	50
#define C_LIME_2	51
#define C_LIME_3	52
#define C_LIME_4	53

#define C_TAN_0		54
#define C_TAN_1		55
#define C_TAN_2		56
#define C_TAN_3		57
#define C_TAN_4		58

#define C_ORANGE_0	59
#define C_ORANGE_1	60
#define C_ORANGE_2	61
#define C_ORANGE_3	62
#define C_ORANGE_4	63


/*
 *  fixed
 */
#define RGB_BLACK	0x00, 0x00, 0x00
#define RGB_WHITE	0xff, 0xff, 0xff
#define RGB_RED		0xff, 0x00, 0x00
#define RGB_DRED	0x30, 0x00, 0x00
#define RGB_GREEN	0x00, 0xff, 0x00
#define RGB_BLUE	0x00, 0x00, 0xff
#define RGB_YELLOW	0xff, 0xff, 0x00
#define RGB_ORANGE	0xff, 0xc0, 0x00
#define RGB_INST_BROWN	0x60, 0x50, 0x40
#define RGB_HBLUE	0x50, 0x90, 0xe0
#define RGB_GREY	0x80, 0x80, 0x80

/*
 *  missile contrail
 */
#define RGB_MC_FLAME	0xff, 0x40, 0x10
#define RGB_MC_TRAIL	0xd0, 0xd0, 0xd0

/*
 * non ramp
 */
#define RGB_DIRT	0x70, 0x60, 0x30
#define RGB_SWAMP	0x50, 0x90, 0x50
#define RGB_SKY		0x50, 0xa0, 0xf0

/* ramps */
#define RGB_GREY_0	0xe0, 0xe0, 0xe0
#define RGB_GREY_1	0xd0, 0xd0, 0xd0
#define RGB_GREY_2	0xc0, 0xc0, 0xc0
#define RGB_GREY_3	0xb0, 0xb0, 0xb0
#define RGB_GREY_4	0xa0, 0xa0, 0xa0
#define RGB_GREY_5	0x90, 0x90, 0x90
#define RGB_GREY_6	0x80, 0x80, 0x80
#define RGB_GREY_7	0x70, 0x70, 0x70
#define RGB_GREY_8	0x60, 0x60, 0x60
#define RGB_GREY_9	0x50, 0x50, 0x50
#define RGB_GREY_10	0x40, 0x40, 0x40
#define RGB_GREY_11	0x30, 0x30, 0x30
#define RGB_GREY_12	0x20, 0x20, 0x20

#define RGB_SILVER_0	160, 176, 188
#define RGB_SILVER_1	150, 166, 178
#define RGB_SILVER_2	140, 156, 168
#define RGB_SILVER_3	130, 146, 158
#define RGB_SILVER_4	120, 136, 148
#define RGB_SILVER_5	110, 126, 138
#define RGB_SILVER_6	100, 116, 128
#define RGB_SILVER_7	 90, 106, 118
#define RGB_SILVER_8	 80,  96, 108
#define RGB_SILVER_9	 70,  86,  98

#define RGB_PURPLE_0	0xd0, 0x00, 0xd0
#define RGB_PURPLE_1	0xb0, 0x00, 0xb0
#define RGB_PURPLE_2	0x90, 0x00, 0x90
#define RGB_PURPLE_3	0x70, 0x00, 0x70
#define RGB_PURPLE_4	0x50, 0x00, 0x50

#define RGB_LIME_0	0xb0, 0xd0, 0xa0
#define RGB_LIME_1	0x90, 0xb0, 0x80
#define RGB_LIME_2	0x70, 0x90, 0x60
#define RGB_LIME_3	0x50, 0x70, 0x40
#define RGB_LIME_4	0x30, 0x50, 0x20

#define RGB_TAN_0	0xb0, 0x90, 0x70
#define RGB_TAN_1	0x9c, 0x7c, 0x5c
#define RGB_TAN_2	0x88, 0x68, 0x48
#define RGB_TAN_3	0x74, 0x54, 0x34
#define RGB_TAN_4	0x60, 0x40, 0x20

#define RGB_ORANGE_0	0xc8, 0x46, 0x00
#define RGB_ORANGE_1	0xa8, 0x39, 0x00
#define RGB_ORANGE_2	0x88, 0x2c, 0x00
#define RGB_ORANGE_3	0x68, 0x1f, 0x00
#define RGB_ORANGE_4	0x48, 0x12, 0x00

#define IDM_APPLICATION_EXIT (101)

#define TARGET_FPS 60
#define FRAME_TIME_MS (1000.0 / TARGET_FPS)

short rgb_table[64][3];

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

float mat_dirt[] = {AMBIENT, 0.2f, 0.15f, 0.08f,
        DIFFUSE, 0.5f, 0.4f, 0.2f,
        SPECULAR, 0.1f, 0.1f, 0.1f,
        SHININESS, 10.0f,
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

float mat_borange[] = {AMBIENT,	  0.0, 0.25, 0.9,
		       DIFFUSE,	  0.0, 0.25, 0.9,
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
			DIFFUSE,   0.1, 0.1, 0.1,
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

void load_rgb_table(int index, unsigned char r, unsigned char g, unsigned char b)
{
    set_iris_colormap(index, r/255.0f, g/255.0f, b/255.0f);
}
void init_color_tables()
{
    load_rgb_table(C_BLACK, RGB_BLACK);
    load_rgb_table(C_WHITE, RGB_WHITE);
    load_rgb_table(C_RED, RGB_RED);
    load_rgb_table(C_DRED, RGB_DRED);
    load_rgb_table(C_GREEN, RGB_GREEN);
    load_rgb_table(C_BLUE, RGB_BLUE);
    load_rgb_table(C_YELLOW, RGB_YELLOW);
    load_rgb_table(C_ORANGE, RGB_ORANGE);
    load_rgb_table(C_INST_BROWN, RGB_INST_BROWN);
    load_rgb_table(C_HBLUE, RGB_HBLUE);
    load_rgb_table(C_GREY, RGB_GREY);
    load_rgb_table(C_MC_FLAME, RGB_MC_FLAME);
    load_rgb_table(C_MC_TRAIL, RGB_MC_TRAIL);
    load_rgb_table(C_DIRT, RGB_DIRT);
    load_rgb_table(C_SWAMP, RGB_SWAMP);
    load_rgb_table(C_SKY, RGB_SKY);
    load_rgb_table(C_GREY_0, RGB_GREY_0);
    load_rgb_table(C_GREY_1, RGB_GREY_1);
    load_rgb_table(C_GREY_2, RGB_GREY_2);
    load_rgb_table(C_GREY_3, RGB_GREY_3);
    load_rgb_table(C_GREY_4, RGB_GREY_4);
    load_rgb_table(C_GREY_5, RGB_GREY_5);
    load_rgb_table(C_GREY_6, RGB_GREY_6);
    load_rgb_table(C_GREY_7, RGB_GREY_7);
    load_rgb_table(C_GREY_8, RGB_GREY_8);
    load_rgb_table(C_GREY_9, RGB_GREY_9);
    load_rgb_table(C_GREY_10, RGB_GREY_10);
    load_rgb_table(C_GREY_11, RGB_GREY_11);
    load_rgb_table(C_GREY_12, RGB_GREY_12);
    load_rgb_table(C_SILVER_0, RGB_SILVER_0);
    load_rgb_table(C_SILVER_1, RGB_SILVER_1);
    load_rgb_table(C_SILVER_2, RGB_SILVER_2);
    load_rgb_table(C_SILVER_3, RGB_SILVER_3);
    load_rgb_table(C_SILVER_4, RGB_SILVER_4);
    load_rgb_table(C_SILVER_5, RGB_SILVER_5);
    load_rgb_table(C_SILVER_6, RGB_SILVER_6);
    load_rgb_table(C_SILVER_7, RGB_SILVER_7);
    load_rgb_table(C_SILVER_8, RGB_SILVER_8);
    load_rgb_table(C_SILVER_9, RGB_SILVER_9);
    load_rgb_table(C_PURPLE_0, RGB_PURPLE_0);
    load_rgb_table(C_PURPLE_1, RGB_PURPLE_1);
    load_rgb_table(C_PURPLE_2, RGB_PURPLE_2);
    load_rgb_table(C_PURPLE_3, RGB_PURPLE_3);
    load_rgb_table(C_PURPLE_4, RGB_PURPLE_4);
    load_rgb_table(C_LIME_0, RGB_LIME_0);
    load_rgb_table(C_LIME_1, RGB_LIME_1);
    load_rgb_table(C_LIME_2, RGB_LIME_2);
    load_rgb_table(C_LIME_3, RGB_LIME_3);
    load_rgb_table(C_LIME_4, RGB_LIME_4);
    load_rgb_table(C_TAN_0, RGB_TAN_0);
    load_rgb_table(C_TAN_1, RGB_TAN_1);
    load_rgb_table(C_TAN_2, RGB_TAN_2);
    load_rgb_table(C_TAN_3, RGB_TAN_3);
    load_rgb_table(C_TAN_4, RGB_TAN_4);
    load_rgb_table(C_ORANGE_0, RGB_ORANGE_0);
    load_rgb_table(C_ORANGE_1, RGB_ORANGE_1);
    load_rgb_table(C_ORANGE_2, RGB_ORANGE_2);
    load_rgb_table(C_ORANGE_3, RGB_ORANGE_3);
    load_rgb_table(C_ORANGE_4, RGB_ORANGE_4);
}
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
        
        GLfloat light_position[] = {-5.0f, 5.0f, 5.0f, 0.0f};
        

        // Dessiner l'objet
        if (obj) {
            GLfloat light_position[] = {1.0f, 1.0f, 1.0f, 0.0f};
            glLightfv(GL_LIGHT0, GL_POSITION, light_position);
            glEnable(GL_LIGHT0);
            
            // Ambient global FAIBLE pour voir les variations d'éclairage
            GLfloat global_ambient[] = {0.1f, 0.1f, 0.1f, 1.0f};
            glLightModelfv(GL_LIGHT_MODEL_AMBIENT, global_ambient);
            
            // S'assurer que la lumière a une bonne couleur diffuse
            GLfloat light_diffuse[] = {1.0f, 1.0f, 1.0f, 1.0f};
            GLfloat light_ambient[] = {0.2f, 0.2f, 0.2f, 1.0f};
            glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
            glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
        
            glEnable(GL_NORMALIZE);
            
            drawobj(obj, 0xFFFF);

            GLfloat check_diffuse[4];
            glGetMaterialfv(GL_FRONT, GL_DIFFUSE, check_diffuse);
            printf("     OpenGL diffuse check: (%f, %f, %f, %f)\n", 
                   check_diffuse[0], check_diffuse[1], check_diffuse[2], check_diffuse[3]);
                
            GLenum error = glGetError();
            if (error != GL_NO_ERROR) {
                printf("ERROR: OpenGL error during macro test: %d\n", error);
            } else {
                printf("Macros work correctly!\n");
            }
            fflush(stdout);
        }
        
        glutSwapBuffers();
    }
}
static void initGL(void) {
    last_frame_time = clock();
    iris_init_colormap();
    init_color_tables();
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
    
    
    
    // Double buffer + RGB + z-buffer
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("Object Viewer (Libgobj) - [SPACE] auto-rotate | [R] reset | [LMB] drag | [WHEEL] zoom");

    int menu = glutCreateMenu(menu_callback);
    glutAddMenuEntry("Exit", IDM_APPLICATION_EXIT);

    glutAttachMenu(GLUT_RIGHT_BUTTON);

    initGL();
    iris_init_colormap();
    init_lighting();
    obj = readobj("f18.d");
    if (!obj) {
        fprintf(stderr, "Failed to load object file 'f18.d'\n");
        return EXIT_FAILURE;
    }
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