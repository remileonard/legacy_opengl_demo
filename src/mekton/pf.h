/*
 * pf.h - the subset of IRIS Performer 2.0 used by Mekton, on top of
 * OpenGL 1.x.
 *
 * Names, argument orders and constants follow Performer so that the game
 * code reads like the original. Only one process and one channel are
 * supported.
 */
#ifndef PF_H
#define PF_H

#include <stddef.h>

typedef float pfVec2[2];
typedef float pfVec3[3];
typedef float pfVec4[4];
typedef float pfMatrix[4][4];

typedef struct { pfVec3 xyz, hpr; } pfCoord;
typedef struct { pfVec3 pos, dir; float length; } pfSeg;

#undef PF_MAX /* winsock defines a protocol family PF_MAX */
#define PF_MAX(a, b) ((a) > (b) ? (a) : (b))
#define PF_MIN(a, b) ((a) < (b) ? (a) : (b))
#define PF_ABS(a) ((a) < 0 ? -(a) : (a))
#define PF_DEG2RAD(d) ((d) * 3.14159265358979 / 180.0)
#define PF_RAD2DEG(r) ((r) * 180.0 / 3.14159265358979)

#define PF_ON 1
#define PF_OFF 0
#define PF_SET 1
#define PF_OR 2
#define PF_AND 3

/* --- math --------------------------------------------------------------- */

void pfSetVec3(pfVec3 v, float x, float y, float z);
void pfCopyVec3(pfVec3 dst, const pfVec3 src);
void pfAddVec3(pfVec3 dst, const pfVec3 a, const pfVec3 b);
void pfSubVec3(pfVec3 dst, const pfVec3 a, const pfVec3 b);
void pfScaleVec3(pfVec3 dst, float s, const pfVec3 v);
float pfDotVec3(const pfVec3 a, const pfVec3 b);
void pfCrossVec3(pfVec3 dst, const pfVec3 a, const pfVec3 b);
float pfLengthVec3(const pfVec3 v);
float pfNormalizeVec3(pfVec3 v);
float pfLengthVec2(const pfVec2 v);
float pfNormalizeVec2(pfVec2 v);
float pfDistancePt3(const pfVec3 a, const pfVec3 b);
float pfSqrt(float x);
void pfSinCos(float deg, float *s, float *c);
float pfArcCos(float x);
float pfArcTan2(float y, float x);

void pfMakeIdentMat(pfMatrix m);
void pfCopyMat(pfMatrix dst, pfMatrix src);
void pfMultMat(pfMatrix dst, pfMatrix a, pfMatrix b);
void pfMakeEulerMat(pfMatrix m, float h, float p, float r);
void pfMakeTransMat(pfMatrix m, float x, float y, float z);
void pfMakeCoordMat(pfMatrix m, const pfCoord *c);
void pfMakeVecRotVecMat(pfMatrix m, const pfVec3 v1, const pfVec3 v2);
void pfInvertOrthoMat(pfMatrix dst, pfMatrix m);
void pfInvertAffMat(pfMatrix dst, pfMatrix m);
void pfXformPt3(pfVec3 dst, const pfVec3 v, pfMatrix m);
void pfXformVec3(pfVec3 dst, const pfVec3 v, pfMatrix m);
void pfMakePtsSeg(pfSeg *seg, const pfVec3 p1, const pfVec3 p2);
int pfClosestPtsOnSeg(const pfSeg *s1, const pfSeg *s2, pfVec3 p1, pfVec3 p2);

/* --- memory ------------------------------------------------------------- */

void *pfGetSharedArena(void);
void *pfMalloc(size_t size, void *arena);
void *pfRealloc(void *p, size_t size);
void pfFree(void *p);
size_t pfGetSize(void *p);

double pfGetTime(void);

/* --- state -------------------------------------------------------------- */

