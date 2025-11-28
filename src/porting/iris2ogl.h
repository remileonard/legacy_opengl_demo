/*
 * iris2ogl.h - IRIS GL to OpenGL/GLUT compatibility layer
 * Provides mapping from IRIS GL API to modern OpenGL with GLUT
 */

#ifndef IRIS2OGL_H
#define IRIS2OGL_H

#ifdef _WIN32
    // Définir WIN32_LEAN_AND_MEAN pour exclure winsock.h de windows.h
    // Nous utiliserons winsock2.h depuis irix_network.h
    #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>
#endif


#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#include <stdint.h>
#include <time.h>
#include <math.h>

// Math constants
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Forward declarations
typedef int32_t Object;

// Font scaling support structure
typedef struct ScaledFont_s {
    void* base_font;
    float scale;
    int is_stroke;
} ScaledFont;
typedef int32_t Tag;
typedef int16_t Angle;
typedef int16_t Screencoord;
typedef int32_t Icoord;
typedef float Coord;
typedef float Matrix[4][4];
typedef uint16_t Colorindex;
typedef uint16_t RGBvalue;
typedef uint16_t Pattern16[16];
typedef int32_t Boolean;
typedef int16_t Device;

// IRIS GL color constants
#define BLACK           0
#define RED             1
#define GREEN           2
#define YELLOW          3
#define BLUE            4
#define MAGENTA         5
#define CYAN            6
#define WHITE           7

// Boolean values
#ifndef TRUE
#define TRUE  1
#define FALSE 0
#endif

// Blending function constants
#define BF_ZERO         0
#define BF_ONE          1
#define BF_SA           4  // Source Alpha
#define BF_MSA          5  // 1 - Source Alpha
#define BF_DC           2  // Destination Color
#define BF_SC           3  // Source Color

// Graphics descriptor constants
#define GD_TEXTURE      1
#define GD_ZBUFFER      2
#define GD_STEREO       3
#define GD_ZMIN         0.0f
#define GD_ZMAX         1.0f
#define GD_BLEND        4
#define GD_BITS_NORM_SNG_RED 5
#define GD_BITS_NORM_SNG_GREEN 6
#define GD_BITS_NORM_SNG_BLUE 7
#define GD_BITS_NORM_ZBUFFER 8
#define GD_LINESMOOTH_RGB 9
#define GD_BITS_NORM_DBL_RED   5
#define GD_BITS_NORM_DBL_GREEN 6
#define GD_BITS_NORM_DBL_BLUE  7

#define MAX_LIGHTS 8
#define MAX_MATERIALS 256

typedef struct {
    GLfloat ambient[4];
    GLfloat diffuse[4];
    GLfloat specular[4];
    GLfloat position[4];
    GLfloat spot_direction[3];
    GLfloat spot_exponent;
    GLfloat spot_cutoff;
    GLfloat attenuation[3]; // constant, linear, quadratic
    Boolean defined;
} LightDef;

typedef struct {
    GLfloat ambient[4];
    GLfloat diffuse[4];
    GLfloat specular[4];
    GLfloat emission[4];
    GLfloat shininess;
    GLfloat alpha;
    Boolean defined;
} MaterialDef;

typedef struct {
    GLfloat ambient[4];
    Boolean local_viewer;
    Boolean defined;
} LightModelDef;

// IRIS GL lighting constants
#define DEFLIGHT    100
#define DEFMATERIAL 101
#define DEFLMODEL   102

#define LMNULL      0
#define AMBIENT     1
#define DIFFUSE     2
#define SPECULAR    3
#define EMISSION    4
#define SHININESS   5
#define POSITION    6
#define SPOTDIRECTION 7
#define SPOTLIGHT   8
#define LCOLOR      9
#define MATERIAL    10
#define ALPHA       11
#define COLORINDEXES 12
#define ATTENUATION  13
#define LMODEL      14
#define LOCALVIEWER 15

#define LIGHT0      0
#define LIGHT1      1
#define LIGHT2      2
#define LIGHT3      3
#define LIGHT4      4
#define LIGHT5      5
#define LIGHT6      6
#define LIGHT7      7


// === Color Management ===
extern float iris_colormap[256][3];
void set_iris_colormap(int index, float r, float g, float b);
void iris_init_colormap(void);
void cpack(uint32_t color);
void mapcolor(Colorindex index, RGBvalue r, RGBvalue g, RGBvalue b);
void RGBcolor(RGBvalue r, RGBvalue g, RGBvalue b);

// color() is a macro for indexed color mode
#define color(i) iris_set_color_index(i)
void iris_set_color_index(int index);

// === Primitives ===
#define clear() glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)
#define zclear() glClear(GL_DEPTH_BUFFER_BIT)

// swapbuffers - swap buffers and request a new display update
void swapbuffers(void);

