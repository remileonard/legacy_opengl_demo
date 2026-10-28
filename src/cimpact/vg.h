/*
 * vg.h - minimal compatibility layer for the parts of Paradigm Vega used by
 * Certain Impact: application definition file (.adf), objects, scenes,
 * parts and classes, DIS appearance mappings, intersections, paths, fonts.
 *
 * Only what the game needs is implemented, on top of the Performer
 * compatibility layer of src/mekton (pf.h) and the .fst loader (fst.h).
 */
#ifndef VG_H
#define VG_H

#include "pf.h"

/* --- files ---------------------------------------------------------------- */

/* sets the data directory searched for models, terrain, images and paths */
void vgDataDir(const char *dir);
/* finds a data file, also with .Z and .gz suffixes; 1 when found */
int vgFindFile(const char *name, char *path, int size);

/* --- application definition file ------------------------------------------ */

typedef struct vgAppmap {
    char sw[32], child[32];
    unsigned int mask, val;
} vgAppmap;

typedef struct vgEnv {
    float skycolor[3];
    float visnear, visfar;
} vgEnv;

int vgLoadADF(const char *file);
vgEnv *vgGetEnv(void);

/* --- objects ----------------------------------------------------------------- */

typedef struct vgObject {
    char name[64];
    char file[128];
    pfNode *model;           /* loaded on first use */
    float pos[6];            /* x y z h p r */
    float scale;
    pfMatrix mat;
    unsigned int cls;        /* isect class */
    vgAppmap app[8];
    int napp;
    unsigned int appearance;
    int enabled;             /* drawn and intersected */
    void *user;
    struct vgIsc *isc;       /* intersection cache */
    struct vgObject *next;   /* list of all objects */
} vgObject;

vgObject *vgFindObj(const char *name);
/* a copy with its own graph (switches, sequences and DCS) sharing the geometry */
vgObject *vgCopyObj(vgObject *o);
void vgDelObj(vgObject *o);
pfNode *vgObjModel(vgObject *o);
void vgObjPos(vgObject *o, float x, float y, float z, float h, float p, float r);
void vgObjPosVec(vgObject *o, const float *xyzhpr);
void vgObjMat(vgObject *o, pfMatrix m);
void vgObjScale(vgObject *o, float s);
void vgObjClass(vgObject *o, unsigned int cls);

/* parts are the named nodes of the object graph */
pfNode *vgFindPart(vgObject *o, const char *name, int occurence);
int vgNumParts(vgObject *o);
pfNode *vgGetPart(vgObject *o, int i);
void vgPartClass(pfNode *part, unsigned int cls);
unsigned int vgGetPartClass(pfNode *part);
/* sets the rotation and translation of a DCS part */
void vgPartPos(pfNode *part, float x, float y, float z, float h, float p, float r);

/* DIS appearance: app = (app & ~mask) | val, then applies the appmap switches */
void vgDISAppearance(vgObject *o, unsigned int mask, unsigned int val);

/* sequences of a graph */
pfNode *vgFindSeq(pfNode *n, int occurence);
void vgSeqFrameTime(pfNode *seq, float t);

/* --- scenes ------------------------------------------------------------------ */

typedef struct vgScene {
    char name[32];
    vgObject **objs;
    int nobjs, maxobjs;
} vgScene;

vgScene *vgFindScene(const char *name);
void vgAddSceneObj(vgScene *s, vgObject *o);
void vgRemSceneObj(vgScene *s, vgObject *o);
int vgSceneHasObj(vgScene *s, vgObject *o);
void vgClearScene(vgScene *s);
/* draws the scene with the current projection and modelview */
void vgDrawScene(vgScene *s, double t);

/* --- intersections ------------------------------------------------------------ */

typedef struct vgHit {
    int hit;
    float dist;
    pfVec3 point, norm;
    vgObject *obj;
    pfNode *part;            /* nearest named node, may be NULL */
    unsigned int cls;        /* class of the hit geometry */
    pfNode *path[16];        /* nodes from the object root to the geode */
    int pathlen;
} vgHit;

/* closest hit of a segment against the objects of a scene whose class matches */
int vgIsectSeg(vgScene *s, unsigned int mask, const pfVec3 pos, const pfVec3 dir, float len, vgHit *h);
/* ground under (x, y): height, and the pitch and roll for heading h */
int vgIsectZ(vgScene *s, unsigned int mask, float x, float y, float h, float *z, float *p, float *r, vgHit *hit);

/* --- paths ---------------------------------------------------------------------- */

typedef struct vgSegment {
    struct vgSegment *prev, *next;
    int type;                /* 1 line, 2 arc, 3 fillet, 4 speed, 5 delay */
    pfVec3 start, end, hpr;
    pfVec3 center, u, v;     /* arcs: center, start radius direction, tangent */
    float radius, sweep;     /* sweep in radians */
    float speed, accel, delay, length;
} vgSegment;

typedef struct vgPath {
    vgSegment *first, *cur;
    float speed, pscale, rscale, target, accel;
    double delay_end;
    float segdist, total;
    pfVec3 pos;
} vgPath;

vgPath *vgNewPath(void);
vgPath *vgSharePath(vgPath *p);
int vgAddPathFile(vgPath *p, const char *file);
void vgAddLine(vgPath *p, const pfVec3 a, const pfVec3 b);
void vgAddFillet(vgPath *p, float radius);
void vgAddSpeed(vgPath *p, float speed, float accel);
void vgAddDelay(vgPath *p, float delay);
void vgClosePath(vgPath *p);
/* moves along the path, returns the travelled distance */
float vgFollowPath(vgPath *p, float dt, double now, pfVec3 pos, pfVec3 hpr);

/* --- fonts ------------------------------------------------------------------------ */

void vgFontSize(float w, float h);
void vgFontPos(float x, float y);
void vgDrawFont(const char *s);

#endif
