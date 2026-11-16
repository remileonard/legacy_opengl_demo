/*
 * Performer/pf.h - IRIS Performer 2.0 libpf/libpr on top of OpenGL 1.x and
 * freeglut, for the sample programs of Performer (perfly and its common
 * code) compiled unchanged.
 *
 * The scene graph, the geosets and the loaders are the Performer layer of
 * Mekton (src/mekton/pf.h); this header adds the rest of the API used by
 * the samples: pipes, windows and channels, the frame loop, the Earth/sky
 * and the fog, frustums, statistics, strings and the libpr state calls.
 *
 * Everything runs in one process: pfConfig forks nothing, the APP, CULL and
 * DRAW callbacks of the channels are called in turn by pfFrame.
 */
#ifndef PERFORMER_PF_H
#define PERFORMER_PF_H

#ifdef _WIN32
/* the GL headers of Windows without windows.h, whose names (WriteFile,
 * near, far...) are those of variables of the Performer programs */
#ifndef APIENTRY
#define APIENTRY __stdcall
#endif
#ifndef WINGDIAPI
#define WINGDIAPI __declspec(dllimport)
#endif
#ifndef CALLBACK
#define CALLBACK __stdcall
#endif
#include <io.h>
#undef near
#undef far
#ifndef R_OK
#define R_OK 4
#endif
#define random() rand()
#define srandom(s) srand(s)
#define strncasecmp _strnicmp
#define strcasecmp _stricmp
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;
#else
#include <sys/types.h>
#include <unistd.h>
#include <strings.h>
#endif

#ifdef __APPLE__
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../../../mekton/pf.h"

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#define PF_X 0
#define PF_Y 1
#define PF_Z 2
#define PF_W 3
#define PF_H 0
#define PF_P 1
#define PF_R 2

#define PF_MAXSTRING 256
#define PF_MIN2(a, b) ((a) < (b) ? (a) : (b))
#define PF_MAX2(a, b) ((a) > (b) ? (a) : (b))
#define PF_MIN3(a, b, c) PF_MIN2(PF_MIN2(a, b), c)
#define PF_MAX3(a, b, c) PF_MAX2(PF_MAX2(a, b), c)
#define PF_SQUARE(x) ((x) * (x))
#define PF_CLAMP(x, lo, hi) ((x) < (lo) ? (lo) : (x) > (hi) ? (hi) : (x))

#define PFSET_VEC3(v, x, y, z) ((v)[0] = (x), (v)[1] = (y), (v)[2] = (z))
#define PFSET_VEC4(v, x, y, z, w) ((v)[0] = (x), (v)[1] = (y), (v)[2] = (z), (v)[3] = (w))
#define PFCOPY_VEC3(d, s) ((d)[0] = (s)[0], (d)[1] = (s)[1], (d)[2] = (s)[2])
#define PFEQUAL_VEC3(a, b) ((a)[0] == (b)[0] && (a)[1] == (b)[1] && (a)[2] == (b)[2])
#define PFALMOST_EQUAL_VEC3(a, b, t) (PF_ABS((a)[0] - (b)[0]) <= (t) && \
    PF_ABS((a)[1] - (b)[1]) <= (t) && PF_ABS((a)[2] - (b)[2]) <= (t))

typedef struct { pfVec3 min, max; } pfBox;
typedef struct { pfVec3 center; float radius; } pfSphere;

/* --- math --------------------------------------------------------------- */

void pfSetVec4(pfVec4 v, float x, float y, float z, float w);
void pfCopyVec4(pfVec4 d, const pfVec4 s);
void pfCombineVec3(pfVec3 d, float a, const pfVec3 u, float b, const pfVec3 v);
float pfTan(float deg);
float pfArcSin(float x);
void pfMakeRotMat(pfMatrix m, float deg, float x, float y, float z);
void pfMakeScaleMat(pfMatrix m, float x, float y, float z);
void pfPreTransMat(pfMatrix d, float x, float y, float z, pfMatrix m);
void pfPostTransMat(pfMatrix d, pfMatrix m, float x, float y, float z);
void pfPreRotMat(pfMatrix d, float deg, float x, float y, float z, pfMatrix m);
void pfPostRotMat(pfMatrix d, pfMatrix m, float deg, float x, float y, float z);
void pfPreMultMat(pfMatrix m, pfMatrix a);
void pfPostMultMat(pfMatrix m, pfMatrix a);
void pfGetOrthoMatCoord(pfMatrix m, pfCoord *c);
void pfInvertFullMat(pfMatrix d, pfMatrix m);

