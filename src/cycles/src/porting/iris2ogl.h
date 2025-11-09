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

// === Color Management ===
extern float iris_colormap[256][3];

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

// === Lighting ===
#define DEFLMODEL   1
#define DEFMATERIAL 2
#define DEFLIGHT    3

#define LMNULL      0
#define EMISSION    1
#define AMBIENT     2
#define DIFFUSE     3
#define SPECULAR    4
#define SHININESS   5
#define ALPHA       6
#define COLORINDEXES 7
#define POSITION    100
#define LCOLOR      101

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

// === Window Management ===
void openwindow(void);
void winopen(const char *title);
void winclose(int win);
int getwindow(void);  // Return current window ID (renamed to avoid conflict with window())
void window(Coord left, Coord right, Coord bottom, Coord top, Coord near, Coord far);  // Set projection window
void winposition(int x, int y, int width, int height);
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

void qdevice(Device dev);
void unqdevice(Device dev);
Boolean qtest(void);
int32_t qread(int16_t *val);
Boolean getbutton(Device dev);
int32_t getvaluator(Device dev);

// === Utility ===
#define getgdesc(x) 0
#define finish() glFinish()

// UNIX compatibility
char* iris_cuserid(char *buf);
#define cuserid(x) iris_cuserid(x)

#include <string.h>

// bstring.h compatibility - only define if not available
#ifdef _WIN32
// Windows doesn't have bcopy/bzero in standard headers
static inline void bcopy(const void *src, void *dst, size_t len) {
    memmove(dst, src, len);
}

static inline void bzero(void *ptr, size_t len) {
    memset(ptr, 0, len);
}
#else
// Unix/Linux systems have bcopy/bzero in strings.h (included by string.h)
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

#endif // IRIS2OGL_H