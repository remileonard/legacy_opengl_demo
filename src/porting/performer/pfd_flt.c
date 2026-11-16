/*
 * pfd_flt.c - an OpenFlight (.flt) loader for the Performer layer, the
 * pfdLoadFile_flt of libpfdu: the 14.2 format and the older format of
 * version 12 (integer coordinates, vertices given inside the faces).
 *
 * Read: the header (units), the color, material and texture palettes, the
 * vertex palette and the vertex lists (14.2), the old vertices (7, 8, 9),
 * the groups, objects, levels of detail, matrices, external references
 * (loaded once and shared), the faces with their subfaces (decals) and
 * the billboard faces. The transform records (78, 80...) only describe
 * the matrix, the other records are skipped.
 *
 * The faces are gathered into geosets of triangles by texture, material
 * and mode under each node. A face is lit when it has a material; its
 * color is then the face color times the diffuse color of the material.
 */
#include "pf_internal.h"

#include <math.h>

/* records */
enum {
    OP_HEADER = 1, OP_GROUP = 2, OP_OLD_LOD = 3, OP_OBJECT = 4, OP_FACE = 5,
    OP_OLD_VERTEX = 7, OP_OLD_VERTEX_COLOR = 8, OP_OLD_VERTEX_NORMAL = 9,
    OP_PUSH = 10, OP_POP = 11, OP_PUSH_SUBFACE = 19, OP_POP_SUBFACE = 20,
    OP_COLOR_PALETTE = 32, OP_MATRIX = 49, OP_EXTREF = 63, OP_TEXTURE = 64,
    OP_MATERIAL_TABLE = 66, OP_VERTEX_PALETTE = 67, OP_VERTEX_C = 68,
    OP_VERTEX_CN = 69, OP_VERTEX_CNT = 70, OP_VERTEX_CT = 71, OP_VERTEX_LIST = 72,
    OP_LOD = 73, OP_MATERIAL = 113
};

typedef struct {
    double xyz[3];
    float nrm[3], uv[2];
    float color[4];
    int hasnrm, hasuv, hascolor;
} FltVertex;

typedef struct {
    float amb[3], dif[3], spec[3], emis[3];
    float shininess, alpha;
} FltMaterial;

/* a face being read */
typedef struct {
    float color[4];
    int texture, material, drawtype, template_, lightmode;
    int subface;
    int n, max;
    FltVertex *v;
} FltFace;

/* the geosets made under a node */
typedef struct {
    int texture, material, drawtype, transp, decal, alphatex;
    int nv, max;
    float *co, *no, *tc, *cl;
} FltBatch;

typedef struct {
    pfNode *node;
    FltBatch *batches;
    int nbatches;
    pfNode *bboards;            /* billboard faces, one geoset each */
} FltBuilder;

typedef struct {
    const char *path;
    char dir[1024];
    int version;
    double unit;                /* meters per unit of the old integer coordinates */
    float palette[1024][4];
    int npalette, oldpalette;
    FltMaterial materials[256];
    int nmaterials;
    pfTexture *textures[1024];
    int alphatex[1024];
    /* the vertex palette: its records by offset */
    int *vindex;
    long vpal_len;
    FltVertex *verts;
    int nverts, maxverts;
    FltBuilder *builders;
    int nbuilders;
} FltFile;

static unsigned int be16(const unsigned char *p) { return (p[0] << 8) | p[1]; }
static int sbe16(const unsigned char *p) { return (short)be16(p); }
static unsigned long be32(const unsigned char *p)
{
    return ((unsigned long)p[0] << 24) | ((unsigned long)p[1] << 16) | ((unsigned long)p[2] << 8) | p[3];
}
static long sbe32(const unsigned char *p) { return (long)(int)be32(p); }

static float bef(const unsigned char *p)
{
    union { unsigned int i; float f; } u;
    u.i = (unsigned int)be32(p);
    return u.f;
}

static double bed(const unsigned char *p)
{
    union { unsigned char b[8]; double d; } u;
    int i, little = 1;
    little = *(unsigned char *)&little;
    for (i = 0; i < 8; i++)
        u.b[i] = little ? p[7 - i] : p[i];
    return u.d;
}