#define bgnpolygon() glBegin(GL_POLYGON)
#define endpolygon() glEnd()
#define bgnclosedline() glBegin(GL_LINE_LOOP)
#define endclosedline() glEnd()
#define bgnline() glBegin(GL_LINE_STRIP)
#define endline() glEnd()
#define bgnpoint() glBegin(GL_POINTS)
#define endpoint() glEnd()
#define bgntmesh() glBegin(GL_TRIANGLE_STRIP)
#define endtmesh() glEnd()
#define bgnqstrip() glBegin(GL_QUAD_STRIP)
#define endqstrip() glEnd()

// Vertex functions
#define v3f(v) glVertex3fv(v)
#define v2f(v) glVertex2fv(v)
#define v3i(v) glVertex3iv(v)
#define v2i(v) glVertex2iv(v)

// Normal functions
#define n3f(n) glNormal3fv(n)

// Matrix functions
#define pushmatrix() glPushMatrix()
#define popmatrix() glPopMatrix()
#define loadmatrix(m) glLoadMatrixf((float*)m)
#define getmatrix(m) glGetFloatv(GL_MODELVIEW_MATRIX, (float*)m)
#define multmatrix(m) glMultMatrixf((float*)m)
#define translate(x, y, z) glTranslatef(x, y, z)
#define scale(x, y, z) glScalef(x, y, z)

// rot() - IRIS GL rotation with character axis
void rot(float angle, char axis);
void rotate(Angle angle, char axis);

// Geometric primitives
void rectf(Coord x1, Coord y1, Coord x2, Coord y2);
void rect(Icoord x1, Icoord y1, Icoord x2, Icoord y2);
void circf(Coord x, Coord y, Coord radius);
void circ(Coord x, Coord y, Coord radius);
void sboxf(Coord x1, Coord y1, Coord x2, Coord y2);
void sboxfi(Icoord x1, Icoord y1, Icoord x2, Icoord y2);

// 2D polygon functions
void polf2(int32_t n, Coord parray[][2]);
void polf2i(int32_t n, Icoord parray[][2]);

// Mesh functions
void swaptmesh(void);

// === Display Lists (Objects) ===
Object genobj(void);
void makeobj(Object obj);
void closeobj(void);
void callobj(Object obj);
void delobj(Object obj);

// === Matrix modes ===
#define MSINGLE     0
#define MPROJECTION 1
#define MVIEWING    2

void resetmaterials(void);
void mmode(int mode);
void perspective(Angle fov, float aspect, Coord near, Coord far);
void ortho(Coord left, Coord right, Coord bottom, Coord top, Coord near, Coord far);
void ortho2(Coord left, Coord right, Coord bottom, Coord top);
void lookat(Coord vx, Coord vy, Coord vz, Coord px, Coord py, Coord pz, Angle twist);

// === Rendering modes ===
void shademodel(int mode);
#define FLAT    GL_FLAT
#define GOURAUD GL_SMOOTH

void zbuffer(Boolean enable);
void linewidth(int width);

// === Patterns and stippling ===
void defpattern(int id, int size, Pattern16 pattern);
void setpattern(int id);





void lmdef(int deftype, int index, int np, float props[]);
void lmbind(int target, int index);
void lmcolor(int mode);

// === Font Management ===
typedef void* fmfonthandle;

void fminit(void);
fmfonthandle fmfindfont(const char *fontname);
void fmsetfont(fmfonthandle font);
void fmprstr(const char *str);
fmfonthandle fmscalefont(fmfonthandle font, float scale);
void charstr(const char *str);

// Internal font scaling support (defined in iris2ogl_missing.c)
int is_scaled_font(fmfonthandle font, ScaledFont** out_sf);
void cmov(Coord x, Coord y, Coord z);
void cmov2(Coord x, Coord y);
void cmov2i(Icoord x, Icoord y); 

// === Window Management ===
void openwindow(void);
void winopen(const char *title);
void winclose(int win);
int winget(void);
int getwindow(void);  // Return current window ID (renamed to avoid conflict with window())
void window(Coord left, Coord right, Coord bottom, Coord top, Coord near, Coord far);  // Set projection window
void winposition(int x, int y, int width, int height);
void wintitle(const char *title);
void getsize(int *width, int *height);
void getorigin(int *x, int *y);
void reshapeviewport(void);
void keepaspect(int x, int y);
void prefposition(int x1, int y1, int x2, int y2);
void prefsize(int width, int height);
void maxsize(int width, int height);
void minsize(int width, int height);
void stepunit(int x, int y);
void foreground(void);

// Graphics configuration
void gconfig(void);
void RGBmode(void);
void cmode(void);  // Color index mode (we simulate it)
void doublebuffer(void);
void frontbuffer(Boolean enable);
void backbuffer(Boolean enable);

// Projection setup helper
void set_win_coords(void);

// === Device/Event Management ===
#define KEYBD           1      // Keyboard device
#define MOUSEX          2      // Mouse X position
#define MOUSEY          3      // Mouse Y position
#define CURSORX         4      // Logical cursor X (alias de MOUSEX)
#define CURSORY         5      // Logical cursor Y (alias de MOUSEY)
#define SBTX            6      // Spaceball translate X
#define SBTY            7      // Spaceball translate Y
#define SBTZ            8      // Spaceball translate Z
#define SBRX            9      // Spaceball rotate X
#define SBRY            10     // Spaceball rotate Y
#define SBRZ            11     // Spaceball rotate Z
#define SBPERIOD        12     // Spaceball polling period (ms)


