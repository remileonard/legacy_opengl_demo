/*
 * yaodl.h - the YAODL objects of powerflip
 *
 * Reconstruction of the YAODL library linked in the IRIX powerflip binary
 * (classes Yobj, YList, group, vertices, polygons...).  A file is parsed
 * into a graph of objects; drawing a model executes its objects with the
 * IRIS GL calls of the porting layer.
 *
 * The execution model is the one of the original C++ classes:
 *  - an object has N elements (vertices, polygons, children of a group...);
 *  - objects associated to it with ':' are attributes (colors, normals,
 *    transformations...) whose element i applies to its element i;
 *  - Execute() pushes the attributes, then for each element i executes the
 *    element i of the attributes and the element i of the object, popping
 *    and pushing the attributes again between two elements (Push(0) and
 *    Pop(0)), and pops them at the end (Pop(1)).
 */
#ifndef YAODL_H
#define YAODL_H

typedef struct Yobj Yobj;

typedef struct YClass {
    const char *name;
    /* builds the object from the arguments of "(name args)", NULL on error */
    Yobj *(*create)(const struct YClass *cls, Yobj **args, int nargs);
    void (*execute)(Yobj *o);
    void (*executeN)(Yobj *o, int i);
    void (*push)(Yobj *o, int full);
    void (*pop)(Yobj *o, int full);
} YClass;

struct Yobj {
    const YClass *cls;
    int count;              /* number of elements */
    Yobj **assoc;           /* associated attributes */
    int nassoc;
    /* data */
    float *f;               /* Yfloat values, Nfloats data */
    int nf;
    int *iv;                /* Yint values */
    int ni;
    int width;              /* Nfloats: floats per element */
    char *str;              /* Ystring */
    Yobj **kids;            /* group children */
    int nkids;
    /* indices: size and first index of each polygon */
    int *isize, **ilist;
    /* compiled display lists, one per drawing style */
    long dlist[2];
    /* textures and contours: state saved by Push */
    int flag;
    void *saved;
    struct YContour *contours;
};

/* the drawing style of the polygons: 0 filled, 3 wireframe (closed lines) */
extern int YAODL_DrawStyle;
/* polygons drawn by the last frames, for the performance display */
extern int indexpolygons_NumPolygonsDrawn;

/* a model: the objects of a file */
typedef struct YModel {
    Yobj **objs;
    int nobjs;
    float radius;           /* farthest vertex from the origin */
    int npolys;             /* polygons and lines */
} YModel;

/* reads a .ydl / .defs file, NULL on error */
YModel *yaodl_read(const char *file);
/* executes the objects of a model */
void yaodl_draw(YModel *m);
/* the viewing matrix, restored by "untransform" */
void yaodl_set_view(void);

#endif
