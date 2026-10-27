/*
 * pf.c - the subset of IRIS Performer 2.0 used by Mekton, on top of
 * OpenGL 1.x. See pf.h.
 */
#ifndef _WIN32
#define _DEFAULT_SOURCE
#endif

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <sys/time.h>
#endif

#ifdef __APPLE__
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pf.h"
#include "pfb.h"

/* --- math --------------------------------------------------------------- */

void pfSetVec3(pfVec3 v, float x, float y, float z) { v[0] = x; v[1] = y; v[2] = z; }
void pfCopyVec3(pfVec3 d, const pfVec3 s) { d[0] = s[0]; d[1] = s[1]; d[2] = s[2]; }
void pfAddVec3(pfVec3 d, const pfVec3 a, const pfVec3 b)
{
    d[0] = a[0] + b[0]; d[1] = a[1] + b[1]; d[2] = a[2] + b[2];
}
void pfSubVec3(pfVec3 d, const pfVec3 a, const pfVec3 b)
{
    d[0] = a[0] - b[0]; d[1] = a[1] - b[1]; d[2] = a[2] - b[2];
}
void pfScaleVec3(pfVec3 d, float s, const pfVec3 v)
{
    d[0] = s * v[0]; d[1] = s * v[1]; d[2] = s * v[2];
}
float pfDotVec3(const pfVec3 a, const pfVec3 b) { return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]; }
void pfCrossVec3(pfVec3 d, const pfVec3 a, const pfVec3 b)
{
    pfVec3 t;
    t[0] = a[1] * b[2] - a[2] * b[1];
    t[1] = a[2] * b[0] - a[0] * b[2];
    t[2] = a[0] * b[1] - a[1] * b[0];
    pfCopyVec3(d, t);
}
float pfLengthVec3(const pfVec3 v) { return (float)sqrt(pfDotVec3(v, v)); }
float pfNormalizeVec3(pfVec3 v)
{
    float l = pfLengthVec3(v);
    if (l > 0) {
        v[0] /= l; v[1] /= l; v[2] /= l;
    }
    return l;
}
float pfLengthVec2(const pfVec2 v) { return (float)sqrt(v[0] * v[0] + v[1] * v[1]); }
float pfNormalizeVec2(pfVec2 v)
{
    float l = pfLengthVec2(v);
    if (l > 0) {
        v[0] /= l; v[1] /= l;
    }
    return l;
}
float pfDistancePt3(const pfVec3 a, const pfVec3 b)
{
    pfVec3 d;
    pfSubVec3(d, a, b);
    return pfLengthVec3(d);
}
float pfSqrt(float x) { return x > 0 ? (float)sqrt(x) : 0.0f; }
void pfSinCos(float deg, float *s, float *c)
{
    double r = PF_DEG2RAD(deg);
    *s = (float)sin(r);
    *c = (float)cos(r);
}
float pfArcCos(float x)
{
    if (x > 1) x = 1;
    if (x < -1) x = -1;
    return (float)PF_RAD2DEG(acos(x));
}
float pfArcTan2(float y, float x) { return (float)PF_RAD2DEG(atan2(y, x)); }

void pfMakeIdentMat(pfMatrix m)
{
    int i, j;
    for (i = 0; i < 4; i++)
        for (j = 0; j < 4; j++)
            m[i][j] = i == j;
}
void pfCopyMat(pfMatrix d, pfMatrix s) { memcpy(d, s, sizeof(pfMatrix)); }
void pfMultMat(pfMatrix d, pfMatrix a, pfMatrix b)
{
    pfMatrix t;
    int i, j;
    for (i = 0; i < 4; i++)
        for (j = 0; j < 4; j++)
            t[i][j] = a[i][0] * b[0][j] + a[i][1] * b[1][j] + a[i][2] * b[2][j] + a[i][3] * b[3][j];
    pfCopyMat(d, t);
}

/* roll about Y, then pitch about X, then heading about Z (row vectors) */
void pfMakeEulerMat(pfMatrix m, float h, float p, float r)
{
    float sh, ch, sp, cp, sr, cr;
    pfSinCos(h, &sh, &ch);
    pfSinCos(p, &sp, &cp);
    pfSinCos(r, &sr, &cr);
    m[0][0] = ch * cr - sh * sr * sp;
    m[0][1] = cr * sh + ch * sr * sp;
    m[0][2] = -sr * cp;
    m[0][3] = 0;
    m[1][0] = -sh * cp;
    m[1][1] = ch * cp;
    m[1][2] = sp;
    m[1][3] = 0;
    m[2][0] = sr * ch + sh * cr * sp;
    m[2][1] = sr * sh - ch * cr * sp;
    m[2][2] = cr * cp;
    m[2][3] = 0;
    m[3][0] = m[3][1] = m[3][2] = 0;
    m[3][3] = 1;
}

void pfMakeTransMat(pfMatrix m, float x, float y, float z)
{
    pfMakeIdentMat(m);
    m[3][0] = x; m[3][1] = y; m[3][2] = z;
}

void pfMakeCoordMat(pfMatrix m, const pfCoord *c)
{
    pfMakeEulerMat(m, c->hpr[0], c->hpr[1], c->hpr[2]);
    m[3][0] = c->xyz[0]; m[3][1] = c->xyz[1]; m[3][2] = c->xyz[2];
}

/* rotation taking v1 onto v2 */
void pfMakeVecRotVecMat(pfMatrix m, const pfVec3 v1, const pfVec3 v2)
{
    pfVec3 a, b, ax;
    float c, s, t, x, y, z;
    pfCopyVec3(a, v1);
    pfCopyVec3(b, v2);
    pfNormalizeVec3(a);
    pfNormalizeVec3(b);
    pfCrossVec3(ax, a, b);
    s = pfNormalizeVec3(ax);
    c = pfDotVec3(a, b);
    pfMakeIdentMat(m);
    if (s < 1e-7f) {
        if (c > 0)
            return;
        /* opposite vectors: half turn about any perpendicular axis */
        if (fabs(a[0]) < 0.9f) pfSetVec3(ax, 1, 0, 0);
        else pfSetVec3(ax, 0, 1, 0);
        pfCrossVec3(ax, a, ax);
        pfNormalizeVec3(ax);
        s = 0;
        c = -1;
    }
    t = 1 - c;
    x = ax[0]; y = ax[1]; z = ax[2];
    /* row-vector form of the Rodrigues rotation */
    m[0][0] = t * x * x + c;     m[0][1] = t * x * y + s * z; m[0][2] = t * x * z - s * y;
    m[1][0] = t * x * y - s * z; m[1][1] = t * y * y + c;     m[1][2] = t * y * z + s * x;
    m[2][0] = t * x * z + s * y; m[2][1] = t * y * z - s * x; m[2][2] = t * z * z + c;
}

void pfInvertOrthoMat(pfMatrix d, pfMatrix m)
{
    pfMatrix t;
    int i, j;
    for (i = 0; i < 3; i++)
        for (j = 0; j < 3; j++)
            t[i][j] = m[j][i];
    for (j = 0; j < 3; j++)
        t[3][j] = -(m[3][0] * t[0][j] + m[3][1] * t[1][j] + m[3][2] * t[2][j]);
    t[0][3] = t[1][3] = t[2][3] = 0;
    t[3][3] = 1;
    pfCopyMat(d, t);
}

