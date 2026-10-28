/*
 * vg.c - minimal Vega compatibility layer for Certain Impact, see vg.h.
 */
#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#ifdef _WIN32
#include <windows.h>
#endif
#include <GL/gl.h>
#endif
#include <GL/freeglut.h>

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vg.h"
#include "fst.h"

/* --- files ---------------------------------------------------------------- */

static char data_dir[1024] = "data/cimpact";

static const char *subdirs[] = {
    "", "models_ogl", "contours_ogl", "img", "bin",
    "visual_data/models_ogl", "visual_data/contours_ogl", "visual_data/img",
    "data/visual_data/models_ogl", "data/visual_data/contours_ogl", "data/visual_data/img",
    NULL
};

void vgDataDir(const char *dir)
{
    strncpy(data_dir, dir, sizeof(data_dir) - 1);
    data_dir[sizeof(data_dir) - 1] = 0;
}

static int exists(const char *p)
{
    FILE *f = fopen(p, "rb");
    if (!f)
        return 0;
    fclose(f);
    return 1;
}

int vgFindFile(const char *name, char *path, int size)
{
    static const char *suffix[] = { "", ".Z", ".gz", NULL };
    char buf[2048];
    int i, k;
    for (i = 0; subdirs[i]; i++) {
        for (k = 0; suffix[k]; k++) {
            if (subdirs[i][0])
                sprintf(buf, "%.900s/%.100s/%.900s%s", data_dir, subdirs[i], name, suffix[k]);
            else
                sprintf(buf, "%.900s/%.900s%s", data_dir, name, suffix[k]);
            if (exists(buf)) {
                strncpy(path, buf, size - 1);
                path[size - 1] = 0;
                return 1;
            }
        }
    }
    if (exists(name)) {
        strncpy(path, name, size - 1);
        path[size - 1] = 0;
        return 1;
    }
    return 0;
}

/* --- application definition file ------------------------------------------ */

static vgObject *objects;
static vgScene scenes[16];
static int nscenes;
static vgEnv env = { { 0.365864f, 0.513054f, 0.53f }, 0, 1500 };

vgEnv *vgGetEnv(void) { return &env; }

typedef struct {
    const char *s;
    char tok[256];
} Lexer;

/* next token: a word, a quoted string, '{', '}' or ';'; 0 at the end */
static int lex(Lexer *l)
{
    const char *s = l->s;
    int n = 0;
    for (;;) {
        while (*s && isspace((unsigned char)*s))
            s++;
        if (s[0] == '/' && s[1] == '/') {
            while (*s && *s != '\n')
                s++;
            continue;
        }
        break;
    }
    if (!*s) {
        l->s = s;
        return 0;
    }
    if (*s == '{' || *s == '}' || *s == ';') {
        l->tok[0] = *s++;
        l->tok[1] = 0;
    } else if (*s == '"') {
        s++;
        while (*s && *s != '"' && n < 255)
            l->tok[n++] = *s++;
        if (*s == '"')
            s++;
        l->tok[n] = 0;
    } else {
        while (*s && !isspace((unsigned char)*s) && *s != ';' && *s != '{' && *s != '}' && n < 255)
            l->tok[n++] = *s++;
        l->tok[n] = 0;
    }
    l->s = s;
    return 1;
}

static vgObject *new_object(const char *name)
{
    vgObject *o = (vgObject *)calloc(1, sizeof(vgObject));
    strncpy(o->name, name, sizeof(o->name) - 1);
    o->scale = 1;
    o->enabled = 1;
    pfMakeIdentMat(o->mat);
    o->next = objects;
    objects = o;
    return o;
}

static vgScene *new_scene(const char *name)
{
    vgScene *s;
    if (nscenes >= 16)
        return &scenes[15];
    s = &scenes[nscenes++];
    strncpy(s->name, name, sizeof(s->name) - 1);
    return s;
}

