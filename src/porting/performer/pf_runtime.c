/*
 * pf_runtime.c - libpf of IRIS Performer 2.0 on freeglut: the pipes and
 * their windows, the channels, the frame loop, the Earth/sky, the fog, the
 * frustums and the statistics. See Performer/pf.h.
 */
/* freeglut first: windows.h before the GL headers of Performer/pf.h */
#include <GL/freeglut.h>

#include "pf_internal.h"
#include <X11/Xlib.h>
#include <math.h>
#include <stdarg.h>
#ifndef _WIN32
#include <time.h>
#endif

#ifndef GL_MULTISAMPLE
#define GL_MULTISAMPLE 0x809D
#endif

#define PFI_VIDEO_RATE 60

static pfPipe pipes[PFI_MAX_PIPES];
static pfPipeWindow pwins[PFI_MAX_PIPES];
static int npipes = 1, npipes_made;
static pfChannel *chans[PFI_MAX_CHANS];
static int nchans;
static pfChannel *cur_chan;
static int frame_count;
static float frame_rate = 30;
static int phase = PFPHASE_FREE_RUN;
static int mp_mode = PFMP_DEFAULT;
static int notify_level = PFNFY_NOTICE;
static void (*isect_func)(void *);
static void (*win_hook)(pfPipeWindow *pw);
static double last_frame_end = -1, last_sync_sleep, frame_start;

/* --- notification ----------------------------------------------------------- */

void pfNotifyLevel(int level) { notify_level = level; }
int pfGetNotifyLevel(void) { return notify_level; }

void pfNotify(int level, int code, const char *fmt, ...)
{
    static const char *names[] = {
        "", "Fatal", "Warning", "Notice", "Info", "Debug", "FPDebug"
    };
    static int last_level = PFNFY_INFO;
    va_list ap;
    if (code == PFNFY_MORE)
        level = last_level;
    else
        last_level = level;
    if (level > notify_level || !fmt)
        return;
    if (code == PFNFY_MORE)
        fprintf(stderr, "PF           ");
    else
        fprintf(stderr, "PF %-9s ", level > 0 && level <= 6 ? names[level] : "");
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    if (fmt[0] && fmt[strlen(fmt) - 1] != '\n')
        fputc('\n', stderr);
    fflush(stderr);
    if (level == PFNFY_FATAL)
        exit(1);
}

/* --- math -------------------------------------------------------------------- */

void pfSetVec4(pfVec4 v, float x, float y, float z, float w) { v[0] = x; v[1] = y; v[2] = z; v[3] = w; }
void pfCopyVec4(pfVec4 d, const pfVec4 s) { d[0] = s[0]; d[1] = s[1]; d[2] = s[2]; d[3] = s[3]; }

void pfCombineVec3(pfVec3 d, float a, const pfVec3 u, float b, const pfVec3 v)
{
    d[0] = a * u[0] + b * v[0];
    d[1] = a * u[1] + b * v[1];
    d[2] = a * u[2] + b * v[2];
}

float pfTan(float deg) { return (float)tan(PF_DEG2RAD(deg)); }
float pfArcSin(float x) { return (float)PF_RAD2DEG(asin(x < -1 ? -1 : x > 1 ? 1 : x)); }

/* rotation of deg around (x, y, z), for row vectors (v' = v m) */
void pfMakeRotMat(pfMatrix m, float deg, float x, float y, float z)
{
    float l = (float)sqrt(x * x + y * y + z * z), s, c, t;
    pfMakeIdentMat(m);
    if (l <= 0)
        return;
    x /= l; y /= l; z /= l;
    s = (float)sin(PF_DEG2RAD(deg));
    c = (float)cos(PF_DEG2RAD(deg));
    t = 1 - c;
    m[0][0] = t * x * x + c;     m[0][1] = t * x * y + s * z; m[0][2] = t * x * z - s * y;
    m[1][0] = t * x * y - s * z; m[1][1] = t * y * y + c;     m[1][2] = t * y * z + s * x;
    m[2][0] = t * x * z + s * y; m[2][1] = t * y * z - s * x; m[2][2] = t * z * z + c;
}

void pfMakeScaleMat(pfMatrix m, float x, float y, float z)
{
    pfMakeIdentMat(m);
    m[0][0] = x; m[1][1] = y; m[2][2] = z;
}

void pfPreMultMat(pfMatrix m, pfMatrix a)
{
    pfMatrix t;
    pfMultMat(t, a, m);
    pfCopyMat(m, t);
}

void pfPostMultMat(pfMatrix m, pfMatrix a)
{
    pfMatrix t;
    pfMultMat(t, m, a);
    pfCopyMat(m, t);
}

void pfPreTransMat(pfMatrix d, float x, float y, float z, pfMatrix m)
{
    pfMatrix t;
    pfMakeTransMat(t, x, y, z);
    pfMultMat(d, t, m);
}

void pfPostTransMat(pfMatrix d, pfMatrix m, float x, float y, float z)
{
    pfMatrix t;
    pfMakeTransMat(t, x, y, z);
    pfMultMat(d, m, t);
}

void pfPreRotMat(pfMatrix d, float deg, float x, float y, float z, pfMatrix m)
{
    pfMatrix t;
    pfMakeRotMat(t, deg, x, y, z);
    pfMultMat(d, t, m);
}

void pfPostRotMat(pfMatrix d, pfMatrix m, float deg, float x, float y, float z)
{
    pfMatrix t;
    pfMakeRotMat(t, deg, x, y, z);
    pfMultMat(d, m, t);
}

/* the position and the Euler angles of an orthonormal matrix */
void pfGetOrthoMatCoord(pfMatrix m, pfCoord *c)
{
    float cp;
    pfSetVec3(c->xyz, m[3][0], m[3][1], m[3][2]);
    c->hpr[1] = pfArcSin(m[1][2]);
    cp = (float)cos(PF_DEG2RAD(c->hpr[1]));
    if (cp > 1e-6f) {
        c->hpr[0] = (float)PF_RAD2DEG(atan2(-m[1][0], m[1][1]));
        c->hpr[2] = (float)PF_RAD2DEG(atan2(-m[0][2], m[2][2]));
    } else {
        c->hpr[0] = (float)PF_RAD2DEG(atan2(m[0][1], m[0][0]));
        c->hpr[2] = 0;
    }
}

void pfInvertFullMat(pfMatrix d, pfMatrix m)
{
    pfInvertAffMat(d, m);
}

/* --- objects ------------------------------------------------------------------ */

void *pfCalloc(size_t n, size_t size, void *arena)
{
    return pfMalloc(n * size, arena);
}

int pfDelete(void *obj)
{
    (void)obj;
    return 0;
}

int pfCopy(void *dst, void *src)
{
    if (!dst || !src)
        return -1;
    memcpy(dst, src, sizeof(pfGeoState));
    return 0;
}

pfType pfGetType(const void *obj) { return obj ? *(const int *)obj : 0; }

static const char *node_type_name(int type)
{
    static const char *names[] = {
        "pfNode", "pfGroup", "pfScene", "pfDCS", "pfSCS", "pfSwitch", "pfSequence",
        "pfLOD", "pfLayer", "pfGeode", "pfMorph", "pfBillboard", "pfLightSource"
    };
    return type >= 0 && type <= PFNODE_LSOURCE ? names[type] : "pfObject";
}

