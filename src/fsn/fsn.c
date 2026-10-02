/*
 * fsn.c - fsn, the 3D File System Navigator
 *
 * Reconstruction of SGI's "fsn" 1.2 ((c) Copyright 1992, 1993, 1996,
 * Silicon Graphics, Inc.), the file system browser seen in Jurassic Park.
 * The code was rebuilt from the IRIX 5.x MIPS executable (disassembly and
 * decompilation) and ported from IRIS GL + Motif to OpenGL 1.x + GLUT.
 *
 * Ported: file system scan, landscape layout, rendering (platforms, files,
 * carpets, lines, labels, sky, spotlights, stroke font), visibility culling,
 * picking, flying (zoom / back / reset), mouse "joystick" movement, the
 * overview window and the original resources and landscapes.
 * Not ported: Motif panels, FAM monitoring, the database cache, file typing
 * icons, warp mode, marks and search.
 *
 * Function names, resource names and default values follow the original
 * binary. See README.md in this directory.
 */
#if !defined(_WIN32)
#define _DEFAULT_SOURCE
#define _XOPEN_SOURCE 500
#endif

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#ifdef __APPLE__
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif
#include <GL/freeglut.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "fsn_font.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define FSN_PATH_MAX 4096

/* IRIS GL packed color: 0xAABBGGRR */
typedef unsigned long Cpack;

/* ------------------------------------------------------------------------ */
/* Resources (Fsn app-defaults of the original binary)                      */
/* ------------------------------------------------------------------------ */

typedef struct Resources {
    int useGouraud;
    int shrinkOnZoom;
    float mouseSpeed;
    float initialX, initialY, initialZ;
    int initialTilt;
    int maxDisplayedFileName;
    int locateHighlightThickness;
    int viewAngle;
    int minNumZoomSteps;
    int zoomSpeed;
    float heightPower;
    float maxFileHeight;
    float maxDirHeight;
    float fileBaseWidth;
    float fileMargin;
    float xDirMargin;
    float yDirMargin;
    float minFileHeight;
    float zoomBottom, zoomBack;
    int zoomTilt;
    float zoomFileBottom, zoomFileBack;
    int zoomFileTilt;
    float minFileTextXDistance, minFileTextYDistance, minFileTextZDistance;
    float minFileIconXDistance, minFileIconYDistance, minFileIconZDistance;
    float minDirTextXYDistance, minDirTextZDistance;
    float minDirLineTextXYDistance, minDirLineTextZDistance;
    float fileAsPlaneDistance, fileAsLineDistance;
    float directoryAsCarpetDistance;
    float carpetHideHeight, carpetFactor;
    float directoryHideRatio;
    float shrinkagePower, shrinkageDistance;
    float skyHeight, skyWidth, skyDistance, groundBack;
    float colorTopValueFactor, colorSideValueFactor, colorBackValueFactor;
    float colorUnselectSaturationFactor, colorUnselectValueFactor;
    float colorSelectSaturationFactor, colorSelectValueFactor;
    float colorShadowSaturationFactor, colorShadowValueFactor;
    Cpack skyColor, topSkyColor, bottomSkyColor;
    Cpack groundColor, farGroundColor, nearGroundColor;
    Cpack overviewBackgroundColor;
    Cpack dirColor, pruneColor, selLineColor, unselLineColor;
    Cpack spotlightColor;
    Cpack fileColor[7];
    int fileAge[6];
} Resources;

static Resources res;

#define UNSET_COLOR 0xffffffffUL

static Cpack parse_color(const char *s)
{
    unsigned long v = strtoul(s + 1, NULL, 16);
    return ((v >> 16) & 0xff) | (v & 0xff00) | ((v & 0xff) << 16);
}

static const char *landscape_names[] = { "grass", "indigo", "desert", "ocean", "space" };
#define NUM_LANDSCAPES 5

/* Fallback resources of the original binary */
static void set_resources(const char *landscape)
{
    int i;
    static const char *file_colors[7] = {
        "#ff0000", "#ff8400", "#fffc00", "#20a098", "#0047ff", "#a800ff", "#9a2667"
    };
    static const int file_ages[6] = { 7, 14, 31, 92, 182, 365 };

    res.useGouraud = 0;
    res.shrinkOnZoom = 1;
    res.mouseSpeed = 0.7f;
    res.initialX = 0.0f;
    res.initialY = -37.0f;
    res.initialZ = 34.0f;
    res.initialTilt = -700;
    res.maxDisplayedFileName = 16;
    res.locateHighlightThickness = 3;
    res.viewAngle = 600;
    res.minNumZoomSteps = 3;
    res.zoomSpeed = 1000000;
    res.heightPower = 0.5f;
    res.maxFileHeight = 4.0f;
    res.maxDirHeight = 2.0f;
    res.fileBaseWidth = 0.5f;
    res.fileMargin = 0.25f;
    res.xDirMargin = 2.0f;
    res.yDirMargin = 8.0f;
    res.minFileHeight = 0.15f;
    res.zoomBottom = 2.3f;
    res.zoomBack = 4.0f;
    res.zoomFileBottom = 1.0f;
    res.zoomFileBack = 1.8f;
    res.minFileTextYDistance = 4.0f;
    res.minFileTextXDistance = 3.0f;
    res.minFileTextZDistance = 8.0f;
    res.minFileIconYDistance = 6.5f;
    res.minFileIconXDistance = 6.0f;
    res.minFileIconZDistance = 15.0f;
    res.minDirLineTextXYDistance = 6.2f;
    res.minDirLineTextZDistance = 6.2f;
    res.minDirTextXYDistance = 22.0f;
    res.minDirTextZDistance = 22.0f;
    res.fileAsPlaneDistance = 18.0f;
    res.fileAsLineDistance = 30.0f;
    res.directoryAsCarpetDistance = 35.0f;
    res.carpetHideHeight = 0.5f;
    res.directoryHideRatio = 50.0f;
    res.carpetFactor = 0.66f;
    res.shrinkagePower = 1.9f;
    res.shrinkageDistance = 8.0f;
    res.skyHeight = 75.0f;
    res.skyWidth = 300.0f;
    res.skyDistance = 450.0f;
    res.groundBack = 50.0f;
    res.colorTopValueFactor = 0.8f;
    res.colorSideValueFactor = 0.55f;
    res.colorBackValueFactor = 0.3f;
    res.colorUnselectSaturationFactor = 0.8f;
    res.colorUnselectValueFactor = 0.6f;
    res.colorSelectSaturationFactor = 0.3f;
    res.colorSelectValueFactor = 1.0f;
    res.colorShadowSaturationFactor = 1.0f;
    res.colorShadowValueFactor = 0.3f;
    res.groundColor = parse_color("#238823");
    res.skyColor = parse_color("#6de1ff");
    res.dirColor = parse_color("#ffffff");
    res.pruneColor = parse_color("#7a8b84");
    res.selLineColor = parse_color("#ffffff");
    res.unselLineColor = parse_color("#c4e0d6");
    res.spotlightColor = parse_color("#fffee2");
    res.topSkyColor = res.bottomSkyColor = UNSET_COLOR;
    res.farGroundColor = res.nearGroundColor = UNSET_COLOR;
    res.overviewBackgroundColor = UNSET_COLOR;
    for (i = 0; i < 7; i++)
        res.fileColor[i] = parse_color(file_colors[i]);
    for (i = 0; i < 6; i++)
        res.fileAge[i] = file_ages[i];

    if (strcmp(landscape, "grass") == 0) {
        res.useGouraud = 1;
        res.groundColor = parse_color("#238823");
        res.skyColor = parse_color("#6de1ff");
        res.farGroundColor = parse_color("#418a59");
        res.nearGroundColor = parse_color("#0f4f1a");
        res.topSkyColor = parse_color("#0087d5");
        res.bottomSkyColor = parse_color("#91fff0");
        res.overviewBackgroundColor = parse_color("#3a7b4e");
        res.pruneColor = parse_color("#62706a");
        res.unselLineColor = parse_color("#7bc1a7");
    } else if (strcmp(landscape, "indigo") == 0) {
        res.useGouraud = 0;
        res.groundColor = parse_color("#00861f");
        res.skyColor = parse_color("#0098bf");
        res.dirColor = parse_color("#f8f8f8");
        res.colorUnselectValueFactor = 0.65f;
    } else if (strcmp(landscape, "desert") == 0) {
        res.useGouraud = 1;
        res.groundColor = parse_color("#8d7a47");
        res.skyColor = parse_color("#8de8ff");
        res.farGroundColor = parse_color("#934e28");
        res.nearGroundColor = parse_color("#d5b36b");
        res.topSkyColor = parse_color("#3916ff");
        res.bottomSkyColor = parse_color("#da756c");
        res.overviewBackgroundColor = parse_color("#705f38");
        res.pruneColor = parse_color("#989285");
    } else if (strcmp(landscape, "ocean") == 0) {
        res.useGouraud = 1;
        res.groundColor = parse_color("#002045");
        res.skyColor = parse_color("#0a1f2b");
        res.farGroundColor = parse_color("#461844");
        res.nearGroundColor = parse_color("#002045");
        res.topSkyColor = parse_color("#00005c");
        res.bottomSkyColor = parse_color("#820037");
        res.overviewBackgroundColor = parse_color("#001835");
        res.pruneColor = parse_color("#45494f");
        res.unselLineColor = parse_color("#7492b4");
    } else if (strcmp(landscape, "space") == 0) {
        res.useGouraud = 0;
        res.groundColor = parse_color("#000000");
        res.skyColor = parse_color("#000000");
    }

    /* resources whose default is another resource */
    if (res.topSkyColor == UNSET_COLOR)
        res.topSkyColor = res.skyColor;
    if (res.bottomSkyColor == UNSET_COLOR)
        res.bottomSkyColor = res.skyColor;
    if (res.farGroundColor == UNSET_COLOR)
        res.farGroundColor = res.groundColor;
    if (res.nearGroundColor == UNSET_COLOR)
        res.nearGroundColor = res.groundColor;
    if (res.overviewBackgroundColor == UNSET_COLOR)
        res.overviewBackgroundColor = res.groundColor;
    res.zoomTilt = res.initialTilt;
    res.zoomFileTilt = res.initialTilt;
}

/* ------------------------------------------------------------------------ */
/* Data structures                                                          */
/* ------------------------------------------------------------------------ */

typedef struct FsnFile {
    char *name;
    int namelen;
    double size;
    long mtime;
    float x, y;            /* position on the directory platform */
    float height;
    unsigned char age;     /* age class 0..6, selects fileColor */
    unsigned char laidout;
    unsigned char flat;    /* lower than carpetHideHeight */
    unsigned char selected;
} FsnFile;

typedef struct FsnDir {
    char *name;
    int namelen;
    int nfiles;
    FsnFile **files;
    int ndirs;
    struct FsnDir **dirs;
    double size_own;       /* sum of the file sizes */
    double size_total;     /* sum over the subtree */
    float height;
    struct FsnDir *parent, *prev, *next;
    float x, y;            /* center of the platform */
    float width;
    float children_extent;
    float subtree_extent;
    float max_child_width;
    float line_x, line_y;  /* where the line from the parent leaves it */
    float max_file_height;
    float scale;           /* lateral shrinkage */
    int index;
    int carpet;            /* dominant age class, 7 if none */
    unsigned char laidout;
    unsigned char visible;
    unsigned char selected;
    unsigned char foreign;   /* on another file system, not scanned */
} FsnDir;

