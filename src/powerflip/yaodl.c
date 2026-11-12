/*
 * yaodl.c - parser and classes of the YAODL objects of powerflip
 *
 * Syntax (from the parser, the writers and the error messages of the
 * powerflip binary):
 *
 *     #YAODL                          optional header, '#' starts a comment
 *     name = object,                  a definition, used later by its name
 *     (class arg arg ..., : attr ...) an object; ": attr" associates attr
 *                                     to the object (several are allowed)
 *     1 2 3                           a list of ints (Yint)
 *     1.0 2 3                         a list of floats (Yfloat)
 *     "file.rgb"  word                strings (Ystring)
 *     @type<NUL><size><data>          a binary block: big endian floats
 *                                     (float), ints (int) or an object
 *                                     (indices: npolys, sizes, indices)
 *     : class args                    after ':', the parentheses may be
 *                                     omitted, up to the next ',' or ')'
 *
 * Commas separate the items, so "1 2 3, 4 5" are two lists.  C comments
 * are allowed.
 *
 * The classes reproduce the Execute, ExecuteN, Push and Pop methods of the
 * original ones; the classes of the later steps (textures, nurbs, lights,
 * animation...) are parsed and ignored for now.
 */
#include <ctype.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "porting/iris2ogl.h"
#include "image.h"
#include "yaodl.h"

int YAODL_DrawStyle = 0;
int indexpolygons_NumPolygonsDrawn = 0;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static Matrix identity = { { 1, 0, 0, 0 }, { 0, 1, 0, 0 }, { 0, 0, 1, 0 }, { 0, 0, 0, 1 } };
static Matrix view_matrix;
static int have_view;

/* the directory of the model being read, where its textures are searched */
static char model_dir[512];

/* measure mode: no drawing, the vertices are transformed to find the size */
static int measuring;
static float measure_max;
static int measure_polys;

static void *grow(void *p, int *max, int n, size_t size)
{
    if (n < *max)
        return p;
    *max = *max ? *max * 2 : 8;
    if (*max <= n)
        *max = n + 1;
    p = realloc(p, (size_t)*max * size);
    if (!p) {
        fprintf(stderr, "yaodl: out of memory\n");
        exit(1);
    }
    return p;
}

static int same(const char *a, const char *b)
{
    return strcmp(a, b) == 0;
}

/* --- YList: execution of the attributes ---------------------------------------- */

static void list_push(Yobj **l, int n, int full)
{
    int i;
    for (i = 0; i < n; i++)
        if (l[i]->cls->push)
            l[i]->cls->push(l[i], full);
}

static void list_pop(Yobj **l, int n, int full)
{
    int i;
    for (i = n - 1; i >= 0; i--)
        if (l[i]->cls->pop)
            l[i]->cls->pop(l[i], full);
}

/* the element i of each attribute, with its own attributes */
static void list_executeN(Yobj **l, int n, int i)
{
    int k;
    for (k = 0; k < n; k++) {
        Yobj *a = l[k];
        list_push(a->assoc, a->nassoc, 1);
        list_executeN(a->assoc, a->nassoc, i);
        if (a->cls->executeN)
            a->cls->executeN(a, i);
        list_pop(a->assoc, a->nassoc, 1);
    }
}

/* Yobj::Execute */
static void execute_elements(Yobj *o)
{
    int i;
    list_push(o->assoc, o->nassoc, 1);
    for (i = 0; i < o->count; i++) {
        if (i) {
            list_pop(o->assoc, o->nassoc, 0);
            list_push(o->assoc, o->nassoc, 0);
        }
        list_executeN(o->assoc, o->nassoc, i);
        if (o->cls->executeN)
            o->cls->executeN(o, i);
    }
    list_pop(o->assoc, o->nassoc, 1);
}

static void execute(Yobj *o)
{
    if (o->cls->execute)
        o->cls->execute(o);
    else
        execute_elements(o);
}

/* execution inside a display list, compiled once for each drawing style */
static void execute_compiled(Yobj *o)
{
    int s = YAODL_DrawStyle == 3;
    if (measuring) {
        execute_elements(o);
        return;
    }
    if (o->dlist[s] == 0) {
        o->dlist[s] = genobj();
        makeobj(o->dlist[s]);
        execute_elements(o);
        closeobj();
    }
    callobj(o->dlist[s]);
}

/* index i of an attribute with n elements (the original reads beyond the
   end: the last element is used instead) */
static int clamp(int i, int n)
{
    return i < n ? i : n - 1;
}

/* --- data: Yfloat, Yint, Ystring -------------------------------------------------- */

static const YClass yfloat_class = { "Yfloat", NULL, NULL, NULL, NULL, NULL };
static const YClass yint_class = { "Yint", NULL, NULL, NULL, NULL, NULL };
static const YClass ystring_class = { "Ystring", NULL, NULL, NULL, NULL, NULL };

static Yobj *new_obj(const YClass *cls)
{
    Yobj *o = (Yobj *)calloc(1, sizeof(Yobj));
    o->cls = cls;
    return o;
}

static int is_data(Yobj *o)
{
    return o->cls == &yfloat_class || o->cls == &yint_class || o->cls == &ystring_class;
}

/* --- Nfloats: vertices, normals, colors, transformations --------------------------- */

static int class_width(const char *name)
{
    if (same(name, "vertices") || same(name, "normals") || same(name, "colors") || same(name, "translates")
        || same(name, "scales"))
        return 3;
    if (same(name, "colors4f") || same(name, "rotates"))
        return 4;
    if (same(name, "texcoords"))
        return 2;
    return 0;
}