static void print_node(pfNode *n, int depth, unsigned int which, unsigned int verbose, FILE *fp)
{
    int i;
    if (!n)
        return;
    fprintf(fp, "%*s%s", 2 * depth, "", node_type_name(n->type));
    if (n->name)
        fprintf(fp, " \"%s\"", n->name);
    if (n->ngsets)
        fprintf(fp, " %d geosets", n->ngsets);
    if (n->nchildren)
        fprintf(fp, " %d children", n->nchildren);
    fputc('\n', fp);
    if (verbose)
        for (i = 0; i < n->ngsets; i++)
            fprintf(fp, "%*s  pfGeoSet prim %d nprims %d\n", 2 * depth, "",
                    n->gsets[i]->prim, n->gsets[i]->nprims);
    if (which & PFTRAV_DESCEND)
        for (i = 0; i < n->nchildren; i++)
            print_node(n->children[i], depth + 1, which, verbose, fp);
}

void pfPrint(void *obj, unsigned int which, unsigned int verbose, FILE *fp)
{
    int type = pfGetType(obj);
    if (!fp)
        fp = stderr;
    if (type == PFI_TYPE_FSTATS) {
        pfFrameStats *fs = (pfFrameStats *)obj;
        fprintf(fp, "Frame statistics: %.1f Hz, frame %.2f ms, app %.2f ms, cull %.2f ms, draw %.2f ms\n",
                fs->fps, fs->ms_frame, fs->ms_app, fs->ms_cull, fs->ms_draw);
        fprintf(fp, "    %d geosets, %d primitives, %d vertices drawn\n",
                fs->gsets, fs->prims, fs->verts);
        return;
    }
    if (type > 0 && type <= PFNODE_LSOURCE)
        print_node((pfNode *)obj, 0, which, verbose, fp);
    else
        fprintf(fp, "pfObject type %d\n", type);
}

pfList *pfNewList(int eltsize, int len, void *arena)
{
    pfList *l = (pfList *)pfMalloc(sizeof(pfList), arena);
    (void)eltsize;
    l->type = PFI_TYPE_LIST;
    l->max = len > 0 ? len : 4;
    l->elts = (void **)calloc(l->max, sizeof(void *));
    return l;
}

void pfAdd(pfList *l, void *elt)
{
    if (l->num == l->max) {
        l->max *= 2;
        l->elts = (void **)realloc(l->elts, sizeof(void *) * l->max);
    }
    l->elts[l->num++] = elt;
}

int pfGetNum(const pfList *l) { return l ? l->num : 0; }
void *pfGetListElt(const pfList *l, int i) { return l && i >= 0 && i < l->num ? l->elts[i] : NULL; }

/* --- initialization -------------------------------------------------------------- */

int pfInit(void)
{
    static int done;
    int argc = 1;
    char *argv[2];
    argv[0] = (char *)"perfly";
    argv[1] = NULL;
    if (done)
        return 1;
    done = 1;
    glutInit(&argc, argv);
    glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_EXIT);
    /* Performer's light sources are on when made */
    pfLSourceDefaultOn(1);
    pfGetTime();
    return 1;
}

int pfMultiprocess(int mode) { mp_mode = mode; return 0; }
int pfGetMultiprocess(void) { return mp_mode == PFMP_DEFAULT ? PFMP_APPCULLDRAW : mp_mode; }
int pfMultipipe(int n) { npipes = n < 1 ? 1 : n > PFI_MAX_PIPES ? PFI_MAX_PIPES : n; return 0; }
int pfGetMultipipe(void) { return npipes; }
int pfHyperpipe(int n) { (void)n; return 0; }
int pfConfig(void) { return 1; }
void pfExit(void) { exit(0); }
void pfIsectFunc(void (*func)(void *)) { isect_func = func; }

void pfStageConfigFunc(int pipe, unsigned int stage, pfStageFuncType func)
{
    int i;
    for (i = 0; i < npipes; i++)
        if (pipe < 0 || pipe == i)
            if (func)
                func(i, stage);
}

int pfConfigStage(int pipe, unsigned int stage) { (void)pipe; (void)stage; return 0; }

int pfQueryFeature(int which, int *dst)
{
    switch (which) {
    case PFQFTR_TEXTURE: *dst = PFQFTR_FAST; break;
    case PFQFTR_DISPLACE_POLYGON: *dst = PFQFTR_FAST; break;
    default: *dst = PFQFTR_FALSE; break;
    }
    return 1;
}

int pfMQueryFeature(int *which, int *dst)
{
    int i;
    for (i = 0; which[i]; i++)
        pfQueryFeature(which[i], &dst[i]);
    return i;
}

/* --- windows --------------------------------------------------------------------- */

int pfuScreenWidth(void) { pfInit(); return glutGet(GLUT_SCREEN_WIDTH); }
int pfuScreenHeight(void) { pfInit(); return glutGet(GLUT_SCREEN_HEIGHT); }
Display *pfGetCurWSConnection(void) { static int dpy; pfInit(); return (Display *)&dpy; }

static void display_nothing(void) {}

pfWindow *pfOpenNewNoPortWin(const char *name, int screen)
{
    pfWindow *w = (pfWindow *)calloc(1, sizeof(pfWindow));
    (void)screen;
    pfInit();
    w->type = PFI_TYPE_WIN;
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(16, 16);
    w->glutwin = glutCreateWindow(name ? name : "Query");
    glutDisplayFunc(display_nothing);
    glutHideWindow();
    return w;
}

void pfCloseWin(pfWindow *w)
{
    if (w && w->type == PFI_TYPE_WIN && w->glutwin) {
        glutDestroyWindow(w->glutwin);
        w->glutwin = 0;
    }
}

void pfSelectWin(pfWindow *w)
{
    if (w && w->glutwin)
        glutSetWindow(w->glutwin);
}

pfPipe *pfGetPipe(int i)
{
    if (i < 0 || i >= PFI_MAX_PIPES)
        return NULL;
    if (!pipes[i].type) {
        pipes[i].type = PFI_TYPE_PIPE;
        pipes[i].id = i;
        if (i >= npipes_made)
            npipes_made = i + 1;
    }
    return &pipes[i];
}

int pfGetId(void *obj)
{
    int type = pfGetType(obj);
    if (type == PFI_TYPE_PIPE) return ((pfPipe *)obj)->id;
    if (type == PFI_TYPE_CHAN) return ((pfChannel *)obj)->id;
    return -1;
}

void pfPipeScreen(pfPipe *p, int screen) { if (p) p->screen = screen; }

void pfGetPipeSize(pfPipe *p, int *xs, int *ys)
{
    *xs = pfuScreenWidth();
    *ys = pfuScreenHeight();
    (void)p;
}

pfPipeWindow *pfGetPipePWin(pfPipe *p, int i) { return p && i == 0 ? p->pwin : NULL; }
int pfGetPipeNumPWins(pfPipe *p) { return p && p->pwin ? 1 : 0; }

pfPipeWindow *pfNewPWin(pfPipe *p)
{
    pfPipeWindow *pw = &pwins[p->id];
    memset(pw, 0, sizeof(*pw));
    pw->type = PFI_TYPE_PWIN;
    pw->pipe = p;
    pw->xs = 640;
    pw->ys = 480;
    pw->index = PFWIN_GFX_WIN;
    pw->win.type = PFI_TYPE_WIN;
    strcpy(pw->name, "IRIS Performer");
    p->pwin = pw;
    return pw;
}

void pfPWinName(pfPipeWindow *pw, const char *name)
{
    strncpy(pw->name, name, sizeof(pw->name) - 1);
    if (pw->glutwin) {
        glutSetWindow(pw->glutwin);
        glutSetWindowTitle(pw->name);
    }
}

void pfPWinOriginSize(pfPipeWindow *pw, int xo, int yo, int xs, int ys)
{
    pw->xo = xo; pw->yo = yo;
    pw->xs = xs > 0 ? xs : 640;
    pw->ys = ys > 0 ? ys : 480;
}