/* box colors, as used by draw_box(): top, front, back, sides */
typedef struct Box {
    Cpack c[4];
} Box;

typedef struct DBox {
    Box unsel;
    Box sel;
    Cpack shadow;
    Cpack carpet;
    Cpack orig;
} DBox;

static FsnDir *topdir;
static FsnDir **dir_index;
static int num_dirs, dir_index_size;
static double maxFileSize, maxDirSize;
static float fileScale = 1.0f, dirScale = 1.0f;
static float minx, maxx, miny, maxy;
static long now_time;
static int displayHeight = 1;          /* 0 none, 1 linear, 2 exaggerated */
static int displayDirectoryHeight = 1;
static DBox boxDir;
static DBox dcolorBoxes[7];
static char topdir_path[FSN_PATH_MAX];
static int current_landscape;

/* Viewing context (curcontext in the original) */
typedef struct Context {
    float x, y, z;
    short rot, tilt, fov;  /* tenths of degrees */
    float sinh, cosh, sint, cost;
    int w, h;
    float aspect;
    FsnDir *seldir;
    FsnFile *selfile;
    int shrink;            /* files of the selected dir shrunk after zoom */
    long frame_us;
} Context;

static Context ctx;

/* ------------------------------------------------------------------------ */
/* IRIS GL helpers                                                          */
/* ------------------------------------------------------------------------ */

static void cpack(Cpack c)
{
    glColor3ub((GLubyte)(c & 0xff), (GLubyte)((c >> 8) & 0xff), (GLubyte)((c >> 16) & 0xff));
}

static void rotate_tenths(int a, char axis)
{
    if (axis == 'x')
        glRotatef(a / 10.0f, 1.0f, 0.0f, 0.0f);
    else if (axis == 'y')
        glRotatef(a / 10.0f, 0.0f, 1.0f, 0.0f);
    else
        glRotatef(a / 10.0f, 0.0f, 0.0f, 1.0f);
}

static void circf(float x, float y, float r)
{
    int i;
    glBegin(GL_POLYGON);
    for (i = 0; i < 32; i++) {
        double a = i * 2.0 * M_PI / 32.0;
        glVertex2f((float)(x + r * cos(a)), (float)(y + r * sin(a)));
    }
    glEnd();
}

static void setpattern(int on)
{
    static GLubyte halftone[128];
    static int init = 0;
    if (!init) {
        int i;
        for (i = 0; i < 128; i++)
            halftone[i] = ((i / 4) & 1) ? 0xaa : 0x55;
        init = 1;
    }
    if (on) {
        glPolygonStipple(halftone);
        glEnable(GL_POLYGON_STIPPLE);
    } else {
        glDisable(GL_POLYGON_STIPPLE);
    }
}

/* text: draws a string with the stroke font of the original binary */
static void text(const char *s)
{
    for (; *s; s++) {
        unsigned char c = (unsigned char)*s;
        const signed char (*cmd)[3];
        if (c >= 128)
            c = '?';
        for (cmd = fsn_chrtbl[c]; (*cmd)[0] != 0; cmd++) {
            switch ((*cmd)[0]) {
            case 1:
                glTranslatef((float)(*cmd)[1], (float)(*cmd)[2], 0.0f);
                break;
            case 2:
                glBegin(GL_LINE_STRIP);
                glVertex2i((*cmd)[1], (*cmd)[2]);
                break;
            case 3:
                glVertex2i((*cmd)[1], (*cmd)[2]);
                break;
            case 4:
                glVertex2i((*cmd)[1], (*cmd)[2]);
                glEnd();
                break;
            }
        }
    }
}

static double time_us(void)
{
#ifdef _WIN32
    return (double)GetTickCount() * 1000.0;
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec * 1000000.0 + (double)tv.tv_usec;
#endif
}

/* ------------------------------------------------------------------------ */
/* Colors (makeColorBox / makeDColorBox)                                    */
/* ------------------------------------------------------------------------ */

static void rgb_to_hsv(float r, float g, float b, float *h, float *s, float *v)
{
    float mx = r > g ? (r > b ? r : b) : (g > b ? g : b);
    float mn = r < g ? (r < b ? r : b) : (g < b ? g : b);
    float d = mx - mn;
    *v = mx;
    *s = mx > 0.0f ? d / mx : 0.0f;
    if (d <= 0.0f) {
        *h = 0.0f;
        return;
    }
    if (r == mx)
        *h = (g - b) / d;
    else if (g == mx)
        *h = 2.0f + (b - r) / d;
    else
        *h = 4.0f + (r - g) / d;
    *h *= 60.0f;
    if (*h < 0.0f)
        *h += 360.0f;
}

static void hsv_to_rgb(float h, float s, float v, float *r, float *g, float *b)
{
    int i;
    float f, p, q, t;
    if (s > 1.0f)
        s = 1.0f;
    if (v > 255.0f)
        v = 255.0f;
    if (s <= 0.0f) {
        *r = *g = *b = v;
        return;
    }
    h /= 60.0f;
    i = (int)floor(h);
    f = h - i;
    p = v * (1.0f - s);
    q = v * (1.0f - s * f);
    t = v * (1.0f - s * (1.0f - f));
    switch (i % 6) {
    case 0: *r = v; *g = t; *b = p; break;
    case 1: *r = q; *g = v; *b = p; break;
    case 2: *r = p; *g = v; *b = t; break;
    case 3: *r = p; *g = q; *b = v; break;
    case 4: *r = t; *g = p; *b = v; break;
    default: *r = v; *g = p; *b = q; break;
    }
}

static Cpack hsv_cpack(float h, float s, float v)
{
    float r, g, b;
    hsv_to_rgb(h, s, v, &r, &g, &b);
    return (Cpack)(int)r | ((Cpack)(int)g << 8) | ((Cpack)(int)b << 16);
}

static void cpack_hsv(Cpack c, float *h, float *s, float *v)
{
    rgb_to_hsv((float)(c & 0xff), (float)((c >> 8) & 0xff), (float)((c >> 16) & 0xff), h, s, v);
}

static void makeColorBox(Cpack c, Box *b)
{
    float h, s, v;
    cpack_hsv(c, &h, &s, &v);
    b->c[1] = c;
    b->c[0] = hsv_cpack(h, s, v * res.colorTopValueFactor);
    b->c[3] = hsv_cpack(h, s, v * res.colorSideValueFactor);
    b->c[2] = hsv_cpack(h, s, v * res.colorBackValueFactor);
}

static void makeDColorBox(Cpack c, DBox *b)
{
    float h, s, v;
    int k;
    cpack_hsv(c, &h, &s, &v);
    makeColorBox(hsv_cpack(h, s * res.colorSelectSaturationFactor,
                           v * res.colorSelectValueFactor), &b->sel);
    makeColorBox(hsv_cpack(h, s * res.colorUnselectSaturationFactor,
                           v * res.colorUnselectValueFactor), &b->unsel);
    b->shadow = hsv_cpack(h, s * res.colorShadowSaturationFactor,
                          v * res.colorShadowValueFactor);
    /* carpet: (directory top + 2 * file top) / 3 */
    b->carpet = 0;
    for (k = 0; k < 24; k += 8) {
        unsigned long d = (boxDir.unsel.c[0] >> k) & 0xff;
        unsigned long f = (b->unsel.c[0] >> k) & 0xff;
        b->carpet |= ((d + 2 * f) / 3) << k;
    }
    b->orig = c;
}

static void makeColorBoxes(void)
{
    int i;
    makeDColorBox(res.dirColor, &boxDir);
    for (i = 0; i < 7; i++)
        makeDColorBox(res.fileColor[i], &dcolorBoxes[i]);
}

/* ------------------------------------------------------------------------ */
/* File system scan                                                         */
/* ------------------------------------------------------------------------ */

static char *xstrdup(const char *s)
{
    size_t n = strlen(s) + 1;
    char *p = (char *)malloc(n);
    if (p)
        memcpy(p, s, n);
    return p;
}

static FsnDir *new_dir(const char *name, FsnDir *parent)
{
    FsnDir *d = (FsnDir *)calloc(1, sizeof(FsnDir));
    d->name = xstrdup(name);
    d->namelen = (int)strlen(name);
    d->parent = parent;
    d->carpet = 7;
    d->scale = 1.0f;
    if (num_dirs == dir_index_size) {
        dir_index_size = dir_index_size ? dir_index_size * 2 : 256;
        dir_index = (FsnDir **)realloc(dir_index, dir_index_size * sizeof(FsnDir *));
    }
    d->index = num_dirs;
    dir_index[num_dirs++] = d;
    return d;
}

static void add_file(FsnDir *d, const char *name, double size, long mtime)
{
    FsnFile *f = (FsnFile *)calloc(1, sizeof(FsnFile));
    f->name = xstrdup(name);
    f->namelen = (int)strlen(name);
    f->size = size;
    f->mtime = mtime;
    d->files = (FsnFile **)realloc(d->files, (d->nfiles + 1) * sizeof(FsnFile *));
    d->files[d->nfiles++] = f;
    if (maxFileSize < size)
        maxFileSize = size;
}

static FsnDir *add_subdir(FsnDir *d, const char *name)
{
    FsnDir *s = new_dir(name, d);
    d->dirs = (FsnDir **)realloc(d->dirs, (d->ndirs + 1) * sizeof(FsnDir *));
    d->dirs[d->ndirs++] = s;
    return s;
}

static int compare_files(const void *a, const void *b)
{
    return strcmp((*(FsnFile * const *)a)->name, (*(FsnFile * const *)b)->name);
}

static int compare_dirs(const void *a, const void *b)
{
    return strcmp((*(FsnDir * const *)a)->name, (*(FsnDir * const *)b)->name);
}

static int scanned_count;

/* reads one directory, then its subdirectories; path holds its full path */
static void scan_dir(FsnDir *d, char *path, size_t len, unsigned long dev)
{
    int i;
#ifdef _WIN32
    WIN32_FIND_DATAA fd;
    HANDLE h;
    (void)dev;
    if (len + 3 >= FSN_PATH_MAX)
        return;
    strcpy(path + len, "\\*");
    h = FindFirstFileA(path, &fd);
    path[len] = '\0';
    if (h == INVALID_HANDLE_VALUE)
        return;
    do {
        double size;
        ULARGE_INTEGER t;
        if (strcmp(fd.cFileName, ".") == 0 || strcmp(fd.cFileName, "..") == 0)
            continue;
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT))
                add_subdir(d, fd.cFileName);
            continue;
        }
        size = (double)fd.nFileSizeHigh * 4294967296.0 + (double)fd.nFileSizeLow;
        t.LowPart = fd.ftLastWriteTime.dwLowDateTime;
        t.HighPart = fd.ftLastWriteTime.dwHighDateTime;
        add_file(d, fd.cFileName, size,
                 (long)((double)t.QuadPart / 10000000.0 - 11644473600.0));
    } while (FindNextFileA(h, &fd));
    FindClose(h);