/* "Expecting one list of m*N floats" (ints are accepted too) */
static Yobj *create_nfloats(const YClass *cls, Yobj **args, int nargs)
{
    Yobj *o = new_obj(cls);
    int k, j, max = 0;
    o->width = class_width(cls->name);
    for (k = 0; k < nargs; k++) {
        if (args[k]->cls == &yfloat_class) {
            for (j = 0; j < args[k]->nf; j++) {
                o->f = (float *)grow(o->f, &max, o->nf, sizeof(float));
                o->f[o->nf++] = args[k]->f[j];
            }
        } else if (args[k]->cls == &yint_class) {
            for (j = 0; j < args[k]->ni; j++) {
                o->f = (float *)grow(o->f, &max, o->nf, sizeof(float));
                o->f[o->nf++] = (float)args[k]->iv[j];
            }
        } else {
            fprintf(stderr, "YAODL: %s: Error: Expecting one list of m*N floats\n", cls->name);
            return NULL;
        }
    }
    if (o->nf == 0 || o->nf % o->width) {
        fprintf(stderr, "YAODL: %s: Error: Expecting one list of m*N floats\n", cls->name);
        return NULL;
    }
    o->count = o->nf / o->width;
    return o;
}

static float *element(Yobj *o, int i)
{
    return o->f + o->width * clamp(i, o->count);
}

static void vertices_executeN(Yobj *o, int i)
{
    float *v = element(o, i);
    if (measuring) {
        Matrix m;
        float x, y, z, d;
        getmatrix(m);
        x = v[0] * m[0][0] + v[1] * m[1][0] + v[2] * m[2][0] + m[3][0];
        y = v[0] * m[0][1] + v[1] * m[1][1] + v[2] * m[2][1] + m[3][1];
        z = v[0] * m[0][2] + v[1] * m[1][2] + v[2] * m[2][2] + m[3][2];
        d = (float)sqrt(x * x + y * y + z * z);
        if (d > measure_max)
            measure_max = d;
        return;
    }
    v3f(v);
}

static void normals_executeN(Yobj *o, int i)
{
    n3f(element(o, i));
}

/* colors and colors4f: the current color (colorBase) */
static void colors_executeN(Yobj *o, int i)
{
    c3f(element(o, i));
}

static void colors4f_executeN(Yobj *o, int i)
{
    c4f(element(o, i));
}

/* transformations: the matrix is restored between the elements */
static void push_matrix(Yobj *o, int full)
{
    pushmatrix();
}

static void pop_matrix(Yobj *o, int full)
{
    popmatrix();
}

static void translates_executeN(Yobj *o, int i)
{
    float *t = element(o, i);
    translate(t[0], t[1], t[2]);
}

static void scales_executeN(Yobj *o, int i)
{
    float *s = element(o, i);
    scale(s[0], s[1], s[2]);
}

/* angle in degrees around the axis x y z (the matrix of a quaternion) */
static void rotates_executeN(Yobj *o, int i)
{
    float *r = element(o, i);
    float a = (float)(r[0] * M_PI / 180.0 * 0.5);
    float s = (float)sin(a), w = (float)cos(a);
    float l = r[1] * r[1] + r[2] * r[2] + r[3] * r[3];
    float x, y, z;
    Matrix m;
    if (l == 0)
        return;
    l = (float)sqrt(l);
    x = r[1] * s / l;
    y = r[2] * s / l;
    z = r[3] * s / l;
    m[0][0] = 1 - 2 * (y * y + z * z);
    m[0][1] = 2 * (x * y + w * z);
    m[0][2] = 2 * (x * z - w * y);
    m[0][3] = 0;
    m[1][0] = 2 * (x * y - w * z);
    m[1][1] = 1 - 2 * (x * x + z * z);
    m[1][2] = 2 * (y * z + w * x);
    m[1][3] = 0;
    m[2][0] = 2 * (x * z + w * y);
    m[2][1] = 2 * (y * z - w * x);
    m[2][2] = 1 - 2 * (x * x + y * y);
    m[2][3] = 0;
    m[3][0] = m[3][1] = m[3][2] = 0;
    m[3][3] = 1;
    multmatrix(m);
}

/* --- group and its derivatives ------------------------------------------------------- */

static Yobj *create_group(const YClass *cls, Yobj **args, int nargs)
{
    Yobj *o = new_obj(cls);
    int k;
    o->kids = (Yobj **)malloc(sizeof(Yobj *) * (nargs ? nargs : 1));
    for (k = 0; k < nargs; k++) {
        if (is_data(args[k])) {
            fprintf(stderr, "YAODL: %s: ignoring a %s argument\n", cls->name, args[k]->cls->name);
            continue;
        }
        o->kids[o->nkids++] = args[k];
    }
    o->count = o->nkids;
    return o;
}

static void group_executeN(Yobj *o, int i)
{
    execute(o->kids[i]);
}

static void group_push(Yobj *o, int full)
{
    int k;
    for (k = 0; k < o->nkids; k++)
        if (o->kids[k]->cls->push)
            o->kids[k]->cls->push(o->kids[k], full);
}

static void group_pop(Yobj *o, int full)
{
    int k;
    for (k = 0; k < o->nkids; k++)
        if (o->kids[k]->cls->pop)
            o->kids[k]->cls->pop(o->kids[k], full);
}