void pfGetPWinSize(pfPipeWindow *pw, int *xs, int *ys) { *xs = pw->xs; *ys = pw->ys; }
void pfGetPWinOrigin(pfPipeWindow *pw, int *xo, int *yo) { *xo = pw->xo; *yo = pw->yo; }
void pfPWinConfigFunc(pfPipeWindow *pw, pfPWinFuncType func) { pw->config = func; }
void pfPWinFBConfigId(pfPipeWindow *pw, int id) { (void)pw; (void)id; }
pfPipe *pfGetPWinPipe(pfPipeWindow *pw) { return pw->pipe; }
void pfPWinIndex(pfPipeWindow *pw, int index) { pw->index = index; }
int pfGetPWinIndex(pfPipeWindow *pw) { return pw->index; }
pfWindow *pfGetPWinSelect(pfPipeWindow *pw) { return pw->glutwin ? &pw->win : NULL; }
pfWindow *pfGetPWinOverlayWin(pfPipeWindow *pw) { (void)pw; return NULL; }
int pfGetPWinType(pfPipeWindow *pw) { (void)pw; return PFPWIN_TYPE_X; }

static pfPipeWindow *pwin_of_glut(int win)
{
    int i;
    for (i = 0; i < PFI_MAX_PIPES; i++)
        if (pwins[i].glutwin == win)
            return &pwins[i];
    return NULL;
}

static void reshape(int w, int h)
{
    pfPipeWindow *pw = pwin_of_glut(glutGetWindow());
    if (pw) {
        pw->xs = w;
        pw->ys = h;
    }
}

void pfConfigPWin(pfPipeWindow *pw)
{
    if (pw->config)
        pw->config(pw);
    else
        pfOpenPWin(pw);
}

void pfOpenPWin(pfPipeWindow *pw)
{
    if (pw->glutwin) {
        glutSetWindow(pw->glutwin);
        return;
    }
    pfInit();
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowPosition(pw->xo, pw->yo);
    glutInitWindowSize(pw->xs, pw->ys);
    pw->glutwin = glutCreateWindow(pw->name);
    pw->win.glutwin = pw->glutwin;
    glutDisplayFunc(display_nothing);
    glutReshapeFunc(reshape);
    glViewport(0, 0, pw->xs, pw->ys);
    glEnable(GL_DEPTH_TEST);
    pfuiWinCreated(pw);
}

void pfClosePWin(pfPipeWindow *pw)
{
    if (pw->glutwin)
        glutDestroyWindow(pw->glutwin);
    pw->glutwin = 0;
}

void pfSelectPWin(pfPipeWindow *pw)
{
    if (pw && pw->glutwin)
        glutSetWindow(pw->glutwin);
}

void pfSwapPWinBuffers(pfPipeWindow *pw)
{
    if (pw && pw->glutwin) {
        glutSetWindow(pw->glutwin);
        glutSwapBuffers();
    }
}

/* --- channels ---------------------------------------------------------------------- */

#define PFCHAN_DEFAULT_SHARE (PFCHAN_FOV | PFCHAN_VIEW | PFCHAN_NEARFAR | PFCHAN_SCENE | \
    PFCHAN_EARTHSKY | PFCHAN_STRESS | PFCHAN_LOD | PFCHAN_SWAPBUFFERS | PFCHAN_APPFUNC | \
    PFCHAN_CULLFUNC | PFCHAN_DRAWFUNC | PFCHAN_STATS_DRAWMODE)

/* the channel that holds an attribute: the master of the group when shared */
static pfChannel *src(pfChannel *c, unsigned int bit)
{
    return c->master && (c->master->share & bit) ? c->master : c;
}

int pfiNumChans(void) { return nchans; }
pfChannel *pfiChan(int i) { return i >= 0 && i < nchans ? chans[i] : NULL; }
int pfiNumPipes(void) { return npipes_made; }
pfChannel *pfGetCurChan(void) { return cur_chan; }
double pfiFrameStart(void) { return frame_start; }

pfChannel *pfNewChan(pfPipe *p)
{
    pfChannel *c;
    int i;
    if (nchans == PFI_MAX_CHANS)
        return NULL;
    c = (pfChannel *)calloc(1, sizeof(pfChannel));
    c->type = PFI_TYPE_CHAN;
    c->id = nchans;
    c->pipe = p;
    c->master = c;
    c->share = PFCHAN_DEFAULT_SHARE;
    c->near_ = 1;
    c->far_ = 1000;
    c->fovh = 45;
    c->fovv = -1;
    c->autoaspect = PFFRUST_CALC_VERT;
    pfMakeIdentMat(c->view);
    c->vl = 0; c->vr = 1; c->vb = 0; c->vt = 1;
    c->lodscale = 1;
    for (i = 0; i < 4; i++)
        c->travmode[i] = PFCULL_ALL;
    c->fstats.type = PFI_TYPE_FSTATS;
    c->fstats.update_secs = 2;
    c->pvchan.type = PFI_TYPE_PVCHAN;
    c->pvchan.xs = c->pvchan.outxs = 1280;
    c->pvchan.ys = c->pvchan.outys = 1024;
    chans[nchans++] = c;
    return c;
}

pfPipe *pfGetChanPipe(pfChannel *c) { return c->pipe; }
pfPipeWindow *pfGetChanPWin(pfChannel *c) { return c->pipe->pwin; }

void pfAttachChan(pfChannel *master, pfChannel *c)
{
    c->master = master->master;
}

void pfChanShare(pfChannel *c, unsigned int mask) { c->master->share = mask; }
unsigned int pfGetChanShare(pfChannel *c) { return c->master->share; }

void pfChanTravFunc(pfChannel *c, int trav, pfChanFuncType func)
{
    static const unsigned int bits[4] = { 0, PFCHAN_APPFUNC, PFCHAN_CULLFUNC, PFCHAN_DRAWFUNC };
    if (trav >= 1 && trav <= 3)
        src(c, bits[trav])->func[trav] = func;
}

static pfChanFuncType chan_func(pfChannel *c, int trav)
{
    static const unsigned int bits[4] = { 0, PFCHAN_APPFUNC, PFCHAN_CULLFUNC, PFCHAN_DRAWFUNC };
    return src(c, bits[trav])->func[trav];
}

void pfChanScene(pfChannel *c, pfScene *s)
{
    src(c, PFCHAN_SCENE)->scene = s;
    /* the worlds of the face demo, when a file "paths" is here */
    pfuiFaceAttach(s, c);
    /* the vehicles of the paths go into the first scene */
    pfuiAttachVehicles(s);
}
pfScene *pfGetChanScene(pfChannel *c) { return src(c, PFCHAN_SCENE)->scene; }
void pfChanESky(pfChannel *c, pfEarthSky *es) { src(c, PFCHAN_EARTHSKY)->esky = es; }
pfEarthSky *pfiChanESky(pfChannel *c) { return src(c, PFCHAN_EARTHSKY)->esky; }

void pfChanNearFar(pfChannel *c, float n, float f)
{
    c = src(c, PFCHAN_NEARFAR);
    c->near_ = n > 0 ? n : 1e-3f;
    c->far_ = f > c->near_ ? f : c->near_ * 2;
}

void pfGetChanNearFar(pfChannel *c, float *n, float *f)
{
    c = src(c, PFCHAN_NEARFAR);
    *n = c->near_;
    *f = c->far_;
}

void pfChanFOV(pfChannel *c, float h, float v)
{
    c = src(c, PFCHAN_FOV);
    c->fovh = h;
    c->fovv = v;
}

void pfChanAutoAspect(pfChannel *c, int which) { src(c, PFCHAN_FOV)->autoaspect = which; }

void pfMakeSimpleChan(pfChannel *c, float fov)
{
    pfChanFOV(c, fov, -1);
}