void pfInvertAffMat(pfMatrix d, pfMatrix m)
{
    pfMatrix t;
    float det;
    int j;
    t[0][0] = m[1][1] * m[2][2] - m[1][2] * m[2][1];
    t[0][1] = m[0][2] * m[2][1] - m[0][1] * m[2][2];
    t[0][2] = m[0][1] * m[1][2] - m[0][2] * m[1][1];
    t[1][0] = m[1][2] * m[2][0] - m[1][0] * m[2][2];
    t[1][1] = m[0][0] * m[2][2] - m[0][2] * m[2][0];
    t[1][2] = m[0][2] * m[1][0] - m[0][0] * m[1][2];
    t[2][0] = m[1][0] * m[2][1] - m[1][1] * m[2][0];
    t[2][1] = m[0][1] * m[2][0] - m[0][0] * m[2][1];
    t[2][2] = m[0][0] * m[1][1] - m[0][1] * m[1][0];
    det = m[0][0] * t[0][0] + m[0][1] * t[1][0] + m[0][2] * t[2][0];
    if (det == 0)
        det = 1;
    for (j = 0; j < 9; j++)
        t[j / 3][j % 3] /= det;
    for (j = 0; j < 3; j++)
        t[3][j] = -(m[3][0] * t[0][j] + m[3][1] * t[1][j] + m[3][2] * t[2][j]);
    t[0][3] = t[1][3] = t[2][3] = 0;
    t[3][3] = 1;
    pfCopyMat(d, t);
}

void pfXformPt3(pfVec3 d, const pfVec3 v, pfMatrix m)
{
    pfVec3 t;
    int j;
    for (j = 0; j < 3; j++)
        t[j] = v[0] * m[0][j] + v[1] * m[1][j] + v[2] * m[2][j] + m[3][j];
    pfCopyVec3(d, t);
}

void pfXformVec3(pfVec3 d, const pfVec3 v, pfMatrix m)
{
    pfVec3 t;
    int j;
    for (j = 0; j < 3; j++)
        t[j] = v[0] * m[0][j] + v[1] * m[1][j] + v[2] * m[2][j];
    pfCopyVec3(d, t);
}

void pfMakePtsSeg(pfSeg *seg, const pfVec3 p1, const pfVec3 p2)
{
    pfCopyVec3(seg->pos, p1);
    pfSubVec3(seg->dir, p2, p1);
    seg->length = pfNormalizeVec3(seg->dir);
}

int pfClosestPtsOnSeg(const pfSeg *s1, const pfSeg *s2, pfVec3 p1, pfVec3 p2)
{
    pfVec3 w;
    float b = pfDotVec3(s1->dir, s2->dir), d, e, den, t1, t2;
    pfSubVec3(w, s1->pos, s2->pos);
    d = pfDotVec3(s1->dir, w);
    e = pfDotVec3(s2->dir, w);
    den = 1 - b * b;
    if (den < 1e-8f) {
        t1 = 0;
        t2 = e;
    } else {
        t1 = (b * e - d) / den;
        t2 = (e - b * d) / den;
    }
    if (t1 < 0) t1 = 0;
    if (t1 > s1->length) t1 = s1->length;
    t2 = pfDotVec3(s2->dir, w) + t1 * b;
    if (t2 < 0) t2 = 0;
    if (t2 > s2->length) t2 = s2->length;
    /* refine t1 from the clamped t2 */
    t1 = t2 * b - d;
    if (t1 < 0) t1 = 0;
    if (t1 > s1->length) t1 = s1->length;
    p1[0] = s1->pos[0] + t1 * s1->dir[0];
    p1[1] = s1->pos[1] + t1 * s1->dir[1];
    p1[2] = s1->pos[2] + t1 * s1->dir[2];
    p2[0] = s2->pos[0] + t2 * s2->dir[0];
    p2[1] = s2->pos[1] + t2 * s2->dir[1];
    p2[2] = s2->pos[2] + t2 * s2->dir[2];
    return 1;
}

/* --- memory ------------------------------------------------------------- */

typedef union { size_t size; double align; void *p; } MemHead;

void *pfGetSharedArena(void) { return NULL; }

void *pfMalloc(size_t size, void *arena)
{
    MemHead *h = (MemHead *)calloc(1, sizeof(MemHead) + size);
    (void)arena;
    if (!h) {
        fprintf(stderr, "pfMalloc: out of memory\n");
        exit(1);
    }
    h->size = size;
    return h + 1;
}

void *pfRealloc(void *p, size_t size)
{
    MemHead *h;
    if (!p)
        return pfMalloc(size, NULL);
    h = (MemHead *)realloc((MemHead *)p - 1, sizeof(MemHead) + size);
    h->size = size;
    return h + 1;
}

void pfFree(void *p)
{
    if (p)
        free((MemHead *)p - 1);
}

size_t pfGetSize(void *p)
{
    return p ? ((MemHead *)p - 1)->size : 0;
}

double pfGetTime(void)
{
    static double t0 = -1;
    double t;
#ifdef _WIN32
    static LARGE_INTEGER f;
    LARGE_INTEGER c;
    if (!f.QuadPart)
        QueryPerformanceFrequency(&f);
    QueryPerformanceCounter(&c);
    t = (double)c.QuadPart / (double)f.QuadPart;
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    t = tv.tv_sec + tv.tv_usec / 1e6;
#endif
    if (t0 < 0)
        t0 = t;
    return t - t0;
}

static double frame_time;
void pfFrameTime(double t) { frame_time = t; }
double pfGetFrameTime(void) { return frame_time; }

/* --- nodes -------------------------------------------------------------- */

static pfNode *new_node(int type)
{
    pfNode *n = (pfNode *)calloc(1, sizeof(pfNode));
    int i;
    n->type = type;
    for (i = 0; i < 4; i++)
        n->travmask[i] = 0xffffffffU;
    pfMakeIdentMat(n->mat);
    pfSetVec3(n->scale, 1, 1, 1);
    n->switchval = PFSWITCH_ON;
    n->interval = PFSEQ_CYCLE;
    n->end = -1;
    n->nreps = -1;
    n->speed = 1;
    n->dir = 1;
    n->morph = -1;
    return n;
}

pfGroup *pfNewGroup(void) { return new_node(PFNODE_GROUP); }
pfScene *pfNewScene(void) { return new_node(PFNODE_SCENE); }
pfDCS *pfNewDCS(void) { return new_node(PFNODE_DCS); }
pfSwitch *pfNewSwitch(void) { return new_node(PFNODE_SWITCH); }
pfGeode *pfNewGeode(void) { return new_node(PFNODE_GEODE); }

static int is_group(const pfNode *n)
{
    return n && n->type != PFNODE_GEODE && n->type != PFNODE_BBOARD &&
           n->type != PFNODE_LSOURCE;
}

int pfAddChild(pfNode *g, pfNode *c)
{
    return pfInsertChild(g, g ? g->nchildren : 0, c);
}

int pfInsertChild(pfNode *g, int index, pfNode *c)
{
    int i;
    if (!is_group(g) || !c)
        return -1;
    if (g->nchildren == g->maxchildren) {
        g->maxchildren = g->maxchildren ? g->maxchildren * 2 : 4;
        g->children = (pfNode **)realloc(g->children, sizeof(pfNode *) * g->maxchildren);
    }
    if (index < 0) index = 0;
    if (index > g->nchildren) index = g->nchildren;
    for (i = g->nchildren; i > index; i--)
        g->children[i] = g->children[i - 1];
    g->children[index] = c;
    return g->nchildren++;
}

int pfRemoveChild(pfNode *g, pfNode *c)
{
    int i;
    if (!g)
        return -1;
    for (i = 0; i < g->nchildren; i++) {
        if (g->children[i] == c) {
            memmove(g->children + i, g->children + i + 1,
                    sizeof(pfNode *) * (g->nchildren - i - 1));
            g->nchildren--;
            return 1;
        }
    }
    return -1;
}

int pfReplaceChild(pfNode *g, pfNode *old, pfNode *n)
{
    int i;
    if (!g)
        return -1;
    for (i = 0; i < g->nchildren; i++) {
        if (g->children[i] == old) {
            g->children[i] = n;
            return 1;
        }
    }
    return -1;
}

pfNode *pfGetChild(const pfNode *g, int i)
{
    return g && i >= 0 && i < g->nchildren ? g->children[i] : NULL;
}

int pfGetNumChildren(const pfNode *g) { return g ? g->nchildren : 0; }

int pfAddGSet(pfNode *geode, pfGeoSet *g)
{
    geode->gsets = (pfGeoSet **)realloc(geode->gsets, sizeof(pfGeoSet *) * (geode->ngsets + 1));
    geode->gsets[geode->ngsets] = g;
    return geode->ngsets++;
}