/* YTypeCheck: every argument of the given class */
static Yobj *create_typed(const YClass *cls, Yobj **args, int nargs, const char *kidclass)
{
    int k;
    for (k = 0; k < nargs; k++) {
        if (!same(args[k]->cls->name, kidclass)) {
            fprintf(stderr, "YAODL: Type Check error: expected: %s, got %s in %s\n", kidclass,
                    args[k]->cls->name, cls->name);
            return NULL;
        }
    }
    return create_group(cls, args, nargs);
}

static Yobj *create_vertices_group(const YClass *cls, Yobj **args, int nargs)
{
    return create_typed(cls, args, nargs, "vertices");
}

/* lines: each list of vertices is a line */
static void lines_executeN(Yobj *o, int i)
{
    if (measuring) {
        measure_polys++;
        execute(o->kids[i]);
        return;
    }
    bgnline();
    execute(o->kids[i]);
    endline();
}

static void lines_execute(Yobj *o)
{
    execute_compiled(o);
}

static void polygons_execute(Yobj *o)
{
    execute_compiled(o);
}

/* polygons: each list of vertices is a polygon */
static void begin_polygon(void)
{
    if (measuring)
        measure_polys++;
    else if (YAODL_DrawStyle == 3)
        bgnclosedline();
    else
        bgnpolygon();
}

static void end_polygon(void)
{
    if (measuring)
        return;
    if (YAODL_DrawStyle == 3)
        endclosedline();
    else
        endpolygon();
}

static void polygons_executeN(Yobj *o, int i)
{
    begin_polygon();
    execute(o->kids[i]);
    end_polygon();
}

/* indices: "npolys, size of each polygon, indices" in binary, a list of ints
   for each polygon in text */
static Yobj *create_indices(const YClass *cls, Yobj **args, int nargs)
{
    Yobj *o = new_obj(cls);
    int k;
    o->isize = (int *)malloc(sizeof(int) * (nargs ? nargs : 1));
    o->ilist = (int **)malloc(sizeof(int *) * (nargs ? nargs : 1));
    for (k = 0; k < nargs; k++) {
        if (args[k]->cls != &yint_class) {
            fprintf(stderr, "YAODL: indices: Type Check error: expected: Yint\n");
            return NULL;
        }
        o->isize[k] = args[k]->ni;
        o->ilist[k] = args[k]->iv;
    }
    o->count = nargs;
    return o;
}

static Yobj *create_binary_indices(const YClass *cls, const int *data, int n)
{
    Yobj *o = new_obj(cls);
    int k, at;
    if (n < 1 || data[0] < 0 || 1 + data[0] > n) {
        fprintf(stderr, "YAODL: indices: bad binary data\n");
        return NULL;
    }
    o->count = data[0];
    o->iv = (int *)malloc(sizeof(int) * n);
    memcpy(o->iv, data, sizeof(int) * n);
    o->isize = (int *)malloc(sizeof(int) * (o->count ? o->count : 1));
    o->ilist = (int **)malloc(sizeof(int *) * (o->count ? o->count : 1));
    at = 1 + o->count;
    for (k = 0; k < o->count; k++) {
        o->isize[k] = o->iv[1 + k];
        if (o->isize[k] < 0 || at + o->isize[k] > n) {
            fprintf(stderr, "YAODL: indices: bad binary data\n");
            return NULL;
        }
        o->ilist[k] = o->iv + at;
        at += o->isize[k];
    }
    return o;
}

/* indices::IExecute: the vertices of the polygon i, with their attributes */
static void indices_iexecute(Yobj *ind, int i, Yobj *verts)
{
    int j;
    list_push(verts->assoc, verts->nassoc, 1);
    for (j = 0; j < ind->isize[i]; j++) {
        int v = ind->ilist[i][j];
        if (v < 0 || v >= verts->count)
            continue;
        if (j) {
            list_pop(verts->assoc, verts->nassoc, 0);
            list_push(verts->assoc, verts->nassoc, 0);
        }
        list_executeN(verts->assoc, verts->nassoc, v);
        vertices_executeN(verts, v);
    }
    list_pop(verts->assoc, verts->nassoc, 1);
}

/* indexpolygons: vertices, indices */
static Yobj *create_indexpolygons(const YClass *cls, Yobj **args, int nargs)
{
    Yobj *o;
    if (nargs != 2 || !same(args[0]->cls->name, "vertices") || !same(args[1]->cls->name, "indices")) {
        fprintf(stderr, "YAODL: Index polygon expects vertices, indices\n");
        return NULL;
    }
    o = create_group(cls, args, nargs);
    o->count = args[1]->count;
    return o;
}

static void indexpolygons_executeN(Yobj *o, int i)
{
    begin_polygon();
    indices_iexecute(o->kids[1], i, o->kids[0]);
    end_polygon();
}

static void indexpolygons_execute(Yobj *o)
{
    execute_compiled(o);
    indexpolygons_NumPolygonsDrawn += o->count;
}

/* untransform: back to the viewing matrix */
static Yobj *create_noargs(const YClass *cls, Yobj **args, int nargs)
{
    Yobj *o = new_obj(cls);
    if (nargs)
        fprintf(stderr, "YAODL: %s: expecting no arguments\n", cls->name);
    o->count = 1;
    return o;
}

static void untransform_push(Yobj *o, int full)
{
    if (!full)
        return;
    pushmatrix();
    if (have_view)
        loadmatrix(view_matrix);
}

static void untransform_pop(Yobj *o, int full)
{
    if (full)
        popmatrix();
}

/* nofliplighting: no lighting, then the lighting model of flip again */
void resetmodel(void);