void pfiChanRect(pfChannel *c, int *x, int *y, int *w, int *h)
{
    pfPipeWindow *pw = c->pipe->pwin;
    pfChannel *v = src(c, PFCHAN_VIEWPORT);
    int ws = pw ? pw->xs : 640, hs = pw ? pw->ys : 480;
    *x = (int)(v->vl * ws + 0.5f);
    *y = (int)(v->vb * hs + 0.5f);
    *w = (int)(v->vr * ws + 0.5f) - *x;
    *h = (int)(v->vt * hs + 0.5f) - *y;
    if (*w < 1) *w = 1;
    if (*h < 1) *h = 1;
}

/* the field of view, the missing angle from the aspect of the viewport */
static void chan_fov(pfChannel *c, float *h, float *v)
{
    pfChannel *f = src(c, PFCHAN_FOV);
    int x, y, w, hh;
    float fh = f->fovh, fv = f->fovv;
    pfiChanRect(c, &x, &y, &w, &hh);
    if (fh <= 0 && fv <= 0)
        fh = 45;
    if (fv <= 0)
        fv = (float)PF_RAD2DEG(2 * atan(tan(PF_DEG2RAD(fh) / 2) * hh / w));
    else if (fh <= 0)
        fh = (float)PF_RAD2DEG(2 * atan(tan(PF_DEG2RAD(fv) / 2) * w / hh));
    *h = fh;
    *v = fv;
}

void pfGetChanFOV(pfChannel *c, float *h, float *v) { chan_fov(c, h, v); }

void pfChanView(pfChannel *c, pfVec3 xyz, pfVec3 hpr)
{
    pfCoord co;
    pfCopyVec3(co.xyz, xyz);
    pfCopyVec3(co.hpr, hpr);
    pfMakeCoordMat(src(c, PFCHAN_VIEW)->view, &co);
}

void pfGetChanView(pfChannel *c, pfVec3 xyz, pfVec3 hpr)
{
    pfCoord co;
    pfGetOrthoMatCoord(src(c, PFCHAN_VIEW)->view, &co);
    pfCopyVec3(xyz, co.xyz);
    pfCopyVec3(hpr, co.hpr);
}

void pfChanViewMat(pfChannel *c, pfMatrix m) { pfCopyMat(src(c, PFCHAN_VIEW)->view, m); }
void pfGetChanViewMat(pfChannel *c, pfMatrix m) { pfCopyMat(m, src(c, PFCHAN_VIEW)->view); }

void pfChanViewOffsets(pfChannel *c, pfVec3 xyz, pfVec3 hpr)
{
    pfCopyVec3(c->offxyz, xyz);
    pfCopyVec3(c->offhpr, hpr);
}

void pfGetChanOffsetViewMat(pfChannel *c, pfMatrix m)
{
    pfCoord co;
    pfMatrix off;
    pfCopyVec3(co.xyz, c->offxyz);
    pfCopyVec3(co.hpr, c->offhpr);
    pfMakeCoordMat(off, &co);
    pfMultMat(m, off, src(c, PFCHAN_VIEW)->view);
}

void pfChanViewport(pfChannel *c, float l, float r, float b, float t)
{
    c = src(c, PFCHAN_VIEWPORT);
    c->vl = l; c->vr = r; c->vb = b; c->vt = t;
}

void pfGetChanViewport(pfChannel *c, float *l, float *r, float *b, float *t)
{
    c = src(c, PFCHAN_VIEWPORT);
    *l = c->vl; *r = c->vr; *b = c->vb; *t = c->vt;
}

void pfGetChanOrigin(pfChannel *c, int *xo, int *yo)
{
    int w, h;
    pfiChanRect(c, xo, yo, &w, &h);
}

void pfGetChanSize(pfChannel *c, int *xs, int *ys)
{
    int x, y;
    pfiChanRect(c, &x, &y, xs, ys);
}

void pfGetChanOutputOrigin(pfChannel *c, int *xo, int *yo) { pfGetChanOrigin(c, xo, yo); }
void pfGetChanOutputSize(pfChannel *c, int *xs, int *ys) { pfGetChanSize(c, xs, ys); }
void pfChanProjMode(pfChannel *c, int mode) { c->projmode = mode; }
void pfChanBinOrder(pfChannel *c, int bin, int order) { (void)c; (void)bin; (void)order; }
void pfChanTravMode(pfChannel *c, int trav, int mode) { if (trav >= 0 && trav < 4) c->travmode[trav] = mode; }
int pfGetChanTravMode(pfChannel *c, int trav) { return trav >= 0 && trav < 4 ? c->travmode[trav] : 0; }

void pfChanLODAttr(pfChannel *c, int attr, float val)
{
    c = src(c, PFCHAN_LOD);
    if (attr == PFLOD_SCALE) c->lodscale = val;
    else if (attr == PFLOD_FADE) c->lodfade = val;
    else if (attr == PFLOD_STRESS_PIX_LIMIT) c->pixlimit = val;
}

float pfGetChanLODAttr(pfChannel *c, int attr)
{
    c = src(c, PFCHAN_LOD);
    if (attr == PFLOD_SCALE) return c->lodscale;
    if (attr == PFLOD_FADE) return c->lodfade;
    if (attr == PFLOD_STRESS_PIX_LIMIT) return c->pixlimit;
    return 0;
}

void pfChanStressFilter(pfChannel *c, float frac, float low, float high, float scale, float max)
{
    c = src(c, PFCHAN_STRESS);
    c->stress[0] = frac; c->stress[1] = low; c->stress[2] = high;
    c->stress[3] = scale; c->stress[4] = max;
}

void pfChanStatsMode(pfChannel *c, unsigned int mode, unsigned int val)
{
    (void)mode;
    c->statsmode = val;
}

void pfChanCullPtope(pfChannel *c, pfPolytope *ptope) { c->cullptope = ptope; }
pfFrameStats *pfGetChanFStats(pfChannel *c) { return &c->fstats; }
pfPipeVideoChannel *pfGetChanPVChan(pfChannel *c) { return &c->pvchan; }

void pfGetPVChanSize(pfPipeVideoChannel *v, int *xs, int *ys) { *xs = v->xs; *ys = v->ys; }
void pfPVChanOutputSize(pfPipeVideoChannel *v, int xs, int ys) { v->outxs = xs; v->outys = ys; }
void pfGetPVChanOutputSize(pfPipeVideoChannel *v, int *xs, int *ys) { *xs = v->outxs; *ys = v->outys; }
void pfPVChanDVRMode(pfPipeVideoChannel *v, int mode) { v->dvr = mode; }
void pfPVChanStressFilter(pfPipeVideoChannel *v, float frac, float low, float high,
                          float pipeload, float scale, float max)
{
    (void)v; (void)frac; (void)low; (void)high; (void)pipeload; (void)scale; (void)max;
}

/* the frustum of a channel without its offsets */
void pfGetChanBaseFrust(pfChannel *c, pfFrustum *f)
{
    float h, v, n, fa;
    chan_fov(c, &h, &v);
    pfGetChanNearFar(c, &n, &fa);
    f->type = PFI_TYPE_FRUST;
    f->near_ = n;
    f->far_ = fa;
    f->r = n * pfTan(h / 2);
    f->l = -f->r;
    f->t = n * pfTan(v / 2);
    f->b = -f->t;
}

void pfiChanProjection(pfChannel *c)
{
    pfFrustum f;
    pfGetChanBaseFrust(c, &f);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(f.l, f.r, f.b, f.t, f.near_, f.far_);
    glMatrixMode(GL_MODELVIEW);
}

/* the eye looks along +Y with +Z up in Performer, along -Z with +Y up in GL */
void pfiChanModelview(pfChannel *c)
{
    pfMatrix m, inv;
    pfGetChanOffsetViewMat(c, m);
    pfInvertAffMat(inv, m);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glRotatef(-90, 1, 0, 0);
    glMultMatrixf(&inv[0][0]);
}