/* --- memory, objects ------------------------------------------------------ */

void *pfCalloc(size_t n, size_t size, void *arena);
/* frees nothing: the objects of a scene graph are shared */
int pfDelete(void *obj);
/* copies a geostate (the only object the samples copy) */
int pfCopy(void *dst, void *src);
#define PFPRINT_VB_OFF 0
#define PFPRINT_VB_ON 1
#define PFPRINT_VB_NOTICE 1
#define PFPRINT_VB_INFO 2
#define PFPRINT_VB_DEBUG 3
/* prints a node and its children, or a pfFrameStats */
void pfPrint(void *obj, unsigned int which, unsigned int verbose, FILE *fp);
pfType pfGetType(const void *obj);

typedef struct pfList pfList;
pfList *pfNewList(int eltsize, int len, void *arena);
void pfAdd(pfList *l, void *elt);
int pfGetNum(const pfList *l);
void *pfGetListElt(const pfList *l, int i);

/* --- notification ---------------------------------------------------------- */

#define PFNFY_ALWAYS 0
#define PFNFY_FATAL 1
#define PFNFY_WARN 2
#define PFNFY_NOTICE 3
#define PFNFY_INFO 4
#define PFNFY_DEBUG 5
#define PFNFY_FP_DEBUG 6

#define PFNFY_MORE 0
#define PFNFY_USAGE 1
#define PFNFY_RESOURCE 2
#define PFNFY_SYSERR 3
#define PFNFY_ASSERT 4
#define PFNFY_PRINT 5
#define PFNFY_INTERNAL 6
#define PFNFY_FP_OVERFLOW 7

void pfNotifyLevel(int level);
int pfGetNotifyLevel(void);
void pfNotify(int level, int code, const char *fmt, ...);

/* --- initialization and processes ----------------------------------------- */

#define PFMP_DEFAULT (-1)
#define PFMP_APPCULLDRAW 0x0
#define PFMP_FORK_ISECT 0x1
#define PFMP_FORK_CULL 0x2
#define PFMP_FORK_DRAW 0x4
#define PFMP_CULLoDRAW 0x8
#define PFMP_FORK_DBASE 0x10
#define PFMP_APP_CULLDRAW PFMP_FORK_CULL
#define PFMP_APPCULL_DRAW PFMP_FORK_DRAW
#define PFMP_APP_CULL_DRAW (PFMP_FORK_CULL | PFMP_FORK_DRAW)
#define PFMP_CULL PFMP_FORK_CULL

#define PFPROC_APP 0x1
#define PFPROC_CULL 0x2
#define PFPROC_DRAW 0x4
#define PFPROC_ISECT 0x8
#define PFPROC_DBASE 0x10

#define PFPHASE_FLOAT 0
#define PFPHASE_LOCK 1
#define PFPHASE_FREE_RUN 2
#define PFPHASE_LIMIT 3

int pfInit(void);
int pfMultiprocess(int mode);
int pfGetMultiprocess(void);
int pfMultipipe(int npipes);
int pfGetMultipipe(void);
int pfHyperpipe(int npipes);
int pfConfig(void);
void pfExit(void);
void pfIsectFunc(void (*func)(void *));
typedef void (*pfStageFuncType)(int pipe, unsigned int stage);
void pfStageConfigFunc(int pipe, unsigned int stage, pfStageFuncType func);
int pfConfigStage(int pipe, unsigned int stage);

void pfFrameRate(float rate);
float pfGetFrameRate(void);
void pfFieldRate(int fields);
int pfGetFieldRate(void);
int pfGetVideoRate(void);
void pfPhase(int phase);
int pfGetPhase(void);
int pfSync(void);
int pfFrame(void);
int pfGetFrameCount(void);
/* the traversals inside the channel callbacks */
void pfApp(void);
void pfCull(void);
void pfDraw(void);

/* --- features -------------------------------------------------------------- */

#define PFQFTR_FALSE 0
#define PFQFTR_TRUE 1
#define PFQFTR_FAST 2
#define PFQFTR_SLOW 3

#define PFQFTR_MULTISAMPLE 1
#define PFQFTR_TAG_CLEAR 2
#define PFQFTR_TEXTURE 3
#define PFQFTR_FOG_SPLINE 4
#define PFQFTR_STEREO_IN_WINDOW 5
#define PFQFTR_DISPLACE_POLYGON 6

int pfQueryFeature(int which, int *dst);
/* the features of a 0 ended list of tokens */
int pfMQueryFeature(int *which, int *dst);