static void nofliplighting_push(Yobj *o, int full)
{
    if (!measuring)
        lmbind(LMODEL, 0);
}

static void nofliplighting_pop(Yobj *o, int full)
{
    if (!measuring)
        resetmodel();
}

/* the classes of the later steps */
static Yobj *create_unknown(const YClass *cls, Yobj **args, int nargs)
{
    Yobj *o = new_obj(cls);
    o->kids = args;
    o->nkids = nargs;
    return o;
}


/* --- textures ------------------------------------------------------------------------ */

static char **texture_names;
static int ntextures, maxtextures;
static int current_texture;

/* textures::def_texture: each file is read once, as a texture of 4
   components modulating the lit color */
static int def_texture(const char *name)
{
    static float tevprops[] = { TV_MODULATE, TV_NULL };
    static float texprops[] = { TX_MINFILTER, TX_MIPMAP_TRILINEAR, TX_MAGFILTER, TX_BILINEAR, TX_NULL };
    unsigned char *image;
    char path[1024];
    int i, w, h;

    if (ntextures == 0) {
        tevdef(1, 0, tevprops);
        tevbind(TV_ENV0, 1);
    }
    for (i = 0; i < ntextures; i++)
        if (same(texture_names[i], name))
            return i + 1;
    image = readImageRGB(name, &w, &h);
    if (!image) {
        /* in the directory of the model, also for the IRIX absolute paths
           (/usr/demos/...) of the original files */
        const char *base = name[0] == '/' ? strrchr(name, '/') + 1 : name;
        snprintf(path, sizeof(path), "%s%s%s", model_dir, model_dir[0] ? "/" : "", base);
        image = readImageRGB(path, &w, &h);
    }
    if (!image) {
        fprintf(stderr, "YAODL: Error: Can't load texture file %s, drawn without it\n", name);
        return -1;
    }
    texture_names = (char **)grow(texture_names, &maxtextures, ntextures, sizeof(char *));
    texture_names[ntextures++] = strdup(name);
    texdef2d(ntextures, 4, w, h, image, 0, texprops);
    free(image);
    return ntextures;
}

/* "Expecting a list of texture file names" */
static Yobj *create_textures(const YClass *cls, Yobj **args, int nargs)
{
    Yobj *o = new_obj(cls);
    int k;
    o->iv = (int *)malloc(sizeof(int) * (nargs ? nargs : 1));
    for (k = 0; k < nargs; k++) {
        if (args[k]->cls != &ystring_class) {
            fprintf(stderr, "YAODL: Error: Expecting a list of texture file names\n");
            return NULL;
        }
        /* a texture that cannot be read is replaced by no texture */
        if ((o->iv[k] = def_texture(args[k]->str)) < 0)
            o->iv[k] = 0;
    }
    o->ni = o->count = nargs;
    return o;
}

/* a texture bound by an enclosing object wins over the inner ones */
static void textures_executeN(Yobj *o, int i)
{
    if (o->flag || measuring || o->count == 0)
        return;
    current_texture = o->iv[clamp(i, o->count)];
    texbind(TX_TEXTURE_0, current_texture);
}

static void textures_push(Yobj *o, int full)
{
    if (current_texture)
        o->flag = 1;
    if (full)
        o->saved = (void *)(intptr_t)current_texture;
}

static void textures_pop(Yobj *o, int full)
{
    int saved = (int)(intptr_t)o->saved;
    if (o->flag || !full || measuring)
        return;
    if (saved != current_texture) {
        texbind(TX_TEXTURE_0, saved);
        current_texture = saved;
    }
}

static void texcoords_executeN(Yobj *o, int i)
{
    if (current_texture && !measuring)
        t2f(element(o, i));
}

/* --- sContours, tContours: texture coordinate generation ------------------------------ */

typedef struct YContour {
    int mode;               /* TG_CONTOUR, TG_LINEAR, TG_SPHEREMAP or TG_OFF */
    int eye;                /* plane in eye coordinates, else object */
    float plane[4];
} YContour;

static YContour *current_contour[2];

static int contour_coord(Yobj *o)
{
    return same(o->cls->name, "tContours") ? TX_T : TX_S;
}

/* "(sContours mode, space, plane, ...)": mode contour, linear, spheremap or
   off, space eye or object, a plane of 4 floats except for spheremap */
static Yobj *create_contours(const YClass *cls, Yobj **args, int nargs)
{
    Yobj *o = new_obj(cls);
    int k = 0, n = 0;
    o->contours = (YContour *)calloc(nargs ? nargs : 1, sizeof(YContour));
    while (k < nargs) {
        YContour *e = &o->contours[n];
        const char *m;
        if (args[k]->cls != &ystring_class) {
            fprintf(stderr, "YAODL: Error: Expecting a contour mode string\n");
            return NULL;
        }
        m = args[k++]->str;
        if (same(m, "contour"))
            e->mode = TG_CONTOUR;
        else if (same(m, "linear"))
            e->mode = TG_LINEAR;
        else if (same(m, "spheremap"))
            e->mode = TG_SPHEREMAP;
        else if (same(m, "off"))
            e->mode = TG_OFF;
        else {
            fprintf(stderr, "YAODL: Error: Expecting contour, linear, or off\n");
            return NULL;
        }
        if (k >= nargs || args[k]->cls != &ystring_class) {
            fprintf(stderr, "YAODL: Error: Expecting a contour transformation space\n");
            return NULL;
        }
        if (same(args[k]->str, "eye"))
            e->eye = 1;
        else if (!same(args[k]->str, "object")) {
            fprintf(stderr, "YAODL: Error: Expecting either eye or object\n");
            return NULL;
        }
        k++;
        if (e->mode != TG_SPHEREMAP && e->mode != TG_OFF) {
            int j;
            Yobj *pl = k < nargs ? args[k] : NULL;
            if (!pl || (pl->cls == &yfloat_class ? pl->nf : pl->cls == &yint_class ? pl->ni : 0) != 4) {
                fprintf(stderr, "YAODL: Error: Expecting a plane specification\n");
                return NULL;
            }
            for (j = 0; j < 4; j++)
                e->plane[j] = pl->cls == &yfloat_class ? pl->f[j] : (float)pl->iv[j];
            k++;
        }
        n++;
    }
    o->count = n;
    return o;
}