int pfGetNumGSets(const pfNode *n) { return n ? n->ngsets : 0; }
pfGeoSet *pfGetGSet(const pfNode *n, int i) { return n && i >= 0 && i < n->ngsets ? n->gsets[i] : NULL; }

pfNode *pfClone(pfNode *n, int mode)
{
    pfNode *c;
    int i;
    (void)mode;
    if (!n)
        return NULL;
    c = (pfNode *)malloc(sizeof(pfNode));
    *c = *n;
    if (n->name) {
        c->name = (char *)malloc(strlen(n->name) + 1);
        strcpy(c->name, n->name);
    }
    if (n->ngsets) {
        c->gsets = (pfGeoSet **)malloc(sizeof(pfGeoSet *) * n->ngsets);
        memcpy(c->gsets, n->gsets, sizeof(pfGeoSet *) * n->ngsets);
    }
    if (n->nframes) {
        c->times = (float *)malloc(sizeof(float) * n->nframes);
        memcpy(c->times, n->times, sizeof(float) * n->nframes);
    }
    c->children = NULL;
    c->nchildren = c->maxchildren = 0;
    for (i = 0; i < n->nchildren; i++)
        pfAddChild(c, pfClone(n->children[i], mode));
    return c;
}

int pfIsOfType(const pfNode *n, pfType type)
{
    if (!n)
        return 0;
    if (type == PFNODE_GROUP)
        return is_group(n);
    return n->type == type;
}

int pfIsExactType(const pfNode *n, pfType type) { return n && n->type == type; }

const char *pfGetNodeName(const pfNode *n) { return n ? n->name : NULL; }

pfNode *pfFindNode(pfNode *n, const char *name)
{
    int i;
    if (!n)
        return NULL;
    if (n->name && strcmp(n->name, name) == 0)
        return n;
    for (i = 0; i < n->nchildren; i++) {
        pfNode *f = pfFindNode(n->children[i], name);
        if (f)
            return f;
    }
    return NULL;
}

void pfNodeTravMask(pfNode *n, int which, unsigned int mask, int setmode, int bitop)
{
    int i;
    if (!n || which < 0 || which > 3)
        return;
    if (setmode & PFTRAV_SELF || !(setmode & PFTRAV_DESCEND)) {
        if (bitop == PF_OR) n->travmask[which] |= mask;
        else if (bitop == PF_AND) n->travmask[which] &= mask;
        else n->travmask[which] = mask;
    }
    if (setmode & PFTRAV_DESCEND) {
        for (i = 0; i < n->nchildren; i++)
            pfNodeTravMask(n->children[i], which, mask, setmode | PFTRAV_SELF, bitop);
        for (i = 0; i < n->ngsets; i++) {
            pfGeoSet *g = n->gsets[i];
            if (which == PFTRAV_ISECT) {
                if (bitop == PF_OR) g->isectmask |= mask;
                else if (bitop == PF_AND) g->isectmask &= mask;
                else g->isectmask = mask;
            }
        }
    }
}

unsigned int pfGetNodeTravMask(const pfNode *n, int which)
{
    return n && which >= 0 && which < 4 ? n->travmask[which] : 0;
}

void pfUserData(void *obj, void *data) { if (obj) ((pfNode *)obj)->userdata = data; }
void *pfGetUserData(void *obj) { return obj ? ((pfNode *)obj)->userdata : NULL; }

/* DCS matrix = scale * rotation * translation */
static void dcs_update(pfDCS *d)
{
    pfMatrix r;
    int i, j;
    pfMakeEulerMat(r, d->rot[0], d->rot[1], d->rot[2]);
    for (i = 0; i < 3; i++)
        for (j = 0; j < 3; j++)
            d->mat[i][j] = r[i][j] * d->scale[i];
    for (j = 0; j < 3; j++)
        d->mat[3][j] = d->trans[j];
    d->mat[0][3] = d->mat[1][3] = d->mat[2][3] = 0;
    d->mat[3][3] = 1;
}

void pfDCSTrans(pfDCS *d, float x, float y, float z)
{
    pfSetVec3(d->trans, x, y, z);
    dcs_update(d);
}

void pfDCSRot(pfDCS *d, float h, float p, float r)
{
    pfSetVec3(d->rot, h, p, r);
    dcs_update(d);
}

void pfDCSScale(pfDCS *d, float s)
{
    pfSetVec3(d->scale, s, s, s);
    dcs_update(d);
}

void pfDCSScaleXYZ(pfDCS *d, float x, float y, float z)
{
    pfSetVec3(d->scale, x, y, z);
    dcs_update(d);
}

/* hpr of a rotation matrix, the inverse of pfMakeEulerMat */
static void mat_hpr(pfMatrix m, pfVec3 hpr)
{
    double sp = m[1][2], cp;
    if (sp > 1) sp = 1;
    if (sp < -1) sp = -1;
    cp = sqrt(1 - sp * sp);
    hpr[1] = (float)PF_RAD2DEG(asin(sp));
    if (cp > 1e-6) {
        hpr[0] = (float)PF_RAD2DEG(atan2(-m[1][0], m[1][1]));
        hpr[2] = (float)PF_RAD2DEG(atan2(-m[0][2], m[2][2]));
    } else {
        hpr[0] = (float)PF_RAD2DEG(atan2(m[0][1], m[0][0]));
        hpr[2] = 0;
    }
}

void pfDCSMat(pfDCS *d, pfMatrix m)
{
    pfMatrix r;
    int i, j;
    pfCopyMat(d->mat, m);
    for (j = 0; j < 3; j++)
        d->trans[j] = m[3][j];
    for (i = 0; i < 3; i++) {
        d->scale[i] = (float)sqrt(m[i][0] * m[i][0] + m[i][1] * m[i][1] + m[i][2] * m[i][2]);
        for (j = 0; j < 3; j++)
            r[i][j] = d->scale[i] > 0 ? m[i][j] / d->scale[i] : 0;
    }
    mat_hpr(r, d->rot);
}

void pfGetDCSMat(pfDCS *d, pfMatrix m) { pfCopyMat(m, d->mat); }

void pfSwitchVal(pfSwitch *s, int val) { if (s) s->switchval = val; }
int pfGetSwitchVal(const pfSwitch *s) { return s ? s->switchval : 0; }

void pfSeqMode(pfSequence *s, int mode)
{
    if (!s)
        return;
    switch (mode) {
    case PFSEQ_START:
        s->frame = s->begin;
        s->dir = s->begin <= (s->end < 0 ? s->nchildren - 1 : s->end) ? 1 : -1;
        s->reps = 0;
        s->frame_start = frame_time;
        s->seqmode = PFSEQ_START;
        break;
    case PFSEQ_STOP:
        s->seqmode = PFSEQ_STOP;
        break;
    case PFSEQ_PAUSE:
        if (s->seqmode == PFSEQ_START)
            s->seqmode = PFSEQ_PAUSE;
        break;
    case PFSEQ_RESUME:
        if (s->seqmode == PFSEQ_PAUSE) {
            s->seqmode = PFSEQ_START;
            s->frame_start = frame_time;
        }
        break;
    }
}

int pfGetSeqMode(const pfSequence *s) { return s ? s->seqmode : 0; }

int pfGetSeqFrame(const pfSequence *s, int *repeat)
{
    if (repeat)
        *repeat = s->reps;
    return s->frame;
}

void pfSeqInterval(pfSequence *s, int mode, int begin, int end)
{
    s->interval = mode;
    s->begin = begin;
    s->end = end;
}

void pfSeqDuration(pfSequence *s, float speed, int nreps)
{
    s->speed = speed;
    s->nreps = nreps;
}