/* --- windows ---------------------------------------------------------------- */

typedef struct pfWindow pfWindow;
typedef struct pfPipe pfPipe;
typedef struct pfPipeWindow pfPipeWindow;
typedef struct pfChannel pfChannel;
typedef struct pfPipeVideoChannel pfPipeVideoChannel;
typedef struct pfEarthSky pfEarthSky;
typedef struct pfFog pfFog;
typedef struct pfFrustum pfFrustum;
typedef pfFrustum pfPolytope;
typedef struct pfFrameStats pfFrameStats;
typedef struct pfLightModel pfLightModel;
typedef struct pfLight pfLight;
typedef struct pfFont pfFont;
typedef struct pfString pfString;
typedef struct pfHighlight pfHighlight;
typedef struct pfDispList pfDispList;
typedef pfNode pfPartition, pfBillboard;

#define PFWIN_GFX_WIN 1
#define PFWIN_OVERLAY_WIN 2
#define PFWIN_STATS_WIN 3

#define PFPWIN_TYPE_X 0x0
#define PFPWIN_TYPE_STATS 0x8

/* a window opened at once, made current: for the queries of the renderer */
pfWindow *pfOpenNewNoPortWin(const char *name, int screen);
void pfCloseWin(pfWindow *w);
void pfSelectWin(pfWindow *w);

pfPipe *pfGetPipe(int i);
int pfGetId(void *obj);
void pfPipeScreen(pfPipe *p, int screen);
void pfGetPipeSize(pfPipe *p, int *xs, int *ys);
pfPipeWindow *pfGetPipePWin(pfPipe *p, int i);
int pfGetPipeNumPWins(pfPipe *p);

pfPipeWindow *pfNewPWin(pfPipe *p);
void pfPWinName(pfPipeWindow *pw, const char *name);
void pfPWinOriginSize(pfPipeWindow *pw, int xo, int yo, int xs, int ys);
void pfGetPWinSize(pfPipeWindow *pw, int *xs, int *ys);
void pfGetPWinOrigin(pfPipeWindow *pw, int *xo, int *yo);
typedef void (*pfPWinFuncType)(pfPipeWindow *pw);
void pfPWinConfigFunc(pfPipeWindow *pw, pfPWinFuncType func);
void pfPWinFBConfigId(pfPipeWindow *pw, int id);
/* calls the configuration function of the window (it opens the window) */
void pfConfigPWin(pfPipeWindow *pw);
void pfOpenPWin(pfPipeWindow *pw);
void pfClosePWin(pfPipeWindow *pw);
pfPipe *pfGetPWinPipe(pfPipeWindow *pw);
void pfPWinIndex(pfPipeWindow *pw, int index);
int pfGetPWinIndex(pfPipeWindow *pw);
pfWindow *pfGetPWinSelect(pfPipeWindow *pw);
pfWindow *pfGetPWinOverlayWin(pfPipeWindow *pw);
int pfGetPWinType(pfPipeWindow *pw);
void pfSelectPWin(pfPipeWindow *pw);
void pfSwapPWinBuffers(pfPipeWindow *pw);

/* --- channels --------------------------------------------------------------- */

#define PFCHAN_FOV 0x1
#define PFCHAN_VIEW 0x2
#define PFCHAN_VIEW_OFFSETS 0x4
#define PFCHAN_NEARFAR 0x8
#define PFCHAN_SCENE 0x10
#define PFCHAN_EARTHSKY 0x20
#define PFCHAN_STRESS 0x40
#define PFCHAN_LOD 0x80
#define PFCHAN_SWAPBUFFERS 0x100
#define PFCHAN_APPFUNC 0x200
#define PFCHAN_CULLFUNC 0x400
#define PFCHAN_DRAWFUNC 0x800
#define PFCHAN_VIEWPORT 0x1000
#define PFCHAN_SWAPBUFFERS_HW 0x2000
#define PFCHAN_STATS_DRAWMODE 0x4000
#define PFCHAN_CULL_VOLUME 0x8000

#define PFCHAN_PROJ_VIEWPORT 0
#define PFCHAN_PROJ_WINDOW 1

#define PFFRUST_CALC_NONE 0
#define PFFRUST_CALC_HORIZ 1
#define PFFRUST_CALC_VERT 2

#define PFCULL_VIEW 0x1
#define PFCULL_SORT 0x2
#define PFCULL_GSET 0x4
#define PFCULL_IGNORE_LSOURCES 0x8
#define PFCULL_ALL (PFCULL_VIEW | PFCULL_SORT | PFCULL_GSET)