/* contourBase::activate: in eye space, the plane is given with the identity
   as viewing matrix */
static void contour_activate(Yobj *o, YContour *e)
{
    int coord = contour_coord(o);
    if (measuring)
        return;
    if (e->mode == TG_OFF) {
        texgen(coord, TG_OFF, NULL);
    } else {
        if (e->eye) {
            pushmatrix();
            loadmatrix(identity);
        }
        texgen(coord, e->mode, e->plane);
        texgen(coord, TG_ON, NULL);
        if (e->eye)
            popmatrix();
    }
    current_contour[coord] = e;
}

static void contours_executeN(Yobj *o, int i)
{
    if (o->count)
        contour_activate(o, &o->contours[clamp(i, o->count)]);
}

static void contours_push(Yobj *o, int full)
{
    if (full)
        o->saved = current_contour[contour_coord(o)];
}

static void contours_pop(Yobj *o, int full)
{
    int coord = contour_coord(o);
    if (!full || measuring)
        return;
    if (o->saved) {
        contour_activate(o, (YContour *)o->saved);
    } else {
        texgen(coord, TG_OFF, NULL);
        current_contour[coord] = NULL;
    }
}

/* --- subdivides: the screen subdivision of IRIX, not needed here ---------------------- */

static Yobj *create_subdivides(const YClass *cls, Yobj **args, int nargs)
{
    Yobj *o = new_obj(cls);
    o->count = 1;
    return o;
}

/* --- regularMesh: "(regularMesh columns rows (vertices ...))" ------------------------- */

static Yobj *create_regularmesh(const YClass *cls, Yobj **args, int nargs)
{
    Yobj *o;
    if (nargs != 2 || args[0]->cls != &yint_class || !same(args[1]->cls->name, "vertices")) {
        fprintf(stderr, "YAODL: Regular mesh expects integer dimensions, vertices\n");
        return NULL;
    }
    if (args[0]->ni != 2) {
        fprintf(stderr, "YAODL: Regular mesh expects 2 integer dimensions, vertices\n");
        return NULL;
    }
    if (args[0]->iv[0] * args[0]->iv[1] != args[1]->count) {
        fprintf(stderr, "YAODL: Regular mesh: expected %d*%d vertices, received %d\n", args[0]->iv[0],
                args[0]->iv[1], args[1]->count);
        return NULL;
    }
    o = create_group(cls, args + 1, 1);
    o->width = args[0]->iv[0];
    o->ni = args[0]->iv[1];
    o->count = 1;
    return o;
}

static void mesh_vertex(Yobj *verts, int v, int first)
{
    if (!first) {
        list_pop(verts->assoc, verts->nassoc, 0);
        list_push(verts->assoc, verts->nassoc, 0);
    }
    list_executeN(verts->assoc, verts->nassoc, v);
    vertices_executeN(verts, v);
}

/* strips between two rows, or the lines of the rows and columns */
static void regularmesh_elements(Yobj *o)
{
    Yobj *verts = o->kids[0];
    int w = o->width, h = o->ni, r, c;
    list_push(verts->assoc, verts->nassoc, 1);
    if (measuring) {
        for (r = 0; r < w * h; r++)
            vertices_executeN(verts, r);
        measure_polys += 2 * (w - 1) * (h - 1);
    } else if (YAODL_DrawStyle == 3) {
        for (r = 0; r < h; r++) {
            bgnline();
            for (c = 0; c < w; c++)
                mesh_vertex(verts, r * w + c, c == 0);
            endline();
        }
        for (c = 0; c < w; c++) {
            bgnline();
            for (r = 0; r < h; r++)
                mesh_vertex(verts, r * w + c, r == 0);
            endline();
        }
    } else {
        for (r = 0; r + 1 < h; r++) {
            bgntmesh();
            for (c = 0; c < w; c++) {
                mesh_vertex(verts, r * w + c, c == 0);
                mesh_vertex(verts, (r + 1) * w + c, 0);
            }
            endtmesh();
        }
    }
    list_pop(verts->assoc, verts->nassoc, 1);
}

static void regularmesh_execute(Yobj *o)
{
    int s = YAODL_DrawStyle == 3;
    list_push(o->assoc, o->nassoc, 1);
    list_executeN(o->assoc, o->nassoc, 0);
    if (measuring) {
        regularmesh_elements(o);
    } else {
        if (o->dlist[s] == 0) {
            o->dlist[s] = genobj();
            makeobj(o->dlist[s]);
            regularmesh_elements(o);
            closeobj();
        }
        callobj(o->dlist[s]);
    }
    list_pop(o->assoc, o->nassoc, 1);
}