#else
    DIR *dp = opendir(path);
    struct dirent *de;
    struct stat st;
    if (dp == NULL)
        return;
    while ((de = readdir(dp)) != NULL) {
        size_t nl = strlen(de->d_name);
        if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0)
            continue;
        if (len + nl + 2 >= FSN_PATH_MAX)
            continue;
        path[len] = '/';
        memcpy(path + len + 1, de->d_name, nl + 1);
        if (lstat(path, &st) < 0) {
            path[len] = '\0';
            continue;
        }
        path[len] = '\0';
        if (S_ISDIR(st.st_mode)) {
            FsnDir *s = add_subdir(d, de->d_name);
            /* other file systems are shown but not entered */
            if ((unsigned long)st.st_dev != dev)
                s->foreign = 1;
        } else {
            add_file(d, de->d_name, (double)st.st_size, (long)st.st_mtime);
        }
    }
    closedir(dp);
#endif
    if (d->nfiles > 1)
        qsort(d->files, d->nfiles, sizeof(FsnFile *), compare_files);
    if (d->ndirs > 1)
        qsort(d->dirs, d->ndirs, sizeof(FsnDir *), compare_dirs);
    if (++scanned_count % 500 == 0) {
        printf("fsn: scanned %d directories\r", scanned_count);
        fflush(stdout);
    }
    for (i = 0; i < d->ndirs; i++) {
        FsnDir *s = d->dirs[i];
        size_t nl = strlen(s->name);
        if (s->foreign || len + nl + 2 >= FSN_PATH_MAX)
            continue;
#ifdef _WIN32
        path[len] = '\\';
#else
        path[len] = '/';
#endif
        memcpy(path + len + 1, s->name, nl + 1);
        scan_dir(s, path, len + 1 + nl, dev);
        path[len] = '\0';
    }
}

static void initialize_db(const char *dirname)
{
    char path[FSN_PATH_MAX];
    unsigned long dev = 0;
    size_t len;
#ifndef _WIN32
    struct stat st;
    if (lstat(dirname, &st) < 0 || !S_ISDIR(st.st_mode)) {
        fprintf(stderr, "%s must be a directory\n", dirname);
        exit(1);
    }
    dev = (unsigned long)st.st_dev;
#endif
    strncpy(path, dirname, FSN_PATH_MAX - 1);
    path[FSN_PATH_MAX - 1] = '\0';
    len = strlen(path);
    while (len > 1 && (path[len - 1] == '/' || path[len - 1] == '\\'))
        path[--len] = '\0';
    strcpy(topdir_path, path);
    printf("fsn: scanning %s...\n", path);
    topdir = new_dir(path, NULL);
    scan_dir(topdir, path, len, dev);
    printf("fsn: %d directories                \n", num_dirs);
}

/* dirToPath: full path of a directory, with a trailing separator */
static void dirToPath(char *buf, FsnDir *d)
{
    size_t n;
    if (d->parent)
        dirToPath(buf, d->parent);
    else
        buf[0] = '\0';
    if (strlen(buf) + d->namelen + 2 >= FSN_PATH_MAX)
        return;
    strcat(buf, d->name);
    n = strlen(buf);
    if (n == 0 || buf[n - 1] != '/') {
        buf[n] = '/';
        buf[n + 1] = '\0';
    }
}

/* ------------------------------------------------------------------------ */
/* Layout                                                                   */
/* ------------------------------------------------------------------------ */

static float height_of(double n)
{
    switch (displayHeight) {
    case 0:
        return 0.0f;
    case 2:
        return (float)pow(n, res.heightPower);
    default:
        return (float)n;
    }
}

static float file_height(FsnFile *f)
{
    float h;
    if (displayHeight == 0)
        return res.minFileHeight;
    h = height_of(f->size) * fileScale;
    return h < res.minFileHeight ? res.minFileHeight : h;
}

static float dir_height(FsnDir *d)
{
    if (displayHeight == 0 || displayDirectoryHeight == 0)
        return 0.0f;
    if (displayDirectoryHeight == 2)
        return height_of(d->size_total) * dirScale;
    return height_of(d->size_own) * dirScale;
}

static void calculate_file_view(FsnFile *f)
{
    long days;
    int i;
    f->height = file_height(f);
    f->flat = f->height < res.carpetHideHeight;
    days = (now_time - f->mtime) / 86400L;
    for (i = 0; i < 6 && days >= res.fileAge[i]; i++)
        ;
    f->age = (unsigned char)i;
}

/* dominant age class of a directory, used to draw it as a carpet */
static void compute_carpet(FsnDir *d)
{
    int count[7];
    int i, threshold;
    memset(count, 0, sizeof(count));
    for (i = 0; i < d->nfiles; i++)
        count[d->files[i]->age]++;
    threshold = (int)(d->nfiles * res.carpetFactor);
    d->carpet = 7;
    for (i = 0; i < 7; i++) {
        if (threshold < count[i]) {
            d->carpet = i;
            break;
        }
    }
}

/* first_traversal: grid of files, directory widths and sizes */
static void first_traversal(FsnDir *d)
{
    int side, col, i;
    float cell, start, x, y;
    FsnDir *prev = NULL;

    d->laidout = 1;
    side = d->nfiles ? (int)sqrt((double)(d->nfiles - 1)) + 1 : 0;
    cell = res.fileBaseWidth + 2.0f * res.fileMargin;
    d->width = side ? side * cell : cell;
    start = (float)((1 - side) * cell * 0.5);
    d->size_own = 0.0;
    d->max_file_height = 0.0f;
    x = y = start;
    col = 0;
    for (i = 0; i < d->nfiles; i++) {
        FsnFile *f = d->files[i];
        col++;
        f->laidout = 1;
        d->size_own += f->size;
        f->x = x;
        f->y = y;
        if (col >= side) {
            x = start;
            col = 0;
            y += cell;
        } else {
            x += cell;
        }
        calculate_file_view(f);
        if (d->max_file_height < f->height)
            d->max_file_height = f->height;
    }
    d->size_total = d->size_own;
    compute_carpet(d);
    if (maxDirSize < d->size_own)
        maxDirSize = d->size_own;
    d->max_child_width = 0.0f;
    for (i = 0; i < d->ndirs; i++) {
        FsnDir *s = d->dirs[i];
        first_traversal(s);
        if (d->max_child_width < s->width)
            d->max_child_width = s->width;
        s->prev = prev;
        s->next = NULL;
        if (prev)
            prev->next = s;
        d->size_total += s->size_total;
        prev = s;
    }
}

/* second_traversal: depth position, shrinkage and lateral extents */
static float second_traversal(FsnDir *d, double y, double scale)
{
    int i;
    double cy, child_scale;
    float child_y, w;

    cy = y + d->width / 2.0;
    d->height = dir_height(d);
    d->y = (float)cy;
    d->scale = (float)scale;
    d->children_extent = 0.0f;
    child_y = (float)(d->width / 2.0 + cy + res.yDirMargin);
    child_scale = 1.0 / (float)pow(res.shrinkagePower, (float)(child_y / res.shrinkageDistance));
    for (i = 0; i < d->ndirs; i++)
        d->children_extent += second_traversal(d->dirs[i], child_y, child_scale);
    w = (float)(d->namelen * 1.2);
    if (w < d->width)
        w = d->width;
    w = (float)((w + res.xDirMargin) * d->scale);
    d->subtree_extent = d->children_extent;
    if (d->subtree_extent < w)
        d->subtree_extent = w;
    return d->subtree_extent;
}

/* spreads n children along an edge of their parent */
static void set_line_starts(FsnDir *d, int first, int count, int dir, int edge)
{
    double half = d->width / 2.0;
    float step = (float)(d->width / (count + 1));
    float pos = (float)(-d->width / 2.0 + step);
    int k;
    for (k = 0; k < count; k++) {
        FsnDir *c = d->dirs[first + dir * k];
        if (edge == 0) {          /* left edge */
            c->line_y = pos;
            c->line_x = (float)(-half * d->scale);
        } else if (edge == 1) {   /* right edge */
            c->line_y = pos;
            c->line_x = (float)(half * d->scale);
        } else {                  /* back edge */
            c->line_y = (float)half;
            c->line_x = (float)(pos * d->scale);
        }
        pos = (float)(pos + step);
    }
}

/* third_traversal: lateral position and where the lines leave the parent */
static void third_traversal(FsnDir *d, double x)
{
    int i, nleft, nright;
    double half = d->width / 2.0, cur;

    d->height = dir_height(d);
    d->x = (float)x;
    if (d->x - half < minx)
        minx = (float)(d->x - half);
    else if (maxx < d->x + half)
        maxx = (float)(d->x + half);
    if (maxy < d->y + half)
        maxy = (float)(d->y + half);

    cur = x - d->children_extent / 2.0;
    for (i = 0; i < d->ndirs; i++) {
        FsnDir *c = d->dirs[i];
        cur += c->subtree_extent / 2.0;
        third_traversal(c, cur);
        cur += c->subtree_extent / 2.0;
    }
    if (d->ndirs == 0)
        return;

    /* children far to the left leave from the left edge */
    nleft = 0;
    while (nleft < d->ndirs) {
        FsnDir *c = d->dirs[nleft];
        if (!(c->y - d->y < (d->x - c->x) / d->scale))
            break;
        nleft++;
    }
    set_line_starts(d, 0, nleft, 1, 0);
    /* children far to the right leave from the right edge */
    nright = 0;
    while (nright < d->ndirs) {
        FsnDir *c = d->dirs[d->ndirs - 1 - nright];
        if (!(c->y - d->y < (c->x - d->x) / d->scale))
            break;
        nright++;
    }
    set_line_starts(d, d->ndirs - 1, nright, -1, 1);
    /* the others leave from the back edge */
    set_line_starts(d, nleft, d->ndirs - nleft - nright, 1, 2);
}

static void layout_db(void)
{
    float h;
    now_time = (long)time(NULL);
    miny = maxy = minx = maxx = 0.0f;
    if (displayHeight == 0) {
        fileScale = 1.0f;
    } else {
        h = height_of(maxFileSize);
        fileScale = h > 0.0f ? res.maxFileHeight / h : 1.0f;
    }
    topdir->prev = topdir->next = NULL;
    first_traversal(topdir);
    if (displayHeight == 0) {
        fileScale = 1.0f;
    } else if (displayDirectoryHeight == 0) {
        dirScale = 1.0f;
    } else {
        h = height_of(displayDirectoryHeight == 2 ? topdir->size_total : maxDirSize);
        dirScale = h > 0.0f ? res.maxDirHeight / h : 1.0f;
    }
    second_traversal(topdir, 0.0,
                     1.0 / (float)pow(res.shrinkagePower,
                                (float)(topdir->width / 2.0 / res.shrinkageDistance)));
    third_traversal(topdir, 0.0);
    maxx += 10.0f;
    minx -= 10.0f;
    maxy += 10.0f;
    miny = (float)(-topdir->width / 2.0 - 10.0);
}

/* ------------------------------------------------------------------------ */
/* Context                                                                  */
/* ------------------------------------------------------------------------ */

static void calc_h_angle(void)
{
    float a = (float)(ctx.rot / 3600.0 * 2.0 * M_PI);
    ctx.sinh = (float)sin(a);
    ctx.cosh = (float)cos(a);
}