int vgLoadADF(const char *file)
{
    char path[2048], *text, block[64], name[64], key[64], vals[16][256];
    FILE *f;
    long size;
    Lexer l;
    int depth = 0, nv;
    vgObject *obj = NULL;
    vgScene *scene = NULL;
    int env_seen = 0, in_env = 0;

    if (!vgFindFile(file, path, sizeof(path)) || !(f = fopen(path, "rb"))) {
        fprintf(stderr, "vg: cannot find %s\n", file);
        return -1;
    }
    fseek(f, 0, SEEK_END);
    size = ftell(f);
    fseek(f, 0, SEEK_SET);
    text = (char *)malloc(size + 1);
    size = (long)fread(text, 1, size, f);
    text[size] = 0;
    fclose(f);

    l.s = text;
    block[0] = name[0] = 0;
    while (lex(&l)) {
        if (depth == 0) {
            if (l.tok[0] == '{') {
                depth = 1;
                obj = NULL;
                scene = NULL;
                in_env = 0;
                if (strcmp(block, "object") == 0)
                    obj = new_object(name);
                else if (strcmp(block, "scene") == 0)
                    scene = new_scene(name);
                else if (strcmp(block, "env") == 0 && !env_seen) {
                    env_seen = in_env = 1;
                }
            } else if (l.tok[0] != ';' && l.tok[0] != '}') {
                if (!block[0] || name[0]) {
                    strncpy(block, l.tok, sizeof(block) - 1);
                    name[0] = 0;
                } else {
                    strncpy(name, l.tok, sizeof(name) - 1);
                }
            }
            continue;
        }
        if (l.tok[0] == '}') {
            depth = 0;
            block[0] = name[0] = 0;
            continue;
        }
        if (l.tok[0] == ';' || l.tok[0] == '{')
            continue;
        /* a statement: key values ; */
        strncpy(key, l.tok, sizeof(key) - 1);
        key[sizeof(key) - 1] = 0;
        nv = 0;
        while (lex(&l) && l.tok[0] != ';' && l.tok[0] != '}') {
            if (nv < 16)
                strcpy(vals[nv++], l.tok);
        }
        if (l.tok[0] == '}') {
            depth = 0;
            block[0] = name[0] = 0;
        }
        if (obj) {
            if (strcmp(key, "file") == 0 && nv >= 1)
                strncpy(obj->file, vals[0], sizeof(obj->file) - 1);
            else if (strcmp(key, "pos") == 0 && nv >= 6) {
                int k;
                for (k = 0; k < 6; k++)
                    obj->pos[k] = (float)atof(vals[k]);
            } else if (strcmp(key, "isclass") == 0 && nv >= 1)
                obj->cls = (unsigned int)strtoul(vals[0], NULL, 0);
            else if (strcmp(key, "appmap") == 0 && nv >= 6 && obj->napp < 8) {
                vgAppmap *a = &obj->app[obj->napp++];
                strncpy(a->sw, vals[0], sizeof(a->sw) - 1);
                strncpy(a->child, vals[2], sizeof(a->child) - 1);
                a->mask = (unsigned int)strtoul(vals[4], NULL, 0);
                a->val = (unsigned int)strtoul(vals[5], NULL, 0);
            }
        } else if (scene) {
            if (strcmp(key, "object") == 0 && nv >= 1) {
                vgObject *o = vgFindObj(vals[0]);
                if (o)
                    vgAddSceneObj(scene, o);
            }
        } else if (in_env) {
            if (strcmp(key, "skycolor") == 0 && nv >= 3) {
                env.skycolor[0] = (float)atof(vals[0]);
                env.skycolor[1] = (float)atof(vals[1]);
                env.skycolor[2] = (float)atof(vals[2]);
            } else if (strcmp(key, "visrng") == 0 && nv >= 2) {
                env.visnear = (float)atof(vals[0]);
                env.visfar = (float)atof(vals[1]);
            }
        }
        if (obj && depth == 0)
            obj = NULL;
    }
    free(text);
    for (obj = objects; obj; obj = obj->next)
        vgObjPosVec(obj, obj->pos);
    return 0;
}

/* --- objects ----------------------------------------------------------------- */

vgObject *vgFindObj(const char *name)
{
    vgObject *o;
    for (o = objects; o; o = o->next)
        if (strcmp(o->name, name) == 0)
            return o;
    return NULL;
}

pfNode *vgObjModel(vgObject *o)
{
    char path[2048];
    if (!o)
        return NULL;
    if (!o->model && o->file[0]) {
        if (vgFindFile(o->file, path, sizeof(path)))
            o->model = fstLoadFile(path);
        if (!o->model) {
            fprintf(stderr, "vg: cannot load %s for object %s\n", o->file, o->name);
            o->model = pfNewGroup();
        }
        vgDISAppearance(o, 0, 0);
    }
    return o->model;
}

vgObject *vgCopyObj(vgObject *src)
{
    vgObject *o;
    char name[64];
    static int count;
    sprintf(name, "%.40s#%d", src->name, ++count);
    o = new_object(name);
    strcpy(o->file, src->file);
    memcpy(o->pos, src->pos, sizeof(o->pos));
    o->scale = src->scale;
    pfCopyMat(o->mat, src->mat);
    o->cls = src->cls;
    memcpy(o->app, src->app, sizeof(o->app));
    o->napp = src->napp;
    o->appearance = src->appearance;
    o->model = pfClone(vgObjModel(src), 0);
    return o;
}

static void free_isc(vgObject *o);

void vgDelObj(vgObject *o)
{
    vgObject **p;
    int i;
    for (p = &objects; *p; p = &(*p)->next)
        if (*p == o) {
            *p = o->next;
            break;
        }
    for (i = 0; i < nscenes; i++)
        vgRemSceneObj(&scenes[i], o);
    free_isc(o);
    /* the cloned nodes are leaked on purpose: geometry is shared */
    free(o);
}

static void update_mat(vgObject *o)
{
    pfCoord c;
    pfSetVec3(c.xyz, o->pos[0], o->pos[1], o->pos[2]);
    pfSetVec3(c.hpr, o->pos[3], o->pos[4], o->pos[5]);
    pfMakeCoordMat(o->mat, &c);
    if (o->scale != 1) {
        int i, j;
        for (i = 0; i < 3; i++)
            for (j = 0; j < 3; j++)
                o->mat[i][j] *= o->scale;
    }
}

void vgObjPos(vgObject *o, float x, float y, float z, float h, float p, float r)
{
    o->pos[0] = x; o->pos[1] = y; o->pos[2] = z;
    o->pos[3] = h; o->pos[4] = p; o->pos[5] = r;
    update_mat(o);
}

void vgObjPosVec(vgObject *o, const float *v)
{
    vgObjPos(o, v[0], v[1], v[2], v[3], v[4], v[5]);
}

void vgObjMat(vgObject *o, pfMatrix m)
{
    pfCopyMat(o->mat, m);
    o->pos[0] = m[3][0];
    o->pos[1] = m[3][1];
    o->pos[2] = m[3][2];
}

void vgObjScale(vgObject *o, float s)
{
    o->scale = s;
    update_mat(o);
}