static const YClass classes[] = {
    { "group", create_group, NULL, group_executeN, group_push, group_pop },
    { "lines", create_vertices_group, lines_execute, lines_executeN, group_push, group_pop },
    { "polygons", create_vertices_group, polygons_execute, polygons_executeN, group_push, group_pop },
    { "indexpolygons", create_indexpolygons, indexpolygons_execute, indexpolygons_executeN, group_push, group_pop },
    { "indices", create_indices, NULL, NULL, NULL, NULL },
    { "vertices", create_nfloats, NULL, vertices_executeN, NULL, NULL },
    { "normals", create_nfloats, NULL, normals_executeN, NULL, NULL },
    { "colors", create_nfloats, NULL, colors_executeN, NULL, NULL },
    { "colors4f", create_nfloats, NULL, colors4f_executeN, NULL, NULL },
    { "translates", create_nfloats, NULL, translates_executeN, push_matrix, pop_matrix },
    { "scales", create_nfloats, NULL, scales_executeN, push_matrix, pop_matrix },
    { "rotates", create_nfloats, NULL, rotates_executeN, push_matrix, pop_matrix },
    { "untransform", create_noargs, NULL, NULL, untransform_push, untransform_pop },
    { "nofliplighting", create_noargs, NULL, NULL, nofliplighting_push, nofliplighting_pop },
    { "textures", create_textures, NULL, textures_executeN, textures_push, textures_pop },
    { "texcoords", create_nfloats, NULL, texcoords_executeN, NULL, NULL },
    { "sContours", create_contours, NULL, contours_executeN, contours_push, contours_pop },
    { "tContours", create_contours, NULL, contours_executeN, contours_push, contours_pop },
    { "subdivides", create_subdivides, NULL, NULL, NULL, NULL },
    { "regularMesh", create_regularmesh, regularmesh_execute, NULL, group_push, group_pop },
};

/* names of the classes still to do (textures, nurbs, lights, animation) */
static const char *later[] = {
    "nurbs", "trimcurves", "light", "lightPosition", "lightAmbient", "lightColor", "spotlight", "spotdirection", "transVector",
    "rotVector", "floatValue", "floatValues", "intValue", "advanceTime", "seconds", "spline",
};

static const YClass *find_class(const char *name)
{
    static YClass unknown[64];
    static int nunknown;
    int i;
    for (i = 0; i < (int)(sizeof(classes) / sizeof(classes[0])); i++)
        if (same(classes[i].name, name))
            return &classes[i];
    for (i = 0; i < nunknown; i++)
        if (same(unknown[i].name, name))
            return &unknown[i];
    for (i = 0; i < (int)(sizeof(later) / sizeof(later[0])); i++)
        if (same(later[i], name))
            break;
    fprintf(stderr, "YAODL: %s %s, ignored\n", name,
            i < (int)(sizeof(later) / sizeof(later[0])) ? "is not supported yet" : "is an unknown type");
    if (nunknown == 64)
        return &unknown[63];
    unknown[nunknown].name = strdup(name);
    unknown[nunknown].create = create_unknown;
    return &unknown[nunknown++];
}

static Yobj *create(const char *name, Yobj **args, int nargs)
{
    const YClass *cls = find_class(name);
    return cls->create(cls, args, nargs);
}

/* --- parser --------------------------------------------------------------------------- */

typedef struct {
    char name[64];
    Yobj *obj;
} Ydef;

typedef struct {
    const unsigned char *s;
    long len, pos;
    int line;
    const char *file;
    Ydef *defs;
    int ndefs, maxdefs;
    int error;
} Parser;

static void parse_error(Parser *p, const char *msg)
{
    if (!p->error)
        fprintf(stderr, "YAODL error: %s: line %d: %s\n", p->file, p->line, msg);
    p->error = 1;
}

/* spaces and comments; commas too when skip_commas */
static void skip(Parser *p, int skip_commas)
{
    while (p->pos < p->len) {
        int c = p->s[p->pos];
        if (c == '\n') {
            p->line++;
            p->pos++;
        } else if (c == '#') {
            while (p->pos < p->len && p->s[p->pos] != '\n')
                p->pos++;
        } else if (c == '/' && p->pos + 1 < p->len && p->s[p->pos + 1] == '*') {
            p->pos += 2;
            while (p->pos + 1 < p->len && !(p->s[p->pos] == '*' && p->s[p->pos + 1] == '/')) {
                if (p->s[p->pos] == '\n')
                    p->line++;
                p->pos++;
            }
            p->pos += 2;
        } else if (isspace(c) || (skip_commas && c == ',')) {
            p->pos++;
        } else {
            break;
        }
    }
}

static int peek(Parser *p, int skip_commas)
{
    skip(p, skip_commas);
    return p->pos < p->len ? p->s[p->pos] : -1;
}

static int is_word_start(int c)
{
    return isalpha(c) || c == '_' || c == '/';
}

static int is_word_char(int c)
{
    return isalnum(c) || c == '_' || c == '.' || c == '/' || c == '-';
}

static int is_number(Parser *p)
{
    int c = p->s[p->pos];
    long q = p->pos;
    if (c == '-' || c == '+')
        q++;
    if (q < p->len && p->s[q] == '.')
        q++;
    return q < p->len && isdigit(p->s[q]);
}

static void read_word(Parser *p, char *buf, int size)
{
    int n = 0;
    while (p->pos < p->len && is_word_char(p->s[p->pos])) {
        if (n < size - 1)
            buf[n++] = (char)p->s[p->pos];
        p->pos++;
    }
    buf[n] = 0;
}