static void calc_v_angle(void)
{
    float a = (float)(ctx.tilt / 3600.0 * 2.0 * M_PI);
    ctx.sint = (float)sin(a);
    ctx.cost = (float)cos(a);
}

static float eye_shrinkage(void)
{
    return (float)pow(res.shrinkagePower,
                (float)((ctx.y - ctx.cosh * ctx.sint * ctx.z) / res.shrinkageDistance));
}

/* modelview set-up shared by drawing, visibility and picking */
static void view_transform(void)
{
    float s;
    glScalef(1.0f / ctx.aspect, 1.0f, 1.0f);
    rotate_tenths(ctx.tilt, 'x');
    rotate_tenths(ctx.rot, 'z');
    s = eye_shrinkage();
    glScalef(s, 1.0f, 1.0f);
    glTranslatef(-ctx.x, -ctx.y, -ctx.z);
}

static void do_perspective(void)
{
    gluPerspective(ctx.fov / 10.0, 1.0, 0.05, 500.0);
}

/* ------------------------------------------------------------------------ */
/* Drawing                                                                  */
/* ------------------------------------------------------------------------ */

static const float box_verts[8][3] = {
    { -0.5f, -0.5f, 0.0f }, { 0.5f, -0.5f, 0.0f }, { 0.5f, 0.5f, 0.0f }, { -0.5f, 0.5f, 0.0f },
    { -0.5f, -0.5f, 1.0f }, { 0.5f, -0.5f, 1.0f }, { 0.5f, 0.5f, 1.0f }, { -0.5f, 0.5f, 1.0f }
};
/* faces: front, left, right, back, top, bottom */
static const int box_faces[6][4] = {
    { 0, 1, 5, 4 }, { 3, 0, 4, 7 }, { 1, 2, 6, 5 }, { 2, 3, 7, 6 }, { 4, 5, 6, 7 }, { 0, 1, 2, 3 }
};
static const int box_diag[4] = { 0, 1, 6, 7 };

static void box_face(const int *f, int outline)
{
    int i;
    glBegin(outline ? GL_LINE_LOOP : GL_QUADS);
    for (i = 0; i < 4; i++)
        glVertex3fv(box_verts[f[i]]);
    glEnd();
}

/*
 * draw_box: unit box from (-.5,-.5,0) to (.5,.5,1).
 * mode 0 filled faces, 1 outline, 2 front face only, 3 vertical line,
 * 4 diagonal quad (picking / visibility).
 * mask: 1 front, 2 left, 4 right, 8 back, 16 top, 32 bottom.
 */
static void draw_box(const Box *b, int mode, int mask)
{
    int use_colors = (mode != 1) && b != NULL;
    if (mode == 4) {
        if (b)
            cpack(b->c[0]);
        box_face(box_diag, 0);
        return;
    }
    if (mode == 3) {
        if (b)
            cpack(b->c[0]);
        glBegin(GL_LINES);
        glVertex3f(0.0f, 0.0f, 0.0f);
        glVertex3f(0.0f, 0.0f, 1.0f);
        glEnd();
        return;
    }
    if (mode == 2) {
        if (b)
            cpack(b->c[0]);
        box_face(box_faces[0], 0);
        return;
    }
    if (mode == 1 && b)
        cpack(b->c[1]);
    if (mask & 1) {
        if (use_colors)
            cpack(b->c[1]);
        box_face(box_faces[0], mode == 1);
    }
    if (use_colors)
        cpack(b->c[3]);
    if (mask & 2)
        box_face(box_faces[1], mode == 1);
    if (mask & 4)
        box_face(box_faces[2], mode == 1);
    if (mask & 8) {
        if (use_colors)
            cpack(b->c[2]);
        box_face(box_faces[3], mode == 1);
    }
    if (mask & 16) {
        if (use_colors)
            cpack(b->c[0]);
        box_face(box_faces[4], mode == 1);
    }
    if (mask & 32)
        box_face(box_faces[5], mode == 1);
}

static const Box *file_box(FsnFile *f)
{
    return f->selected ? &dcolorBoxes[f->age].sel : &dcolorBoxes[f->age].unsel;
}

static const Box *dir_box(FsnDir *d)
{
    return d->selected ? &boxDir.sel : &boxDir.unsel;
}

/* spotlight: a light cone from far behind the landscape onto a spot */
static void spotlight(float ax, float ay, float az, float cx, float cy, float cz,
                      float r, int picking, Cpack color)
{
    float apex[3], p1[3], p2[3];
    apex[0] = ax; apex[1] = ay; apex[2] = az;
    p1[0] = cx - r; p1[1] = cy; p1[2] = cz;
    p2[0] = cx + r; p2[1] = cy; p2[2] = cz;
    if (picking) {
        glBegin(GL_LINE_STRIP);
        glVertex3fv(p1); glVertex3fv(apex); glVertex3fv(p2);
        glEnd();
        return;
    }
    cpack(color);
    glPushMatrix();
    glTranslatef(0.0f, 0.0f, cz + 0.01f);
    circf(cx, cy, r);
    glPopMatrix();
    glEnable(GL_LINE_SMOOTH);
    glBegin(GL_LINE_STRIP);
    glVertex3fv(p1); glVertex3fv(apex); glVertex3fv(p2);
    glEnd();
    glDisable(GL_LINE_SMOOTH);
    setpattern(1);
    glBegin(GL_TRIANGLES);
    glVertex3fv(p1); glVertex3fv(apex); glVertex3fv(p2);
    glEnd();
    setpattern(0);
}

/* draw_file_pointers: arrows between the last file of a row and the next */
static void draw_file_pointers(FsnDir *d, int i, int j)
{
    FsnFile *a = d->files[i], *b = d->files[j];
    float step = res.fileBaseWidth + res.fileMargin;
    glLoadName(j);
    glPushMatrix();
    glTranslatef(a->x + step, a->y, 0.0f);
    glScalef(res.fileBaseWidth, res.fileBaseWidth, 1.0f);
    cpack(file_box(b)->c[0]);
    glBegin(GL_TRIANGLES);
    glVertex2f(-0.5f, -0.5f); glVertex2f(-0.5f, 0.5f); glVertex2f(0.5f, 0.0f);
    glEnd();
    glPopMatrix();
    glLoadName(i);
    glPushMatrix();
    glTranslatef(b->x - step, b->y, 0.0f);
    glScalef(res.fileBaseWidth, res.fileBaseWidth, 1.0f);
    cpack(file_box(a)->c[0]);
    glBegin(GL_TRIANGLES);
    glVertex2f(0.5f, -0.5f); glVertex2f(0.5f, 0.5f); glVertex2f(-0.5f, 0.0f);
    glEnd();
    glPopMatrix();
}

static void draw_file(FsnDir *d, FsnFile *f, int showtext, int mode, int mask)
{
    int shrink;
    float h;
    if (!f->laidout)
        return;
    shrink = ctx.shrink && d->selected && res.shrinkOnZoom;
    h = shrink ? res.minFileHeight : f->height;
    glPushMatrix();
    glTranslatef(f->x, f->y, 0.0f);
    if (shrink && res.minFileHeight < f->height) {
        /* thin marker showing the real height of a shrunk file */
        glPushMatrix();
        glScalef(res.fileBaseWidth / 5.0f, res.fileBaseWidth / 5.0f, f->height);
        glTranslatef(-2.0f, 2.0f, 0.0f);
        draw_box(file_box(f), mode, mask);
        glPopMatrix();
    }
    if (showtext) {
        int n = f->namelen < res.maxDisplayedFileName ? f->namelen : res.maxDisplayedFileName;
        cpack(0);
        glPushMatrix();
        glTranslatef((float)(n * -0.03), (float)(-res.fileBaseWidth / 2.0 - 0.2), 0.03f);
        rotate_tenths(-ctx.tilt, 'x');
        glScalef(0.01f, 0.01f, 1.0f);
        if (res.maxDisplayedFileName < f->namelen)
            glScalef((float)res.maxDisplayedFileName / (float)f->namelen, 1.0f, 1.0f);
        text(f->name);
        glPopMatrix();
    }
    glScalef(res.fileBaseWidth, res.fileBaseWidth, h);
    draw_box(file_box(f), mode, mask);
    glPopMatrix();
}

static void draw_files(FsnDir *d, int picking, int carpet, int mask)
{
    int i;
    float dz = ctx.z - d->height;
    float ex = ctx.x - ctx.sinh * ctx.sint * dz;
    float ey = ctx.y - ctx.cosh * ctx.sint * dz;

    for (i = 0; i < d->nfiles; i++) {
        FsnFile *f = d->files[i];
        float dx, dy;
        int showtext = 0, mode, fmask = mask;

        /* small files of the carpet color are part of the carpet */
        if (!picking && f->age == carpet && f->flat)
            continue;
        glPushName(i);
        dx = (float)fabs((ex - d->x) / d->scale - f->x);
        dy = (float)fabs((ey - d->y) - f->y);
        if (picking)
            fmask = 0x1f;
        if (!picking)
            showtext = ctx.z < res.minFileTextZDistance + d->height &&
                       dx < res.minFileTextXDistance && dy < res.minFileTextYDistance;
        if (picking)
            mode = 4;
        else if (res.fileAsLineDistance < dy)
            mode = 3;
        else if (res.fileAsPlaneDistance < dy)
            mode = 2;
        else
            mode = 0;
        draw_file(d, f, showtext, mode, fmask);
        if (f->selected) {
            glPushMatrix();
            glTranslatef(-d->x, -d->y, -d->height);
            spotlight(0.0f, maxy * 4.0f, maxx - minx, d->x + f->x, d->y + f->y, d->height,
                      res.fileBaseWidth / 2.0f + res.fileMargin, picking, res.spotlightColor);
            glPopMatrix();
            if (i > 0 && f->x < d->files[i - 1]->x)
                draw_file_pointers(d, i - 1, i);
            if (i < d->nfiles - 1 && d->files[i + 1]->x < f->x)
                draw_file_pointers(d, i, i + 1);
        }
        glPopName();
    }
}