void vgObjClass(vgObject *o, unsigned int cls) { o->cls = cls; }

typedef struct {
    const char *name;
    int want, count;
    pfNode *found;
    int index;
} FindState;

static void find_part(pfNode *n, FindState *fs, int depth)
{
    int i;
    if (!n || fs->found || depth > 64)
        return;
    if (n->name && n->name[0]) {
        if (fs->name ? strcmp(n->name, fs->name) == 0 : 1) {
            if (fs->count++ == fs->want) {
                fs->found = n;
                return;
            }
        }
    }
    for (i = 0; i < n->nchildren; i++)
        find_part(n->children[i], fs, depth + 1);
}

pfNode *vgFindPart(vgObject *o, const char *name, int occurence)
{
    FindState fs;
    fs.name = name;
    fs.want = occurence;
    fs.count = 0;
    fs.found = NULL;
    find_part(vgObjModel(o), &fs, 0);
    return fs.found;
}

int vgNumParts(vgObject *o)
{
    FindState fs;
    fs.name = NULL;
    fs.want = -1;
    fs.count = 0;
    fs.found = NULL;
    find_part(vgObjModel(o), &fs, 0);
    return fs.count;
}

pfNode *vgGetPart(vgObject *o, int i)
{
    return vgFindPart(o, NULL, i);
}

/* classes of parts */
static struct { pfNode *n; unsigned int cls; } *part_cls;
static int npart_cls, maxpart_cls, part_cls_gen;

void vgPartClass(pfNode *part, unsigned int cls)
{
    int i;
    if (!part)
        return;
    part_cls_gen++;
    for (i = 0; i < npart_cls; i++)
        if (part_cls[i].n == part) {
            part_cls[i].cls = cls;
            return;
        }
    if (npart_cls == maxpart_cls) {
        maxpart_cls = maxpart_cls ? 2 * maxpart_cls : 256;
        part_cls = realloc(part_cls, sizeof(*part_cls) * maxpart_cls);
    }
    part_cls[npart_cls].n = part;
    part_cls[npart_cls].cls = cls;
    npart_cls++;
}

static int find_part_cls(pfNode *part, unsigned int *cls)
{
    int i;
    for (i = 0; i < npart_cls; i++)
        if (part_cls[i].n == part) {
            *cls = part_cls[i].cls;
            return 1;
        }
    return 0;
}

unsigned int vgGetPartClass(pfNode *part)
{
    unsigned int c = 0;
    find_part_cls(part, &c);
    return c;
}

void vgPartPos(pfNode *part, float x, float y, float z, float h, float p, float r)
{
    if (!part || part->type != PFNODE_DCS)
        return;
    pfSetVec3(part->rot, h, p, r);
    pfDCSTrans(part, x, y, z);
}

static void apply_appmap(pfNode *n, const vgAppmap *a, int depth)
{
    int i;
    if (!n || depth > 64)
        return;
    if (n->type == PFNODE_SWITCH && n->name && strcmp(n->name, a->sw) == 0) {
        for (i = 0; i < n->nchildren; i++)
            if (n->children[i]->name && strcmp(n->children[i]->name, a->child) == 0)
                pfSwitchVal(n, i);
    }
    for (i = 0; i < n->nchildren; i++)
        apply_appmap(n->children[i], a, depth + 1);
}

void vgDISAppearance(vgObject *o, unsigned int mask, unsigned int val)
{
    int i;
    o->appearance = (o->appearance & ~mask) | (val & mask);
    if (!o->model)
        return;
    for (i = 0; i < o->napp; i++)
        if ((o->appearance & o->app[i].mask) == o->app[i].val)
            apply_appmap(o->model, &o->app[i], 0);
}

static void find_seq(pfNode *n, FindState *fs, int depth)
{
    int i;
    if (!n || fs->found || depth > 64)
        return;
    if (n->type == PFNODE_SEQUENCE && fs->count++ == fs->want) {
        fs->found = n;
        return;
    }
    for (i = 0; i < n->nchildren; i++)
        find_seq(n->children[i], fs, depth + 1);
}

pfNode *vgFindSeq(pfNode *n, int occurence)
{
    FindState fs;
    fs.want = occurence;
    fs.count = 0;
    fs.found = NULL;
    find_seq(n, &fs, 0);
    return fs.found;
}

void vgSeqFrameTime(pfNode *seq, float t)
{
    int i;
    if (!seq)
        return;
    for (i = 0; i < seq->nframes; i++)
        seq->times[i] = t;
}

/* --- scenes ------------------------------------------------------------------ */

vgScene *vgFindScene(const char *name)
{
    int i;
    for (i = 0; i < nscenes; i++)
        if (strcmp(scenes[i].name, name) == 0)
            return &scenes[i];
    return new_scene(name);
}

int vgSceneHasObj(vgScene *s, vgObject *o)
{
    int i;
    for (i = 0; i < s->nobjs; i++)
        if (s->objs[i] == o)
            return 1;
    return 0;
}

void vgAddSceneObj(vgScene *s, vgObject *o)
{
    if (!o || vgSceneHasObj(s, o))
        return;
    if (s->nobjs == s->maxobjs) {
        s->maxobjs = s->maxobjs ? 2 * s->maxobjs : 32;
        s->objs = (vgObject **)realloc(s->objs, sizeof(vgObject *) * s->maxobjs);
    }
    s->objs[s->nobjs++] = o;
}