#define LEFTMOUSE       100
#define MIDDLEMOUSE     101
#define RIGHTMOUSE      102
#define LEFTARROWKEY    103
#define RIGHTARROWKEY   104
#define UPARROWKEY      105
#define DOWNARROWKEY    106
#define ESCKEY          107
#define RETKEY          108
#define SPACEKEY        109
#define HKEY            110
#define AKEY            111
#define REDRAW          112
#define INPUTCHANGE     113
#define WINQUIT         114

void qdevice(Device dev);
void unqdevice(Device dev);
Boolean qtest(void);
int32_t qread(int16_t *val);
Boolean getbutton(Device dev);
int32_t getvaluator(Device dev);

// === Rendering State ===
void backface(Boolean enable);
void blendfunction(int sfactor, int dfactor);
void zwritemask(unsigned long mask);
void wmpack(unsigned long mask);

// === Texture coordinates ===
void t2f(float texcoord[2]);

// === Picking/Feedback ===
void feedback(short *buffer, long size);
int endfeedback(short *buffer);
void loadname(short name);
void pushname(short name);
void popname(void);

// === Utility ===
int getgdesc(int descriptor);
#define finish() glFinish()

// UNIX compatibility
char* iris_cuserid(char *buf);
#define cuserid(x) iris_cuserid(x)

#include <string.h>


#ifdef _WIN32
// Windows doesn't have bcopy/bzero in standard headers
static inline void bcopy(const void *src, void *dst, size_t len) {
    memmove(dst, src, len);
}

static inline void bzero(void *ptr, size_t len) {
    memset(ptr, 0, len);
}

static inline int bcmp(const void *s1, const void *s2, size_t n) {
    return memcmp(s1, s2, n);
}
#else
// Unix/Linux systems have bcopy/bzero/bcmp in strings.h (included by string.h)
// No need to redefine them
#include <strings.h>
#endif

// getopt compatibility for Windows
#ifdef _WIN32
extern char *optarg;
extern int optind;
int getopt(int argc, char * const argv[], const char *optstring);
#else
#include <unistd.h>
#endif

// ioctl pour Windows (stub)
#ifdef _WIN32
#define ioctl(fd, cmd, arg) (-1)
#endif

// Timing compatibility
#ifdef _WIN32
    // Windows doesn't have struct tms
    struct tms {
        clock_t tms_utime;
        clock_t tms_stime;
        clock_t tms_cutime;
        clock_t tms_cstime;
    };
    #ifndef HZ
    #define HZ 1000  // Windows clock() is in milliseconds
    #endif
#else
    #include <sys/times.h>
    #ifndef HZ
    #include <sys/param.h>  // For HZ on UNIX
    #endif
#endif

// === Compatibility types ===
typedef struct {
    Coord x, y, z;
} Point;


#define ZF_NEVER    0
#define ZF_LESS     1
#define ZF_EQUAL    2
#define ZF_LEQUAL   3
#define ZF_GREATER  4
#define ZF_NOTEQUAL 5
#define ZF_GEQUAL   6
#define ZF_ALWAYS   7
void zfunction(int func);

void czclear(unsigned long cval, unsigned long zval);
void smoothline(Boolean on);
void subpixel(Boolean on);
char *gversion(char *machinetype);

double drand48(void);
void srand48(long seedval);




// Pup menu style flags (for setpup)
#define PUP_BOX    0x0001
#define PUP_CHECK  0x0002

typedef int32_t Menu;

// Type de callback IRISGL utilisé dans flip.c : soit void f(void),
// soit void f(int) (flip passe souvent une int "n", "ls", etc.)
typedef void (*PupFunc0)(void);
typedef void (*PupFunc1)(int);

#define MAX_PUP_MENUS   64
#define MAX_PUP_ITEMS   64

typedef struct {
    char     label[128];
    int      value;        // valeur %x passée à la fonction
    int      flags;        // PUP_BOX / PUP_CHECK (visuel/logique)
} PupItem;

typedef struct {
    int       used;
    int       glut_menu_id;
    int       item_count;
    PupItem   items[MAX_PUP_ITEMS];

    // callback commun pour ce menu lorsqu'un item %F / %f est choisi
    PupFunc1  callback_int;   // utilisé quand %F est présent (void f(int))
    PupFunc0  callback_void;  // utilisé quand %f (void f(void))
    int       has_F;          // ce menu fournit une valeur %x à la fonction
    int       has_f;          // ce menu appelle juste f() sans param
} PupMenu;

// Pup menu API
Menu  newpup(void);
Menu  addtopup(Menu m, const char *label, ...);
void  setpup(Menu m, int item, int flags);
int   dopup(Menu m);
void  freepup(Menu m);



void setdepth(unsigned long znear, unsigned long zfar);
void lsetdepth(unsigned long znear, unsigned long zfar);

void qenter(Device dev, int16_t val);
void gexit(void);
#endif // IRIS2OGL_H