typedef struct pfTexture {
    char *name;
    unsigned int gl_id;
    unsigned int *image;   /* RGBA, bottom row first */
    int xsize, ysize, comps;
    int wrap_s, wrap_t;    /* GL wrap mode, 0 for GL_REPEAT */
} pfTexture;

typedef struct pfTexEnv {
    int mode;              /* GL mode */
    pfVec4 blend;
} pfTexEnv;

typedef struct pfMaterial {
    float alpha, shininess;
    pfVec3 ambient, diffuse, specular, emission;
    int colormode;         /* 0 off, else GL_AMBIENT_AND_DIFFUSE */
} pfMaterial;

#define PFSTATE_TRANSPARENCY 0x10
#define PFSTATE_ENLIGHTING 0x1
#define PFSTATE_ENTEXTURE 0x4
#define PFSTATE_FRONTMTL 0x2
#define PFSTATE_TEXTURE 0x8
#define PFSTATE_TEXENV 0x80

#define PFEN_LIGHTING 1
#define PFEN_TEXTURE 2

typedef struct pfGeoState {
    int lighting, texturing, lpoint;   /* -1 inherit, 0 off, 1 on */
    int transparency;                  /* -1 inherit, 0 off, 1 on */
    unsigned int cullface;             /* GL face, 0 off, ~0 inherit */
    unsigned int alphafunc;            /* GL function, 0 off */
    float alpharef;
    pfMaterial *frontmtl;
    pfTexture *texture;
    pfTexEnv *texenv;
} pfGeoState;

/* --- geosets ------------------------------------------------------------ */

#define PFGS_POINTS 0
#define PFGS_LINES 1
#define PFGS_LINESTRIPS 2
#define PFGS_TRIS 3
#define PFGS_QUADS 4
#define PFGS_TRISTRIPS 5
#define PFGS_FLAT_LINESTRIPS 6
#define PFGS_FLAT_TRISTRIPS 7
#define PFGS_POLYS 8
#define PFGS_TRIFANS 9

#define PFGS_COLOR4 0
#define PFGS_NORMAL3 1
#define PFGS_TEXCOORD2 2
#define PFGS_COORD3 3

#define PFGS_OFF 0
#define PFGS_OVERALL 1
#define PFGS_PER_PRIM 2
#define PFGS_PER_VERTEX 3

typedef struct pfGeoSet {
    int prim, nprims;
    int *lengths;
    void *attr[4];         /* indexed by PFGS_COLOR4 ... PFGS_COORD3 */
    int bind[4];
    int nverts;            /* number of coordinates */
    int flatshade;
    float linewidth, pntsize;
    unsigned int isectmask;
    pfGeoState *gstate;
    int ntris;             /* intersection cache */
    int *tris;
} pfGeoSet;

pfGeoSet *pfNewGSet(void *arena);
void pfGSetPrimType(pfGeoSet *g, int type);
void pfGSetNumPrims(pfGeoSet *g, int n);
void pfGSetPrimLengths(pfGeoSet *g, int *lengths);
void pfGSetAttr(pfGeoSet *g, int attr, int bind, void *alist, unsigned short *ilist);
void pfGSetPntSize(pfGeoSet *g, float s);
void pfGSetLineWidth(pfGeoSet *g, float w);
void pfGSetGState(pfGeoSet *g, pfGeoState *gs);
pfGeoState *pfGetGSetGState(pfGeoSet *g);
void *pfGetGStateAttr(pfGeoState *gs, int attr);
void pfDrawGSet(pfGeoSet *g);

/* --- nodes -------------------------------------------------------------- */

enum {
    PFNODE_GROUP = 1, PFNODE_SCENE, PFNODE_DCS, PFNODE_SCS, PFNODE_SWITCH,
    PFNODE_SEQUENCE, PFNODE_LOD, PFNODE_LAYER, PFNODE_GEODE, PFNODE_MORPH,
    PFNODE_BBOARD, PFNODE_LSOURCE
};