void vgRemSceneObj(vgScene *s, vgObject *o)
{
    int i;
    for (i = 0; i < s->nobjs; i++)
        if (s->objs[i] == o) {
            memmove(s->objs + i, s->objs + i + 1, sizeof(vgObject *) * (s->nobjs - i - 1));
            s->nobjs--;
            return;
        }
}

void vgClearScene(vgScene *s) { s->nobjs = 0; }

void vgDrawScene(vgScene *s, double t)
{
    int i;
    pfFrameTime(t);
    for (i = 0; i < s->nobjs; i++) {
        vgObject *o = s->objs[i];
        if (!o->enabled)
            continue;
        glPushMatrix();
        glMultMatrixf(&o->mat[0][0]);
        pfDrawScene(vgObjModel(o), t);
        glPopMatrix();
    }
}

/* --- intersections ------------------------------------------------------------ */

#define MAXSW 8
#define MAXPATH 16

typedef struct {
    pfGeoSet *g;
    pfMatrix m;              /* geoset to object space */
    float c[3], r;           /* bounding sphere in object space */
    float lo[2], hi[2];
    unsigned int partcls;    /* 0: the class of the object */
    int has_partcls;
    int nsw;
    pfNode *sw[MAXSW];       /* switches and sequences above the geoset */
    int swc[MAXSW];
    int pathlen;
    pfNode *path[MAXPATH];
    int stamp;
} IscGset;

typedef struct vgIsc {
    IscGset *gs;
    int n, max, gen;
    float c[3], r;
    /* grid over x, y for large objects */
    int gx, gy;
    float x0, y0, cell;
    int *start, *items;
    int stamp;
} vgIsc;

typedef struct {
    vgIsc *isc;
    int nsw;
    pfNode *sw[MAXSW];
    int swc[MAXSW];
    int depth;
    pfNode *path[MAXPATH];
    unsigned int cls;
    int has_cls;
} BuildState;

static void free_isc(vgObject *o)
{
    if (!o->isc)
        return;
    free(o->isc->gs);
    free(o->isc->start);
    free(o->isc->items);
    free(o->isc);
    o->isc = NULL;
}

static void build_gset(BuildState *bs, pfGeoSet *g, pfMatrix m)
{
    vgIsc *isc = bs->isc;
    IscGset *e;
    const float *co = (const float *)g->attr[PFGS_COORD3];
    const int *tris;
    int nt, i, k;
    float lo[3] = { 1e30f, 1e30f, 1e30f }, hi[3] = { -1e30f, -1e30f, -1e30f }, r2 = 0;
    if (!co || g->prim < PFGS_TRIS)
        return;
    nt = pfGetGSetTris(g, &tris);
    if (nt <= 0)
        return;
    if (isc->n == isc->max) {
        isc->max = isc->max ? 2 * isc->max : 64;
        isc->gs = (IscGset *)realloc(isc->gs, sizeof(IscGset) * isc->max);
    }
    e = &isc->gs[isc->n++];
    memset(e, 0, sizeof(*e));
    e->g = g;
    pfCopyMat(e->m, m);
    for (i = 0; i < 3 * nt; i++) {
        pfVec3 p;
        pfXformPt3(p, co + 3 * tris[i], m);
        for (k = 0; k < 3; k++) {
            if (p[k] < lo[k]) lo[k] = p[k];
            if (p[k] > hi[k]) hi[k] = p[k];
        }
    }
    for (k = 0; k < 3; k++)
        e->c[k] = 0.5f * (lo[k] + hi[k]);
    for (i = 0; i < 3 * nt; i++) {
        pfVec3 p;
        float d;
        pfXformPt3(p, co + 3 * tris[i], m);
        d = (p[0] - e->c[0]) * (p[0] - e->c[0]) + (p[1] - e->c[1]) * (p[1] - e->c[1]) +
            (p[2] - e->c[2]) * (p[2] - e->c[2]);
        if (d > r2)
            r2 = d;
    }
    e->r = sqrtf(r2) + 0.01f;
    e->lo[0] = lo[0]; e->lo[1] = lo[1];
    e->hi[0] = hi[0]; e->hi[1] = hi[1];
    e->partcls = bs->cls;
    e->has_partcls = bs->has_cls;
    e->nsw = bs->nsw;
    memcpy(e->sw, bs->sw, sizeof(e->sw));
    memcpy(e->swc, bs->swc, sizeof(e->swc));
    e->pathlen = bs->depth < MAXPATH ? bs->depth : MAXPATH;
    memcpy(e->path, bs->path, sizeof(pfNode *) * e->pathlen);
}

static void build_node(BuildState *bs, pfNode *n, pfMatrix m, int depth)
{
    pfMatrix mm;
    unsigned int save_cls = bs->cls, c;
    int save_has = bs->has_cls, save_nsw = bs->nsw, i;
    if (!n || depth > 64)
        return;
    if (depth < MAXPATH)
        bs->path[depth] = n;
    bs->depth = depth + 1;
    if (find_part_cls(n, &c)) {
        bs->cls = c;
        bs->has_cls = 1;
    }
    if (n->type == PFNODE_DCS || n->type == PFNODE_SCS) {
        pfMultMat(mm, n->mat, m);
        m = mm;
    }
    for (i = 0; i < n->ngsets; i++)
        build_gset(bs, n->gsets[i], m);
    if (n->type == PFNODE_LOD) {
        if (n->nchildren)
            build_node(bs, n->children[0], m, depth + 1);
    } else {
        int sw = (n->type == PFNODE_SWITCH || n->type == PFNODE_SEQUENCE) && bs->nsw < MAXSW;
        for (i = 0; i < n->nchildren; i++) {
            if (sw) {
                bs->sw[bs->nsw] = n;
                bs->swc[bs->nsw] = i;
                bs->nsw = save_nsw + 1;
            }
            build_node(bs, n->children[i], m, depth + 1);
            bs->nsw = save_nsw;
        }
    }
    bs->nsw = save_nsw;
    bs->cls = save_cls;
    bs->has_cls = save_has;
    bs->depth = depth;
}