/* a color index: the color index / 128, at the intensity index % 128 */
static void index_color(FltFile *f, unsigned int c, float *rgba)
{
    int base, k;
    float in;
    if (f->oldpalette && c >= 4096) {
        base = 32 + (int)(c - 4096);
        in = 1;
    } else {
        base = (int)(c / 128);
        in = (c % 128) / 127.0f;
    }
    if (base >= f->npalette) {
        rgba[0] = rgba[1] = rgba[2] = rgba[3] = 1;
        return;
    }
    for (k = 0; k < 3; k++)
        rgba[k] = f->palette[base][k] * in;
    rgba[3] = 1;
}

static void packed_color(const unsigned char *p, float *rgba)
{
    /* alpha, blue, green, red */
    rgba[0] = p[3] / 255.0f;
    rgba[1] = p[2] / 255.0f;
    rgba[2] = p[1] / 255.0f;
    rgba[3] = 1;
}

static int add_vertex(FltFile *f, const FltVertex *v)
{
    if (f->nverts == f->maxverts) {
        f->maxverts = f->maxverts ? 2 * f->maxverts : 1024;
        f->verts = (FltVertex *)realloc(f->verts, sizeof(FltVertex) * f->maxverts);
    }
    f->verts[f->nverts] = *v;
    return f->nverts++;
}

static void face_vertex(FltFace *fc, const FltVertex *v)
{
    if (fc->n == fc->max) {
        fc->max = fc->max ? 2 * fc->max : 16;
        fc->v = (FltVertex *)realloc(fc->v, sizeof(FltVertex) * fc->max);
    }
    fc->v[fc->n++] = *v;
}

/* the vertex records of the 14.2 palette */
static void palette_vertex(FltFile *f, int op, const unsigned char *r, int len, int flags_color)
{
    FltVertex v;
    int k, flags = sbe16(r + 6);
    memset(&v, 0, sizeof(v));
    for (k = 0; k < 3; k++)
        v.xyz[k] = bed(r + 8 + 8 * k);
    switch (op) {
    case OP_VERTEX_CN:
        for (k = 0; k < 3; k++) v.nrm[k] = bef(r + 32 + 4 * k);
        v.hasnrm = 1;
        break;
    case OP_VERTEX_CNT:
        for (k = 0; k < 3; k++) v.nrm[k] = bef(r + 32 + 4 * k);
        v.uv[0] = bef(r + 44);
        v.uv[1] = bef(r + 48);
        v.hasnrm = v.hasuv = 1;
        break;
    case OP_VERTEX_CT:
        v.uv[0] = bef(r + 32);
        v.uv[1] = bef(r + 36);
        v.hasuv = 1;
        break;
    }
    (void)len;
    (void)flags_color;
    /* bit 2: no color, bit 3: packed color (bits from the high one) */
    if (!(flags & 0x2000)) {
        int pc = op == OP_VERTEX_C ? 32 : op == OP_VERTEX_CN ? 44 : op == OP_VERTEX_CNT ? 52 : 40;
        if (flags & 0x1000)
            packed_color(r + pc, v.color);
        else
            index_color(f, be16(r + 4), v.color);
        v.hascolor = 1;
    }
    add_vertex(f, &v);
}

/* the old vertices: integer coordinates in units, normals in 2^30 */
static void old_vertex(FltFile *f, FltFace *fc, int op, const unsigned char *r, int len)
{
    FltVertex v;
    int k;
    memset(&v, 0, sizeof(v));
    for (k = 0; k < 3; k++)
        v.xyz[k] = sbe32(r + 4 + 4 * k) * f->unit;
    if (op == OP_OLD_VERTEX && len >= 24) {
        v.uv[0] = bef(r + 16);
        v.uv[1] = bef(r + 20);
        v.hasuv = 1;
    } else if (op == OP_OLD_VERTEX_NORMAL) {
        for (k = 0; k < 3; k++)
            v.nrm[k] = (float)(sbe32(r + 16 + 4 * k) / 1073741824.0);
        v.hasnrm = 1;
        if (len >= 40) {
            v.uv[0] = bef(r + 32);
            v.uv[1] = bef(r + 36);
            v.hasuv = 1;
        }
    } else if (op == OP_OLD_VERTEX_COLOR && len >= 28) {
        v.uv[0] = bef(r + 20);
        v.uv[1] = bef(r + 24);
        v.hasuv = 1;
    }
    face_vertex(fc, &v);
}