/* --- frustums --------------------------------------------------------------------- */

pfFrustum *pfNewFrust(void *arena)
{
    pfFrustum *f = (pfFrustum *)pfMalloc(sizeof(pfFrustum), arena);
    f->type = PFI_TYPE_FRUST;
    f->near_ = 1;
    f->far_ = 1000;
    pfMakeSimpleFrust(f, 45);
    return f;
}

void pfMakeSimpleFrust(pfFrustum *f, float fov)
{
    f->r = f->near_ * pfTan(fov / 2);
    f->l = -f->r;
    f->t = f->r;
    f->b = -f->r;
}

void pfMakePerspFrust(pfFrustum *f, float l, float r, float b, float t)
{
    f->l = l; f->r = r; f->b = b; f->t = t;
}

void pfFrustNearFar(pfFrustum *f, float n, float fa)
{
    /* the extents stay those of the same angles */
    if (n > 0 && f->near_ > 0) {
        float k = n / f->near_;
        f->l *= k; f->r *= k; f->b *= k; f->t *= k;
    }
    f->near_ = n;
    f->far_ = fa;
}

void pfGetFrustNearFar(const pfFrustum *f, float *n, float *fa) { *n = f->near_; *fa = f->far_; }

void pfGetFrustNear(const pfFrustum *f, pfVec3 ll, pfVec3 lr, pfVec3 ul, pfVec3 ur)
{
    pfSetVec3(ll, f->l, f->near_, f->b);
    pfSetVec3(lr, f->r, f->near_, f->b);
    pfSetVec3(ul, f->l, f->near_, f->t);
    pfSetVec3(ur, f->r, f->near_, f->t);
}

void pfApplyFrust(pfFrustum *f)
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(f->l, f->r, f->b, f->t, f->near_, f->far_);
    glMatrixMode(GL_MODELVIEW);
}

/* --- state --------------------------------------------------------------------------- */

pfGeoState *pfNewGState(void *arena)
{
    pfGeoState *gs = (pfGeoState *)pfMalloc(sizeof(pfGeoState), arena);
    gs->lighting = gs->texturing = gs->lpoint = gs->transparency = -1;
    gs->cullface = ~0U;
    return gs;
}

void pfMakeBasicGState(pfGeoState *gs)
{
    memset(gs, 0, sizeof(*gs));
}

pfMaterial *pfNewMtl(void *arena)
{
    pfMaterial *m = (pfMaterial *)pfMalloc(sizeof(pfMaterial), arena);
    m->alpha = 1;
    pfSetVec3(m->ambient, 0.2f, 0.2f, 0.2f);
    pfSetVec3(m->diffuse, 0.8f, 0.8f, 0.8f);
    return m;
}

void pfMtlColorMode(pfMaterial *m, int side, int mode)
{
    (void)side;
    m->colormode = mode != PFMTL_CMODE_OFF;
}

void pfApplyMtl(pfMaterial *m)
{
    float c[4];
    c[3] = m->alpha;
    memcpy(c, m->ambient, sizeof(pfVec3));
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, c);
    memcpy(c, m->diffuse, sizeof(pfVec3));
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, c);
    memcpy(c, m->specular, sizeof(pfVec3));
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, c);
    memcpy(c, m->emission, sizeof(pfVec3));
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, c);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, m->shininess);
    if (m->colormode) {
        glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
        glEnable(GL_COLOR_MATERIAL);
    } else {
        glDisable(GL_COLOR_MATERIAL);
    }
}

pfLightModel *pfNewLModel(void *arena)
{
    pfLightModel *lm = (pfLightModel *)pfMalloc(sizeof(pfLightModel), arena);
    lm->type = PFI_TYPE_LMODEL;
    pfSetVec3(lm->ambient, 0.2f, 0.2f, 0.2f);
    return lm;
}

void pfLModelAmbient(pfLightModel *lm, float r, float g, float b) { pfSetVec3(lm->ambient, r, g, b); }
void pfLModelLocal(pfLightModel *lm, int local) { lm->local = local; }

void pfApplyLModel(pfLightModel *lm)
{
    float a[4];
    a[0] = lm->ambient[0]; a[1] = lm->ambient[1]; a[2] = lm->ambient[2]; a[3] = 1;
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, a);
    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, lm->local ? GL_TRUE : GL_FALSE);
}

void pfBasicState(void)
{
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_FOG);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    glDisable(GL_ALPHA_TEST);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void pfAntialias(int mode)
{
    if (mode == PFAA_ON)
        glEnable(GL_MULTISAMPLE);
    else
        glDisable(GL_MULTISAMPLE);
}

pfLight *pfNewLight(void *arena)
{
    static int next = 7;
    pfLight *l = (pfLight *)pfMalloc(sizeof(pfLight), arena);
    l->type = PFI_TYPE_LIGHT;
    l->index = next > 0 ? next-- : 0;
    l->pos[2] = 1;
    pfSetVec3(l->diffuse, 1, 1, 1);
    pfSetVec3(l->specular, 1, 1, 1);
    return l;
}

void pfLightPos(pfLight *l, float x, float y, float z, float w) { pfSetVec4(l->pos, x, y, z, w); }

void pfLightColor(pfLight *l, int which, float r, float g, float b)
{
    float *c = which == PFLT_AMBIENT ? l->ambient : which == PFLT_SPECULAR ? l->specular : l->diffuse;
    pfSetVec3(c, r, g, b);
}

void pfLightOn(pfLight *l)
{
    GLenum id = GL_LIGHT0 + l->index;
    float c[4];
    c[3] = 1;
    memcpy(c, l->ambient, sizeof(pfVec3));
    glLightfv(id, GL_AMBIENT, c);
    memcpy(c, l->diffuse, sizeof(pfVec3));
    glLightfv(id, GL_DIFFUSE, c);
    memcpy(c, l->specular, sizeof(pfVec3));
    glLightfv(id, GL_SPECULAR, c);
    glLightfv(id, GL_POSITION, l->pos);
    glEnable(id);
}

void pfLightOff(pfLight *l) { glDisable(GL_LIGHT0 + l->index); }

void pfClear(int which, const pfVec4 col)
{
    GLbitfield bits = 0;
    if (col)
        glClearColor(col[0], col[1], col[2], col[3]);
    if (which & PFCL_COLOR) bits |= GL_COLOR_BUFFER_BIT;
    if (which & (PFCL_DEPTH | PFCL_MSDEPTH)) bits |= GL_DEPTH_BUFFER_BIT;
    if (which & PFCL_STENCIL) bits |= GL_STENCIL_BUFFER_BIT;
    glClear(bits);
}

void pfPushMatrix(void) { glPushMatrix(); }
void pfPopMatrix(void) { glPopMatrix(); }
void pfPushIdentMatrix(void) { glPushMatrix(); glLoadIdentity(); }
void pfLoadMatrix(pfMatrix m) { glLoadMatrixf(&m[0][0]); }
void pfMultMatrix(pfMatrix m) { glMultMatrixf(&m[0][0]); }
void pfTranslate(float x, float y, float z) { glTranslatef(x, y, z); }
void pfScale(float x, float y, float z) { glScalef(x, y, z); }

void pfRotate(int axis, float deg)
{
    glRotatef(deg, axis == PF_X, axis == PF_Y, axis == PF_Z);
}

/* --- Earth/sky and fog -------------------------------------------------------------------- */