static vgIsc *get_isc(vgObject *o)
{
    BuildState bs;
    pfMatrix ident;
    vgIsc *isc;
    int i, k;
    float lo[3] = { 1e30f, 1e30f, 1e30f }, hi[3] = { -1e30f, -1e30f, -1e30f }, r = 0;
    if (o->isc && o->isc->gen == part_cls_gen)
        return o->isc;
    free_isc(o);
    isc = o->isc = (vgIsc *)calloc(1, sizeof(vgIsc));
    isc->gen = part_cls_gen;
    memset(&bs, 0, sizeof(bs));
    bs.isc = isc;
    pfMakeIdentMat(ident);
    build_node(&bs, vgObjModel(o), ident, 0);
    for (i = 0; i < isc->n; i++)
        for (k = 0; k < 3; k++) {
            if (isc->gs[i].c[k] - isc->gs[i].r < lo[k]) lo[k] = isc->gs[i].c[k] - isc->gs[i].r;
            if (isc->gs[i].c[k] + isc->gs[i].r > hi[k]) hi[k] = isc->gs[i].c[k] + isc->gs[i].r;
        }
    for (k = 0; k < 3; k++)
        isc->c[k] = 0.5f * (lo[k] + hi[k]);
    for (i = 0; i < isc->n; i++) {
        float d = pfDistancePt3(isc->c, isc->gs[i].c) + isc->gs[i].r;
        if (d > r)
            r = d;
    }
    isc->r = r;
    if (isc->n > 64) {
        /* grid over the x, y extent */
        int cx, cy, total = 0, *count;
        float w = hi[0] - lo[0], h = hi[1] - lo[1];
        isc->cell = (w > h ? w : h) / 64;
        if (isc->cell < 1)
            isc->cell = 1;
        isc->x0 = lo[0];
        isc->y0 = lo[1];
        isc->gx = (int)(w / isc->cell) + 1;
        isc->gy = (int)(h / isc->cell) + 1;
        count = (int *)calloc(isc->gx * isc->gy + 1, sizeof(int));
        isc->start = (int *)calloc(isc->gx * isc->gy + 1, sizeof(int));
        for (k = 0; k < 2; k++) {
            for (i = 0; i < isc->n; i++) {
                IscGset *e = &isc->gs[i];
                int x0 = (int)((e->lo[0] - isc->x0) / isc->cell), x1 = (int)((e->hi[0] - isc->x0) / isc->cell);
                int y0 = (int)((e->lo[1] - isc->y0) / isc->cell), y1 = (int)((e->hi[1] - isc->y0) / isc->cell);
                if (x0 < 0) x0 = 0;
                if (y0 < 0) y0 = 0;
                if (x1 >= isc->gx) x1 = isc->gx - 1;
                if (y1 >= isc->gy) y1 = isc->gy - 1;
                for (cy = y0; cy <= y1; cy++)
                    for (cx = x0; cx <= x1; cx++) {
                        int c = cy * isc->gx + cx;
                        if (k == 0)
                            count[c]++;
                        else
                            isc->items[isc->start[c] + count[c]++] = i;
                    }
            }
            if (k == 0) {
                for (i = 0; i < isc->gx * isc->gy; i++) {
                    isc->start[i] = total;
                    total += count[i];
                    count[i] = 0;
                }
                isc->start[isc->gx * isc->gy] = total;
                isc->items = (int *)malloc(sizeof(int) * (total + 1));
            }
        }
        free(count);
    }
    return isc;
}

static int seg_sphere(const pfVec3 p, const pfVec3 d, float len, const float *c, float r)
{
    pfVec3 v;
    float t, dist2;
    pfSubVec3(v, c, p);
    t = pfDotVec3(v, d);
    if (t < 0) t = 0;
    if (t > len) t = len;
    v[0] -= t * d[0];
    v[1] -= t * d[1];
    v[2] -= t * d[2];
    dist2 = pfDotVec3(v, v);
    return dist2 <= r * r;
}

static int gset_active(const IscGset *e)
{
    int i;
    for (i = 0; i < e->nsw; i++) {
        pfNode *s = e->sw[i];
        if (s->type == PFNODE_SWITCH) {
            if (s->switchval != PFSWITCH_ON && s->switchval != e->swc[i])
                return 0;
        } else if (s->frame != e->swc[i]) {
            return 0;
        }
    }
    return 1;
}