/* advances a running sequence to the frame of the given time */
static void seq_update(pfSequence *s, double t)
{
    int first = s->begin, last = s->end < 0 ? s->nchildren - 1 : s->end, guard = 0;
    if (s->nchildren == 0)
        return;
    if (first < 0) first = 0;
    if (last >= s->nchildren) last = s->nchildren - 1;
    if (first > s->nchildren - 1) first = s->nchildren - 1;
    if (s->seqmode != PFSEQ_START)
        return;
    while (guard++ < 1000) {
        float dt = s->frame >= 0 && s->frame < s->nframes ? s->times[s->frame] : 0.033f;
        double len;
        if (dt < 0) {
            /* negative time: hold this frame until the mode changes */
            return;
        }
        len = dt / (s->speed > 0 ? s->speed : 1.0f);
        if (t - s->frame_start < len)
            return;
        s->frame_start += len;
        if (s->frame == last && s->dir > 0) {
            if (s->interval == PFSEQ_SWING && first != last) {
                s->dir = -1;
                s->frame--;
            } else {
                s->reps++;
                if (s->nreps >= 0 && s->reps >= s->nreps) {
                    s->seqmode = PFSEQ_STOP;
                    return;
                }
                s->frame = first;
            }
        } else if (s->frame == first && s->dir < 0) {
            s->reps++;
            if (s->nreps >= 0 && s->reps >= s->nreps) {
                s->seqmode = PFSEQ_STOP;
                return;
            }
            s->dir = 1;
            s->frame++;
        } else {
            s->frame += s->dir;
        }
        if (s->frame < 0) s->frame = 0;
        if (s->frame >= s->nchildren) s->frame = s->nchildren - 1;
    }
    s->frame_start = t;
}

void pfMorphWeights(pfMorph *m, int attr, float *weights)
{
    (void)attr;
    if (m && m->pfb)
        pfb_morph(m->pfb, m->morph, weights);
}

/* --- geosets and states ------------------------------------------------- */

pfGeoSet *pfNewGSet(void *arena)
{
    pfGeoSet *g = (pfGeoSet *)calloc(1, sizeof(pfGeoSet));
    (void)arena;
    g->isectmask = 0xffffffffU;
    g->linewidth = 1;
    g->pntsize = 1;
    return g;
}

void pfGSetPrimType(pfGeoSet *g, int type) { g->prim = type; }
void pfGSetNumPrims(pfGeoSet *g, int n) { g->nprims = n; }
void pfGSetPrimLengths(pfGeoSet *g, int *lengths) { g->lengths = lengths; }
void pfGSetAttr(pfGeoSet *g, int attr, int bind, void *alist, unsigned short *ilist)
{
    (void)ilist;
    if (attr < 0 || attr > 3)
        return;
    g->attr[attr] = alist;
    g->bind[attr] = attr == PFGS_COORD3 ? PFGS_PER_VERTEX : bind;
}
void pfGSetPntSize(pfGeoSet *g, float s) { g->pntsize = s; }
void pfGSetLineWidth(pfGeoSet *g, float w) { g->linewidth = w; }
void pfGSetGState(pfGeoSet *g, pfGeoState *gs) { g->gstate = gs; }
pfGeoState *pfGetGSetGState(pfGeoSet *g) { return g ? g->gstate : NULL; }
void *pfGetGStateAttr(pfGeoState *gs, int attr)
{
    if (!gs)
        return NULL;
    switch (attr) {
    case PFSTATE_TEXTURE: return gs->texture;
    case PFSTATE_TEXENV: return gs->texenv;
    case PFSTATE_FRONTMTL: return gs->frontmtl;
    }
    return NULL;
}

static int gset_vertex_count(const pfGeoSet *g)
{
    int i, n = 0;
    switch (g->prim) {
    case PFGS_POINTS: return g->nprims;
    case PFGS_LINES: return 2 * g->nprims;
    case PFGS_TRIS: return 3 * g->nprims;
    case PFGS_QUADS: return 4 * g->nprims;
    }
    if (g->lengths)
        for (i = 0; i < g->nprims; i++)
            n += g->lengths[i];
    return n;
}

/* --- global drawing state ----------------------------------------------- */

static int en_lighting = 1, en_texture = 1;
static int saved_lighting, saved_texture;

void pfEnable(int what)
{
    if (what == PFEN_LIGHTING) en_lighting = 1;
    if (what == PFEN_TEXTURE) en_texture = 1;
}

void pfDisable(int what)
{
    if (what == PFEN_LIGHTING) en_lighting = 0;
    if (what == PFEN_TEXTURE) en_texture = 0;
}

int pfGetEnable(int what)
{
    return what == PFEN_LIGHTING ? en_lighting : what == PFEN_TEXTURE ? en_texture : 0;
}

void pfPushState(void)
{
    saved_lighting = en_lighting;
    saved_texture = en_texture;
    glPushAttrib(GL_ALL_ATTRIB_BITS);
}

void pfPopState(void)
{
    glPopAttrib();
    en_lighting = saved_lighting;
    en_texture = saved_texture;
}

static void apply_gstate(const pfGeoState *gs)
{
    int lighting = gs && gs->lighting > 0 && en_lighting && !(gs->lpoint > 0);
    int texturing = gs && gs->texturing > 0 && gs->texture && en_texture;

    if (lighting) {
        glEnable(GL_LIGHTING);
        if (gs->frontmtl) {
            const pfMaterial *m = gs->frontmtl;
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
            glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, m->shininess > 128 ? 128 : m->shininess);
            if (m->colormode) {
                glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
                glEnable(GL_COLOR_MATERIAL);
            } else {
                glDisable(GL_COLOR_MATERIAL);
            }
        } else {
            glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
            glEnable(GL_COLOR_MATERIAL);
        }
    } else {
        glDisable(GL_LIGHTING);
        glDisable(GL_COLOR_MATERIAL);
    }
    if (texturing) {
        pfApplyTex(gs->texture);
        glEnable(GL_TEXTURE_2D);
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE,
                  gs->texenv ? gs->texenv->mode : GL_MODULATE);
        if (gs->texenv)
            glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, gs->texenv->blend);
    } else {
        glDisable(GL_TEXTURE_2D);
    }
    if (gs && gs->cullface && gs->cullface != ~0U) {
        glEnable(GL_CULL_FACE);
        glCullFace(gs->cullface);
    } else {
        glDisable(GL_CULL_FACE);
    }
    if (gs && gs->alphafunc) {
        glEnable(GL_ALPHA_TEST);
        glAlphaFunc(gs->alphafunc, 0.0f);
    } else {
        glDisable(GL_ALPHA_TEST);
    }
    if (gs && gs->lpoint > 0) {
        glEnable(GL_POINT_SMOOTH);
    } else {
        glDisable(GL_POINT_SMOOTH);
    }
}

static const float *gattr(const pfGeoSet *g, int a, int idx, int n)
{
    if (!g->attr[a] || g->bind[a] == PFGS_OFF)
        return NULL;
    if (g->bind[a] == PFGS_OVERALL)
        idx = 0;
    return (const float *)g->attr[a] + idx * n;
}