pfEarthSky *pfNewESky(void)
{
    pfEarthSky *es = (pfEarthSky *)calloc(1, sizeof(pfEarthSky));
    es->type = PFI_TYPE_ESKY;
    es->mode = PFES_FAST;
    pfSetVec4(es->skytop, 0.0f, 0.1f, 0.6f, 1);
    pfSetVec4(es->skybot, 0.3f, 0.5f, 0.9f, 1);
    pfSetVec4(es->horiz, 0.8f, 0.6f, 0.6f, 1);
    pfSetVec4(es->grndfar, 0.4f, 0.3f, 0.1f, 1);
    pfSetVec4(es->grndnear, 0.3f, 0.2f, 0.0f, 1);
    pfSetVec4(es->clear, 0, 0, 0, 1);
    es->horizangle = 10;
    return es;
}

void pfESkyMode(pfEarthSky *es, int which, int mode) { if (which == PFES_BUFFER_CLEAR) es->mode = mode; }
int pfGetESkyMode(pfEarthSky *es, int which) { return which == PFES_BUFFER_CLEAR ? es->mode : 0; }

void pfESkyAttr(pfEarthSky *es, int which, float val)
{
    if (which == PFES_GRND_HT) es->grndht = val;
    else if (which == PFES_HORIZ_ANGLE) es->horizangle = val;
}

float pfGetESkyAttr(pfEarthSky *es, int which)
{
    return which == PFES_GRND_HT ? es->grndht : which == PFES_HORIZ_ANGLE ? es->horizangle : 0;
}

static float *esky_color(pfEarthSky *es, int which)
{
    switch (which) {
    case PFES_SKY_TOP: return es->skytop;
    case PFES_SKY_BOT: return es->skybot;
    case PFES_HORIZ: return es->horiz;
    case PFES_GRND_FAR: return es->grndfar;
    case PFES_GRND_NEAR: return es->grndnear;
    case PFES_CLEAR: return es->clear;
    }
    return NULL;
}

void pfESkyColor(pfEarthSky *es, int which, float r, float g, float b, float a)
{
    float *c = esky_color(es, which);
    if (c)
        pfSetVec4(c, r, g, b, a);
}

void pfGetESkyColor(pfEarthSky *es, int which, float *r, float *g, float *b, float *a)
{
    float *c = esky_color(es, which);
    if (c) {
        *r = c[0]; *g = c[1]; *b = c[2]; *a = c[3];
    }
}

void pfESkyFog(pfEarthSky *es, int which, pfFog *fog) { if (which == PFES_GENERAL) es->fog = fog; }

pfFog *pfNewFog(void *arena)
{
    pfFog *f = (pfFog *)pfMalloc(sizeof(pfFog), arena);
    f->type = PFI_TYPE_FOG;
    f->fogtype = PFFOG_PIX_EXP2;
    pfSetVec3(f->color, 1, 1, 1);
    f->onset = 0;
    f->opaque = 1000;
    return f;
}

void pfFogType(pfFog *f, int type) { f->fogtype = type; }
void pfFogColor(pfFog *f, float r, float g, float b) { pfSetVec3(f->color, r, g, b); }
void pfFogRange(pfFog *f, float onset, float opaque) { f->onset = onset; f->opaque = opaque; }

void pfApplyFog(pfFog *f)
{
    float c[4];
    float range = f->opaque > 0 ? f->opaque : 1;
    if (!f || f->fogtype == PFFOG_OFF) {
        glDisable(GL_FOG);
        return;
    }
    c[0] = f->color[0]; c[1] = f->color[1]; c[2] = f->color[2]; c[3] = 1;
    glFogfv(GL_FOG_COLOR, c);
    switch (f->fogtype) {
    case PFFOG_PIX_EXP: case PFFOG_VTX_EXP:
        glFogi(GL_FOG_MODE, GL_EXP);
        /* opaque (1/256) at the far range */
        glFogf(GL_FOG_DENSITY, 5.545f / range);
        break;
    case PFFOG_PIX_EXP2: case PFFOG_VTX_EXP2:
        glFogi(GL_FOG_MODE, GL_EXP2);
        glFogf(GL_FOG_DENSITY, 2.355f / range);
        break;
    default:
        glFogi(GL_FOG_MODE, GL_LINEAR);
        glFogf(GL_FOG_START, f->onset);
        glFogf(GL_FOG_END, f->opaque > f->onset ? f->opaque : f->onset + 1);
        break;
    }
    glHint(GL_FOG_HINT, f->fogtype == PFFOG_VTX_LIN || f->fogtype == PFFOG_VTX_EXP ||
                        f->fogtype == PFFOG_VTX_EXP2 ? GL_FASTEST : GL_NICEST);
    glEnable(GL_FOG);
}

/* the color of the sky at an elevation (degrees) */
static void sky_color(pfEarthSky *es, float elev, int ground, float *c)
{
    const float *a, *b;
    float t;
    int k;
    if (elev < 0) {
        if (!ground) {
            memcpy(c, es->mode == PFES_SKY_CLEAR ? es->clear : es->horiz, sizeof(pfVec4));
            return;
        }
        a = es->grndfar;
        b = es->grndnear;
        t = -elev / 90;
    } else if (elev < es->horizangle) {
        a = es->horiz;
        b = es->skybot;
        t = es->horizangle > 0 ? elev / es->horizangle : 1;
    } else {
        a = es->skybot;
        b = es->skytop;
        t = (elev - es->horizangle) / (90 - es->horizangle);
    }
    t = t < 0 ? 0 : t > 1 ? 1 : t;
    for (k = 0; k < 4; k++)
        c[k] = a[k] + (b[k] - a[k]) * t;
}

/* a dome around the eye, oriented like the world, without depth */
static void draw_sky(pfChannel *c, pfEarthSky *es)
{
    static const float elevs[] = { -90, -10, -1, 0, 0.5f, 1, 2, 4, 7, 10, 20, 35, 55, 90 };
    int ground = es->mode == PFES_SKY_GRND;
    float h, v, n = 0.1f;
    pfMatrix m, inv;
    int i, j, ne = (int)(sizeof(elevs) / sizeof(elevs[0]));

    chan_fov(c, &h, &v);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glFrustum(-n * pfTan(h / 2), n * pfTan(h / 2), -n * pfTan(v / 2), n * pfTan(v / 2), n, 10);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    pfGetChanOffsetViewMat(c, m);
    m[3][0] = m[3][1] = m[3][2] = 0;
    pfInvertOrthoMat(inv, m);
    glLoadIdentity();
    glRotatef(-90, 1, 0, 0);
    glMultMatrixf(&inv[0][0]);
    glPushAttrib(GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_FOG);
    glDisable(GL_CULL_FACE);
    glShadeModel(GL_SMOOTH);
    for (i = 0; i + 1 < ne; i++) {
        float e0 = elevs[i], e1 = elevs[i + 1];
        float c0[4], c1[4];
        float e0r = (float)PF_DEG2RAD(e0), e1r = (float)PF_DEG2RAD(e1);
        sky_color(es, e0 < 0 ? e0 : e0 + 1e-4f, ground, c0);
        sky_color(es, e1 <= 0 ? (e1 == 0 ? -1e-4f : e1) : e1, ground, c1);
        if (e0 == 0 && e1 > 0)
            sky_color(es, 0, ground, c0);
        glBegin(GL_QUAD_STRIP);
        for (j = 0; j <= 32; j++) {
            float a = (float)(j * 2 * 3.14159265358979 / 32);
            float ca = (float)cos(a), sa = (float)sin(a);
            glColor4fv(c1);
            glVertex3f((float)cos(e1r) * ca, (float)cos(e1r) * sa, (float)sin(e1r));
            glColor4fv(c0);
            glVertex3f((float)cos(e0r) * ca, (float)cos(e0r) * sa, (float)sin(e0r));
        }
        glEnd();
    }
    glPopAttrib();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}