static void draw_directory(FsnDir *d, int picking)
{
    int i, carpet = 7, mask;
    float dist, half, xs;
    float p1[3], p2[3];

    if (!d->laidout)
        return;
    dist = (float)fabs((ctx.y - ctx.cosh * ctx.sint * (ctx.z - d->height)) - d->y);
    p1[2] = p2[2] = -0.05f;

    if (d->visible || d->selected) {
        half = d->width / 2.0f;
        xs = half * d->scale;
        /* faces turned toward the eye */
        mask = 0x10;
        if (ctx.x <= d->x + xs)
            mask |= 2;
        if (d->x - xs <= ctx.x)
            mask |= 4;
        if (ctx.y <= d->y + half)
            mask |= 1;
        if (d->y - half <= ctx.y)
            mask |= 8;

        glPushName(d->index);
        glPushMatrix();
        glTranslatef(d->x, d->y, 0.0f);
        glScalef(d->scale, 1.0f, 1.0f);

        if (res.directoryAsCarpetDistance < dist && !d->selected)
            carpet = d->carpet;
        glPushMatrix();
        if (picking)
            glScalef(d->width, d->width, d->height + d->max_file_height);
        else
            glScalef(d->width, d->width, d->height);
        if (picking) {
            if (!d->selected)
                draw_box(NULL, 0, 0x3f);
        } else if (carpet == 7) {
            draw_box(dir_box(d), 0, d->height > 0.0f ? mask : 0x10);
        } else {
            if (d->height > 0.0f)
                draw_box(dir_box(d), 0, mask & ~0x10);
            cpack(dcolorBoxes[carpet].carpet);
            draw_box(NULL, 0, 0x10);
        }
        glPopMatrix();

        /* directory name, flat in front of the platform */
        if (ctx.z < res.minDirTextZDistance + d->height && dist < res.minDirTextXYDistance) {
            glPushMatrix();
            glTranslatef((float)(d->namelen * -0.6), (float)(-d->width / 2.0 - 2.0), 0.0f);
            glScalef(0.2f, 0.2f, 1.0f);
            cpack(d->selected ? res.selLineColor : res.unselLineColor);
            text(d->name);
            glPopMatrix();
        }

        glPushMatrix();
        glTranslatef(0.0f, 0.0f, d->height);
        if (!picking)
            draw_files(d, 0, carpet, mask);
        glPopMatrix();
        glPopMatrix();
        glPopName();
    }

    for (i = 0; i < d->ndirs; i++) {
        FsnDir *c = d->dirs[i];
        if (!c->laidout)
            continue;
        draw_directory(c, picking);
        glPushName(c->index);
        cpack(c->selected ? res.selLineColor : res.unselLineColor);
        p1[0] = d->x + c->line_x;
        p1[1] = d->y + c->line_y;
        p2[0] = c->x;
        p2[1] = c->y - c->width / 2.0f;
        if (c->selected)
            glLineWidth(3.0f);
        glBegin(GL_LINES);
        glVertex3fv(p1);
        glVertex3fv(p2);
        glEnd();
        if (c->selected)
            glLineWidth(1.0f);
        /* name along the line */
        if (!picking && (d->selected || c->selected) &&
            ctx.z < res.minDirLineTextZDistance + d->height &&
            dist < res.minDirLineTextXYDistance) {
            float ang = (float)atan2(p2[1] - p1[1], (p2[0] - p1[0]) / d->scale);
            glPushMatrix();
            glTranslatef(p1[0], p1[1], 0.0f);
            glScalef(d->scale, 1.0f, 1.0f);
            if (c->x < d->x) {
                rotate_tenths((int)(ang / (2.0 * M_PI) * 3600.0 + 1800.0), 'z');
                glTranslatef((float)(-1.0 - c->namelen * 0.12), -0.02f, 0.0f);
            } else {
                rotate_tenths((int)(ang / (2.0 * M_PI) * 3600.0), 'z');
                glTranslatef(1.0f, -0.02f, 0.0f);
            }
            glScalef(0.02f, 0.02f, 0.0f);
            text(c->name);
            glPopMatrix();
        }
        glPopName();
    }
}

static void draw_directories(int picking)
{
    float hw;
    if (!topdir)
        return;
    if (!picking) {
        /* sky and ground around the eye */
        hw = res.skyWidth * ctx.aspect / eye_shrinkage();
        if (res.useGouraud) {
            glShadeModel(GL_SMOOTH);
            glBegin(GL_QUADS);
            cpack(res.bottomSkyColor);
            glVertex3f(ctx.x - hw, ctx.y + res.skyDistance, -0.5f);
            glVertex3f(ctx.x + hw, ctx.y + res.skyDistance, -0.5f);
            cpack(res.topSkyColor);
            glVertex3f(ctx.x + hw, ctx.y + res.skyDistance, ctx.z + res.skyHeight);
            glVertex3f(ctx.x - hw, ctx.y + res.skyDistance, ctx.z + res.skyHeight);
            glEnd();
            glShadeModel(GL_FLAT);
            glBegin(GL_QUADS);
            cpack(res.topSkyColor);
            glVertex3f(ctx.x + hw, ctx.y + res.skyDistance, ctx.z + res.skyHeight);
            glVertex3f(ctx.x + hw, ctx.y + res.skyDistance, ctx.z + 1000.0f);
            glVertex3f(ctx.x - hw, ctx.y + res.skyDistance, ctx.z + 1000.0f);
            glVertex3f(ctx.x - hw, ctx.y + res.skyDistance, ctx.z + res.skyHeight);
            glEnd();
            glShadeModel(GL_SMOOTH);
            glBegin(GL_QUADS);
            cpack(res.nearGroundColor);
            glVertex3f(ctx.x - hw, ctx.y - res.groundBack, -0.5f);
            glVertex3f(ctx.x + hw, ctx.y - res.groundBack, -0.5f);
            cpack(res.farGroundColor);
            glVertex3f(ctx.x + hw, ctx.y + res.skyDistance, -0.5f);
            glVertex3f(ctx.x - hw, ctx.y + res.skyDistance, -0.5f);
            glEnd();
            glShadeModel(GL_FLAT);
        } else {
            cpack(res.skyColor);
            glBegin(GL_QUADS);
            glVertex3f(ctx.x - hw, ctx.y + res.skyDistance, -0.5f);
            glVertex3f(ctx.x + hw, ctx.y + res.skyDistance, -0.5f);
            glVertex3f(ctx.x + hw, ctx.y + res.skyDistance, ctx.z + 1000.0f);
            glVertex3f(ctx.x - hw, ctx.y + res.skyDistance, ctx.z + 1000.0f);
            glEnd();
        }
    }
    draw_directory(topdir, picking);
}

/* ------------------------------------------------------------------------ */
/* Visibility (draw_visibility, with GL_SELECT instead of gselect)          */
/* ------------------------------------------------------------------------ */

static GLuint *selbuf;
static int selbuf_size;

static void ensure_selbuf(void)
{
    int want = 4 * (num_dirs + 64) + 4096;
    if (selbuf_size < want) {
        selbuf_size = want;
        selbuf = (GLuint *)realloc(selbuf, selbuf_size * sizeof(GLuint));
    }
}

static void draw_visible_directory(FsnDir *d)
{
    int i;
    float dist;
    if (!d->laidout)
        return;
    d->visible = 0;
    dist = (float)fabs(ctx.y - d->y);
    if (dist / d->width < res.directoryHideRatio ||
        dist / (d->height + d->max_file_height) < res.directoryHideRatio) {
        glLoadName(d->index);
        glPushMatrix();
        glTranslatef(d->x, d->y, 0.0f);
        glScalef(d->scale, 1.0f, 1.0f);
        glScalef(d->width, d->width, d->height);
        draw_box(NULL, 4, 0x1f);
        glPopMatrix();
    }
    for (i = 0; i < d->ndirs; i++)
        draw_visible_directory(d->dirs[i]);
}

static void markAllVisible(FsnDir *d)
{
    int i;
    d->visible = 1;
    for (i = 0; i < d->ndirs; i++)
        markAllVisible(d->dirs[i]);
}

static void check_visibility(void)
{
    int hits, i, k;
    if (!topdir)
        return;
    ensure_selbuf();
    glSelectBuffer(selbuf_size, selbuf);
    glRenderMode(GL_SELECT);
    glInitNames();
    glPushName(0);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    do_perspective();
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    view_transform();
    draw_visible_directory(topdir);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    hits = glRenderMode(GL_RENDER);
    if (hits < 0) {
        markAllVisible(topdir);
        return;
    }
    for (i = 0, k = 0; i < hits; i++) {
        GLuint n = selbuf[k];
        if (n == 1 && selbuf[k + 3] < (GLuint)num_dirs)
            dir_index[selbuf[k + 3]]->visible = 1;
        k += 3 + n;
    }
}

/* ------------------------------------------------------------------------ */
/* Picking (pickLandscape)                                                  */
/* ------------------------------------------------------------------------ */

static void draw_second_pick(FsnDir *d)
{
    float p1[3], p2[3];
    if (!d->laidout)
        return;
    glLoadName(2);
    glPushName(d->index);
    glPushMatrix();
    glTranslatef(d->x, d->y, 0.0f);
    glScalef(d->scale, 1.0f, 1.0f);
    glPushMatrix();
    glScalef(d->width, d->width, d->height);
    draw_box(NULL, 0, 0x3f);
    glPopMatrix();
    glTranslatef(0.0f, 0.0f, d->height);
    draw_files(d, 1, 7, 0x1f);
    glPopName();
    glPopMatrix();
    if (!d->parent)
        return;
    glLoadName(1);
    glPushName(d->index);
    p1[0] = d->parent->x + d->line_x;
    p1[1] = d->parent->y + d->line_y;
    p2[0] = d->x;
    p2[1] = d->y - d->width / 2.0f;
    p1[2] = p2[2] = -0.05f;
    glBegin(GL_LINES);
    glVertex3fv(p1);
    glVertex3fv(p2);
    glEnd();
    glPopName();
}

static void pick_selected(FsnDir *d)
{
    int i;
    if (!d->laidout)
        return;
    if (d->selected)
        draw_second_pick(d);
    for (i = 0; i < d->ndirs; i++)
        pick_selected(d->dirs[i]);
}

static void pick_begin(int mx, int my)
{
    GLint vp[4];
    glGetIntegerv(GL_VIEWPORT, vp);
    glSelectBuffer(selbuf_size, selbuf);
    glRenderMode(GL_SELECT);
    glInitNames();
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluPickMatrix((GLdouble)mx, (GLdouble)(vp[3] - my), 5.0, 5.0, vp);
    do_perspective();
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    view_transform();
}

static int pick_end(void)
{
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    return glRenderMode(GL_RENDER);
}

static void pickLandscape(int mx, int my, FsnDir **pdir, FsnFile **pfile, FsnDir **pline)
{
    int hits, i, k, n;
    int dir_idx = -1, line_idx = -1, file_idx = -1;
    int *first = NULL, nfirst = 0;

    *pdir = NULL;
    *pfile = NULL;
    *pline = NULL;
    if (!topdir)
        return;
    ensure_selbuf();

    /* first pass: directories under the cursor */
    pick_begin(mx, my);
    draw_directories(1);
    hits = pick_end();
    if (hits > 0) {
        first = (int *)malloc(hits * sizeof(int));
        for (i = 0, k = 0; i < hits; i++) {
            n = (int)selbuf[k];
            if (n == 1)
                first[nfirst++] = (int)selbuf[k + 3];
            k += 3 + n;
        }
    }

    /* second pass: their files and lines, plus the selected directory */
    pick_begin(mx, my);
    glPushName(0);
    for (i = 0; i < nfirst; i++)
        if (first[i] >= 0 && first[i] < num_dirs)
            draw_second_pick(dir_index[first[i]]);
    pick_selected(topdir);
    hits = pick_end();
    free(first);

    for (i = 0, k = 0; i < hits; i++) {
        n = (int)selbuf[k];
        if (n == 2) {
            if (selbuf[k + 3] == 1 && line_idx < 0)
                line_idx = (int)selbuf[k + 4];
            else if (selbuf[k + 3] == 2 && dir_idx < 0)
                dir_idx = (int)selbuf[k + 4];
        } else if (n == 3) {
            dir_idx = (int)selbuf[k + 4];
            file_idx = (int)selbuf[k + 5];
            break;
        }
        k += 3 + n;
    }
    if (dir_idx >= 0 && dir_idx < num_dirs) {
        *pdir = dir_index[dir_idx];
        if (file_idx >= 0 && file_idx < (*pdir)->nfiles)
            *pfile = (*pdir)->files[file_idx];
    } else if (line_idx >= 0 && line_idx < num_dirs) {
        *pline = dir_index[line_idx];
    }
}