static FltBuilder *builder(FltFile *f, pfNode *node)
{
    int i;
    for (i = 0; i < f->nbuilders; i++)
        if (f->builders[i].node == node)
            return &f->builders[i];
    f->builders = (FltBuilder *)realloc(f->builders, sizeof(FltBuilder) * (f->nbuilders + 1));
    memset(&f->builders[f->nbuilders], 0, sizeof(FltBuilder));
    f->builders[f->nbuilders].node = node;
    return &f->builders[f->nbuilders++];
}

static void batch_vertex(FltBatch *b, const FltVertex *v, const float *nrm, const float *color,
                         const double *offset)
{
    int k;
    if (b->nv == b->max) {
        b->max = b->max ? 2 * b->max : 192;
        b->co = (float *)realloc(b->co, sizeof(float) * 3 * b->max);
        b->no = (float *)realloc(b->no, sizeof(float) * 3 * b->max);
        b->tc = (float *)realloc(b->tc, sizeof(float) * 2 * b->max);
        b->cl = (float *)realloc(b->cl, sizeof(float) * 4 * b->max);
    }
    for (k = 0; k < 3; k++) {
        b->co[3 * b->nv + k] = (float)(v->xyz[k] - (offset ? offset[k] : 0));
        b->no[3 * b->nv + k] = nrm[k];
    }
    b->tc[2 * b->nv] = v->uv[0];
    b->tc[2 * b->nv + 1] = v->uv[1];
    memcpy(b->cl + 4 * b->nv, color, sizeof(float) * 4);
    b->nv++;
}