void pfClearChan(pfChannel *c)
{
    pfEarthSky *es = src(c, PFCHAN_EARTHSKY)->esky;
    int x, y, w, h;
    pfiChanRect(c, &x, &y, &w, &h);
    glPushAttrib(GL_SCISSOR_BIT | GL_COLOR_BUFFER_BIT);
    glEnable(GL_SCISSOR_TEST);
    glScissor(x, y, w, h);
    if (!es) {
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    } else {
        glClearColor(es->clear[0], es->clear[1], es->clear[2], es->clear[3]);
        if (es->mode == PFES_FAST || es->mode == PFES_TAG) {
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        } else {
            glClear(GL_DEPTH_BUFFER_BIT);
            draw_sky(c, es);
        }
    }
    glPopAttrib();
}

/* --- drawing ---------------------------------------------------------------------------- */

static pfScene *scene_gs_scene[8];
static pfGeoState *scene_gs[8];

void pfSceneGState(pfScene *s, pfGeoState *gs)
{
    int i;
    for (i = 0; i < 8; i++)
        if (scene_gs_scene[i] == s || !scene_gs_scene[i]) {
            scene_gs_scene[i] = s;
            scene_gs[i] = gs;
            return;
        }
}

pfGeoState *pfGetSceneGState(pfScene *s)
{
    int i;
    for (i = 0; i < 8; i++)
        if (scene_gs_scene[i] == s)
            return scene_gs[i];
    return NULL;
}

void pfApp(void) {}
void pfCull(void) {}

void pfDraw(void)
{
    pfChannel *c = cur_chan;
    pfScene *scene;
    pfEarthSky *es;
    if (!c || (scene = pfGetChanScene(c)) == NULL)
        return;
    es = src(c, PFCHAN_EARTHSKY)->esky;
    pfiChanProjection(c);
    pfiChanModelview(c);
    pfInheritState(pfGetSceneGState(scene));
    pfLODScale(pfGetChanLODAttr(c, PFLOD_SCALE) > 0 ? pfGetChanLODAttr(c, PFLOD_SCALE) : 1);
    if (es && es->fog)
        pfApplyFog(es->fog);
    pfPlaceLights(scene);
    pfDrawScene(scene, pfGetFrameTime());
    glDisable(GL_FOG);
    pfInheritState(NULL);
    pfLODScale(1);
}

/* --- the frame loop ---------------------------------------------------------------------- */

void pfiSetWinHook(void (*draw)(pfPipeWindow *pw)) { win_hook = draw; }

void pfFrameRate(float rate)
{
    int fields;
    if (rate <= 0)
        rate = 30;
    fields = (int)(PFI_VIDEO_RATE / rate + 0.5f);
    if (fields < 1)
        fields = 1;
    frame_rate = (float)PFI_VIDEO_RATE / fields;
}

float pfGetFrameRate(void) { return frame_rate; }
void pfFieldRate(int fields) { frame_rate = (float)PFI_VIDEO_RATE / (fields < 1 ? 1 : fields); }
int pfGetFieldRate(void) { return (int)(PFI_VIDEO_RATE / frame_rate + 0.5f); }
int pfGetVideoRate(void) { return PFI_VIDEO_RATE; }
void pfPhase(int p) { phase = p; }
int pfGetPhase(void) { return phase; }
int pfGetFrameCount(void) { return frame_count; }

static void sleep_secs(double s)
{
#ifdef _WIN32
    if (s > 0.001)
        Sleep((DWORD)(s * 1000));
#else
    struct timespec ts;
    if (s <= 0)
        return;
    ts.tv_sec = (time_t)s;
    ts.tv_nsec = (long)((s - (double)ts.tv_sec) * 1e9);
    nanosleep(&ts, NULL);
#endif
}

/* waits for the start of the next frame: the frame rate, or the video
 * rate in free run (the swaps of Performer wait for the retrace) */
int pfSync(void)
{
    double period = phase == PFPHASE_FREE_RUN ? 1.0 / PFI_VIDEO_RATE : 1.0 / frame_rate;
    double now = pfGetTime(), wait = frame_start + period - now;
    last_sync_sleep = 0;
    if (frame_start > 0 && wait > 0) {
        sleep_secs(wait);
        last_sync_sleep = pfGetTime() - now;
    }
    pfuiUpdateVehicles();
    return 0;
}

static void stats_record(double tframe, double tapp, double tcull, double tdraw)
{
    int i, gsets, prims, verts;
    double now = pfGetTime();
    pfGetDrawCounts(&gsets, &prims, &verts, 1);
    for (i = 0; i < nchans; i++) {
        pfFrameStats *fs = &chans[i]->fstats;
        int k = fs->last = (fs->last + 1) % PFI_STATS_FRAMES;
        fs->frame[k] = (float)tframe;
        fs->app[k] = (float)tapp;
        fs->cull[k] = (float)tcull;
        fs->draw[k] = (float)tdraw;
        if (fs->count < PFI_STATS_FRAMES)
            fs->count++;
        fs->sum_frame += tframe;
        fs->sum_app += tapp;
        fs->sum_cull += tcull;
        fs->sum_draw += tdraw;
        fs->avg_frames++;
        if (fs->avg_start <= 0)
            fs->avg_start = now;
        if (now - fs->avg_start >= (fs->update_secs > 0 ? fs->update_secs : 2) || fs->fps == 0) {
            double n = fs->avg_frames;
            fs->fps = (float)(n / (now - fs->avg_start > 1e-6 ? now - fs->avg_start : 1e-6));
            fs->ms_frame = (float)(1000 * fs->sum_frame / n);
            fs->ms_app = (float)(1000 * fs->sum_app / n);
            fs->ms_cull = (float)(1000 * fs->sum_cull / n);
            fs->ms_draw = (float)(1000 * fs->sum_draw / n);
            fs->avg_start = now;
            fs->avg_frames = 0;
            fs->sum_frame = fs->sum_app = fs->sum_cull = fs->sum_draw = 0;
        }
        fs->gsets = gsets;
        fs->prims = prims;
        fs->verts = verts;
    }
}

int pfFrame(void)
{
    double t0 = pfGetTime(), tapp, tcull = 0, tdraw = 0, t;
    int p, i;

    tapp = last_frame_end >= 0 ? t0 - last_frame_end - last_sync_sleep : 0;
    if (tapp < 0)
        tapp = 0;
    frame_count++;
    pfFrameTime(t0);

    if (isect_func)
        isect_func(NULL);

    /* APP */
    for (i = 0; i < nchans; i++) {
        pfChanFuncType f = chan_func(chans[i], PFTRAV_APP);
        cur_chan = chans[i];
        if (f)
            f(chans[i], NULL);
        else
            pfApp();
    }
    tapp += pfGetTime() - t0;

    /* CULL and DRAW, window by window */
    for (p = 0; p < PFI_MAX_PIPES; p++) {
        pfPipeWindow *pw = &pwins[p];
        if (!pw->glutwin)
            continue;
        glutSetWindow(pw->glutwin);
        for (i = 0; i < nchans; i++) {
            pfChannel *c = chans[i];
            pfChanFuncType f;
            int x, y, w, h;
            if (c->pipe->pwin != pw)
                continue;
            cur_chan = c;
            t = pfGetTime();
            if ((f = chan_func(c, PFTRAV_CULL)) != NULL)
                f(c, NULL);
            else
                pfCull();
            tcull += pfGetTime() - t;
            t = pfGetTime();
            pfiChanRect(c, &x, &y, &w, &h);
            glViewport(x, y, w, h);
            glScissor(x, y, w, h);
            pfiChanProjection(c);
            pfiChanModelview(c);
            if ((f = chan_func(c, PFTRAV_DRAW)) != NULL) {
                f(c, NULL);
            } else {
                pfClearChan(c);
                pfDraw();
            }
            tdraw += pfGetTime() - t;
        }
        cur_chan = NULL;
        if (nchans && chans[0]->master->pipe->pwin == pw)
            pfuiDrawVehicleName(chans[0]->master);
        glViewport(0, 0, pw->xs, pw->ys);
        if (win_hook)
            win_hook(pw);
    }
    for (p = 0; p < PFI_MAX_PIPES; p++)
        if (pwins[p].glutwin) {
            glutSetWindow(pwins[p].glutwin);
            glutSwapBuffers();
        }
    /* the events of the windows */
    glutMainLoopEvent();

    last_frame_end = pfGetTime();
    stats_record(frame_start > 0 ? t0 - frame_start : 0, tapp, tcull, tdraw);
    frame_start = t0;
    return frame_count;
}