/* ------------------------------------------------------------------------ */
/* Selection                                                                */
/* ------------------------------------------------------------------------ */

static void unselect_file(void)
{
    if (!ctx.selfile)
        return;
    ctx.selfile->selected = 0;
    ctx.selfile = NULL;
}

static void select_file(FsnFile *f)
{
    if (ctx.selfile)
        ctx.selfile->selected = 0;
    f->selected = 1;
    ctx.selfile = f;
}

static void unselect_directory(void)
{
    if (!ctx.seldir)
        return;
    ctx.shrink = 0;
    unselect_file();
    ctx.seldir->selected = 0;
    ctx.seldir = NULL;
}

static void select_directory(FsnDir *d)
{
    if (d == ctx.seldir) {
        unselect_file();
        return;
    }
    if (ctx.seldir) {
        ctx.seldir->selected = 0;
        unselect_file();
        ctx.shrink = 0;
    }
    d->selected = 1;
    ctx.seldir = d;
}

/* ------------------------------------------------------------------------ */
/* Work procedures: movement and zoom animation                             */
/* ------------------------------------------------------------------------ */

typedef int (*WorkProc)(void *);

static WorkProc workproc;
static void *workproc_data;
static void idle(void);

static void set_workproc(WorkProc proc, void *data)
{
    workproc = proc;
    workproc_data = data;
    glutIdleFunc(proc ? idle : NULL);
}

static void movehoriz(float fwd, float side)
{
    float s = (float)pow(res.shrinkagePower, (float)(ctx.y / res.shrinkageDistance));
    ctx.x += (ctx.sinh * fwd + ctx.cosh * side) / s;
    ctx.y += -ctx.sinh * side + ctx.cosh * fwd;
}

typedef struct Motion {
    float side, fwd;
    short rot;
    float vz;
} Motion;

static Motion motion;

/* applies the mouse "joystick" each frame */
static int move_step(void *data)
{
    Motion *m = (Motion *)data;
    float dt = (float)(ctx.frame_us / 100000.0);
    ctx.z += m->vz * dt;
    if (ctx.z < 0.03f)
        ctx.z = 0.03f;
    if (m->rot) {
        ctx.rot = (short)(ctx.rot - m->rot * dt);
        calc_h_angle();
    }
    movehoriz(m->fwd * dt, m->side * dt);
    return 0;
}

typedef struct Zoom {
    float tx, ty, tz;
    float xref, yref, slope;
    short rot, tilt;
    int rotChange, tiltChange, shrinkPath;
    double start;
    int steps;
    void (*func)(void);
} Zoom;

static Zoom zoom;

static int zoom_step(void *data)
{
    Zoom *z = (Zoom *)data;
    double elapsed = time_us() - z->start;
    long frame = ctx.frame_us > 0 ? ctx.frame_us : 1;
    int s = (int)((res.zoomSpeed - elapsed + frame / 2) / frame);
    int n = z->steps;
    z->steps = n - 1;
    if (s < n)
        s = n;
    if (s <= 0)
        s = 1;
    if (z->shrinkPath) {
        float ny = ctx.y + (z->ty - ctx.y) / s;
        float sh = (float)pow(res.shrinkagePower, ny / res.shrinkageDistance);
        ctx.x = z->xref - z->slope * (z->yref - ny) / sh;
        ctx.y = ny;
    } else {
        ctx.x += (z->tx - ctx.x) / s;
        ctx.y += (z->ty - ctx.y) / s;
    }
    ctx.z += (z->tz - ctx.z) / s;
    if (z->rotChange) {
        ctx.rot = (short)(ctx.rot + (z->rot - ctx.rot) / s);
        calc_h_angle();
    }
    if (z->tiltChange) {
        ctx.tilt = (short)(ctx.tilt + (z->tilt - ctx.tilt) / s);
        calc_v_angle();
    }
    if (s == 1) {
        if (z->func)
            z->func();
        return 1;
    }
    return 0;
}

static void zoomto(float x, float y, float z, int rot, int tilt, void (*func)(void))
{
    set_workproc(NULL, NULL);
    zoom.tx = x;
    zoom.ty = y;
    zoom.tz = z;
    zoom.rot = (short)rot;
    zoom.tilt = (short)tilt;
    zoom.rotChange = zoom.rot != ctx.rot;
    zoom.tiltChange = zoom.tilt != ctx.tilt;
    if (y == ctx.y) {
        zoom.shrinkPath = 0;
    } else {
        zoom.shrinkPath = 1;
        if (ctx.y < y) {
            zoom.xref = x;
            zoom.yref = y;
            zoom.slope = (float)pow(res.shrinkagePower, ctx.y / res.shrinkageDistance) *
                         (x - ctx.x) / (y - ctx.y);
        } else {
            zoom.xref = ctx.x;
            zoom.yref = ctx.y;
            zoom.slope = (float)pow(res.shrinkagePower, y / res.shrinkageDistance) *
                         (ctx.x - x) / (ctx.y - y);
        }
    }
    zoom.start = time_us();
    zoom.steps = res.minNumZoomSteps;
    zoom.func = func;
    set_workproc(zoom_step, &zoom);
}

static void shrinkDueToZoom(void)
{
    ctx.shrink = 1;
}

/* zoom history (pushzoom / popzoom) */
typedef struct Position {
    int valid;
    float x, y, z;
    short rot, tilt;
    FsnDir *dir;
    FsnFile *file;
} Position;

#define NUM_POSITIONS 10
static Position positions[NUM_POSITIONS];
static int position_index;

static int equal_position(const Position *p)
{
    return p->valid && p->x == ctx.x && p->y == ctx.y && p->z == ctx.z &&
           p->rot == ctx.rot && p->tilt == ctx.tilt;
}

static void pushzoom(void)
{
    Position *p;
    if (equal_position(&positions[position_index]))
        return;
    position_index = (position_index + 1) % NUM_POSITIONS;
    p = &positions[position_index];
    p->valid = 1;
    p->x = ctx.x;
    p->y = ctx.y;
    p->z = ctx.z;
    p->rot = ctx.rot;
    p->tilt = ctx.tilt;
    p->dir = ctx.seldir;
    p->file = ctx.selfile;
}

static void popzoom(void)
{
    int k;
    for (k = 0; k < NUM_POSITIONS; k++) {
        Position *p = &positions[position_index];
        position_index = (position_index + NUM_POSITIONS - 1) % NUM_POSITIONS;
        if (p->valid && !equal_position(p)) {
            if (p->dir)
                select_directory(p->dir);
            else
                unselect_directory();
            if (p->file)
                select_file(p->file);
            zoomto(p->x, p->y, p->z, p->rot, p->tilt, p->file ? shrinkDueToZoom : NULL);
            p->valid = 0;
            return;
        }
        p->valid = 0;
    }
}

static void reset_eye(void)
{
    pushzoom();
    zoomto(res.initialX, res.initialY, res.initialZ, 0, res.initialTilt, NULL);
}

/* findzoom_landscape: selects what is under the cursor, computes where to fly */
static int findzoom_landscape(int mx, int my, float *x, float *y, float *z,
                              int *rot, int *tilt, int *isfile)
{
    FsnDir *d, *line;
    FsnFile *f;
    *rot = 0;
    *isfile = 0;
    pickLandscape(mx, my, &d, &f, &line);
    if (f) {
        select_directory(d);
        select_file(f);
        *x = d->x + f->x * d->scale - ctx.sinh * res.zoomFileBack;
        *y = d->y + f->y - ctx.cosh * res.zoomFileBack;
        if (res.shrinkOnZoom) {
            *isfile = 1;
            *z = res.zoomFileBottom + d->height + res.minFileHeight;
        } else {
            *z = res.zoomFileBottom + d->height + f->height;
        }
        *tilt = res.zoomFileTilt;
        return 1;
    }
    if (!d && line) {
        d = line;
        /* the line of the selected directory leads back to its parent */
        if (d == ctx.seldir && d->parent)
            d = d->parent;
    }
    if (d) {
        if (f == NULL)
            unselect_file();
        select_directory(d);
        *x = d->x - ctx.sinh * (d->width / 2.0f + res.zoomBack);
        *y = d->y - ctx.cosh * (d->width / 2.0f + res.zoomBack);
        *z = res.zoomBottom + d->height;
        *tilt = res.zoomTilt;
        return 1;
    }
    unselect_directory();
    return 0;
}

/* ------------------------------------------------------------------------ */
/* Locate highlight and messages                                            */
/* ------------------------------------------------------------------------ */

static FsnDir *hl_dir;
static FsnFile *hl_file;
static int show_help = 1;

static void highlight_draw(void)
{
    if (!hl_dir)
        return;
    glDisable(GL_DEPTH_TEST);
    glLineWidth((float)res.locateHighlightThickness);
    glColor3ub(255, 255, 255);
    glPushMatrix();
    glTranslatef(hl_dir->x, hl_dir->y, 0.0f);
    glScalef(hl_dir->scale, 1.0f, 1.0f);
    if (hl_file) {
        int shrink = ctx.shrink && hl_dir->selected && res.shrinkOnZoom;
        glTranslatef(hl_file->x, hl_file->y, hl_dir->height);
        glScalef(res.fileBaseWidth, res.fileBaseWidth,
                 shrink ? res.minFileHeight : hl_file->height);
    } else {
        glScalef(hl_dir->width, hl_dir->width, hl_dir->height);
    }
    draw_box(NULL, 1, 0x3f);
    glPopMatrix();
    glLineWidth(1.0f);
    glEnable(GL_DEPTH_TEST);
}

static void bitmap_text(int x, int y, const char *s)
{
    glRasterPos2i(x, y);
    glutBitmapString(GLUT_BITMAP_HELVETICA_12, (const unsigned char *)s);
}

static void format_size(char *buf, double size)
{
    if (size >= 1073741824.0)
        sprintf(buf, "%.1f GB", size / 1073741824.0);
    else if (size >= 1048576.0)
        sprintf(buf, "%.1f MB", size / 1048576.0);
    else if (size >= 1024.0)
        sprintf(buf, "%.1f KB", size / 1024.0);
    else
        sprintf(buf, "%.0f bytes", size);
}