#define PFSORT_OPAQUE_BIN 0
#define PFSORT_TRANSP_BIN 1
#define PFSORT_NO_ORDER 0
#define PFSORT_FRONT_TO_BACK 1
#define PFSORT_BACK_TO_FRONT 2

#define PFLOD_SCALE 1
#define PFLOD_FADE 2
#define PFLOD_STRESS_PIX_LIMIT 3

#define PFCSTATS_DRAW 1

typedef void (*pfChanFuncType)(pfChannel *chan, void *data);

pfChannel *pfNewChan(pfPipe *p);
pfPipe *pfGetChanPipe(pfChannel *c);
pfPipeWindow *pfGetChanPWin(pfChannel *c);
void pfAttachChan(pfChannel *master, pfChannel *c);
void pfChanShare(pfChannel *c, unsigned int mask);
unsigned int pfGetChanShare(pfChannel *c);
void pfChanTravFunc(pfChannel *c, int trav, pfChanFuncType func);
void pfChanScene(pfChannel *c, pfScene *s);
pfScene *pfGetChanScene(pfChannel *c);
void pfChanESky(pfChannel *c, pfEarthSky *es);
void pfChanNearFar(pfChannel *c, float n, float f);
void pfGetChanNearFar(pfChannel *c, float *n, float *f);
void pfChanFOV(pfChannel *c, float h, float v);
void pfGetChanFOV(pfChannel *c, float *h, float *v);
void pfChanAutoAspect(pfChannel *c, int which);
void pfMakeSimpleChan(pfChannel *c, float fov);
void pfChanView(pfChannel *c, pfVec3 xyz, pfVec3 hpr);
void pfGetChanView(pfChannel *c, pfVec3 xyz, pfVec3 hpr);
void pfChanViewMat(pfChannel *c, pfMatrix m);
void pfGetChanViewMat(pfChannel *c, pfMatrix m);
void pfChanViewOffsets(pfChannel *c, pfVec3 xyz, pfVec3 hpr);
void pfGetChanOffsetViewMat(pfChannel *c, pfMatrix m);
void pfChanViewport(pfChannel *c, float l, float r, float b, float t);
void pfGetChanViewport(pfChannel *c, float *l, float *r, float *b, float *t);
void pfGetChanOrigin(pfChannel *c, int *xo, int *yo);
void pfGetChanSize(pfChannel *c, int *xs, int *ys);
void pfGetChanOutputOrigin(pfChannel *c, int *xo, int *yo);
void pfGetChanOutputSize(pfChannel *c, int *xs, int *ys);
void pfChanProjMode(pfChannel *c, int mode);
void pfChanBinOrder(pfChannel *c, int bin, int order);
void pfChanTravMode(pfChannel *c, int trav, int mode);
int pfGetChanTravMode(pfChannel *c, int trav);
void pfChanLODAttr(pfChannel *c, int attr, float val);
float pfGetChanLODAttr(pfChannel *c, int attr);
void pfChanStressFilter(pfChannel *c, float frac, float low, float high, float scale, float max);
void pfChanStatsMode(pfChannel *c, unsigned int mode, unsigned int val);
void pfChanCullPtope(pfChannel *c, pfPolytope *ptope);
void pfGetChanBaseFrust(pfChannel *c, pfFrustum *f);
pfFrameStats *pfGetChanFStats(pfChannel *c);
pfPipeVideoChannel *pfGetChanPVChan(pfChannel *c);
void pfClearChan(pfChannel *c);
void pfDrawChanStats(pfChannel *c);
/* the channel whose callback is running */
pfChannel *pfGetCurChan(void);

/* dynamic video resolution: the sizes are kept, nothing is resized */
#define PFPVC_DVR_OFF 0
#define PFPVC_DVR_MANUAL 1
#define PFPVC_DVR_AUTO 2
#define PFPVC_DVR_AUTOMATIC PFPVC_DVR_AUTO
void pfGetPVChanSize(pfPipeVideoChannel *v, int *xs, int *ys);
void pfPVChanOutputSize(pfPipeVideoChannel *v, int xs, int ys);
void pfGetPVChanOutputSize(pfPipeVideoChannel *v, int *xs, int *ys);
void pfPVChanDVRMode(pfPipeVideoChannel *v, int mode);
void pfPVChanStressFilter(pfPipeVideoChannel *v, float frac, float low, float high,
                          float pipeload, float scale, float max);