/* segment (object space) against one geoset; updates the best distance */
static int isect_entry(IscGset *e, const pfVec3 p, const pfVec3 d, float len, float *best, pfVec3 nbest)
{
    const float *co = (const float *)e->g->attr[PFGS_COORD3];
    const int *tris;
    int nt = pfGetGSetTris(e->g, &tris), t, found = 0;
    for (t = 0; t < nt; t++) {
        pfVec3 a, b, c, e1, e2, pv, tv, qv;
        float det, u, v, dist;
        pfXformPt3(a, co + 3 * tris[3 * t], e->m);
        pfXformPt3(b, co + 3 * tris[3 * t + 1], e->m);
        pfXformPt3(c, co + 3 * tris[3 * t + 2], e->m);
        pfSubVec3(e1, b, a);
        pfSubVec3(e2, c, a);
        pfCrossVec3(pv, d, e2);
        det = pfDotVec3(e1, pv);
        if (det > -1e-12f && det < 1e-12f)
            continue;
        pfSubVec3(tv, p, a);
        u = pfDotVec3(tv, pv) / det;
        if (u < 0 || u > 1)
            continue;
        pfCrossVec3(qv, tv, e1);
        v = pfDotVec3(d, qv) / det;
        if (v < 0 || u + v > 1)
            continue;
        dist = pfDotVec3(e2, qv) / det;
        if (dist < 0 || dist > len || dist >= *best)
            continue;
        *best = dist;
        pfCrossVec3(nbest, e1, e2);
        found = 1;
    }
    return found;
}

static void test_entry(vgObject *o, IscGset *e, unsigned int mask, const pfVec3 p, const pfVec3 d,
                       float len, float *best, vgHit *h, pfMatrix om)
{
    pfVec3 n;
    unsigned int cls = e->has_partcls ? e->partcls : o->cls;
    if (!(cls & mask) || !gset_active(e))
        return;
    if (!seg_sphere(p, d, len, e->c, e->r))
        return;
    if (isect_entry(e, p, d, len, best, n)) {
        int i;
        h->hit = 1;
        h->obj = o;
        h->cls = cls;
        h->part = NULL;
        for (i = e->pathlen - 1; i >= 0; i--)
            if (e->path[i]->name && e->path[i]->name[0]) {
                h->part = e->path[i];
                break;
            }
        h->pathlen = e->pathlen;
        memcpy(h->path, e->path, sizeof(pfNode *) * e->pathlen);
        pfXformVec3(h->norm, n, om);
        pfNormalizeVec3(h->norm);
    }
}

int vgIsectSeg(vgScene *s, unsigned int mask, const pfVec3 pos, const pfVec3 dir, float len, vgHit *h)
{
    int i, j;
    float bestw = 1e30f;
    vgHit tmp;
    memset(h, 0, sizeof(*h));
    for (i = 0; i < s->nobjs; i++) {
        vgObject *o = s->objs[i];
        vgIsc *isc;
        pfMatrix inv;
        pfVec3 p, d, wp;
        float scale, l, best;
        if (!o->enabled || !o->model)
            continue;
        isc = get_isc(o);
        if (!isc->n)
            continue;
        /* the segment in object space */
        pfInvertAffMat(inv, o->mat);
        pfXformPt3(p, pos, inv);
        pfXformVec3(d, dir, inv);
        scale = pfNormalizeVec3(d);
        if (scale <= 0)
            continue;
        l = len * scale;
        if (!seg_sphere(p, d, l, isc->c, isc->r))
            continue;
        best = bestw < 1e29f ? bestw * scale : l;
        memset(&tmp, 0, sizeof(tmp));
        if (isc->items) {
            float x0 = p[0] < p[0] + d[0] * l ? p[0] : p[0] + d[0] * l;
            float x1 = p[0] > p[0] + d[0] * l ? p[0] : p[0] + d[0] * l;
            float y0 = p[1] < p[1] + d[1] * l ? p[1] : p[1] + d[1] * l;
            float y1 = p[1] > p[1] + d[1] * l ? p[1] : p[1] + d[1] * l;
            int cx0 = (int)floorf((x0 - isc->x0) / isc->cell), cx1 = (int)floorf((x1 - isc->x0) / isc->cell);
            int cy0 = (int)floorf((y0 - isc->y0) / isc->cell), cy1 = (int)floorf((y1 - isc->y0) / isc->cell);
            int cx, cy;
            if (cx0 < 0) cx0 = 0;
            if (cy0 < 0) cy0 = 0;
            if (cx1 >= isc->gx) cx1 = isc->gx - 1;
            if (cy1 >= isc->gy) cy1 = isc->gy - 1;
            isc->stamp++;
            for (cy = cy0; cy <= cy1; cy++)
                for (cx = cx0; cx <= cx1; cx++) {
                    int c = cy * isc->gx + cx;
                    for (j = isc->start[c]; j < isc->start[c + 1]; j++) {
                        IscGset *e = &isc->gs[isc->items[j]];
                        if (e->stamp == isc->stamp)
                            continue;
                        e->stamp = isc->stamp;
                        test_entry(o, e, mask, p, d, l, &best, &tmp, o->mat);
                    }
                }
        } else {
            for (j = 0; j < isc->n; j++)
                test_entry(o, &isc->gs[j], mask, p, d, l, &best, &tmp, o->mat);
        }
        if (tmp.hit) {
            float w = best / scale;
            if (w < bestw) {
                bestw = w;
                *h = tmp;
                h->dist = w;
                wp[0] = pos[0] + dir[0] * w;
                wp[1] = pos[1] + dir[1] * w;
                wp[2] = pos[2] + dir[2] * w;
                pfCopyVec3(h->point, wp);
            }
        }
    }
    return h->hit;
}