static void draw_prims(const pfGeoSet *g)
{
    int flat = g->prim == PFGS_FLAT_TRISTRIPS || g->prim == PFGS_FLAT_LINESTRIPS;
    int skip = g->prim == PFGS_FLAT_TRISTRIPS ? 2 : 1;
    int vtx = 0, fvtx = 0, p, k, batch;
    GLenum mode;
    const float *c;

    switch (g->prim) {
    case PFGS_TRIFANS: mode = GL_TRIANGLE_FAN; break;
    case PFGS_TRISTRIPS: case PFGS_FLAT_TRISTRIPS: mode = GL_TRIANGLE_STRIP; break;
    case PFGS_TRIS: mode = GL_TRIANGLES; break;
    case PFGS_POINTS: mode = GL_POINTS; break;
    case PFGS_LINES: mode = GL_LINES; break;
    case PFGS_LINESTRIPS: case PFGS_FLAT_LINESTRIPS: mode = GL_LINE_STRIP; break;
    case PFGS_QUADS: mode = GL_QUADS; break;
    case PFGS_POLYS: mode = GL_POLYGON; break;
    default: return;
    }
    if (!g->attr[PFGS_COORD3])
        return;
    batch = mode == GL_POINTS || mode == GL_LINES || mode == GL_TRIANGLES || mode == GL_QUADS;
    glShadeModel(g->flatshade || flat ? GL_FLAT : GL_SMOOTH);
    if (!g->attr[PFGS_COLOR4] || g->bind[PFGS_COLOR4] == PFGS_OFF)
        glColor4f(1, 1, 1, 1);
    if (g->linewidth > 0)
        glLineWidth(g->linewidth);
    if (g->pntsize > 0)
        glPointSize(g->pntsize);
    if (batch && g->nprims > 0)
        glBegin(mode);
    for (p = 0; p < g->nprims; p++) {
        int n;
        switch (g->prim) {
        case PFGS_POINTS: n = 1; break;
        case PFGS_LINES: n = 2; break;
        case PFGS_TRIS: n = 3; break;
        case PFGS_QUADS: n = 4; break;
        default: n = g->lengths ? g->lengths[p] : 0; break;
        }
        if (!batch)
            glBegin(mode);
        if (g->bind[PFGS_COLOR4] == PFGS_OVERALL || g->bind[PFGS_COLOR4] == PFGS_PER_PRIM) {
            if ((c = gattr(g, PFGS_COLOR4, p, 4)) != NULL) glColor4fv(c);
        }
        if (g->bind[PFGS_NORMAL3] == PFGS_OVERALL || g->bind[PFGS_NORMAL3] == PFGS_PER_PRIM) {
            if ((c = gattr(g, PFGS_NORMAL3, p, 3)) != NULL) glNormal3fv(c);
        }
        for (k = 0; k < n; k++, vtx++) {
            int ai = flat ? fvtx + (k < skip ? 0 : k - skip) : vtx;
            if (g->bind[PFGS_COLOR4] == PFGS_PER_VERTEX && (c = gattr(g, PFGS_COLOR4, ai, 4)) != NULL)
                glColor4fv(c);
            if (g->bind[PFGS_NORMAL3] == PFGS_PER_VERTEX && (c = gattr(g, PFGS_NORMAL3, ai, 3)) != NULL)
                glNormal3fv(c);
            if (g->bind[PFGS_TEXCOORD2] == PFGS_PER_VERTEX && (c = gattr(g, PFGS_TEXCOORD2, vtx, 2)) != NULL)
                glTexCoord2fv(c);
            glVertex3fv((const float *)g->attr[PFGS_COORD3] + 3 * vtx);
        }
        if (flat)
            fvtx += n > skip ? n - skip : 0;
        if (!batch)
            glEnd();
    }
    if (batch && g->nprims > 0)
        glEnd();
    glLineWidth(1);
    glPointSize(1);
}

void pfDrawGSet(pfGeoSet *g)
{
    if (!g)
        return;
    if (g->gstate)
        apply_gstate(g->gstate);
    draw_prims(g);
}

/* --- textures ----------------------------------------------------------- */

static char file_path[4096] = ".";

void pfFilePath(const char *path)
{
    strncpy(file_path, path, sizeof(file_path) - 1);
}

int pfFindFile(const char *file, char *path, int amode)
{
    const char *p = file_path, *base;
    FILE *fp;
    (void)amode;
    if ((fp = fopen(file, "rb")) != NULL) {
        fclose(fp);
        strcpy(path, file);
        return 1;
    }
    base = strrchr(file, '/');
    base = base ? base + 1 : file;
    while (*p) {
        const char *e = strchr(p, ':');
        size_t n = e ? (size_t)(e - p) : strlen(p);
        if (n > 0 && n < 2000) {
            sprintf(path, "%.*s/%.1000s", (int)n, p, base);
            if ((fp = fopen(path, "rb")) != NULL) {
                fclose(fp);
                return 1;
            }
        }
        p += n;
        if (*p == ':')
            p++;
    }
    return 0;
}

static unsigned int be16(const unsigned char *p) { return (p[0] << 8) | p[1]; }
static unsigned long be32(const unsigned char *p)
{
    return ((unsigned long)p[0] << 24) | ((unsigned long)p[1] << 16) |
           ((unsigned long)p[2] << 8) | p[3];
}

/* SGI .rgb reader; RGBA words in memory order R, G, B, A */
static unsigned int *read_rgb(const char *path, int *w, int *h, int *comps)
{
    FILE *fp = fopen(path, "rb");
    unsigned char *file, *img;
    long size;
    int rle, bpc, xs, ys, zs, x, y, z;

    if (!fp)
        return NULL;
    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    if (size < 512) {
        fclose(fp);
        return NULL;
    }
    file = (unsigned char *)malloc(size);
    if (fread(file, 1, size, fp) != (size_t)size || be16(file) != 474) {
        fclose(fp);
        free(file);
        return NULL;
    }
    fclose(fp);
    rle = file[2];
    bpc = file[3];
    xs = be16(file + 6);
    ys = be16(file + 8);
    zs = be16(file + 10);
    if (zs < 1)
        zs = 1;
    if (bpc != 1 || xs == 0 || ys == 0 || zs > 4 ||
        (!rle && 512L + (long)xs * ys * zs > size) ||
        (rle && 512L + 8L * ys * zs > size)) {
        free(file);
        return NULL;
    }
    img = (unsigned char *)malloc((size_t)xs * ys * 4);
    for (x = 0; x < xs * ys; x++) {
        img[4 * x] = img[4 * x + 1] = img[4 * x + 2] = 0;
        img[4 * x + 3] = 255;
    }
    for (z = 0; z < zs; z++) {
        for (y = 0; y < ys; y++) {
            unsigned char row[65536];
            if (rle) {
                unsigned long start = be32(file + 512 + 4L * (z * ys + y));
                unsigned long len = be32(file + 512 + 4L * ys * zs + 4L * (z * ys + y));
                const unsigned char *s = file + start, *e = file + start + len;
                int n = 0;
                if (start + len > (unsigned long)size)
                    continue;
                while (s < e && n < xs) {
                    int c = *s++, cnt = c & 0x7f;
                    if (!cnt)
                        break;
                    if (c & 0x80) {
                        while (cnt-- && s < e && n < xs)
                            row[n++] = *s++;
                    } else if (s < e) {
                        unsigned char v = *s++;
                        while (cnt-- && n < xs)
                            row[n++] = v;
                    }
                }
                while (n < xs)
                    row[n++] = 0;
            } else {
                memcpy(row, file + 512 + ((size_t)z * ys + y) * xs, xs);
            }
            for (x = 0; x < xs; x++) {
                unsigned char *p = img + ((size_t)y * xs + x) * 4;
                if (zs < 3) {
                    if (z == 0)
                        p[0] = p[1] = p[2] = row[x];
                    else
                        p[3] = row[x];
                } else {
                    p[z] = row[x];
                }
            }
        }
    }
    free(file);
    *w = xs;
    *h = ys;
    *comps = zs;
    return (unsigned int *)img;
}

pfTexture *pfNewTex(void *arena)
{
    (void)arena;
    return (pfTexture *)calloc(1, sizeof(pfTexture));
}

int pfLoadTexFile(pfTexture *t, const char *file)
{
    char path[4096];
    if (!pfFindFile(file, path, 4)) {
        fprintf(stderr, "mekton: texture %s not found\n", file);
        return 0;
    }
    t->image = read_rgb(path, &t->xsize, &t->ysize, &t->comps);
    if (!t->image)
        return 0;
    free(t->name);
    t->name = (char *)malloc(strlen(file) + 1);
    strcpy(t->name, file);
    return 1;
}

void pfGetTexImage(pfTexture *t, unsigned int **image, int *comp, int *ns, int *nt, int *nr)
{
    if (image) *image = t->image;
    if (comp) *comp = t->comps;
    if (ns) *ns = t->xsize;
    if (nt) *nt = t->ysize;
    if (nr) *nr = 1;
}