/* the triangles of a face, added to the batch of its modes */
static void add_face(FltFile *f, FltBuilder *bu, FltFace *fc)
{
    FltBatch *b = NULL, local;
    float fn[3], col[4];
    double center[3];
    int i, k, transp, decal = fc->subface > 0, alphatex, billboard, lines, points;
    const FltMaterial *m = fc->material >= 0 && fc->material < f->nmaterials ? &f->materials[fc->material] : NULL;

    if (fc->n < (fc->drawtype >= 8 ? 1 : fc->drawtype >= 2 ? 2 : 3))
        return;
    points = fc->drawtype >= 8;
    lines = !points && (fc->drawtype == 2 || fc->drawtype == 3 || fc->n < 3);
    alphatex = fc->texture >= 0 && fc->texture < 1024 && f->alphatex[fc->texture];
    transp = fc->color[3] < 1 || fc->template_ == 1 || fc->template_ == 2 || fc->template_ == 4 ||
             (m && m->alpha < 1);
    billboard = fc->template_ == 2 || fc->template_ == 4;

    /* the normal of the face, Newell's method */
    fn[0] = fn[1] = fn[2] = 0;
    center[0] = center[1] = center[2] = 0;
    for (i = 0; i < fc->n; i++) {
        const double *a = fc->v[i].xyz, *c = fc->v[(i + 1) % fc->n].xyz;
        fn[0] += (float)((a[1] - c[1]) * (a[2] + c[2]));
        fn[1] += (float)((a[2] - c[2]) * (a[0] + c[0]));
        fn[2] += (float)((a[0] - c[0]) * (a[1] + c[1]));
        for (k = 0; k < 3; k++)
            center[k] += a[k] / fc->n;
    }
    pfNormalizeVec3(fn);

    if (billboard) {
        memset(&local, 0, sizeof(local));
        b = &local;
    } else {
        for (i = 0; i < bu->nbatches; i++) {
            FltBatch *c = &bu->batches[i];
            if (c->texture == fc->texture && c->material == (m ? fc->material : -1) &&
                c->drawtype == (points ? 8 : lines ? 2 : fc->drawtype) && c->transp == transp &&
                c->decal == decal && c->alphatex == alphatex) {
                b = c;
                break;
            }
        }
        if (!b) {
            bu->batches = (FltBatch *)realloc(bu->batches, sizeof(FltBatch) * (bu->nbatches + 1));
            b = &bu->batches[bu->nbatches++];
            memset(b, 0, sizeof(*b));
        }
    }
    b->texture = fc->texture;
    b->material = m ? fc->material : -1;
    b->drawtype = points ? 8 : lines ? 2 : fc->drawtype;
    b->transp = transp;
    b->decal = decal;
    b->alphatex = alphatex;

    if (lines) {
        /* the edges, as pairs of vertices (draw type 3: open) */
        for (i = 0; i < fc->n; i++) {
            if (fc->drawtype == 3 && i == fc->n - 1)
                break;
            batch_vertex(b, &fc->v[i], fn, fc->color, NULL);
            batch_vertex(b, &fc->v[(i + 1) % fc->n], fn, fc->color, NULL);
        }
    } else {
        /* the points, or a fan of triangles */
        int nt = points ? fc->n : 3 * (fc->n - 2), t;
        for (t = 0; t < nt; t++) {
            int vi = points ? t : t % 3 == 0 ? 0 : t / 3 + t % 3;
            const FltVertex *v = &fc->v[vi];
            const float *nrm = (fc->lightmode >= 2 || f->version < 13) && v->hasnrm ? v->nrm : fn;
            if ((fc->lightmode == 1 || fc->lightmode == 3) && v->hascolor)
                memcpy(col, v->color, sizeof(col));
            else
                memcpy(col, fc->color, sizeof(col));
            if (m) {
                for (k = 0; k < 3; k++)
                    col[k] *= m->dif[k];
                col[3] *= m->alpha;
            }
            batch_vertex(b, v, nrm, col, billboard ? center : NULL);
        }
    }

    if (billboard) {
        FltBatch *nb;
        pfVec3 *bb;
        if (!bu->bboards) {
            bu->bboards = pfNewGeode();
            bu->bboards->type = PFNODE_BBOARD;
        }
        bu->batches = (FltBatch *)realloc(bu->batches, sizeof(FltBatch) * (bu->nbatches + 1));
        nb = &bu->batches[bu->nbatches++];
        *nb = local;
        /* the billboard batches are marked by their geoset slot */
        nb->drawtype = 100 + bu->bboards->ngsets;
        bb = (pfVec3 *)realloc(bu->bboards->bbpos, sizeof(pfVec3) * (bu->bboards->ngsets + 1));
        bu->bboards->bbpos = bb;
        pfSetVec3(bb[bu->bboards->ngsets], (float)center[0], (float)center[1], (float)center[2]);
        /* the geoset is made at the end; the slot keeps the count */
        pfAddGSet(bu->bboards, NULL);
    }
}

static pfGeoState *batch_gstate(FltFile *f, const FltBatch *b)
{
    pfGeoState *gs = pfNewGState(NULL);
    const FltMaterial *m = b->material >= 0 ? &f->materials[b->material] : NULL;
    pfTexture *tex = b->texture >= 0 && b->texture < 1024 ? f->textures[b->texture] : NULL;
    gs->lighting = m != NULL;
    gs->texturing = tex != NULL;
    gs->texture = tex;
    gs->lpoint = 0;
    gs->transparency = b->transp;
    gs->cullface = b->drawtype == 0 && !b->decal ? GL_BACK : 0;
    if (b->alphatex) {
        gs->alphafunc = GL_GREATER;
        gs->alpharef = 0.5f;
    }
    if (m) {
        pfMaterial *mtl = pfNewMtl(NULL);
        memcpy(mtl->ambient, m->amb, sizeof(pfVec3));
        memcpy(mtl->diffuse, m->dif, sizeof(pfVec3));
        memcpy(mtl->specular, m->spec, sizeof(pfVec3));
        memcpy(mtl->emission, m->emis, sizeof(pfVec3));
        mtl->shininess = m->shininess;
        mtl->alpha = m->alpha;
        /* the vertex colors are the face colors times the diffuse color */
        mtl->colormode = 1;
        gs->frontmtl = mtl;
    }
    return gs;
}