typedef int pfType;
#define pfGetGroupClassType() (PFNODE_GROUP)
#define pfGetDCSClassType() (PFNODE_DCS)
#define pfGetGeodeClassType() (PFNODE_GEODE)

#define PFTRAV_ISECT 0
#define PFTRAV_APP 1
#define PFTRAV_CULL 2
#define PFTRAV_DRAW 3
#define PFTRAV_SELF 0x10
#define PFTRAV_DESCEND 0x20

#define PFSWITCH_ON (-1)
#define PFSWITCH_OFF (-2)

#define PFSEQ_STOP 0
#define PFSEQ_START 1
#define PFSEQ_PAUSE 2
#define PFSEQ_RESUME 3
#define PFSEQ_CYCLE 0
#define PFSEQ_SWING 1


typedef struct pfNode {
    int type;
    char *name;
    unsigned int travmask[4];
    void *userdata;
    /* groups */
    int nchildren, maxchildren;
    struct pfNode **children;
    /* geodes */
    int ngsets;
    pfGeoSet **gsets;
    /* DCS and SCS */
    pfMatrix mat;
    pfVec3 trans, rot, scale;
    /* switch */
    int switchval;
    /* sequence */
    int nframes;
    float *times;
    int seqmode, interval, begin, end, nreps;
    float speed;
    int frame, dir, reps;
    double frame_start;
    /* morph */
    struct PfbFile *pfb;
    int morph;
    /* light source */
    struct pfLightSource *lsource;
    /* billboard: one position per geoset, turning around Z (axial) */
    pfVec3 *bbpos;
    int bbmode;
} pfNode;

typedef pfNode pfGroup, pfDCS, pfSCS, pfSwitch, pfSequence, pfGeode, pfScene, pfMorph;

pfGroup *pfNewGroup(void);
pfScene *pfNewScene(void);
pfDCS *pfNewDCS(void);
pfSwitch *pfNewSwitch(void);
pfGeode *pfNewGeode(void);
int pfAddChild(pfNode *g, pfNode *child);
int pfInsertChild(pfNode *g, int index, pfNode *child);
int pfRemoveChild(pfNode *g, pfNode *child);
int pfReplaceChild(pfNode *g, pfNode *old, pfNode *n);
pfNode *pfGetChild(const pfNode *g, int i);
int pfGetNumChildren(const pfNode *g);
int pfAddGSet(pfNode *geode, pfGeoSet *g);
int pfGetNumGSets(const pfNode *geode);
pfGeoSet *pfGetGSet(const pfNode *geode, int i);
pfNode *pfClone(pfNode *n, int mode);
int pfIsOfType(const pfNode *n, pfType type);
int pfIsExactType(const pfNode *n, pfType type);
const char *pfGetNodeName(const pfNode *n);
pfNode *pfFindNode(pfNode *n, const char *name);

void pfNodeTravMask(pfNode *n, int which, unsigned int mask, int setmode, int bitop);
unsigned int pfGetNodeTravMask(const pfNode *n, int which);
void pfUserData(void *obj, void *data);
void *pfGetUserData(void *obj);

void pfDCSTrans(pfDCS *d, float x, float y, float z);
void pfDCSRot(pfDCS *d, float h, float p, float r);
void pfDCSScale(pfDCS *d, float s);
void pfDCSScaleXYZ(pfDCS *d, float x, float y, float z);
void pfDCSMat(pfDCS *d, pfMatrix m);
void pfGetDCSMat(pfDCS *d, pfMatrix m);

void pfSwitchVal(pfSwitch *s, int val);
int pfGetSwitchVal(const pfSwitch *s);

void pfSeqMode(pfSequence *s, int mode);
int pfGetSeqMode(const pfSequence *s);
int pfGetSeqFrame(const pfSequence *s, int *repeat);
void pfSeqInterval(pfSequence *s, int mode, int begin, int end);
void pfSeqDuration(pfSequence *s, float speed, int nreps);