static Yobj *find_def(Parser *p, const char *name)
{
    int i;
    for (i = p->ndefs - 1; i >= 0; i--)
        if (same(p->defs[i].name, name))
            return p->defs[i].obj;
    return NULL;
}

static void define(Parser *p, const char *name, Yobj *o)
{
    if (!o)
        return;
    p->defs = (Ydef *)grow(p->defs, &p->maxdefs, p->ndefs, sizeof(Ydef));
    strncpy(p->defs[p->ndefs].name, name, sizeof(p->defs[0].name) - 1);
    p->defs[p->ndefs].name[sizeof(p->defs[0].name) - 1] = 0;
    p->defs[p->ndefs++].obj = o;
}

/* a run of numbers up to a comma or something else: Yint, or Yfloat when one
   of them has a point or an exponent */
static Yobj *read_numbers(Parser *p)
{
    double *v = NULL;
    int n = 0, max = 0, isfloat = 0, i;
    Yobj *o;
    for (;;) {
        char buf[64];
        int k = 0;
        skip(p, 0);
        if (p->pos >= p->len || !is_number(p))
            break;
        while (p->pos < p->len && k < 63) {
            int c = p->s[p->pos];
            if (!(isdigit(c) || c == '.' || c == '-' || c == '+' || c == 'e' || c == 'E'))
                break;
            if (c == '.' || c == 'e' || c == 'E')
                isfloat = 1;
            buf[k++] = (char)c;
            p->pos++;
        }
        buf[k] = 0;
        v = (double *)grow(v, &max, n, sizeof(double));
        v[n++] = atof(buf);
    }
    if (isfloat) {
        o = new_obj(&yfloat_class);
        o->f = (float *)malloc(sizeof(float) * n);
        for (i = 0; i < n; i++)
            o->f[i] = (float)v[i];
        o->nf = o->count = n;
    } else {
        o = new_obj(&yint_class);
        o->iv = (int *)malloc(sizeof(int) * n);
        for (i = 0; i < n; i++)
            o->iv[i] = (int)v[i];
        o->ni = o->count = n;
    }
    free(v);
    return o;
}

static unsigned int be32(const unsigned char *b)
{
    return ((unsigned int)b[0] << 24) | ((unsigned int)b[1] << 16) | ((unsigned int)b[2] << 8) | b[3];
}

/* "@type<NUL><size><data>" */
static Yobj *read_binary(Parser *p)
{
    char type[64];
    int k = 0, i, n;
    unsigned int size;
    const unsigned char *d;
    Yobj *o;
    p->pos++;
    while (p->pos < p->len && p->s[p->pos] != 0) {
        if (!isspace(p->s[p->pos]) && k < 63)
            type[k++] = (char)p->s[p->pos];
        p->pos++;
    }
    type[k] = 0;
    p->pos++;
    if (p->pos + 4 > p->len) {
        parse_error(p, "truncated binary block");
        return NULL;
    }
    size = be32(p->s + p->pos);
    p->pos += 4;
    if (size > (unsigned int)(p->len - p->pos)) {
        parse_error(p, "truncated binary block");
        return NULL;
    }
    d = p->s + p->pos;
    p->pos += size;
    n = (int)(size / 4);
    if (same(type, "float")) {
        o = new_obj(&yfloat_class);
        o->f = (float *)malloc(sizeof(float) * (n ? n : 1));
        for (i = 0; i < n; i++) {
            unsigned int u = be32(d + 4 * i);
            memcpy(&o->f[i], &u, 4);
        }
        o->nf = o->count = n;
        return o;
    }
    {
        int *iv = (int *)malloc(sizeof(int) * (n ? n : 1));
        for (i = 0; i < n; i++)
            iv[i] = (int)be32(d + 4 * i);
        if (same(type, "int")) {
            o = new_obj(&yint_class);
            o->iv = iv;
            o->ni = o->count = n;
            return o;
        }
        if (same(type, "indices")) {
            o = create_binary_indices(find_class("indices"), iv, n);
            free(iv);
            return o;
        }
        free(iv);
    }
    fprintf(stderr, "YAODL: Type does not support binary format: %s\n", type);
    return NULL;
}

static Yobj *parse_item(Parser *p, int bare_ok);

/* the arguments of an object up to ')' (or ',' / ')' for a bare object), the
   attributes after ':' go to assoc */
static void parse_args(Parser *p, int bare, Yobj ***args, int *nargs, Yobj ***assoc, int *nassoc)
{
    int maxargs = 0, maxassoc = 0, attributes = 0;
    for (;;) {
        int c = peek(p, !bare);
        if (c < 0) {
            if (!bare)
                parse_error(p, "unexpected end of file");
            return;
        }
        if (c == ')') {
            if (!bare)
                p->pos++;
            return;
        }
        if (bare && c == ',')
            return;
        if (c == ':') {
            /* the items after ':' are attributes of the object */
            p->pos++;
            attributes = 1;
            continue;
        }
        {
            Yobj *a = parse_item(p, attributes);
            if (a && attributes) {
                *assoc = (Yobj **)grow(*assoc, &maxassoc, *nassoc, sizeof(Yobj *));
                (*assoc)[(*nassoc)++] = a;
            } else if (a) {
                *args = (Yobj **)grow(*args, &maxargs, *nargs, sizeof(Yobj *));
                (*args)[(*nargs)++] = a;
            }
        }
        if (p->error)
            return;
    }
}

