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
typedef int16_t Scoord;  // Short coordinate (alias for Screencoord)
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
#define GD_BLEND        4
#define GD_BITS_NORM_SNG_RED 5
#define GD_BITS_NORM_SNG_GREEN 6
#define GD_BITS_NORM_SNG_BLUE 7
#define GD_BITS_NORM_ZBUFFER 8
#define GD_LINESMOOTH_RGB 9
#define GD_BITS_NORM_DBL_RED   5
#define GD_BITS_NORM_DBL_GREEN 6
#define GD_BITS_NORM_DBL_BLUE  7
#define GD_FOGVERTEX    10  // Vertex fog support
#define GD_XPMAX        11  // Maximum X screen coordinate
#define GD_YPMAX        12  // Maximum Y screen coordinate
#define GD_BITS_NORM_DBL_CMODE 13  // Color mode bits double buffer
#define GD_MULTISAMPLE  14  // Multisampling support
#define GD_BITS_UNDR_SNG_CMODE 15  // Underlay single buffer color mode bits
#define GD_ZMAX         16
#define GD_ZMIN         17
#define GD_TEXTURE_PERSP 18

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
    GLfloat attenuation[2]; 
    Boolean local_viewer;
    Boolean two_side;
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
#define TWO_SIDE    16

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
void init_texture_system(void);
void cpack(uint32_t color);
void mapcolor(Colorindex index, RGBvalue r, RGBvalue g, RGBvalue b);
void RGBcolor(RGBvalue r, RGBvalue g, RGBvalue b);

// color() is a macro for indexed color mode
#define color(i) iris_set_color_index(i)
void iris_set_color_index(int index);

// === Primitives ===
void clear();
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

#define multmatrix(m) glMultMatrixf((float*)m)
#define translate(x, y, z) glTranslatef(x, y, z)
#define scale(x, y, z) glScalef(x, y, z)
void getmatrix(Matrix m);
void loadmatrix(Matrix m);
// rot() - IRIS GL rotation with character axis
void rot(float angle, char axis);
void rotate(Angle angle, char axis);

// Geometric primitives
void rectf(Coord x1, Coord y1, Coord x2, Coord y2);
void rectfs(Scoord x1, Scoord y1, Scoord x2, Scoord y2);  // Short coordinate version
void rect(Icoord x1, Icoord y1, Icoord x2, Icoord y2);
void circf(Coord x, Coord y, Coord radius);
void circ(Coord x, Coord y, Coord radius);
void sboxf(Coord x1, Coord y1, Coord x2, Coord y2);
void sboxfi(Icoord x1, Icoord y1, Icoord x2, Icoord y2);

// 2D polygon functions
void polf2(int32_t n, Coord parray[][2]);
void polf2i(int32_t n, Icoord parray[][2]);
void polf2s(int32_t n, Scoord parray[][2]);  // Short coordinate version

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
#define MTEXTURE    3

void resetmaterials(void);
void mmode(int mode);
long getmmode(void);
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
void winconstraints();
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
#define CURSORX         2      // Logical cursor X (alias de MOUSEX)
#define CURSORY         3      // Logical cursor Y (alias de MOUSEY)
#define SBTX            6      // Spaceball translate X
#define SBTY            7      // Spaceball translate Y
#define SBTZ            8      // Spaceball translate Z
#define SBRX            9      // Spaceball rotate X
#define SBRY            10     // Spaceball rotate Y
#define SBRZ            11     // Spaceball rotate Z
#define SBPERIOD        12     // Spaceball polling period (ms)

// Dial devices
#define DIAL0           13
#define DIAL1           14
#define DIAL2           15
#define DIAL3           16
#define DIAL4           17
#define DIAL5           18
#define DIAL6           19
#define DIAL7           20

// Additional mouse buttons
#define MOUSE1          102
#define MOUSE2          101
#define MOUSE3          100

// Spaceball buttons
#define SBPICK          24
#define SBBUT1          25
#define SBBUT2          26
#define SBBUT3          27
#define SBBUT4          28
#define SBBUT5          29
#define SBBUT6          30
#define SBBUT7          31
#define SBBUT8          32

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
#define F1KEY           115
#define F2KEY           116
#define F3KEY           117
#define F4KEY           118
#define F5KEY           119
#define F6KEY           120
#define F7KEY           121
#define F8KEY           122
#define F9KEY           123
#define F10KEY          124
#define F11KEY          125
#define F12KEY          126
#define PAUSEKEY        127
#define HOMEKEY         128
#define GKEY            139
#define PKEY            140
#define BKEY            142