void pfApplyTex(pfTexture *t)
{
    if (!t || !t->image)
        return;
    if (!t->gl_id) {
        glGenTextures(1, &t->gl_id);
        glBindTexture(GL_TEXTURE_2D, t->gl_id);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA, t->xsize, t->ysize, GL_RGBA,
                          GL_UNSIGNED_BYTE, t->image);
    } else {
        glBindTexture(GL_TEXTURE_2D, t->gl_id);
    }
}

void pfTexFilter(pfTexture *t, int filt, int val) { (void)t; (void)filt; (void)val; }

/* --- lights ------------------------------------------------------------- */

#define MAX_LIGHTS 8
static pfLightSource *lights[MAX_LIGHTS];
static int nlights;

pfLightSource *pfNewLSource(void)
{
    pfLightSource *l = (pfLightSource *)calloc(1, sizeof(pfLightSource));
    l->pos[2] = 1;
    pfSetVec3(l->diffuse, 1, 1, 1);
    pfSetVec3(l->specular, 1, 1, 1);
    if (nlights < MAX_LIGHTS)
        lights[nlights++] = l;
    return l;
}

void pfLSourcePos(pfLightSource *l, float x, float y, float z, float w)
{
    l->pos[0] = x; l->pos[1] = y; l->pos[2] = z; l->pos[3] = w;
}

void pfLSourceColor(pfLightSource *l, int which, float r, float g, float b)
{
    float *c = which == PFLT_AMBIENT ? l->ambient : which == PFLT_SPECULAR ? l->specular : l->diffuse;
    pfSetVec3(c, r, g, b);
}

void pfLSourceOn(pfLightSource *l) { l->on = 1; }
void pfLSourceOff(pfLightSource *l) { l->on = 0; }

pfNode *pfLSourceNode(pfLightSource *l)
{
    pfNode *n = new_node(PFNODE_LSOURCE);
    n->lsource = l;
    return n;
}

/* lights are placed with the matrix in effect where their node is found */
static void place_light(pfLightSource *l)
{
    int i;
    for (i = 0; i < nlights; i++) {
        if (lights[i] == l) {
            float a[4], d[4], s[4];
            GLenum id = GL_LIGHT0 + i;
            a[3] = d[3] = s[3] = 1;
            memcpy(a, l->ambient, sizeof(pfVec3));
            memcpy(d, l->diffuse, sizeof(pfVec3));
            memcpy(s, l->specular, sizeof(pfVec3));
            glLightfv(id, GL_AMBIENT, a);
            glLightfv(id, GL_DIFFUSE, d);
            glLightfv(id, GL_SPECULAR, s);
            glLightfv(id, GL_POSITION, l->pos);
            if (l->on) glEnable(id);
            else glDisable(id);
        }
    }
}

/* --- drawing ------------------------------------------------------------ */

static int draw_pass;

static void draw_node(pfNode *n, int depth)
{
    int i;
    if (!n || depth > 64 || !n->travmask[PFTRAV_DRAW])
        return;
    if (n->type == PFNODE_DCS || n->type == PFNODE_SCS) {
        glPushMatrix();
        glMultMatrixf(&n->mat[0][0]);
    }
    if (n->type == PFNODE_LSOURCE && draw_pass == 0)
        place_light(n->lsource);
    for (i = 0; i < n->ngsets; i++) {
        pfGeoSet *g = n->gsets[i];
        int transp = g->gstate && (g->gstate->transparency > 0 || g->gstate->lpoint > 0);
        if (transp != draw_pass)
            continue;
        pfDrawGSet(g);
    }
    switch (n->type) {
    case PFNODE_SWITCH:
        if (n->switchval == PFSWITCH_ON) {
            for (i = 0; i < n->nchildren; i++)
                draw_node(n->children[i], depth + 1);
        } else if (n->switchval >= 0 && n->switchval < n->nchildren) {
            draw_node(n->children[n->switchval], depth + 1);
        }
        break;
    case PFNODE_SEQUENCE:
        if (n->seqmode != PFSEQ_STOP)
            if (n->frame >= 0 && n->frame < n->nchildren)
                draw_node(n->children[n->frame], depth + 1);
        break;
    case PFNODE_LOD:
        if (n->nchildren)
            draw_node(n->children[0], depth + 1);
        break;
    default:
        for (i = 0; i < n->nchildren; i++)
            draw_node(n->children[i], depth + 1);
        break;
    }
    if (n->type == PFNODE_DCS || n->type == PFNODE_SCS)
        glPopMatrix();
}

static void update_node(pfNode *n, double t, int depth)
{
    int i;
    if (!n || depth > 64)
        return;
    if (n->type == PFNODE_SEQUENCE)
        seq_update(n, t);
    for (i = 0; i < n->nchildren; i++)
        update_node(n->children[i], t, depth + 1);
}

void pfDrawScene(pfNode *root, double t)
{
    update_node(root, t, 0);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_NORMALIZE);
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
    draw_pass = 0;
    draw_node(root, 0);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    draw_pass = 1;
    draw_node(root, 0);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_CULL_FACE);
    glDisable(GL_ALPHA_TEST);
}

/* --- intersection ------------------------------------------------------- */

static void build_tris(pfGeoSet *g)
{
    int nv = gset_vertex_count(g), cap = nv * 3 + 3, v = 0, p, k, n;
    g->tris = (int *)malloc(sizeof(int) * cap);
    g->ntris = 0;
    for (p = 0; p < g->nprims; p++) {
        switch (g->prim) {
        case PFGS_TRIS: n = 3; break;
        case PFGS_QUADS: n = 4; break;
        case PFGS_POINTS: n = 1; break;
        case PFGS_LINES: n = 2; break;
        default: n = g->lengths ? g->lengths[p] : 0; break;
        }
        switch (g->prim) {
        case PFGS_TRIS:
            g->tris[3 * g->ntris] = v; g->tris[3 * g->ntris + 1] = v + 1; g->tris[3 * g->ntris + 2] = v + 2;
            g->ntris++;
            break;
        case PFGS_QUADS:
        case PFGS_POLYS:
        case PFGS_TRIFANS:
            for (k = 2; k < n; k++) {
                g->tris[3 * g->ntris] = v; g->tris[3 * g->ntris + 1] = v + k - 1; g->tris[3 * g->ntris + 2] = v + k;
                g->ntris++;
            }
            break;
        case PFGS_TRISTRIPS:
        case PFGS_FLAT_TRISTRIPS:
            for (k = 2; k < n; k++) {
                int a = v + k - 2, b = v + k - 1;
                if (k & 1) { int x = a; a = b; b = x; }
                g->tris[3 * g->ntris] = a; g->tris[3 * g->ntris + 1] = b; g->tris[3 * g->ntris + 2] = v + k;
                g->ntris++;
            }
            break;
        }
        v += n;
    }
}

typedef struct {
    pfSegSet *ss;
    int depth;
    pfNode *path[16];
    pfHit hit[PFIS_MAX_SEGS];
    float best[PFIS_MAX_SEGS];
} IsectState;