int vgIsectZ(vgScene *s, unsigned int mask, float x, float y, float h, float *z, float *p, float *r, vgHit *hit)
{
    vgHit tmp;
    pfVec3 pos, dir;
    if (!hit)
        hit = &tmp;
    pfSetVec3(pos, x, y, 50000);
    pfSetVec3(dir, 0, 0, -1);
    if (!vgIsectSeg(s, mask, pos, dir, 100000, hit))
        return 0;
    if (z)
        *z = hit->point[2];
    if (p || r) {
        float sh, ch, nf, nr, nz = hit->norm[2];
        pfSinCos(h, &sh, &ch);
        if (nz < 0) {
            pfScaleVec3(hit->norm, -1, hit->norm);
            nz = -nz;
        }
        nf = -sh * hit->norm[0] + ch * hit->norm[1];
        nr = ch * hit->norm[0] + sh * hit->norm[1];
        if (p)
            *p = (float)PF_RAD2DEG(atan2(-nf, nz));
        if (r)
            *r = (float)PF_RAD2DEG(atan2(nr, nz));
    }
    return 1;
}

/* --- paths ---------------------------------------------------------------------- */

vgPath *vgNewPath(void)
{
    vgPath *p = (vgPath *)calloc(1, sizeof(vgPath));
    p->speed = 1;
    p->pscale = 1;
    p->rscale = 0;
    return p;
}

vgPath *vgSharePath(vgPath *src)
{
    vgPath *p = vgNewPath();
    p->first = src->first;
    return p;
}

static vgSegment *last_seg(vgPath *p)
{
    vgSegment *s = p->first;
    if (!s)
        return NULL;
    while (s->next && s->next != p->first)
        s = s->next;
    return s;
}

static vgSegment *add_seg(vgPath *p, int type)
{
    vgSegment *s = (vgSegment *)calloc(1, sizeof(vgSegment)), *l = last_seg(p);
    s->type = type;
    if (!l) {
        p->first = s;
    } else {
        s->prev = l;
        l->next = s;
    }
    return s;
}

static void line_hpr(vgSegment *s)
{
    pfVec3 d;
    pfSubVec3(d, s->end, s->start);
    s->length = pfLengthVec3(d);
    if (s->length > 0) {
        s->hpr[0] = (float)-PF_RAD2DEG(atan2(d[0], d[1]));
        s->hpr[1] = (float)PF_RAD2DEG(atan2(d[2], sqrt(d[0] * d[0] + d[1] * d[1])));
    }
    s->hpr[2] = 0;
}

/* turns the fillet f between the lines a and b into an arc */
static void make_fillet(vgSegment *a, vgSegment *f, vgSegment *b)
{
    pfVec3 d1, d2, c, bis, t1, t2;
    float th, t, r = f->radius, dist;
    pfCopyVec3(c, a->end);
    pfSubVec3(d1, a->end, a->start);
    pfSubVec3(d2, b->end, b->start);
    if (pfNormalizeVec3(d1) <= 0 || pfNormalizeVec3(d2) <= 0)
        return;
    th = (float)acos(PF_MAX(-1.0f, PF_MIN(1.0f, pfDotVec3(d1, d2))));
    f->type = 2;
    if (th < 1e-4f || r <= 0) {
        f->length = 0;
        pfCopyVec3(f->center, c);
        f->radius = 0;
        return;
    }
    t = r * tanf(th / 2);
    if (t > 0.999f * a->length) t = 0.999f * a->length;
    if (t > 0.999f * b->length) t = 0.999f * b->length;
    r = t / tanf(th / 2);
    t1[0] = c[0] - d1[0] * t; t1[1] = c[1] - d1[1] * t; t1[2] = c[2] - d1[2] * t;
    t2[0] = c[0] + d2[0] * t; t2[1] = c[1] + d2[1] * t; t2[2] = c[2] + d2[2] * t;
    pfSubVec3(bis, d2, d1);
    pfNormalizeVec3(bis);
    dist = r / cosf(th / 2);
    f->center[0] = c[0] + bis[0] * dist;
    f->center[1] = c[1] + bis[1] * dist;
    f->center[2] = c[2] + bis[2] * dist;
    pfSubVec3(f->u, t1, f->center);
    pfNormalizeVec3(f->u);
    pfCopyVec3(f->v, d1);
    f->radius = r;
    f->sweep = th;
    f->length = r * th;
    pfCopyVec3(a->end, t1);
    pfCopyVec3(b->start, t2);
    a->length = pfDistancePt3(a->start, a->end);
    b->length = pfDistancePt3(b->start, b->end);
}

void vgAddLine(vgPath *p, const pfVec3 a, const pfVec3 b)
{
    vgSegment *s = add_seg(p, 1);
    pfCopyVec3(s->start, a);
    pfCopyVec3(s->end, b);
    line_hpr(s);
    if (s->prev && s->prev->type == 3 && s->prev->prev && s->prev->prev->type == 1) {
        vgSegment *l = s->prev->prev;
        if (pfDistancePt3(l->end, s->start) < 1e-3f)
            make_fillet(l, s->prev, s);
    }
}

void vgAddFillet(vgPath *p, float radius)
{
    vgSegment *s = add_seg(p, 3);
    s->radius = radius;
}

void vgAddSpeed(vgPath *p, float speed, float accel)
{
    vgSegment *s = add_seg(p, 4);
    s->speed = speed;
    s->accel = accel;
}

void vgAddDelay(vgPath *p, float delay)
{
    vgSegment *s = add_seg(p, 5);
    s->delay = delay;
}

void vgClosePath(vgPath *p)
{
    vgSegment *l = last_seg(p);
    if (!l)
        return;
    p->first->prev = l;
    l->next = p->first;
}