/* --- statistics ------------------------------------------------------------------------------- */

/* updateStats of perfly can pass a pointer it did not set (it keeps the
 * value of a register on IRIX): only the statistics of a channel are read */
static pfFrameStats *valid_fstats(pfFrameStats *fs)
{
    int i;
    for (i = 0; i < nchans; i++)
        if (fs == &chans[i]->fstats)
            return fs;
    return nchans ? &chans[0]->fstats : NULL;
}

void pfFStatsAttr(pfFrameStats *fs, int attr, float val)
{
    if ((fs = valid_fstats(fs)) != NULL && attr == PFFSTATS_UPDATE_SECS)
        fs->update_secs = val;
}

unsigned int pfFStatsClass(pfFrameStats *fs, unsigned int mask, int val)
{
    unsigned int old;
    if ((fs = valid_fstats(fs)) == NULL)
        return 0;
    old = fs->classes;
    switch (val) {
    case PFSTATS_ON: fs->classes |= mask; break;
    case PFSTATS_OFF: fs->classes &= ~mask; break;
    case PFSTATS_DEFAULT: fs->classes &= ~mask; break;
    case PFSTATS_SET: fs->classes = mask; break;
    }
    return old;
}

unsigned int pfGetFStatsClass(pfFrameStats *fs, unsigned int mask)
{
    fs = valid_fstats(fs);
    return fs ? fs->classes & mask : 0;
}

void pfFStatsClassMode(pfFrameStats *fs, int cls, unsigned int mask, int val)
{
    (void)fs; (void)cls; (void)mask; (void)val;
}

void pfHlightMode(pfHighlight *h, unsigned int mode) { (void)h; (void)mode; }

/* the graph of the process times of the last frames and their averages */
void pfDrawChanStats(pfChannel *c)
{
    pfFrameStats *fs = &c->fstats;
    int x, y, w, h, gx, gy, gw = 240, gh = 80, i, lines;
    float target = 1.0f / frame_rate, scale;
    char buf[160];

    pfiChanRect(c, &x, &y, &w, &h);
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, w, 0, h, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    pfBasicState();
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    lines = 2 + ((fs->classes & (PFSTATS_ENGFX | PFFSTATS_ENDB | PFFSTATS_ENCULL)) ? 1 : 0);
    gx = 10;
    gy = 10;
    glColor4f(0, 0, 0, 0.5f);
    glRectf((float)gx - 6, (float)gy - 6, (float)(gx + gw + 6), (float)(gy + gh + 8 + 14 * lines));

    /* the times of the frames, stacked: app, cull, draw; the line is the
     * time of a frame at the target rate */
    scale = gh / (2 * target);
    glBegin(GL_LINES);
    for (i = 0; i < fs->count; i++) {
        int k = (fs->last - i + PFI_STATS_FRAMES) % PFI_STATS_FRAMES;
        float px = gx + gw - i * (float)gw / PFI_STATS_FRAMES;
        float a = fs->app[k] * scale, cu = fs->cull[k] * scale, d = fs->draw[k] * scale;
        float fr = fs->frame[k] * scale;
        if (fr > gh) fr = (float)gh;
        glColor4f(0.3f, 0.3f, 0.3f, 0.8f);
        glVertex2f(px, (float)gy); glVertex2f(px, gy + fr);
        glColor4f(0.2f, 0.9f, 0.2f, 1);
        glVertex2f(px, (float)gy); glVertex2f(px, gy + PF_MIN2(a, gh));
        glColor4f(0.9f, 0.9f, 0.2f, 1);
        glVertex2f(px, gy + PF_MIN2(a, gh)); glVertex2f(px, gy + PF_MIN2(a + cu, gh));
        glColor4f(0.9f, 0.3f, 0.3f, 1);
        glVertex2f(px, gy + PF_MIN2(a + cu, gh)); glVertex2f(px, gy + PF_MIN2(a + cu + d, gh));
    }
    glColor4f(1, 1, 1, 0.8f);
    glVertex2f((float)gx, gy + gh / 2.0f); glVertex2f((float)(gx + gw), gy + gh / 2.0f);
    glEnd();

    glColor4f(1, 1, 1, 1);
    y = gy + gh + 6;
    if (lines > 2) {
        sprintf(buf, "%d geosets  %d prims  %d verts", fs->gsets, fs->prims, fs->verts);
        pfuiDrawText((float)gx, (float)y, buf, 0);
        y += 14;
    }
    sprintf(buf, "app %.1f  cull %.1f  draw %.1f  frame %.1f ms",
            fs->ms_app, fs->ms_cull, fs->ms_draw, fs->ms_frame);
    glColor4f(0.8f, 0.9f, 1, 1);
    pfuiDrawText((float)gx, (float)y, buf, 0);
    y += 14;
    sprintf(buf, "%.1f Hz (target %.0f Hz)", fs->fps, frame_rate);
    glColor4f(1, 1, 0.4f, 1);
    pfuiDrawText((float)gx, (float)y, buf, 0);

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glPopAttrib();
}

/* --- strings: drawn with the stroke font of freeglut ------------------------------------------ */

pfString *pfNewString(void *arena)
{
    pfString *s = (pfString *)pfMalloc(sizeof(pfString), arena);
    s->type = PFI_TYPE_STRING;
    pfSetVec4(s->color, 1, 1, 1, 1);
    pfMakeIdentMat(s->mat);
    return s;
}

void pfStringMode(pfString *s, int mode, int val) { if (mode == PFSTR_JUSTIFY) s->justify = val; }
void pfStringFont(pfString *s, pfFont *f) { (void)s; (void)f; }
void pfStringColor(pfString *s, float r, float g, float b, float a) { pfSetVec4(s->color, r, g, b, a); }
void pfStringMat(pfString *s, pfMatrix m) { pfCopyMat(s->mat, m); }
void pfFlattenString(pfString *s) { (void)s; }

void pfStringString(pfString *s, const char *text)
{
    float w = 0;
    const char *p;
    free(s->text);
    s->text = (char *)malloc(strlen(text) + 1);
    strcpy(s->text, text);
    for (p = text; *p; p++)
        w += glutStrokeWidth(GLUT_STROKE_ROMAN, *p) / 119.05f;
    pfSetVec3(s->bbox.min, s->justify == PFSTR_MIDDLE ? -w / 2 : 0, 0, 0);
    pfSetVec3(s->bbox.max, s->bbox.min[0] + w, 0, 1);
}

const pfBox *pfGetStringBBox(pfString *s) { return &s->bbox; }

void pfDrawString(pfString *s)
{
    const char *p;
    if (!s->text)
        return;
    glPushMatrix();
    glMultMatrixf(&s->mat[0][0]);
    glColor4fv(s->color);
    /* in the XZ plane, facing -Y */
    glTranslatef(s->bbox.min[0], 0, 0);
    glRotatef(90, 1, 0, 0);
    glScalef(1 / 119.05f, 1 / 119.05f, 1 / 119.05f);
    for (p = s->text; *p; p++)
        glutStrokeCharacter(GLUT_STROKE_ROMAN, *p);
    glPopMatrix();
}