/* an object made of a class name and its arguments */
static Yobj *parse_object(Parser *p, const char *name, int bare)
{
    Yobj **args = NULL, **assoc = NULL, *o;
    int nargs = 0, nassoc = 0;
    parse_args(p, bare, &args, &nargs, &assoc, &nassoc);
    if (p->error)
        return NULL;
    o = create(name, args, nargs);
    if (!o) {
        parse_error(p, "bad arguments");
        return NULL;
    }
    if (o->kids != args)
        free(args);
    if (nassoc) {
        int k;
        o->assoc = (Yobj **)realloc(o->assoc, sizeof(Yobj *) * (o->nassoc + nassoc));
        for (k = 0; k < nassoc; k++)
            o->assoc[o->nassoc++] = assoc[k];
    }
    free(assoc);
    return o;
}

/* one item; bare_ok: "class args" without parentheses (after ':' and at the
   top level) */
static Yobj *parse_item(Parser *p, int bare_ok)
{
    char word[256];
    int c = peek(p, 1);
    if (c < 0)
        return NULL;
    if (c == '(') {
        p->pos++;
        skip(p, 1);
        read_word(p, word, sizeof(word));
        if (!word[0]) {
            parse_error(p, "Syntax Error");
            return NULL;
        }
        return parse_object(p, word, 0);
    }
    if (c == '@')
        return read_binary(p);
    if (is_number(p))
        return read_numbers(p);
    if (c == '"') {
        Yobj *o = new_obj(&ystring_class);
        long start = ++p->pos;
        while (p->pos < p->len && p->s[p->pos] != '"')
            p->pos++;
        o->str = (char *)malloc(p->pos - start + 1);
        memcpy(o->str, p->s + start, p->pos - start);
        o->str[p->pos - start] = 0;
        o->count = 1;
        p->pos++;
        return o;
    }
    if (is_word_start(c)) {
        Yobj *d;
        read_word(p, word, sizeof(word));
        if (peek(p, 0) == '=') {
            /* a definition inside a list ("cat file.ydl" in a group) */
            p->pos++;
            define(p, word, parse_item(p, 1));
            return NULL;
        }
        if ((d = find_def(p, word)) != NULL)
            return d;
        if (bare_ok)
            return parse_object(p, word, 1);
        d = new_obj(&ystring_class);
        d->str = strdup(word);
        d->count = 1;
        return d;
    }
    parse_error(p, "Syntax Error");
    return NULL;
}

/* the top level: definitions and objects */
static void parse_file(Parser *p, YModel *m)
{
    int max = 0;
    for (;;) {
        int c = peek(p, 1);
        Yobj *o;
        if (c < 0 || p->error)
            return;
        if (is_word_start(c)) {
            /* "name = object" */
            long save = p->pos;
            int line = p->line;
            char name[64];
            read_word(p, name, sizeof(name));
            if (peek(p, 0) == '=') {
                p->pos++;
                define(p, name, parse_item(p, 1));
                continue;
            }
            p->pos = save;
            p->line = line;
        }
        if (c == ':') {
            p->pos++;
            o = parse_item(p, 1);
            if (o && m->nobjs) {
                Yobj *t = m->objs[m->nobjs - 1];
                t->assoc = (Yobj **)realloc(t->assoc, sizeof(Yobj *) * (t->nassoc + 1));
                t->assoc[t->nassoc++] = o;
            }
            continue;
        }
        o = parse_item(p, 1);
        if (o && !is_data(o)) {
            m->objs = (Yobj **)grow(m->objs, &max, m->nobjs, sizeof(Yobj *));
            m->objs[m->nobjs++] = o;
        }
    }
}

/* --- models ------------------------------------------------------------------------- */

YModel *yaodl_read(const char *file)
{
    FILE *f;
    Parser p;
    YModel *m;
    unsigned char *buf;
    long len;
    int i;

    if ((f = fopen(file, "rb")) == NULL)
        return NULL;
    fseek(f, 0, SEEK_END);
    len = ftell(f);
    fseek(f, 0, SEEK_SET);
    buf = (unsigned char *)malloc(len + 1);
    if (!buf || fread(buf, 1, len, f) != (size_t)len) {
        fclose(f);
        free(buf);
        return NULL;
    }
    fclose(f);

    {
        const char *slash = strrchr(file, '/');
        size_t n = slash ? (size_t)(slash - file) : 0;
        if (n >= sizeof(model_dir))
            n = 0;
        memcpy(model_dir, file, n);
        model_dir[n] = 0;
    }
    memset(&p, 0, sizeof(p));
    p.s = buf;
    p.len = len;
    p.line = 1;
    p.file = file;
    m = (YModel *)calloc(1, sizeof(YModel));
    parse_file(&p, m);
    free(buf);
    free(p.defs);
    if (m->nobjs == 0) {
        fprintf(stderr, "YAODL: %s: nothing to draw\n", file);
        free(m);
        return NULL;
    }

    /* the size and the number of polygons, with the transformations */
    measuring = 1;
    measure_max = 0;
    measure_polys = 0;
    pushmatrix();
    loadmatrix(identity);
    for (i = 0; i < m->nobjs; i++)
        execute(m->objs[i]);
    popmatrix();
    measuring = 0;
    m->radius = measure_max;
    m->npolys = measure_polys;
    return m;
}

void yaodl_draw(YModel *m)
{
    int i;
    for (i = 0; i < m->nobjs; i++)
        execute(m->objs[i]);
}

void yaodl_set_view(void)
{
    getmatrix(view_matrix);
    have_view = 1;
}