// Pad buttons (game controller or dials)
#define PAD0            129
#define PAD1            130
#define PAD2            131
#define PAD3            132
#define PAD4            133
#define PAD5            134
#define PAD6            135
#define PAD7            136
#define PAD8            137
#define PAD9            138

// Screen dimension queries (returned by getgdesc)
#define XMAXSCREEN      139
#define YMAXSCREEN      140

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
void t2i(Icoord texcoord[2]);
void debug_texture_coordinates(void);
void t2s(Scoord texcoord[2]);

// === Picking/Feedback ===
void feedback(short *buffer, long size);
int endfeedback(short *buffer);
void loadname(short name);
void pushname(short name);
void popname(void);

// === Utility ===
int getgdesc(int descriptor);
#define finish() glFinish()

// === Drawing modes (IRIS GL layer modes) ===
#define NORMALDRAW      0
#define PUPDRAW         1
#define OVERDRAW        2
#define UNDERDRAW       3
#define CURSORDRAW      4

// === Fog constants ===
#define FG_OFF          0
#define FG_ON           1
#define FG_DEFINE       2
#define FG_VTX_LIN      3
#define FG_VTX_EXP      4
#define FG_VTX_EXP2     5
#define FG_PIX_LIN      6
#define FG_PIX_EXP      7
#define FG_PIX_EXP2     8

// === Graphics config constants ===
#define GLC_SLOWMAPCOLORS 100
#define GLC_OLDPOLYGON    101
#define GC_MS_SAMPLES     102  // Multisampling samples config

// === Smoothing constants ===
#define SML_OFF     0
#define SML_ON      1

// === Pattern constants ===
#define PATTERN_16  16


// === Texture constants (IRIS GL texture mapping) ===

#define TX_NULL             0
#define TX_MINFILTER        1
#define TX_BILINEAR         2
#define TX_POINT            3
#define TX_MIPMAP_LINEAR    4
#define TX_MIPMAP_POINT     5
#define TX_WRAP             6
#define TX_CLAMP            7
#define TX_MAGFILTER        8
#define TX_WRAP_S           9
#define TX_WRAP_T           10
#define TX_MIPMAP_BILINEAR  11
#define TX_MIPMAP_TRILINEAR 12


#define TX_TEXTURE_0       0
#define TX_TEXTURE_1       1
#define TX_TEXTURE_2       2
#define TX_TEXTURE_3       3
#define TX_TEXTURE_4       4

#define TV_ENV0   0
#define TV_ENV1   1
#define TV_ENV2   2
#define TV_ENV3   3
#define TV_ENV4   4

// Texture environment modes

#define TV_NULL     0
#define TV_DECAL    1
#define TV_BLEND    2
#define TV_COLOR    3
#define TV_MODULATE 4
#define TV_REPLACE  5
#define TV_SCALE    6

// Texture filter modes

// Texture wrap modes
#define TX_REPEAT  1

// Texture properties

#define MAX_TEX_ENVS 32

typedef struct {
    Boolean defined;
    int mode;           // GL_MODULATE, GL_DECAL, GL_BLEND, GL_REPLACE
    float color[4];     // Couleur pour GL_BLEND
    float scale[2];     // Facteurs d'échelle (si utilisés)
} TexEnvDef;

// === Texture Object Management ===
#define MAX_TEXTURES 256

typedef struct {
    Boolean defined;
    GLuint gl_id;
    int width;
    int height;
    int components;
} TextureDef;
// UNIX compatibility
char* iris_cuserid(char *buf);
#define cuserid(x) iris_cuserid(x)

#include <string.h>


#ifdef _WIN32
// Windows doesn't have bcopy/bzero in standard headers
inline void bcopy(const void *src, void *dst, size_t len) {
    memmove(dst, src, len);
}

inline void bzero(void *ptr, size_t len) {
    memset(ptr, 0, len);
}