int vgAddPathFile(vgPath *p, const char *file)
{
    char path[2048], line[256], word[64];
    FILE *f;
    if (!vgFindFile(file, path, sizeof(path))) {
        fprintf(stderr, "ERROR: unable to find file \"%s\"\n", file);
        return -1;
    }
    if (!(f = fopen(path, "r"))) {
        fprintf(stderr, "ERROR: error opening file \"%s\"\n", file);
        return -2;
    }
    while (fgets(line, sizeof(line), f)) {
        char *c = strchr(line, '#');
        pfVec3 a, b;
        float x, y;
        if (c)
            *c = 0;
        if (sscanf(line, "%63s", word) != 1)
            continue;
        if (strcmp(word, "line") == 0) {
            if (sscanf(line, "%*s %f %f %f %f %f %f", &a[0], &a[1], &a[2], &b[0], &b[1], &b[2]) == 6)
                vgAddLine(p, a, b);
        } else if (strcmp(word, "fillet") == 0) {
            if (sscanf(line, "%*s %f", &x) == 1)
                vgAddFillet(p, x);
        } else if (strcmp(word, "speed") == 0) {
            if (sscanf(line, "%*s %f %f", &x, &y) == 2)
                vgAddSpeed(p, x, y);
        } else if (strcmp(word, "delay") == 0) {
            if (sscanf(line, "%*s %f", &x) == 1)
                vgAddDelay(p, x);
        } else if (strcmp(word, "close") == 0) {
            vgClosePath(p);
        } else {
            fprintf(stderr, "ERROR: unknown keyword in segment file (%s)\n", word);
        }
    }
    fclose(f);
    return 0;
}

static void arc_point(const vgSegment *s, float u, pfVec3 out)
{
    float a = u * s->sweep, ca = cosf(a), sa = sinf(a);
    int k;
    for (k = 0; k < 3; k++)
        out[k] = s->center[k] + s->radius * (ca * s->u[k] + sa * s->v[k]);
}

static void seg_event(vgPath *p, vgSegment *s, double now, float *step)
{
    if (s->type == 4) {
        p->target = s->speed;
        p->accel = s->accel;
    } else if (s->type == 5) {
        p->delay_end = s->delay + now;
        *step = 0;
    }
}

float vgFollowPath(vgPath *p, float dt, double now, pfVec3 pos, pfVec3 hpr)
{
    vgSegment *s;
    float step = 0, u, dir = 1;
    if (!p->cur)
        p->cur = p->first;
    if (!p->cur)
        return 0;
    if (p->delay_end != 0) {
        if (p->delay_end <= now)
            p->delay_end = 0;
    } else {
        p->speed += p->accel;
        if ((p->accel > 0 && p->target <= p->speed) || (p->accel < 0 && p->speed <= p->target)) {
            p->speed = p->target;
            p->accel = 0;
        }
        step = p->speed * dt;
        p->total += step;
    }
    if (step > 0) {
        while (step > 0 && p->cur->length - p->segdist <= step) {
            s = p->cur;
            step -= s->length - p->segdist;
            seg_event(p, s, now, &step);
            if (s->next) {
                p->cur = s->next;
                p->segdist = 0;
            } else {
                p->speed = 0;
                p->segdist = s->length;
                step = 0;
            }
        }
        p->segdist += step;
    } else if (step < 0) {
        dir = -1;
        step = -step;
        while (step > 0 && p->segdist <= step) {
            s = p->cur;
            step -= p->segdist;
            seg_event(p, s, now, &step);
            if (s->prev) {
                p->cur = s->prev;
                p->segdist = p->cur->length;
            } else {
                p->speed = 0;
                p->segdist = 0;
                step = 0;
            }
        }
        p->segdist -= step;
    }
    s = p->cur;
    u = s->length > 0 ? p->segdist / s->length : 0;
    if (s->type == 1) {
        int k;
        for (k = 0; k < 3; k++)
            pos[k] = s->end[k] * u + (1 - u) * s->start[k];
        pfCopyVec3(hpr, s->hpr);
    } else if (s->type == 2) {
        pfVec3 nx, d;
        if (s->radius > 0) {
            arc_point(s, u, pos);
            arc_point(s, u + 0.05f, nx);
            pfSubVec3(d, nx, pos);
            hpr[0] = (float)-PF_RAD2DEG(atan2(d[0], d[1]));
            hpr[1] = (float)PF_RAD2DEG(atan2(d[2], sqrt(d[0] * d[0] + d[1] * d[1])));
        } else {
            pfCopyVec3(pos, s->center);
        }
        hpr[2] = 0;
    } else if (s->type == 3) {
        fprintf(stderr, "ERROR: encountered fillet segment in path\n");
        return 0;
    }
    if ((s->type == 1 || s->type == 2) && dir < 0) {
        hpr[0] += 180;
        hpr[1] = -hpr[1];
    }
    pfCopyVec3(p->pos, pos);
    hpr[1] *= p->pscale;
    hpr[2] *= p->rscale;
    return p->total;
}

/* --- fonts ------------------------------------------------------------------------ */

static float font_w = 0.01f, font_h = 0.015f, font_x, font_y;

void vgFontSize(float w, float h)
{
    font_w = w;
    font_h = h;
}

void vgFontPos(float x, float y)
{
    font_x = x;
    font_y = y;
}

void vgDrawFont(const char *s)
{
    glPushMatrix();
    glTranslatef(font_x, font_y, 0);
    glScalef(font_w / 104.76f, font_h / 166.0f, 1);
    while (*s)
        glutStrokeCharacter(GLUT_STROKE_MONO_ROMAN, *s++);
    glPopMatrix();
}