static void draw_messages(void)
{
    char line[FSN_PATH_MAX + 128];
    char sz[32];
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0.0, (double)ctx.w, 0.0, (double)ctx.h);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glDisable(GL_DEPTH_TEST);

    glColor3ub(255, 255, 255);
    if (hl_dir) {
        dirToPath(line, hl_dir);
        if (hl_file) {
            time_t t = (time_t)hl_file->mtime;
            char *ts = ctime(&t);
            size_t n = strlen(line);
            if (n + hl_file->namelen + 64 < sizeof(line)) {
                strcat(line, hl_file->name);
                format_size(sz, hl_file->size);
                sprintf(line + strlen(line), "   %s   %.24s", sz, ts ? ts : "");
            }
        } else {
            format_size(sz, hl_dir->size_own);
            sprintf(line + strlen(line), "   %d files, %s", hl_dir->nfiles, sz);
        }
        bitmap_text(8, 8, line);
    } else {
        bitmap_text(8, 8, "fsn - a 3D File System Navigator");
    }
    if (ctx.seldir) {
        dirToPath(line, ctx.seldir);
        if (ctx.selfile && strlen(line) + ctx.selfile->namelen < sizeof(line) - 1)
            strcat(line, ctx.selfile->name);
        bitmap_text(8, ctx.h - 18, line);
    }
    if (show_help) {
        glColor3ub(230, 230, 230);
        bitmap_text(8, 26, "left: fly to   shift+left: select   middle or ctrl+left drag: move "
                    "(shift: height, ctrl: dive)   arrows: turn/tilt   b: back   r: reset   "
                    "o: overview   h: help   esc: quit");
    }

    glEnable(GL_DEPTH_TEST);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

/* ------------------------------------------------------------------------ */
/* Overview window (top view of the whole landscape)                        */
/* ------------------------------------------------------------------------ */

#define MAIN_X 40
#define MAIN_Y 40
#define MAIN_W 800
#define MAIN_H 600

static int main_win, overview_win;
static int overviewActive = 1;   /* initialOverview */
static int ov_w = 1, ov_h = 1;
static int ov_button;            /* button held in the overview, plus one */
static int ov_shift;

/* drawOverviewDirectory: platforms as rectangles, lines to the children */
static void drawOverviewDirectory(FsnDir *d, int picking)
{
    int i;
    float half, xs;
    if (!d->laidout)
        return;
    glLoadName(d->index);
    cpack(dir_box(d)->c[0]);
    half = d->width / 2.0f;
    xs = half * d->scale;
    glRectf(d->x - xs, d->y - half, d->x + xs, d->y + half);
    for (i = 0; i < d->ndirs; i++) {
        FsnDir *c = d->dirs[i];
        if (!c->laidout)
            continue;
        if (picking) {
            drawOverviewDirectory(c, picking);
            glLoadName(c->index);
        }
        cpack(c->selected ? res.selLineColor : res.unselLineColor);
        glBegin(GL_LINES);
        glVertex2f(d->x + c->line_x * d->scale, d->y + c->line_y);
        glVertex2f(c->x, c->y - c->width / 2.0f);
        glEnd();
        if (!picking)
            drawOverviewDirectory(c, picking);
    }
}

static void overview_ortho(void)
{
    gluOrtho2D(minx, maxx, miny, maxy);
}

/* highlightOverviewDir and drawOverviewOverlayCursor */
static void drawOverviewOverlay(void)
{
    float px = (float)ov_w / (maxx - minx);
    float py = (float)ov_h / (maxy - miny);
    if (hl_dir) {
        float half = hl_dir->width / 2.0f, xs = half * hl_dir->scale;
        glColor3ub(255, 255, 255);
        glLineWidth(2.0f);
        glBegin(GL_LINE_LOOP);
        glVertex2f(hl_dir->x - xs, hl_dir->y - half);
        glVertex2f(hl_dir->x + xs, hl_dir->y - half);
        glVertex2f(hl_dir->x + xs, hl_dir->y + half);
        glVertex2f(hl_dir->x - xs, hl_dir->y + half);
        glEnd();
    }
    /* cross where the view is looking */
    glPushMatrix();
    glTranslatef(ctx.x - ctx.sinh * ctx.sint * ctx.z, ctx.y - ctx.cosh * ctx.sint * ctx.z, 0.0f);
    glScalef(1.0f / px, 1.0f / py, 1.0f);
    glColor3ub(255, 255, 255);
    glLineWidth(3.0f);
    glBegin(GL_LINES);
    glVertex2i(-8, -8); glVertex2i(8, 8);
    glVertex2i(8, -8); glVertex2i(-8, 8);
    glEnd();
    glLineWidth(1.0f);
    glPopMatrix();
}