/* --- scenes and nodes --------------------------------------------------------- */

#define PFBOUND_STATIC 1
#define PFBOUND_DYNAMIC 0

pfSCS *pfNewSCS(pfMatrix m);
pfPartition *pfNewPart(void);
#define PFPART_DEBUG 1
#define PFPART_ORIGIN 1
#define PFPART_MIN_SPACING 2
#define PFPART_MAX_SPACING 3
void pfPartAttr(pfPartition *p, int which, void *attr);
void pfPartVal(pfPartition *p, int which, float val);
void pfBuildPart(pfPartition *p);
void pfUpdatePart(pfPartition *p);
int pfGetNodeBSphere(pfNode *n, pfSphere *s);
int pfNodeBSphere(pfNode *n, pfSphere *s, int mode);
/* applies the static transformations under a node to its geometry */
int pfFlatten(pfNode *n, int mode);
void pfSceneGState(pfScene *s, pfGeoState *gs);
pfGeoState *pfGetSceneGState(pfScene *s);
void pfGetGSetAttrLists(pfGeoSet *g, int attr, void **alist, unsigned short **ilist);

/* --- state ------------------------------------------------------------------- */

#define PFAA_OFF 0
#define PFAA_ON 1

#define PFMTL_FRONT 0x1
#define PFMTL_BACK 0x2
#define PFMTL_BOTH 0x3
#define PFMTL_CMODE_OFF 0
#define PFMTL_CMODE_AD 1
#define PFMTL_CMODE_AMBIENT_AND_DIFFUSE PFMTL_CMODE_AD

#define PFTEX_SHOW 1

pfGeoState *pfNewGState(void *arena);
void pfMakeBasicGState(pfGeoState *gs);
pfMaterial *pfNewMtl(void *arena);
void pfMtlColorMode(pfMaterial *m, int side, int mode);
void pfApplyMtl(pfMaterial *m);
pfLightModel *pfNewLModel(void *arena);
void pfLModelAmbient(pfLightModel *lm, float r, float g, float b);
void pfLModelLocal(pfLightModel *lm, int local);
void pfApplyLModel(pfLightModel *lm);
void pfBasicState(void);
void pfAntialias(int mode);

pfLight *pfNewLight(void *arena);
void pfLightPos(pfLight *l, float x, float y, float z, float w);
void pfLightColor(pfLight *l, int which, float r, float g, float b);
void pfLightOn(pfLight *l);
void pfLightOff(pfLight *l);

#define PFCL_COLOR 0x1
#define PFCL_DEPTH 0x2
#define PFCL_MSDEPTH 0x4
#define PFCL_STENCIL 0x8
void pfClear(int which, const pfVec4 col);

void pfPushMatrix(void);
void pfPopMatrix(void);
void pfPushIdentMatrix(void);
void pfLoadMatrix(pfMatrix m);
void pfMultMatrix(pfMatrix m);
void pfTranslate(float x, float y, float z);
void pfScale(float x, float y, float z);
void pfRotate(int axis, float deg);

/* --- frustums ----------------------------------------------------------------- */

pfFrustum *pfNewFrust(void *arena);
void pfMakeSimpleFrust(pfFrustum *f, float fov);
void pfMakePerspFrust(pfFrustum *f, float l, float r, float b, float t);
void pfFrustNearFar(pfFrustum *f, float n, float fa);
void pfGetFrustNearFar(const pfFrustum *f, float *n, float *fa);
/* the corners of the near plane, in the frame of the eye (Y forward, Z up) */
void pfGetFrustNear(const pfFrustum *f, pfVec3 ll, pfVec3 lr, pfVec3 ul, pfVec3 ur);
void pfApplyFrust(pfFrustum *f);

/* --- Earth/sky and fog -------------------------------------------------------- */

#define PFES_BUFFER_CLEAR 1
#define PFES_FAST 0
#define PFES_TAG 1
#define PFES_SKY 2
#define PFES_SKY_GRND 3
#define PFES_SKY_CLEAR 4

#define PFES_SKY_TOP 1
#define PFES_SKY_BOT 2
#define PFES_HORIZ 3
#define PFES_GRND_FAR 4
#define PFES_GRND_NEAR 5
#define PFES_CLEAR 6

#define PFES_GRND_HT 1
#define PFES_HORIZ_ANGLE 2
#define PFES_GENERAL 1
#define PFES_CLOUD_TOP 2
#define PFES_CLOUD_BOT 3