static void isect_gset(IsectState *st, pfNode *geode, pfGeoSet *g, pfMatrix m, int prim_ok)
{
    const float *co = (const float *)g->attr[PFGS_COORD3];
    int t, s;
    (void)prim_ok;
    if (!co || !(g->isectmask & st->ss->isectMask))
        return;
    if (!g->tris)
        build_tris(g);
    for (t = 0; t < g->ntris; t++) {
        pfVec3 a, b, c, e1, e2, nrm;
        pfXformPt3(a, co + 3 * g->tris[3 * t], m);
        pfXformPt3(b, co + 3 * g->tris[3 * t + 1], m);
        pfXformPt3(c, co + 3 * g->tris[3 * t + 2], m);
        pfSubVec3(e1, b, a);
        pfSubVec3(e2, c, a);
        pfCrossVec3(nrm, e1, e2);
        for (s = 0; s < PFIS_MAX_SEGS; s++) {
            const pfSeg *sg = &st->ss->segs[s];
            pfVec3 pv, tv, qv;
            float det, u, v, d;
            if (!(st->ss->activeMask & (1U << s)))
                continue;
            if ((st->ss->mode & PFTRAV_IS_CULL_BACK) && pfDotVec3(nrm, sg->dir) > 0)
                continue;
            if ((st->ss->mode & PFTRAV_IS_CULL_FRONT) && pfDotVec3(nrm, sg->dir) < 0)
                continue;
            pfCrossVec3(pv, sg->dir, e2);
            det = pfDotVec3(e1, pv);
            if (det > -1e-9f && det < 1e-9f)
                continue;
            pfSubVec3(tv, sg->pos, a);
            u = pfDotVec3(tv, pv) / det;
            if (u < 0 || u > 1)
                continue;
            pfCrossVec3(qv, tv, e1);
            v = pfDotVec3(sg->dir, qv) / det;
            if (v < 0 || u + v > 1)
                continue;
            d = pfDotVec3(e2, qv) / det;
            if (d < 0 || d > sg->length || d >= st->best[s])
                continue;
            st->best[s] = d;
            st->hit[s].flags = PFHIT_ISECT | PFHIT_POINT | PFHIT_NORM | PFHIT_PRIM | PFHIT_TRI;
            st->hit[s].segnum = s;
            st->hit[s].seg = *sg;
            st->hit[s].point[0] = sg->pos[0] + d * sg->dir[0];
            st->hit[s].point[1] = sg->pos[1] + d * sg->dir[1];
            st->hit[s].point[2] = sg->pos[2] + d * sg->dir[2];
            pfCopyVec3(st->hit[s].norm, nrm);
            pfNormalizeVec3(st->hit[s].norm);
            st->hit[s].node = geode;
            st->hit[s].gset = g;
            st->hit[s].tri = t;
            pfMakeIdentMat(st->hit[s].xform);
            st->hit[s].pathlen = st->depth < 16 ? st->depth : 16;
            memcpy(st->hit[s].path, st->path, sizeof(pfNode *) * st->hit[s].pathlen);
        }
    }
}

static void isect_node(IsectState *st, pfNode *n, pfMatrix m, int depth)
{
    pfMatrix mm;
    int i;
    if (!n || depth > 64 || !(n->travmask[PFTRAV_ISECT] & st->ss->isectMask))
        return;
    if (depth < 16)
        st->path[depth] = n;
    st->depth = depth + 1;
    if (n->type == PFNODE_DCS || n->type == PFNODE_SCS) {
        pfMultMat(mm, n->mat, m);
        m = mm;
    }
    for (i = 0; i < n->ngsets; i++)
        isect_gset(st, n, n->gsets[i], m, 1);
    if (n->type == PFNODE_SWITCH) {
        if (n->switchval == PFSWITCH_ON) {
            for (i = 0; i < n->nchildren; i++)
                isect_node(st, n->children[i], m, depth + 1);
        } else if (n->switchval >= 0 && n->switchval < n->nchildren) {
            isect_node(st, n->children[n->switchval], m, depth + 1);
        }
    } else if (n->type == PFNODE_SEQUENCE) {
        if (n->frame >= 0 && n->frame < n->nchildren)
            isect_node(st, n->children[n->frame], m, depth + 1);
    } else {
        for (i = 0; i < n->nchildren; i++)
            isect_node(st, n->children[i], m, depth + 1);
    }
    st->depth = depth;
}

int pfNodeIsectSegs(pfNode *n, pfSegSet *ss, pfHit **hits[])
{
    static IsectState st;
    static pfHit *hitp[PFIS_MAX_SEGS];
    pfMatrix ident;
    int s, count = 0;

    st.ss = ss;
    for (s = 0; s < PFIS_MAX_SEGS; s++) {
        st.best[s] = 1e30f;
        memset(&st.hit[s], 0, sizeof(pfHit));
        hitp[s] = &st.hit[s];
        hits[s] = &hitp[s];
    }
    pfMakeIdentMat(ident);
    isect_node(&st, n, ident, 0);
    for (s = 0; s < PFIS_MAX_SEGS; s++)
        if (st.hit[s].flags)
            count++;
    return count;
}

int pfQueryHit(const pfHit *h, unsigned int which, void *dst)
{
    if (!h)
        return 0;
    switch (which) {
    case PFQHIT_FLAGS: *(int *)dst = h->flags; break;
    case PFQHIT_SEGNUM: *(int *)dst = h->segnum; break;
    case PFQHIT_SEG: *(pfSeg *)dst = h->seg; break;
    case PFQHIT_POINT: pfCopyVec3((float *)dst, h->point); break;
    case PFQHIT_NORM: pfCopyVec3((float *)dst, h->norm); break;
    case PFQHIT_GSET: *(pfGeoSet **)dst = h->gset; break;
    case PFQHIT_NODE: *(pfNode **)dst = h->node; break;
    case PFQHIT_XFORM: memcpy(dst, h->xform, sizeof(pfMatrix)); break;
    case PFQHIT_PATH: *(const pfHit **)dst = h; break;
    default: return 0;
    }
    return 1;
}

void *pfGet(void *path, int i)
{
    const pfHit *h = (const pfHit *)path;
    return h && i >= 0 && i < h->pathlen ? h->path[i] : NULL;
}

/* --- loader ------------------------------------------------------------- */

static int ntexcache;
static pfTexture **texcache;

static pfTexture *find_texture(const char *name)
{
    const char *base = strrchr(name, '/');
    pfTexture *t;
    int i;
    base = base ? base + 1 : name;
    for (i = 0; i < ntexcache; i++)
        if (strcmp(texcache[i]->name, base) == 0)
            return texcache[i];
    t = pfNewTex(NULL);
    if (!pfLoadTexFile(t, base)) {
        free(t);
        return NULL;
    }
    texcache = (pfTexture **)realloc(texcache, sizeof(pfTexture *) * (ntexcache + 1));
    texcache[ntexcache++] = t;
    return t;
}

static pfNode *build_node(PfbFile *f, int idx, pfNode **made, pfGeoSet **gsets, int depth)
{
    static const int prim_map[10] = {
        PFGS_TRIFANS, PFGS_TRISTRIPS, PFGS_TRIS, PFGS_POINTS, PFGS_LINES,
        PFGS_LINESTRIPS, PFGS_FLAT_LINESTRIPS, PFGS_QUADS, PFGS_FLAT_TRISTRIPS, PFGS_POLYS
    };
    PfbNode *pn;
    pfNode *n;
    int i;
    (void)prim_map;
    if (idx < 0 || idx >= f->nnode || depth > 64)
        return NULL;
    if (made[idx])
        return made[idx];
    pn = &f->node[idx];
    switch (pn->type) {
    case PFB_NODE_GEODE: case PFB_NODE_BBOARD: case PFB_NODE_TEXT:
        n = new_node(pn->type == PFB_NODE_BBOARD ? PFNODE_BBOARD : PFNODE_GEODE); break;
    case PFB_NODE_DCS: n = new_node(PFNODE_DCS); break;
    case PFB_NODE_SCS: n = new_node(PFNODE_SCS); break;
    case PFB_NODE_SWITCH: n = new_node(PFNODE_SWITCH); n->switchval = pn->switch_val; break;
    case PFB_NODE_SEQUENCE: n = new_node(PFNODE_SEQUENCE); break;
    case PFB_NODE_LOD: n = new_node(PFNODE_LOD); break;
    case PFB_NODE_LAYER: n = new_node(PFNODE_LAYER); break;
    case PFB_NODE_SCENE: n = new_node(PFNODE_SCENE); break;
    case PFB_NODE_MORPH: n = new_node(PFNODE_MORPH); n->pfb = f; n->morph = pn->ref; break;
    default: n = new_node(PFNODE_GROUP); break;
    }
    made[idx] = n;
    if (pn->name) {
        n->name = (char *)malloc(strlen(pn->name) + 1);
        strcpy(n->name, pn->name);
    }
    for (i = 0; i < 4; i++)
        n->travmask[i] = pn->travmask[i];
    if (n->type == PFNODE_DCS)
        pfDCSMat(n, (float (*)[4])pn->matrix);
    else if (n->type == PFNODE_SCS)
        memcpy(n->mat, pn->matrix, sizeof(pfMatrix));
    if (n->type == PFNODE_SEQUENCE) {
        n->nframes = pn->nframes;
        n->times = (float *)malloc(sizeof(float) * (pn->nframes + 1));
        memcpy(n->times, pn->times, sizeof(float) * pn->nframes);
        n->interval = pn->seq_mode == 2 ? PFSEQ_SWING : PFSEQ_CYCLE;
        n->begin = pn->seq_begin;
        n->end = pn->seq_end;
        n->speed = pn->seq_speed;
        n->nreps = pn->seq_reps;
    }
    if (pn->userdata >= 0 && pn->userdata < f->nud && f->ud[pn->userdata]) {
        void *u = pfMalloc(f->udsize[pn->userdata], NULL);
        memcpy(u, f->ud[pn->userdata], f->udsize[pn->userdata]);
        n->userdata = u;
    }
    for (i = 0; i < pn->ngsets; i++)
        if (pn->gsets[i] >= 0 && pn->gsets[i] < f->ngset)
            pfAddGSet(n, gsets[pn->gsets[i]]);
    for (i = 0; i < pn->nchildren; i++) {
        pfNode *c = build_node(f, pn->children[i], made, gsets, depth + 1);
        if (c)
            pfAddChild(n, c);
    }
    /* sequences from files start running, as pfdLoadFile_pfb sets their
     * mode from the file (Mekton stores PFSEQ_START) */
    if (n->type == PFNODE_SEQUENCE && pn->seq_mode >= 0)
        pfSeqMode(n, PFSEQ_START);
    return n;
}