static pfGeoSet *batch_gset(FltFile *f, FltBatch *b)
{
    pfGeoSet *g = pfNewGSet(NULL);
    int prim = b->drawtype == 8 ? PFGS_POINTS : b->drawtype == 2 || b->drawtype == 3 ? PFGS_LINES : PFGS_TRIS;
    int per = prim == PFGS_POINTS ? 1 : prim == PFGS_LINES ? 2 : 3;
    pfGSetPrimType(g, prim);
    pfGSetNumPrims(g, b->nv / per);
    pfGSetAttr(g, PFGS_COORD3, PFGS_PER_VERTEX, b->co, NULL);
    pfGSetAttr(g, PFGS_NORMAL3, PFGS_PER_VERTEX, b->no, NULL);
    pfGSetAttr(g, PFGS_COLOR4, PFGS_PER_VERTEX, b->cl, NULL);
    if (b->texture >= 0 && b->texture < 1024 && f->textures[b->texture])
        pfGSetAttr(g, PFGS_TEXCOORD2, PFGS_PER_VERTEX, b->tc, NULL);
    g->nverts = b->nv;
    if (prim == PFGS_POINTS)
        pfGSetPntSize(g, 3);
    pfGSetGState(g, batch_gstate(f, b));
    return g;
}

/* the geodes of the faces read under each node */
static void build_geometry(FltFile *f)
{
    int i, j;
    for (i = 0; i < f->nbuilders; i++) {
        FltBuilder *bu = &f->builders[i];
        pfGeode *base = NULL, *decals = NULL;
        for (j = 0; j < bu->nbatches; j++) {
            FltBatch *b = &bu->batches[j];
            if (b->nv == 0)
                continue;
            if (b->drawtype >= 100) {
                int k = b->drawtype - 100;
                b->drawtype = 0;
                bu->bboards->gsets[k] = batch_gset(f, b);
                continue;
            }
            if (b->decal) {
                if (!decals)
                    decals = pfNewGeode();
                pfAddGSet(decals, batch_gset(f, b));
            } else {
                if (!base)
                    base = pfNewGeode();
                pfAddGSet(base, batch_gset(f, b));
            }
        }
        if (decals) {
            /* the subfaces are drawn over their faces */
            pfLayer *layer = pfNewLayer();
            pfAddChild(layer, base ? base : pfNewGeode());
            pfAddChild(layer, decals);
            pfAddChild(bu->node, layer);
        } else if (base) {
            pfAddChild(bu->node, base);
        }
        if (bu->bboards)
            pfAddChild(bu->node, bu->bboards);
        free(bu->batches);
    }
}

/* --- the external references: each file is loaded once ---------------------- */

static struct { char path[1024]; pfNode *node; } refcache[256];
static int nrefcache;
static int load_depth;

static pfNode *load_flt(const char *file);

static pfNode *external_ref(FltFile *f, const char *name)
{
    char path[2048], found[4096];
    const char *base;
    int i;
    /* "file.flt<node>" names a node: the whole file is used */
    char nm[256];
    char *lt;
    strncpy(nm, name, sizeof(nm) - 1);
    nm[sizeof(nm) - 1] = '\0';
    if ((lt = strchr(nm, '<')) != NULL)
        *lt = '\0';
    sprintf(path, "%s%s", f->dir, nm);
    if (!pfFindFile(path, found, R_OK)) {
        base = strrchr(nm, '/');
        if (!pfFindFile(base ? base + 1 : nm, found, R_OK)) {
            pfNotify(PFNFY_WARN, PFNFY_RESOURCE, "pfdLoadFile_flt: %s not found", nm);
            return NULL;
        }
    }
    for (i = 0; i < nrefcache; i++)
        if (strcmp(refcache[i].path, found) == 0)
            return refcache[i].node;
    if (load_depth > 16)
        return NULL;
    load_depth++;
    {
        pfNode *n = load_flt(found);
        load_depth--;
        if (nrefcache < 256) {
            strncpy(refcache[nrefcache].path, found, sizeof(refcache[0].path) - 1);
            refcache[nrefcache++].node = n;
        }
        return n;
    }
}