pfEarthSky *pfNewESky(void);
void pfESkyMode(pfEarthSky *es, int which, int mode);
int pfGetESkyMode(pfEarthSky *es, int which);
void pfESkyAttr(pfEarthSky *es, int which, float val);
float pfGetESkyAttr(pfEarthSky *es, int which);
void pfESkyColor(pfEarthSky *es, int which, float r, float g, float b, float a);
void pfGetESkyColor(pfEarthSky *es, int which, float *r, float *g, float *b, float *a);
void pfESkyFog(pfEarthSky *es, int which, pfFog *fog);

#define PFFOG_OFF 0
#define PFFOG_PIX_LIN 1
#define PFFOG_PIX_EXP 2
#define PFFOG_PIX_EXP2 3
#define PFFOG_PIX_SPLINE 4
#define PFFOG_VTX_LIN 5
#define PFFOG_VTX_EXP 6
#define PFFOG_VTX_EXP2 7

pfFog *pfNewFog(void *arena);
void pfFogType(pfFog *f, int type);
void pfFogColor(pfFog *f, float r, float g, float b);
void pfFogRange(pfFog *f, float onset, float opaque);
void pfApplyFog(pfFog *f);

/* --- statistics --------------------------------------------------------------- */

#define PFSTATS_OFF 0
#define PFSTATS_ON 1
#define PFSTATS_DEFAULT 2
#define PFSTATS_SET 3

#define PFFSTATS_ENPFTIMES 0x1
#define PFFSTATS_ENDB 0x2
#define PFFSTATS_ENCULL 0x4
#define PFSTATS_ENGFX 0x10
#define PFFSTATS_ENGFXPFTIMES 0x20
#define PFSTATSHW_ENGFXPIPE_TIMES 0x40
#define PFSTATSHW_ENGFXPIPE_FILL 0x80
#define PFSTATS_ALL 0xff

#define PFFSTATS_PFTIMES PFFSTATS_ENPFTIMES
#define PFSTATS_GFX PFSTATS_ENGFX
#define PFSTATSHW_GFXPIPE_FILL PFSTATSHW_ENGFXPIPE_FILL
#define PFFSTATS_PFTIMES_BASIC 0x1
#define PFFSTATS_PFTIMES_MASK 0xf
#define PFSTATSHW_GFXPIPE_FILL_DEPTHCMP 0x1
#define PFSTATSHW_GFXPIPE_FILL_TRANSPARENT 0x2
#define PFSTATS_GFX_ATTR_COUNTS 0x1
#define PFSTATS_GFX_TSTRIP_LENGTHS 0x2

#define PFFSTATS_UPDATE_FRAMES 1
#define PFFSTATS_UPDATE_SECS 2
#define PFFSTATS_BUF_PREV 0x100
#define PFFSTATS_BUF_CUR 0x200
#define PFFSTATS_BUF_AVG 0x400

void pfFStatsAttr(pfFrameStats *fs, int attr, float val);
unsigned int pfFStatsClass(pfFrameStats *fs, unsigned int mask, int val);
unsigned int pfGetFStatsClass(pfFrameStats *fs, unsigned int mask);
void pfFStatsClassMode(pfFrameStats *fs, int cls, unsigned int mask, int val);

/* --- highlights --------------------------------------------------------------- */

#define PFHL_LINES 0x1
#define PFHL_LINESPAT 0x2
#define PFHL_FILL 0x4
#define PFHL_FILLPAT 0x8
#define PFHL_FILLTEX 0x10
#define PFHL_SKIP_BASE 0x20
#define PFHL_POINTS 0x40
#define PFHL_NORMALS 0x80
#define PFHL_BBOX_LINES 0x100
#define PFHL_BBOX_FILL 0x200
#define PFHL_LINES_R 0x400
void pfHlightMode(pfHighlight *h, unsigned int mode);

/* --- strings and fonts -------------------------------------------------------- */

#define PFSTR_JUSTIFY 1
#define PFSTR_LEFT 0
#define PFSTR_MIDDLE 1
#define PFSTR_RIGHT 2

pfString *pfNewString(void *arena);
void pfStringMode(pfString *s, int mode, int val);
void pfStringFont(pfString *s, pfFont *f);
void pfStringColor(pfString *s, float r, float g, float b, float a);
void pfStringString(pfString *s, const char *text);
void pfStringMat(pfString *s, pfMatrix m);
void pfFlattenString(pfString *s);
const pfBox *pfGetStringBBox(pfString *s);
void pfDrawString(pfString *s);

#endif