pfNode *pfdLoadFile(const char *file)
{
    static const int prim_map[10] = {
        PFGS_TRIFANS, PFGS_TRISTRIPS, PFGS_TRIS, PFGS_POINTS, PFGS_LINES,
        PFGS_LINESTRIPS, PFGS_FLAT_LINESTRIPS, PFGS_QUADS, PFGS_FLAT_TRISTRIPS, PFGS_POLYS
    };
    static const int bind_map[5] = { PFGS_OFF, PFGS_OFF, PFGS_PER_VERTEX, PFGS_PER_PRIM, PFGS_OVERALL };
    static const int list_type[4] = { 5, 6, 7, 8 };
    static const int attr_map[4] = { PFGS_COORD3, PFGS_COLOR4, PFGS_NORMAL3, PFGS_TEXCOORD2 };
    static const GLenum cull_tab[5] = { 0, 0, GL_BACK, GL_FRONT, GL_FRONT_AND_BACK };
    static const GLenum afunc_tab[10] = {
        GL_ALWAYS, GL_ALWAYS, GL_NEVER, GL_LESS, GL_EQUAL, GL_LEQUAL,
        GL_GREATER, GL_NOTEQUAL, GL_GEQUAL, GL_ALWAYS
    };
    static const GLenum tenv_tab[5] = { GL_MODULATE, GL_MODULATE, GL_BLEND, GL_DECAL, GL_REPLACE };
    char path[4096];
    PfbFile *f;
    pfMaterial *mtl;
    pfTexEnv *tenv;
    pfTexture **tex;
    pfGeoState *gst;
    pfGeoSet **gsets;
    pfNode **made, *root;
    int i, a;

    if (!pfFindFile(file, path, 4) || (f = pfb_load(path)) == NULL) {
        fprintf(stderr, "mekton: cannot load %s\n", file);
        return NULL;
    }
    mtl = (pfMaterial *)calloc(f->nmtl + 1, sizeof(pfMaterial));
    for (i = 0; i < f->nmtl; i++) {
        mtl[i].alpha = f->mtl[i].alpha;
        mtl[i].shininess = f->mtl[i].shininess;
        memcpy(mtl[i].ambient, f->mtl[i].ambient, sizeof(pfVec3));
        memcpy(mtl[i].diffuse, f->mtl[i].diffuse, sizeof(pfVec3));
        memcpy(mtl[i].specular, f->mtl[i].specular, sizeof(pfVec3));
        memcpy(mtl[i].emission, f->mtl[i].emission, sizeof(pfVec3));
        mtl[i].colormode = f->mtl[i].cmode == 2;
    }
    tenv = (pfTexEnv *)calloc(f->ntenv + 1, sizeof(pfTexEnv));
    for (i = 0; i < f->ntenv; i++) {
        int m = f->tenv[i].mode;
        tenv[i].mode = (int)tenv_tab[m >= 0 && m < 5 ? m : 1];
        memcpy(tenv[i].blend, f->tenv[i].blend, sizeof(pfVec4));
    }
    tex = (pfTexture **)calloc(f->ntex + 1, sizeof(pfTexture *));
    for (i = 0; i < f->ntex; i++)
        if (f->tex[i].name)
            tex[i] = find_texture(f->tex[i].name);
    gst = (pfGeoState *)calloc(f->ngstate + 1, sizeof(pfGeoState));
    for (i = 0; i < f->ngstate; i++) {
        const PfbGState *s = &f->gstate[i];
        gst[i].lighting = s->lighting == PFB_INHERIT ? -1 : s->lighting == 2;
        gst[i].texturing = s->texturing == PFB_INHERIT ? -1 : s->texturing == 2;
        gst[i].lpoint = s->lpoint == PFB_INHERIT ? -1 : s->lpoint == 2;
        gst[i].transparency = s->transparency == PFB_INHERIT ? -1 : s->transparency >= 2;
        gst[i].cullface = s->cullface >= 0 && s->cullface < 5 ? cull_tab[s->cullface] : 0;
        gst[i].alphafunc = s->alphafunc >= 0 && s->alphafunc < 10 ? afunc_tab[s->alphafunc] : 0;
        if (s->frontmtl >= 0 && s->frontmtl < f->nmtl) gst[i].frontmtl = &mtl[s->frontmtl];
        if (s->texture >= 0 && s->texture < f->ntex) gst[i].texture = tex[s->texture];
        if (s->texenv >= 0 && s->texenv < f->ntenv) gst[i].texenv = &tenv[s->texenv];
    }
    gsets = (pfGeoSet **)calloc(f->ngset + 1, sizeof(pfGeoSet *));
    for (i = 0; i < f->ngset; i++) {
        const PfbGSet *s = &f->gset[i];
        pfGeoSet *g = pfNewGSet(NULL);
        g->prim = s->prim >= 0 && s->prim < 10 ? prim_map[s->prim] : PFGS_POINTS;
        g->nprims = s->nprims;
        if (s->lengths >= 0 && s->lengths < f->nlist[4])
            g->lengths = f->list[4][s->lengths].i;
        for (a = 0; a < 4; a++) {
            const PfbAttr *pa = &s->attr[a];
            int t = list_type[a];
            if (pa->list >= 0 && pa->list < f->nlist[t] && pa->bind >= 0 && pa->bind < 5) {
                g->attr[attr_map[a]] = f->list[t][pa->list].f;
                g->bind[attr_map[a]] = bind_map[pa->bind];
                if (a == 0)
                    g->nverts = f->list[t][pa->list].count;
            }
        }
        g->bind[PFGS_COORD3] = PFGS_PER_VERTEX;
        g->flatshade = s->flatshade;
        g->linewidth = s->linewidth;
        g->pntsize = s->pointsize > 0 ? s->pointsize : (s->prim == PFB_POINTS ? 4.0f : 1.0f);
        g->isectmask = s->isectmask;
        if (s->gstate >= 0 && s->gstate < f->ngstate)
            g->gstate = &gst[s->gstate];
        /* geosets must not read past their attribute lists */
        if (g->attr[PFGS_COORD3] && gset_vertex_count(g) > g->nverts)
            g->nprims = 0;
        gsets[i] = g;
    }
    made = (pfNode **)calloc(f->nnode + 1, sizeof(pfNode *));
    root = build_node(f, 0, made, gsets, 0);
    free(made);
    free(gsets);
    free(tex);
    /* materials, texenvs, gstates and the file data stay alive with the
     * geosets that use them */
    return root;
}