static void load_texture(FltFile *f, const unsigned char *r, int len)
{
    char name[256], path[2048];
    int idx, namelen = f->version >= 13 ? 200 : 80;
    pfTexture *t;
    const char *ext;
    if (len < namelen + 8)
        return;
    memcpy(name, r + 4, namelen < 255 ? namelen : 255);
    name[namelen < 255 ? namelen : 255] = '\0';
    idx = (int)sbe32(r + 4 + namelen);
    if (idx < 0 || idx >= 1024)
        return;
    t = pfNewTex(NULL);
    sprintf(path, "%s%s", f->dir, name);
    if (!pfLoadTexFile(t, path)) {
        const char *base = strrchr(name, '/');
        if (!pfLoadTexFile(t, base ? base + 1 : name)) {
            free(t);
            return;
        }
    }
    f->textures[idx] = t;
    ext = strrchr(name, '.');
    f->alphatex[idx] = t->comps == 2 || t->comps == 4 ||
                       (ext && (strcmp(ext, ".rgba") == 0 || strcmp(ext, ".inta") == 0));
}

/* --- reading ------------------------------------------------------------------- */

typedef struct {
    pfNode *container;          /* NULL: the level of a face */
    FltFace face;               /* the face whose vertices follow */
} Level;

static pfNode *load_flt(const char *file)
{
    FltFile *f;
    FILE *fp;
    unsigned char *data;
    long size, pos, vpal_start = -1;
    Level stack[256];
    int depth = 0, subface = 0, k;
    pfGroup *root;
    pfNode *last = NULL, *lastparent = NULL;
    int lastface = 0;
    FltFace face;

    if ((fp = fopen(file, "rb")) == NULL)
        return NULL;
    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    data = (unsigned char *)malloc(size > 0 ? size : 1);
    if (fread(data, 1, size, fp) != (size_t)size) {
        fclose(fp);
        free(data);
        return NULL;
    }
    fclose(fp);
    if (size < 4 || be16(data) != OP_HEADER) {
        free(data);
        return NULL;
    }

    f = (FltFile *)calloc(1, sizeof(FltFile));
    f->path = file;
    {
        const char *sl = strrchr(file, '/');
        if (sl)
            sprintf(f->dir, "%.*s/", (int)(sl - file), file);
    }
    f->unit = 1;
    memset(&face, 0, sizeof(face));
    root = pfNewGroup();
    root->name = (char *)malloc(strlen(file) + 1);
    strcpy(root->name, file);
    stack[0].container = root;
    last = root;

    for (pos = 0; pos + 4 <= size;) {
        const unsigned char *r = data + pos;
        int op = (int)be16(r), len = (int)be16(r + 2);
        pfNode *parent = stack[depth].container;
        if (len < 4 || pos + len > size)
            break;
        switch (op) {
        case OP_HEADER:
            if (len >= 64) {
                int mult = sbe16(r + 60);
                f->version = (int)sbe32(r + 12);
                /* the old integer coordinates: multiplied or divided */
                f->unit = mult > 0 ? mult : mult < 0 ? 1.0 / -mult : 1;
            }
            break;
        case OP_COLOR_PALETTE:
            if (f->version >= 13) {
                /* 128 reserved bytes, then the colors */
                f->npalette = PF_MIN2((len - 132) / 4, 1024);
                for (k = 0; k < f->npalette; k++)
                    packed_color(r + 132 + 4 * k, f->palette[k]);
                break;
            }
            /* the old palette: 32 colors and 56 fixed ones, unsigned
             * shorts r, g, b from 0 to 255 */
            f->oldpalette = 1;
            f->npalette = PF_MIN2((len - 4) / 6, 1024);
            for (k = 0; k < f->npalette; k++) {
                f->palette[k][0] = be16(r + 4 + 6 * k) / 255.0f;
                f->palette[k][1] = be16(r + 6 + 6 * k) / 255.0f;
                f->palette[k][2] = be16(r + 8 + 6 * k) / 255.0f;
                f->palette[k][3] = 1;
            }
            break;
        case OP_MATERIAL_TABLE:
            /* 64 materials of 184 bytes */
            f->nmaterials = PF_MIN2((len - 4) / 184, 256);
            for (k = 0; k < f->nmaterials; k++) {
                const unsigned char *e = r + 4 + 184 * k;
                FltMaterial *m = &f->materials[k];
                int j;
                for (j = 0; j < 3; j++) {
                    m->amb[j] = bef(e + 4 * j);
                    m->dif[j] = bef(e + 12 + 4 * j);
                    m->spec[j] = bef(e + 24 + 4 * j);
                    m->emis[j] = bef(e + 36 + 4 * j);
                }
                m->shininess = bef(e + 48);
                m->alpha = bef(e + 52);
                if (m->alpha <= 0)
                    m->alpha = 1;
            }
            break;
        case OP_MATERIAL:
            if (len >= 84) {
                int idx = (int)sbe32(r + 4), j;
                if (idx >= 0 && idx < 256) {
                    FltMaterial *m = &f->materials[idx];
                    for (j = 0; j < 3; j++) {
                        m->amb[j] = bef(r + 24 + 4 * j);
                        m->dif[j] = bef(r + 36 + 4 * j);
                        m->spec[j] = bef(r + 48 + 4 * j);
                        m->emis[j] = bef(r + 60 + 4 * j);
                    }
                    m->shininess = bef(r + 72);
                    m->alpha = bef(r + 76);
                    if (idx >= f->nmaterials)
                        f->nmaterials = idx + 1;
                }
            }
            break;
        case OP_TEXTURE:
            load_texture(f, r, len);
            break;
        case OP_VERTEX_PALETTE:
            vpal_start = pos;
            f->vpal_len = len >= 8 ? (long)be32(r + 4) : 0;
            if (f->vpal_len > 0) {
                f->vindex = (int *)malloc(sizeof(int) * (f->vpal_len / 4 + 1));
                for (k = 0; k <= f->vpal_len / 4; k++)
                    f->vindex[k] = -1;
            }
            break;
        case OP_VERTEX_C: case OP_VERTEX_CN: case OP_VERTEX_CNT: case OP_VERTEX_CT:
            if (vpal_start >= 0 && f->vindex && pos - vpal_start < f->vpal_len) {
                f->vindex[(pos - vpal_start) / 4] = f->nverts;
                palette_vertex(f, op, r, len, 0);
            }
            break;
        case OP_GROUP:
        case OP_OBJECT:
        case OP_LOD:
        case OP_OLD_LOD:
        case OP_EXTREF: {
            pfNode *n, *child = NULL;
            if (!parent)
                break;
            if (op == OP_LOD || op == OP_OLD_LOD) {
                pfVec3 c;
                double in, out;
                n = pfNewLOD();
                if (op == OP_LOD && len >= 64) {
                    in = bed(r + 16);
                    out = bed(r + 24);
                    for (k = 0; k < 3; k++)
                        c[k] = (float)bed(r + 40 + 8 * k);
                } else {
                    in = sbe32(r + 12) * f->unit;
                    out = sbe32(r + 16) * f->unit;
                    for (k = 0; k < 3; k++)
                        c[k] = (float)(sbe32(r + 28 + 4 * k) * f->unit);
                }
                pfLODRange(n, 0, (float)out);
                pfLODRange(n, 1, (float)in);
                pfLODCenter(n, c);
                child = pfNewGroup();
                pfAddChild(n, child);
            } else if (op == OP_EXTREF) {
                char name[256];
                memcpy(name, r + 4, PF_MIN2(len - 4, 200));
                name[PF_MIN2(len - 4, 200)] = '\0';
                n = pfNewGroup();
                child = external_ref(f, name);
                if (child)
                    pfAddChild(n, child);
                child = n;
            } else {
                n = pfNewGroup();
                child = n;
            }
            if (len >= 12) {
                n->name = (char *)malloc(9);
                memcpy(n->name, r + 4, 8);
                n->name[8] = '\0';
            }
            pfAddChild(parent, n);
            last = n;
            lastparent = parent;
            /* the children of the next push go into child */
            n->userdata = child;
            lastface = 0;
            break;
        }
        case OP_MATRIX:
            if (last && lastparent && len >= 68) {
                pfMatrix m;
                pfSCS *scs;
                for (k = 0; k < 16; k++)
                    m[k / 4][k % 4] = bef(r + 4 + 4 * k);
                if (f->version < 13)
                    for (k = 0; k < 3; k++)
                        m[3][k] *= (float)f->unit;
                scs = pfNewSCS(m);
                pfReplaceChild(lastparent, last, scs);
                pfAddChild(scs, last);
                lastparent = scs;
            }
            break;
        case OP_FACE:
            free(face.v);
            memset(&face, 0, sizeof(face));
            face.subface = subface;
            face.drawtype = len > 18 ? r[18] : 0;
            face.template_ = len > 25 ? r[25] : 0;
            face.texture = len >= 30 ? sbe16(r + 28) : -1;
            face.material = len >= 32 ? sbe16(r + 30) : -1;
            face.lightmode = len > 48 && f->version >= 13 ? r[48] : 0;
            {
                unsigned long flags = len >= 48 && f->version >= 13 ? be32(r + 44) : 0;
                if (flags & 0x40000000UL)
                    face.color[0] = face.color[1] = face.color[2] = face.color[3] = 1;
                else if ((flags & 0x10000000UL) && len >= 60)
                    packed_color(r + 56, face.color);
                else
                    index_color(f, be16(r + 20), face.color);
                if (face.texture >= 0 && len > 19 && r[19])
                    face.color[0] = face.color[1] = face.color[2] = 1;
                face.color[3] = 1;
                if (f->version >= 13 && len >= 42)
                    face.color[3] = 1 - be16(r + 40) / 65535.0f;
            }
            lastface = 1;
            break;
        case OP_VERTEX_LIST:
            if (depth > 0 && !stack[depth].container && f->vindex) {
                for (k = 4; k + 4 <= len; k += 4) {
                    long off = (long)be32(r + k);
                    int vi = off >= 0 && off < f->vpal_len ? f->vindex[off / 4] : -1;
                    if (vi >= 0)
                        face_vertex(&stack[depth].face, &f->verts[vi]);
                }
            }
            break;
        case OP_OLD_VERTEX: case OP_OLD_VERTEX_COLOR: case OP_OLD_VERTEX_NORMAL:
            if (depth > 0 && !stack[depth].container)
                old_vertex(f, &stack[depth].face, op, r, len);
            break;
        case OP_PUSH:
            if (depth + 1 < 256) {
                depth++;
                memset(&stack[depth].face, 0, sizeof(FltFace));
                if (lastface) {
                    /* the face takes its vertices */
                    stack[depth].container = NULL;
                    stack[depth].face = face;
                    memset(&face, 0, sizeof(face));
                } else
                    stack[depth].container = last && last->userdata ? (pfNode *)last->userdata :
                                             last ? last : parent;
            }
            lastface = 0;
            break;
        case OP_POP:
            if (depth > 0) {
                if (!stack[depth].container) {
                    pfNode *owner = stack[depth - 1].container;
                    int d = depth - 1;
                    while (!owner && d > 0)
                        owner = stack[--d].container;
                    if (owner)
                        add_face(f, builder(f, owner), &stack[depth].face);
                    free(stack[depth].face.v);
                }
                depth--;
            }
            lastface = 0;
            break;
        case OP_PUSH_SUBFACE:
            subface++;
            break;
        case OP_POP_SUBFACE:
            if (subface > 0)
                subface--;
            break;
        }
        pos += len;
    }
    free(face.v);
    build_geometry(f);
    free(f->builders);
    free(f->verts);
    free(f->vindex);
    free(data);
    free(f);
    return root;
}

static void clear_userdata(pfNode *n, int depth)
{
    int i;
    if (!n || depth > 64)
        return;
    n->userdata = NULL;
    for (i = 0; i < n->nchildren; i++)
        clear_userdata(n->children[i], depth + 1);
}

pfNode *pfdLoadFile_flt(const char *file)
{
    char path[4096];
    pfNode *n;
    if (!pfFindFile(file, path, R_OK)) {
        pfNotify(PFNFY_WARN, PFNFY_RESOURCE, "pfdLoadFile_flt: cannot open %s", file);
        return NULL;
    }
    n = load_flt(path);
    if (!n)
        pfNotify(PFNFY_WARN, PFNFY_RESOURCE, "pfdLoadFile_flt: %s is not an OpenFlight file", file);
    else
        clear_userdata(n, 0);
    return n;
}