inline int bcmp(const void *s1, const void *s2, size_t n) {
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

void srandom(unsigned int seed);
long random(void);
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
    PupFunc1 callback_int; // pour %F : void f(int)
    PupFunc0 callback_void;// pour %f : void f(void)
    int      has_F;
    int      has_f;
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

// === Additional IRIS GL functions ===
// Polygon drawing functions
void pmv(Coord x, Coord y, Coord z);
void pdr(Coord x, Coord y, Coord z);
void pclos(void);
void pmv2(Coord x, Coord y);
void pdr2(Coord x, Coord y);
void pmv2i(Icoord x, Icoord y);
void pdr2i(Icoord x, Icoord y);
void pmv2s(Scoord x, Scoord y);
void pdr2s(Scoord x, Scoord y);
// 2D drawing functions
void move(Coord x, Coord y, Coord z);
void draw(Coord x, Coord y, Coord z);
void move2(Coord x, Coord y);
void draw2(Coord x, Coord y);
void move2i(Icoord x, Icoord y);
void draw2i(Icoord x, Icoord y);
void move2s(Scoord x, Scoord y);
void draw2s(Scoord x, Scoord y);
void cmov2s(Scoord x, Scoord y);
void pnt2(Coord x, Coord y);
void pnt2s(Scoord x, Scoord y);
void poly2(int32_t n, Coord parray[][2]);
void recti(Icoord x1, Icoord y1, Icoord x2, Icoord y2);
void rectfi(Icoord x1, Icoord y1, Icoord x2, Icoord y2);
void rects(Scoord x1, Scoord y1, Scoord x2, Scoord y2);
void circfs(Scoord x, Scoord y, Scoord radius);

// Text functions
int strwidth(const char* str);
int getheight(void);
int getdescender(void);
void getcpos(Scoord* x, Scoord* y);

// Viewport functions
void pushviewport(void);
void popviewport(void);
void getviewport(Screencoord* left, Screencoord* right, Screencoord* bottom, Screencoord* top);
void scrmask(Screencoord left, Screencoord right, Screencoord bottom, Screencoord top);

// Line style functions
void setlinestyle(int style);
void deflinestyle(int style, unsigned short pattern);
void linesmooth(unsigned long mode);

// Coordinate mapping functions
void mapw(Object obj, Screencoord sx, Screencoord sy, 
          Coord *x1, Coord *y1, Coord *z1, 
          Coord *x2, Coord *y2, Coord *z2);
void mapw2(Screencoord sx, Screencoord sy, Coord *wx, Coord *wy);



// Cursor functions
void cursoff(void);
void curson(void);
void defcursor(int index, unsigned short cursor[16]);
void curorigin(int index, Scoord x, Scoord y);
void setcursor(int index, Colorindex color, Colorindex writemask);

// Window management functions
void noborder(void);
void icontitle(const char* title);
void mssize(int samples, int coverage, int z);
void underlay(int planes);
void overlay(int planes);
void swapinterval(int interval);
long getgconfig(int what);
void greset(void);

// Drawing mode
void drawmode(int mode);

// Viewport function (IRIS GL version - sets viewport parameters)
void viewport(Screencoord left, Screencoord right, Screencoord bottom, Screencoord top);

// Event queue functions
void qreset(void);
void setvaluator(Device dev, int16_t init, int16_t vmin, int16_t vmax);
void setbell(int volume);
void ringbell(void);
void tie(Device button, Device val1, Device val2);  // <-- Ajouter cette ligne

// Math functions
void gl_sincos(int angle_decidegrees, float* sinval, float* cosval);
float fasin(float x);
float fcos(float x);
float fsqrt(float x);
float fexp(float x);
#define _ABS(x) ((x) < 0 ? -(x) : (x))

// Fog functions
void fogvertex(int mode, float density[]);

// Texture functions
void texdef2d(int texid, int nc, int width, int height, void* image, int np, float props[]);
void tevdef(int texid, int nc, float props[]);
void texbind(int target, int texid);
void tevbind(int target, int texid);

// Color map functions
void gflush(void);
void getmcolor(Colorindex index, RGBvalue* r, RGBvalue* g, RGBvalue* b);
void glcompat(int mode, int value);
void polarview(Coord dist, Angle azim, Angle inc, Angle twist);

void lrectread(Screencoord x1, Screencoord y1, Screencoord x2, Screencoord y2, uint32_t *buffer);
void rectread(Screencoord x1, Screencoord y1, Screencoord x2, Screencoord y2, uint16_t *buffer);

#endif // IRIS2OGL_H