static void drawOverview(void)
{
    Cpack bg = res.overviewBackgroundColor;
    glClearColor((bg & 0xff) / 255.0f, ((bg >> 8) & 0xff) / 255.0f,
                 ((bg >> 16) & 0xff) / 255.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    overview_ortho();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    if (topdir) {
        glInitNames();
        glPushName(0);
        drawOverviewDirectory(topdir, 0);
        drawOverviewOverlay();
    }
    glutSwapBuffers();
}

static void overview_reshape(int w, int h)
{
    ov_w = w > 0 ? w : 1;
    ov_h = h > 0 ? h : 1;
    glViewport(0, 0, ov_w, ov_h);
}

static void post_redisplay_all(void)
{
    if (main_win)
        glutPostWindowRedisplay(main_win);
    if (overview_win && overviewActive)
        glutPostWindowRedisplay(overview_win);
}

/* overviewPickPointer: directory under the cursor in the overview */
static FsnDir *overviewPickPointer(int mx, int my)
{
    GLint vp[4];
    int hits, i, k;
    if (!topdir)
        return NULL;
    ensure_selbuf();
    glGetIntegerv(GL_VIEWPORT, vp);
    glSelectBuffer(selbuf_size, selbuf);
    glRenderMode(GL_SELECT);
    glInitNames();
    glPushName(0);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluPickMatrix((GLdouble)mx, (GLdouble)(vp[3] - my), 2.0, 2.0, vp);
    overview_ortho();
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    drawOverviewDirectory(topdir, 1);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    hits = glRenderMode(GL_RENDER);
    for (i = 0, k = 0; i < hits; i++) {
        GLuint n = selbuf[k];
        if (n == 1 && selbuf[k + 3] < (GLuint)num_dirs)
            return dir_index[selbuf[k + 3]];
        k += 3 + n;
    }
    return NULL;
}

/* overviewmove: puts the point the view is looking at under the cursor */
static void overviewmove(int mx, int my)
{
    int px = mx, py = ov_h - my - 1;
    float wx, wy;
    if (px < 0)
        px = 0;
    else if (px >= ov_w)
        px = ov_w - 1;
    if (py < 0)
        py = 0;
    else if (py >= ov_h)
        py = ov_h - 1;
    wx = (float)px / (float)ov_w * (maxx - minx) + minx;
    wy = (float)py / (float)ov_h * (maxy - miny) + miny;
    pushzoom();
    zoomto(wx + ctx.sinh * ctx.sint * ctx.z, wy + ctx.cosh * ctx.sint * ctx.z, ctx.z,
           ctx.rot, ctx.tilt, NULL);
}

/* overviewzoom: flies to the directory under the cursor */
static void overviewzoom(int mx, int my)
{
    FsnDir *d = overviewPickPointer(mx, my);
    if (!d) {
        overviewmove(mx, my);
        return;
    }
    pushzoom();
    select_directory(d);
    zoomto(d->x - ctx.sinh * (d->width / 2.0f + res.zoomBack),
           d->y - ctx.cosh * (d->width / 2.0f + res.zoomBack),
           res.zoomBottom + d->height, ctx.rot, ctx.tilt, NULL);
}

/* overviewselect: selects the directory under the cursor */
static void overviewselect(int mx, int my)
{
    FsnDir *d = overviewPickPointer(mx, my);
    if (!d || d == ctx.seldir)
        return;
    pushzoom();
    select_directory(d);
}

static void overview_action(int x, int y)
{
    if (ov_button == GLUT_MIDDLE_BUTTON + 1)
        overviewmove(x, y);
    else if (ov_button == GLUT_LEFT_BUTTON + 1 && ov_shift)
        overviewselect(x, y);
    else if (ov_button == GLUT_LEFT_BUTTON + 1)
        overviewzoom(x, y);
    post_redisplay_all();
}

static void overview_mouse(int button, int state, int x, int y)
{
    if (state == GLUT_DOWN) {
        if (button != GLUT_LEFT_BUTTON && button != GLUT_MIDDLE_BUTTON)
            return;
        ov_button = button + 1;
        ov_shift = (glutGetModifiers() & GLUT_ACTIVE_SHIFT) != 0;
        overview_action(x, y);
    } else if (button + 1 == ov_button) {
        ov_button = 0;
    }
}

static void overview_motion(int x, int y)
{
    if (ov_button)
        overview_action(x, y);
}

/* overviewLocateHighlight: the directory under the cursor is outlined in both views */
static void overview_passive(int x, int y)
{
    FsnDir *d = overviewPickPointer(x, y);
    if (d != hl_dir || hl_file) {
        hl_dir = d;
        hl_file = NULL;
        post_redisplay_all();
    }
}

static void overview_close(void)
{
    overview_win = 0;
    overviewActive = 0;
}

static void main_close(void)
{
    glutLeaveMainLoop();
}

static void keyboard(unsigned char key, int x, int y);
static void special(int key, int x, int y);

static void createOverview(void)
{
    int w = (int)((maxx - minx) * 3.0f), h = (int)(maxy - miny);
    int cur = glutGetWindow();
    /* the original sizes it from the landscape; keep it on screen */
    if (w > 480) {
        h = h * 480 / w;
        w = 480;
    }
    if (h > 480) {
        w = w * 480 / h;
        h = 480;
    }
    if (w < 160)
        w = 160;
    if (h < 160)
        h = 160;
    /* next to the main window */
    glutInitWindowPosition(MAIN_X + MAIN_W + 16, MAIN_Y);
    glutInitWindowSize(w, h);
    overview_win = glutCreateWindow("fsn overview");
    glDisable(GL_DEPTH_TEST);
    glShadeModel(GL_FLAT);
    glutDisplayFunc(drawOverview);
    glutReshapeFunc(overview_reshape);
    glutMouseFunc(overview_mouse);
    glutMotionFunc(overview_motion);
    glutPassiveMotionFunc(overview_passive);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(special);
    glutCloseFunc(overview_close);
    if (cur)
        glutSetWindow(cur);
}

/* showOverview / hideOverview */
static void toggleOverview(void)
{
    int cur = glutGetWindow();
    if (overviewActive) {
        overviewActive = 0;
        if (overview_win) {
            glutSetWindow(overview_win);
            glutHideWindow();
        }
    } else {
        overviewActive = 1;
        if (!overview_win) {
            createOverview();
        } else {
            glutSetWindow(overview_win);
            glutShowWindow();
        }
    }
    if (cur)
        glutSetWindow(cur);
}

/* ------------------------------------------------------------------------ */
/* Scene                                                                    */
/* ------------------------------------------------------------------------ */

static double last_frame;

static void draw_scene(void)
{
    double t;
    Cpack bg;

    if (ctx.x < minx)
        ctx.x = minx;
    if (maxx < ctx.x)
        ctx.x = maxx;
    if (ctx.y < miny - 50.0f)
        ctx.y = miny - 50.0f;
    if (maxy < ctx.y)
        ctx.y = maxy;

    check_visibility();

    bg = res.useGouraud ? res.farGroundColor : res.groundColor;
    glClearColor((bg & 0xff) / 255.0f, ((bg >> 8) & 0xff) / 255.0f,
                 ((bg >> 16) & 0xff) / 255.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    do_perspective();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glPushMatrix();
    view_transform();
    draw_directories(0);
    highlight_draw();
    glPopMatrix();
    draw_messages();
    glutSwapBuffers();

    /* frame time drives movement and zoom speed */
    t = time_us();
    if (last_frame > 0.0 && t > last_frame && t - last_frame < 250000.0)
        ctx.frame_us = (long)(t - last_frame);
    last_frame = t;
    if (ctx.frame_us > 1000000)
        ctx.frame_us = 1000000;

    /* the overview follows the view */
    if (overview_win && overviewActive)
        glutPostWindowRedisplay(overview_win);
}

/* ------------------------------------------------------------------------ */
/* GLUT callbacks                                                           */
/* ------------------------------------------------------------------------ */

static float mark_x, mark_y;
static int moving;      /* button held to move, plus one */
static int move_mode;   /* 0 move, 1 move vertical, 2 dive */

static void norm_pointer(int x, int y, float *fx, float *fy)
{
    *fx = (float)(2 * (x - ctx.w / 2)) / (float)ctx.w;
    *fy = (float)(2 * (ctx.h / 2 - y)) / (float)ctx.h;
}

static void idle(void)
{
    if (workproc && workproc(workproc_data))
        set_workproc(NULL, NULL);
    post_redisplay_all();
}

static void display(void)
{
    draw_scene();
}

static void reshape(int w, int h)
{
    ctx.w = w > 0 ? w : 1;
    ctx.h = h > 0 ? h : 1;
    ctx.aspect = (float)ctx.w / (float)ctx.h;
    glViewport(0, 0, ctx.w, ctx.h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    do_perspective();
    glMatrixMode(GL_MODELVIEW);
}

static void zoom_action(int x, int y)
{
    float tx, ty, tz;
    int rot, tilt, isfile;
    pushzoom();
    if (findzoom_landscape(x, y, &tx, &ty, &tz, &rot, &tilt, &isfile)) {
        if (!isfile)
            ctx.shrink = 0;
        zoomto(tx, ty, tz, rot, tilt, isfile ? shrinkDueToZoom : NULL);
    }
    glutPostRedisplay();
}

static void select_action(int x, int y)
{
    float tx, ty, tz;
    int rot, tilt, isfile;
    pushzoom();
    findzoom_landscape(x, y, &tx, &ty, &tz, &rot, &tilt, &isfile);
    glutPostRedisplay();
}

static void mouse(int button, int state, int x, int y)
{
    int mods = glutGetModifiers();
    if (state == GLUT_DOWN) {
        if (button == GLUT_MIDDLE_BUTTON ||
            (button == GLUT_LEFT_BUTTON && (mods & GLUT_ACTIVE_CTRL))) {
            /* mousemark */
            pushzoom();
            hl_dir = NULL;
            hl_file = NULL;
            norm_pointer(x, y, &mark_x, &mark_y);
            moving = button + 1;
            if (button == GLUT_MIDDLE_BUTTON && (mods & GLUT_ACTIVE_SHIFT))
                move_mode = 1;
            else if (button == GLUT_MIDDLE_BUTTON && (mods & GLUT_ACTIVE_CTRL))
                move_mode = 2;
            else
                move_mode = 0;
        } else if (button == GLUT_LEFT_BUTTON) {
            if (mods & GLUT_ACTIVE_SHIFT)
                select_action(x, y);
            else
                zoom_action(x, y);
        }
    } else if (moving && button + 1 == moving) {
        /* mousedone */
        moving = 0;
        set_workproc(NULL, NULL);
        glutPostRedisplay();
    }
}

static void motion_cb(int x, int y)
{
    float fx, fy, dx, dy;
    if (!moving)
        return;
    norm_pointer(x, y, &fx, &fy);
    dx = fx - mark_x;
    dy = fy - mark_y;
    memset(&motion, 0, sizeof(motion));
    if (move_mode == 0) {
        /* mousemove: forward / sideways */
        motion.fwd = dy * res.mouseSpeed;
        motion.side = dx * res.mouseSpeed;
    } else if (move_mode == 1) {
        /* mousemovevert: along the view, faster when high */
        motion.fwd = dy * ctx.z * res.mouseSpeed;
        motion.vz = -motion.fwd * ctx.cost;
        motion.side = dx * ctx.z * res.mouseSpeed;
    } else {
        /* mousemovemvert */
        motion.fwd = dy * ctx.z * res.mouseSpeed;
        motion.vz = motion.fwd * ctx.cost;
        motion.side = dx * ctx.z * res.mouseSpeed;
    }
    set_workproc(move_step, &motion);
}

static void passive_motion(int x, int y)
{
    FsnDir *d, *line;
    FsnFile *f;
    if (workproc)
        return;
    pickLandscape(x, y, &d, &f, &line);
    if (!d && line)
        d = line;
    if (d != hl_dir || f != hl_file) {
        hl_dir = d;
        hl_file = f;
        glutPostRedisplay();
    }
}

static void relayout(void)
{
    layout_db();
    post_redisplay_all();
}

enum {
    MENU_BACK = 1, MENU_RESET, MENU_HEIGHT_NONE, MENU_HEIGHT_LINEAR,
    MENU_HEIGHT_EXAGGERATED, MENU_SHRINK, MENU_HELP, MENU_OVERVIEW, MENU_QUIT,
    MENU_LANDSCAPE = 100
};

static void set_landscape(int i)
{
    current_landscape = i;
    set_resources(landscape_names[i]);
    makeColorBoxes();
    relayout();
}

static void menu_cb(int value)
{
    switch (value) {
    case MENU_BACK: popzoom(); break;
    case MENU_RESET: reset_eye(); break;
    case MENU_HEIGHT_NONE: displayHeight = 0; relayout(); break;
    case MENU_HEIGHT_LINEAR: displayHeight = 1; relayout(); break;
    case MENU_HEIGHT_EXAGGERATED: displayHeight = 2; relayout(); break;
    case MENU_SHRINK: res.shrinkOnZoom = !res.shrinkOnZoom; break;
    case MENU_HELP: show_help = !show_help; break;
    case MENU_OVERVIEW: toggleOverview(); break;
    case MENU_QUIT: glutLeaveMainLoop(); return;
    default:
        if (value >= MENU_LANDSCAPE && value < MENU_LANDSCAPE + NUM_LANDSCAPES)
            set_landscape(value - MENU_LANDSCAPE);
        break;
    }
    post_redisplay_all();
}

static void keyboard(unsigned char key, int x, int y)
{
    (void)x;
    (void)y;
    switch (key) {
    case 27:
    case 'q':
        glutLeaveMainLoop();
        return;
    case 'b': popzoom(); break;
    case 'r': reset_eye(); break;
    case 'h': show_help = !show_help; break;
    case 'o': toggleOverview(); break;
    case 'n': displayHeight = 0; relayout(); break;
    case 'l': displayHeight = 1; relayout(); break;
    case 'e': displayHeight = 2; relayout(); break;
    case '+': if (ctx.fov > 300) ctx.fov -= 50; break;
    case '-': if (ctx.fov < 1500) ctx.fov += 50; break;
    default: return;
    }
    post_redisplay_all();
}

static void special(int key, int x, int y)
{
    (void)x;
    (void)y;
    switch (key) {
    case GLUT_KEY_LEFT: ctx.rot = (short)(ctx.rot - 50); calc_h_angle(); break;
    case GLUT_KEY_RIGHT: ctx.rot = (short)(ctx.rot + 50); calc_h_angle(); break;
    case GLUT_KEY_UP: if (ctx.tilt > -900) ctx.tilt = (short)(ctx.tilt - 25); calc_v_angle(); break;
    case GLUT_KEY_DOWN: if (ctx.tilt < 0) ctx.tilt = (short)(ctx.tilt + 25); calc_v_angle(); break;
    case GLUT_KEY_PAGE_UP: ctx.z *= 1.15f; break;
    case GLUT_KEY_PAGE_DOWN: ctx.z /= 1.15f; if (ctx.z < 0.03f) ctx.z = 0.03f; break;
    default: return;
    }
    post_redisplay_all();
}

static void create_menu(void)
{
    int i, landscapes, height;
    landscapes = glutCreateMenu(menu_cb);
    for (i = 0; i < NUM_LANDSCAPES; i++)
        glutAddMenuEntry(landscape_names[i], MENU_LANDSCAPE + i);
    height = glutCreateMenu(menu_cb);
    glutAddMenuEntry("None (n)", MENU_HEIGHT_NONE);
    glutAddMenuEntry("Linear (l)", MENU_HEIGHT_LINEAR);
    glutAddMenuEntry("Exaggerated (e)", MENU_HEIGHT_EXAGGERATED);
    glutCreateMenu(menu_cb);
    glutAddMenuEntry("Back (b)", MENU_BACK);
    glutAddMenuEntry("Reset view (r)", MENU_RESET);
    glutAddSubMenu("Height", height);
    glutAddSubMenu("Landscape", landscapes);
    glutAddMenuEntry("Toggle shrink on zoom", MENU_SHRINK);
    glutAddMenuEntry("Toggle overview (o)", MENU_OVERVIEW);
    glutAddMenuEntry("Toggle help (h)", MENU_HELP);
    glutAddMenuEntry("Quit (esc)", MENU_QUIT);
    glutAttachMenu(GLUT_RIGHT_BUTTON);
}

static void usage(const char *prog)
{
    fprintf(stderr, "usage:  %s [-landscape <landscapeName>] [dirname]\n"
                    "\tKnown landscapes are grass, ocean, desert, space,\n"
                    "and indigo (similar to grass, but looks better on indigo)\n", prog);
    exit(1);
}

int main(int argc, char **argv)
{
    const char *dirname = NULL;
    const char *landscape = "grass";
    int i;

    glutInit(&argc, argv);
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-landscape") == 0 && i + 1 < argc)
            landscape = argv[++i];
        else if (argv[i][0] == '-')
            usage(argv[0]);
        else
            dirname = argv[i];
    }
    for (i = 0; i < NUM_LANDSCAPES; i++)
        if (strcmp(landscape, landscape_names[i]) == 0)
            current_landscape = i;
    set_resources(landscape_names[current_landscape]);

    if (dirname == NULL) {
        dirname = getenv("HOME");
        if (dirname == NULL)
            dirname = getenv("USERPROFILE");
        if (dirname == NULL)
            dirname = ".";
    }
    initialize_db(dirname);

    ctx.x = res.initialX;
    ctx.y = res.initialY;
    ctx.z = res.initialZ;
    ctx.rot = 0;
    ctx.tilt = (short)res.initialTilt;
    ctx.fov = (short)res.viewAngle;
    ctx.frame_us = 100000;
    ctx.w = ctx.h = 600;
    ctx.aspect = 1.0f;
    calc_h_angle();
    calc_v_angle();

    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowPosition(MAIN_X, MAIN_Y);
    glutInitWindowSize(MAIN_W, MAIN_H);
    main_win = glutCreateWindow("fsn - 3D File System Navigator");
    glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_CONTINUE_EXECUTION);
    glutCloseFunc(main_close);

    makeColorBoxes();
    layout_db();

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glShadeModel(GL_FLAT);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutMouseFunc(mouse);
    glutMotionFunc(motion_cb);
    glutPassiveMotionFunc(passive_motion);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(special);
    create_menu();
    if (overviewActive)
        createOverview();

    glutMainLoop();
    return 0;
}