void pfMorphWeights(pfMorph *m, int attr, float *weights);

/* --- intersection ------------------------------------------------------- */

#define PFIS_MAX_SEGS 32

typedef struct pfHit {
    int flags;
    int segnum;
    pfSeg seg;
    pfVec3 point, norm;
    pfNode *node;
    pfGeoSet *gset;
    int prim, tri;
    pfMatrix xform;
    int pathlen;
    pfNode *path[16];      /* nodes from the root of the intersection */
} pfHit;

typedef struct pfSegSet {
    int mode;
    void *userData;
    pfSeg segs[PFIS_MAX_SEGS];
    unsigned int activeMask;
    unsigned int isectMask;
    void *bound;
    int (*discFunc)(pfHit *);
} pfSegSet;

#define PFTRAV_IS_PRIM 0x1
#define PFTRAV_IS_GSET 0x2
#define PFTRAV_IS_GEODE 0x4
#define PFTRAV_IS_NORM 0x8
#define PFTRAV_IS_CULL_BACK 0x10
#define PFTRAV_IS_CULL_FRONT 0x20

#define PFHIT_NONE 0
#define PFHIT_ISECT 0x1
#define PFHIT_POINT 0x2
#define PFHIT_NORM 0x4
#define PFHIT_PRIM 0x8
#define PFHIT_TRI 0x10
#define PFHIT_XFORM 0x40

#define PFQHIT_FLAGS 1
#define PFQHIT_SEGNUM 2
#define PFQHIT_SEG 3
#define PFQHIT_POINT 4
#define PFQHIT_NORM 5
#define PFQHIT_GSET 9
#define PFQHIT_NODE 10
#define PFQHIT_XFORM 22
#define PFQHIT_PATH 23

int pfNodeIsectSegs(pfNode *n, pfSegSet *segset, pfHit **hits[]);
/* triangles of a geoset (index triples into the coordinates) */
int pfGetGSetTris(pfGeoSet *g, const int **tris);
int pfQueryHit(const pfHit *h, unsigned int which, void *dst);
/* node i of a hit path (PFQHIT_PATH) */
void *pfGet(void *path, int i);

/* --- lights ------------------------------------------------------------- */

#define PFLT_AMBIENT 0x1200
#define PFLT_DIFFUSE 0x1201
#define PFLT_SPECULAR 0x1202

typedef struct pfLightSource {
    pfVec4 pos;
    pfVec3 ambient, diffuse, specular;
    int on;
} pfLightSource;

pfLightSource *pfNewLSource(void);
void pfLSourcePos(pfLightSource *l, float x, float y, float z, float w);
void pfLSourceColor(pfLightSource *l, int which, float r, float g, float b);
void pfLSourceOn(pfLightSource *l);
void pfLSourceOff(pfLightSource *l);
pfNode *pfLSourceNode(pfLightSource *l);

/* --- loading and drawing ------------------------------------------------ */

void pfFilePath(const char *path);
int pfFindFile(const char *file, char *path, int amode);
pfNode *pfdLoadFile(const char *file);
pfTexture *pfNewTex(void *arena);
int pfLoadTexFile(pfTexture *t, const char *file);
void pfGetTexImage(pfTexture *t, unsigned int **image, int *comp, int *ns, int *nt, int *nr);
void pfApplyTex(pfTexture *t);
void pfTexFilter(pfTexture *t, int filt, int val);

void pfEnable(int what);
void pfDisable(int what);
int pfGetEnable(int what);
void pfPushState(void);
void pfPopState(void);

/* draws a scene with the current OpenGL projection and modelview */
void pfDrawScene(pfNode *root, double time);
/* one pass of pfDrawScene: 0 opaque geometry, 1 transparent geometry */
void pfDrawScenePass(pfNode *root, double time, int pass);
void pfFrameTime(double t);
double pfGetFrameTime(void);

#endif
